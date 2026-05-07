#!/usr/bin/env python3

from __future__ import annotations

import argparse
import base64
import json
import math
import socket
import struct
import threading
import time
from dataclasses import dataclass, field
from typing import Dict, List, Optional


BLOCK_WIDTH_DEG = 48.0 / 60.0
WIDE_BLOCK_WIDTH_DEG = 96.0 / 60.0
BLOCK_HEIGHT_DEG = 4.0 / 60.0
BLOCK_THRESHOLD = 405000
BLOCKS_PER_RING = 450

CAN_ID_HEARTBEAT = 0x500
CAN_ID_TRAFFIC = 0x510
CAN_ID_WEATHER_HEADER = 0x520
CAN_ID_WEATHER_PAYLOAD = 0x521


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ADSB / FIS-B bridge to CAN FD.")
    parser.add_argument("--stratux-endpoint", default="ws://192.168.10.1", help="Base Stratux websocket endpoint.")
    parser.add_argument("--can-iface", default="vcan0", help="SocketCAN interface name.")
    parser.add_argument("--dry-run", action="store_true", help="Print frames instead of writing to CAN.")
    return parser.parse_args()


def websocket_url(base: str, suffix: str) -> str:
    value = base.rstrip("/")
    if value.endswith("/weather"):
        value = value[: -len("/weather")]
    if value.endswith("/jsonio"):
        value = value[: -len("/jsonio")]
    return value + suffix


def block_scale_multiplier(scale_factor: int) -> float:
    if scale_factor == 1:
        return 5.0
    if scale_factor == 2:
        return 9.0
    return 1.0


def block_location(block_number: int, southern_hemisphere: bool, scale_factor: int) -> tuple[float, float, float, float]:
    scale = block_scale_multiplier(scale_factor)
    normalized_block = block_number
    if normalized_block >= BLOCK_THRESHOLD:
        normalized_block &= ~1

    raw_lat = BLOCK_HEIGHT_DEG * float(normalized_block // BLOCKS_PER_RING)
    raw_lon = float(normalized_block % BLOCKS_PER_RING) * BLOCK_WIDTH_DEG
    lon_size = (WIDE_BLOCK_WIDTH_DEG if normalized_block >= BLOCK_THRESHOLD else BLOCK_WIDTH_DEG) * scale
    lat_size = BLOCK_HEIGHT_DEG * scale

    if southern_hemisphere:
        raw_lat = -raw_lat
    else:
        raw_lat += BLOCK_HEIGHT_DEG

    if raw_lon > 180.0:
        raw_lon -= 360.0

    return raw_lat, raw_lon, lat_size, lon_size


def positive_modulo(value: int, modulus: int) -> int:
    result = value % modulus
    return result + modulus if result < 0 else result


@dataclass
class TrafficTarget:
    identifier: str
    latitude_deg: float
    longitude_deg: float
    altitude_ft: int
    track_deg: int
    ground_speed_kt: int
    age_seconds: int = 0


@dataclass
class NexradBlock:
    product_id: int
    scale_factor: int
    block_number: int
    lat_north_deg: float
    lon_west_deg: float
    lat_height_deg: float
    lon_width_deg: float
    intensity: List[int]
    received_unix: float = field(default_factory=time.time)


class CanPublisher:
    def __init__(self, interface_name: str, dry_run: bool = False) -> None:
        self.interface_name = interface_name
        self.dry_run = dry_run
        self.socket: Optional[socket.socket] = None
        if not dry_run:
            self.socket = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
            self.socket.bind((interface_name,))

    def send(self, arbitration_id: int, payload: bytes) -> None:
        if self.dry_run:
            print(f"CAN 0x{arbitration_id:03X} {payload.hex()}")
            return

        assert self.socket is not None
        can_frame = struct.pack("=IB3x64s", arbitration_id, len(payload), payload.ljust(64, b"\x00"))
        self.socket.send(can_frame)

    def publish_heartbeat(self, traffic_count: int, weather_block_count: int) -> None:
        payload = struct.pack("<HHH", 1, traffic_count & 0xFFFF, weather_block_count & 0xFFFF)
        self.send(CAN_ID_HEARTBEAT, payload)

    def publish_traffic(self, target: TrafficTarget) -> None:
        ident = target.identifier.encode("ascii", "ignore")[:8].ljust(8, b"\x00")
        payload = struct.pack(
            "<8siiHHB",
            ident,
            int(round(target.latitude_deg * 1000000.0)),
            int(round(target.longitude_deg * 1000000.0)),
            max(0, min(65535, int(target.altitude_ft))),
            max(0, min(65535, int(target.ground_speed_kt))),
            max(0, min(255, int(target.track_deg))),
            max(0, min(255, int(target.age_seconds))),
        )
        self.send(CAN_ID_TRAFFIC, payload)

    def publish_nexrad_block(self, block: NexradBlock) -> None:
        header = struct.pack(
            "<BBIhhhhH",
            block.product_id & 0xFF,
            block.scale_factor & 0xFF,
            block.block_number & 0xFFFFFFFF,
            int(round(block.lat_north_deg * 100)),
            int(round(block.lon_west_deg * 100)),
            int(round(block.lat_height_deg * 100)),
            int(round(block.lon_width_deg * 100)),
            len(block.intensity) & 0xFFFF,
        )
        self.send(CAN_ID_WEATHER_HEADER, header)

        chunk_size = 48
        for offset in range(0, len(block.intensity), chunk_size):
            chunk = bytes(block.intensity[offset : offset + chunk_size])
            payload = struct.pack("<H", offset) + chunk
            self.send(CAN_ID_WEATHER_PAYLOAD, payload)


class AdsbBridge:
    def __init__(self, publisher: CanPublisher) -> None:
        self.publisher = publisher
        self.traffic_targets: Dict[str, TrafficTarget] = {}
        self.weather_blocks: Dict[str, NexradBlock] = {}
        self.lock = threading.Lock()

    def heartbeat_forever(self) -> None:
        while True:
            with self.lock:
                self.publisher.publish_heartbeat(len(self.traffic_targets), len(self.weather_blocks))
            time.sleep(1.0)

    def handle_jsonio_message(self, payload: str) -> None:
        try:
            message = json.loads(payload)
        except json.JSONDecodeError:
            return

        if not isinstance(message, dict):
            return

        if "Product_id" in message and "FISB_data" in message:
            self._handle_weather_frame(message)
            return

        if "Icao_addr" in message or "Lat" in message:
            self._handle_traffic(message)

    def _handle_traffic(self, message: dict) -> None:
        identifier = str(message.get("Tail", "") or message.get("Icao_addr", "")).strip().upper()
        latitude_deg = message.get("Lat")
        longitude_deg = message.get("Lng")
        if not identifier or latitude_deg is None or longitude_deg is None:
            return

        target = TrafficTarget(
            identifier=identifier,
            latitude_deg=float(latitude_deg),
            longitude_deg=float(longitude_deg),
            altitude_ft=int(round(float(message.get("Alt", 0.0)))),
            track_deg=int(round(float(message.get("Track", 0.0)))) % 360,
            ground_speed_kt=int(round(float(message.get("Speed", 0.0)))),
            age_seconds=int(round(float(message.get("Age", 0.0)))),
        )

        with self.lock:
            self.traffic_targets[identifier] = target
            self.publisher.publish_traffic(target)

    def _handle_weather_frame(self, message: dict) -> None:
        product_id = int(message.get("Product_id", -1))
        if product_id < 51 or product_id > 64:
            return

        raw_b64 = message.get("FISB_data", "")
        if not isinstance(raw_b64, str) or not raw_b64:
            return

        try:
            fisb_data = base64.b64decode(raw_b64)
        except Exception:
            return

        blocks = decode_nexrad_blocks(product_id, fisb_data)
        with self.lock:
            for block in blocks:
                key = f"{block.product_id}:{block.scale_factor}:{block.block_number}"
                self.weather_blocks[key] = block
                self.publisher.publish_nexrad_block(block)


def decode_nexrad_blocks(product_id: int, fisb_data: bytes) -> List[NexradBlock]:
    if len(fisb_data) < 4:
        return []

    byte0 = fisb_data[0]
    run_length_encoded = (byte0 & 0x80) != 0
    southern_hemisphere = (byte0 & 0x40) != 0
    block_number = ((byte0 & 0x0F) << 16) | (fisb_data[1] << 8) | fisb_data[2]
    scale_factor = (byte0 & 0x30) >> 4
    blocks: List[NexradBlock] = []

    if run_length_encoded:
        lat_north, lon_west, lat_height, lon_width = block_location(block_number, southern_hemisphere, scale_factor)
        intensity: List[int] = []
        for value in fisb_data[3:]:
            run_length = (value >> 3) + 1
            intensity_value = value & 0x07
            intensity.extend([intensity_value] * run_length)
        if intensity:
            blocks.append(
                NexradBlock(
                    product_id=product_id,
                    scale_factor=scale_factor,
                    block_number=block_number,
                    lat_north_deg=lat_north,
                    lon_west_deg=lon_west,
                    lat_height_deg=lat_height,
                    lon_width_deg=lon_width,
                    intensity=intensity,
                )
            )
        return blocks

    if block_number >= BLOCK_THRESHOLD:
        row_start = block_number - ((block_number - BLOCK_THRESHOLD) % 225)
        row_size = 225
    else:
        row_start = block_number - (block_number % 450)
        row_size = 450

    row_offset = block_number - row_start
    mask_byte_count = fisb_data[3] & 0x0F
    if len(fisb_data) < mask_byte_count + 3:
        return []

    for i in range(mask_byte_count):
        if i == 0:
            mask_byte = (fisb_data[3] & 0xF0) | 0x08
        else:
            mask_byte = fisb_data[i + 3]

        for bit in range(8):
            if (mask_byte & (1 << bit)) == 0:
                continue
            row_x = positive_modulo(row_offset + 8 * i + bit - 3, row_size)
            derived_block = row_start + row_x
            lat_north, lon_west, lat_height, lon_width = block_location(derived_block, southern_hemisphere, scale_factor)
            fill_value = 1 if product_id == 64 else 0
            blocks.append(
                NexradBlock(
                    product_id=product_id,
                    scale_factor=scale_factor,
                    block_number=derived_block,
                    lat_north_deg=lat_north,
                    lon_west_deg=lon_west,
                    lat_height_deg=lat_height,
                    lon_width_deg=lon_width,
                    intensity=[fill_value] * 128,
                )
            )

    return blocks


def run_websocket(url: str, on_message) -> None:
    import websocket

    ws = websocket.WebSocketApp(url, on_message=lambda _ws, payload: on_message(payload))
    ws.run_forever()


def main() -> int:
    args = parse_args()
    publisher = CanPublisher(args.can_iface, dry_run=args.dry_run)
    bridge = AdsbBridge(publisher)

    threading.Thread(target=bridge.heartbeat_forever, daemon=True).start()
    threading.Thread(
        target=run_websocket,
        args=(websocket_url(args.stratux_endpoint, "/jsonio"), bridge.handle_jsonio_message),
        daemon=True,
    ).start()

    while True:
        time.sleep(1.0)


if __name__ == "__main__":
    raise SystemExit(main())
