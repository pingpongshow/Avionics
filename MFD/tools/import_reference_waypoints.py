#!/usr/bin/env python3

import argparse
import csv
import html
import json
import math
import re
import sys
import zipfile
from dataclasses import dataclass
from datetime import datetime, timezone
from io import BytesIO
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import requests


FAA_SUBSCRIPTION_INDEX = "https://www.faa.gov/air_traffic/flight_info/aeronav/aero_data/NASR_Subscription/"
OPENAIP_EXPORTS_URL = "https://www.openaip.net/data/exports"


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Build the MFD national reference waypoint database from FAA and openAIP sources."
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("MFD/data/reference_waypoints.json"),
        help="Output JSON path.",
    )
    parser.add_argument(
        "--metadata-output",
        type=Path,
        default=Path("MFD/data/reference_waypoints.metadata.json"),
        help="Optional metadata summary JSON path.",
    )
    parser.add_argument(
        "--no-openaip",
        action="store_true",
        help="Skip the openAIP supplemental import.",
    )
    parser.add_argument(
        "--country",
        default="us",
        help="openAIP export country prefix to import. Defaults to us.",
    )
    parser.add_argument(
        "--max-openaip-pages",
        type=int,
        default=24,
        help="Maximum export pages to inspect when scraping openAIP export links.",
    )
    return parser.parse_args(argv)


def fetch_text(url: str) -> str:
    response = requests.get(url, timeout=120)
    response.raise_for_status()
    return response.text


def fetch_bytes(url: str) -> bytes:
    response = requests.get(url, timeout=240)
    response.raise_for_status()
    return response.content


def parse_float(value: str) -> Optional[float]:
    try:
        if value is None or str(value).strip() == "":
            return None
        return float(str(value).strip())
    except ValueError:
        return None


def normalized_ident(value: str) -> str:
    return str(value or "").strip().upper()


def unique_aliases(*values: Iterable[str]) -> List[str]:
    aliases: List[str] = []
    for candidate in values:
        if isinstance(candidate, (list, tuple)):
            sources = candidate
        else:
            sources = [candidate]
        for source in sources:
            ident = normalized_ident(source)
            if ident and ident not in aliases:
                aliases.append(ident)
    return aliases


def haversine_nm(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    earth_radius_nm = 3440.065
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlambda = math.radians(lon2 - lon1)
    a = math.sin(dphi / 2.0) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(dlambda / 2.0) ** 2
    return earth_radius_nm * 2.0 * math.atan2(math.sqrt(a), math.sqrt(max(0.0, 1.0 - a)))


def choose_latest_faa_cycle_page() -> Tuple[str, str]:
    page = fetch_text(FAA_SUBSCRIPTION_INDEX)
    matches = re.findall(r'NASR_Subscription/(\d{4}-\d{2}-\d{2})', page)
    if not matches:
        raise RuntimeError("Could not discover FAA NASR subscription cycle links")

    current_cycle = sorted(set(matches))[-2 if len(set(matches)) >= 2 else -1]
    cycle_page = f"{FAA_SUBSCRIPTION_INDEX.rstrip('/')}/{current_cycle}/"
    return current_cycle, cycle_page


def discover_faa_csv_urls() -> Tuple[str, Dict[str, str]]:
    cycle, cycle_page = choose_latest_faa_cycle_page()
    page = fetch_text(cycle_page)
    urls: Dict[str, str] = {}
    for dataset in ("APT", "FIX", "NAV"):
        match = re.search(
            rf'(https://nfdc\.faa\.gov/webContent/28DaySub/extra/[^\s"\'<>]*_{dataset}_CSV\.zip)',
            page,
            re.IGNORECASE,
        )
        if not match:
            raise RuntimeError(f"Could not find FAA {dataset} CSV ZIP on {cycle_page}")
        urls[dataset.lower()] = html.unescape(match.group(1))
    return cycle, urls


def csv_rows_from_zip(url: str, base_name: str) -> List[dict]:
    archive = zipfile.ZipFile(BytesIO(fetch_bytes(url)))
    try:
        csv_name = next(name for name in archive.namelist() if name.upper() == base_name.upper())
    except StopIteration as exc:
        raise RuntimeError(f"{url} did not contain {base_name}") from exc

    with archive.open(csv_name) as handle:
        reader = csv.DictReader((line.decode("latin1") for line in handle))
        return list(reader)


@dataclass
class MergeStats:
    faa_airports: int = 0
    faa_fixes: int = 0
    faa_navaids: int = 0
    openaip_airports: int = 0
    openaip_navaids: int = 0
    merged_duplicates: int = 0


class WaypointStore:
    def __init__(self) -> None:
        self.items: List[dict] = []
        self._bucket: Dict[Tuple[str, str], List[int]] = {}
        self.stats = MergeStats()

    def add(self, item: dict) -> None:
        key = (normalized_ident(item["ident"]), item.get("symbolType", "unknown"))
        candidates = self._bucket.get(key, [])
        for index in candidates:
            existing = self.items[index]
            if haversine_nm(
                existing["latitudeDeg"],
                existing["longitudeDeg"],
                item["latitudeDeg"],
                item["longitudeDeg"],
            ) <= 2.0:
                existing["aliases"] = unique_aliases(existing.get("aliases", []), item.get("aliases", []))
                existing["sources"] = sorted(set(existing.get("sources", [existing.get("source", "unknown")]) + item.get("sources", [item.get("source", "unknown")])))
                if existing.get("name", "").startswith("Unnamed") and item.get("name"):
                    existing["name"] = item["name"]
                self.stats.merged_duplicates += 1
                return

        self.items.append(item)
        self._bucket.setdefault(key, []).append(len(self.items) - 1)


def faa_airport_item(row: dict) -> Optional[dict]:
    lat = parse_float(row.get("LAT_DECIMAL"))
    lon = parse_float(row.get("LONG_DECIMAL"))
    if lat is None or lon is None:
        return None

    ident = normalized_ident(row.get("ICAO_ID") or row.get("ARPT_ID"))
    if not ident:
        return None

    aliases = unique_aliases(row.get("ARPT_ID"), row.get("ICAO_ID"))
    return {
        "ident": ident,
        "name": (row.get("ARPT_NAME") or ident).strip(),
        "type": "airport",
        "symbolType": "airport",
        "latitudeDeg": lat,
        "longitudeDeg": lon,
        "source": "faa",
        "sources": ["faa"],
        "countryCode": row.get("COUNTRY_CODE", "US").strip(),
        "stateCode": row.get("STATE_CODE", "").strip(),
        "city": row.get("CITY", "").strip(),
        "aliases": aliases,
        "publicUse": row.get("FACILITY_USE_CODE", "").strip() == "PU",
    }


def faa_fix_item(row: dict) -> Optional[dict]:
    lat = parse_float(row.get("LAT_DECIMAL"))
    lon = parse_float(row.get("LONG_DECIMAL"))
    ident = normalized_ident(row.get("FIX_ID"))
    if lat is None or lon is None or not ident:
        return None

    aliases = unique_aliases(row.get("FIX_ID"), row.get("FIX_ID_OLD"))
    return {
        "ident": ident,
        "name": ident,
        "type": "fix",
        "symbolType": "fix",
        "latitudeDeg": lat,
        "longitudeDeg": lon,
        "source": "faa",
        "sources": ["faa"],
        "countryCode": row.get("COUNTRY_CODE", "US").strip(),
        "stateCode": row.get("STATE_CODE", "").strip(),
        "aliases": aliases,
        "fixUseCode": row.get("FIX_USE_CODE", "").strip(),
    }


def faa_navaid_item(row: dict) -> Optional[dict]:
    lat = parse_float(row.get("LAT_DECIMAL"))
    lon = parse_float(row.get("LONG_DECIMAL"))
    ident = normalized_ident(row.get("NAV_ID"))
    if lat is None or lon is None or not ident:
        return None

    aliases = unique_aliases(row.get("NAV_ID"))
    navaid_type = (row.get("NAV_TYPE") or "NAVAID").strip()
    return {
        "ident": ident,
        "name": ((row.get("NAME") or ident).strip() + f" {navaid_type}").strip(),
        "type": navaid_type.lower().replace("/", "_").replace(" ", "_"),
        "symbolType": "navaid",
        "latitudeDeg": lat,
        "longitudeDeg": lon,
        "source": "faa",
        "sources": ["faa"],
        "countryCode": row.get("COUNTRY_CODE", "US").strip(),
        "stateCode": row.get("STATE_CODE", "").strip(),
        "city": row.get("CITY", "").strip(),
        "aliases": aliases,
    }


def discover_openaip_urls(country: str, max_pages: int) -> Dict[str, str]:
    preferred_extensions = ("ndgeojson", "geojson", "json", "xml", "cupx", "cup")
    wanted_prefixes = ("apt", "nav")
    found: Dict[str, Dict[str, str]] = {}

    for page_number in range(1, max_pages + 1):
        page = fetch_text(f"{OPENAIP_EXPORTS_URL}?page={page_number}")
        for raw_url in re.findall(r'https://storage\.googleapis\.com/[^"\']+', page):
            url = html.unescape(raw_url)
            if "filename%3D%22" not in url:
                continue
            filename = url.split("filename%3D%22", 1)[1].split("%22", 1)[0]
            if not filename.startswith(f"{country.lower()}_"):
                continue
            stem, extension = filename.rsplit(".", 1)
            prefix = stem.split("_", 1)[1]
            if prefix not in wanted_prefixes:
                continue
            found.setdefault(prefix, {})
            found[prefix].setdefault(extension, url)

    urls: Dict[str, str] = {}
    for prefix in wanted_prefixes:
        variants = found.get(prefix, {})
        for extension in preferred_extensions:
            if extension in variants:
                urls[prefix] = variants[extension]
                break
    return urls


def iter_ndgeojson_features(url: str) -> Iterable[dict]:
    response = requests.get(url, timeout=240)
    response.raise_for_status()
    for line in response.text.splitlines():
        line = line.strip()
        if not line:
            continue
        yield json.loads(line)


def openaip_airport_item(feature: dict) -> Optional[dict]:
    geometry = feature.get("geometry") or {}
    properties = feature.get("properties") or {}
    coords = geometry.get("coordinates") or []
    if geometry.get("type") != "Point" or len(coords) < 2:
        return None

    ident = normalized_ident(
        properties.get("icaoCode")
        or properties.get("iataCode")
        or properties.get("identifier")
        or properties.get("altIdentifier")
    )
    if not ident:
        return None

    aliases = unique_aliases(
        ident,
        properties.get("icaoCode"),
        properties.get("iataCode"),
        properties.get("identifier"),
        properties.get("altIdentifier"),
    )
    return {
        "ident": ident,
        "name": (properties.get("name") or ident).strip(),
        "type": "airport",
        "symbolType": "airport",
        "latitudeDeg": float(coords[1]),
        "longitudeDeg": float(coords[0]),
        "source": "openaip",
        "sources": ["openaip"],
        "countryCode": (properties.get("country") or "US").strip(),
        "aliases": aliases,
        "private": bool(properties.get("private", False)),
    }


def openaip_navaid_item(feature: dict) -> Optional[dict]:
    geometry = feature.get("geometry") or {}
    properties = feature.get("properties") or {}
    coords = geometry.get("coordinates") or []
    if geometry.get("type") != "Point" or len(coords) < 2:
        return None

    ident = normalized_ident(properties.get("identifier"))
    if not ident:
        return None

    raw_type = properties.get("type")
    type_label = f"openaip_{raw_type}" if raw_type is not None else "openaip_navaid"
    return {
        "ident": ident,
        "name": (properties.get("name") or ident).strip(),
        "type": type_label,
        "symbolType": "navaid",
        "latitudeDeg": float(coords[1]),
        "longitudeDeg": float(coords[0]),
        "source": "openaip",
        "sources": ["openaip"],
        "countryCode": (properties.get("country") or "US").strip(),
        "aliases": unique_aliases(properties.get("identifier")),
    }


def build_database(country: str, include_openaip: bool, max_openaip_pages: int) -> Tuple[List[dict], dict]:
    cycle, faa_urls = discover_faa_csv_urls()
    store = WaypointStore()

    for row in csv_rows_from_zip(faa_urls["apt"], "APT_BASE.csv"):
        item = faa_airport_item(row)
        if item is not None:
            store.add(item)
            store.stats.faa_airports += 1

    for row in csv_rows_from_zip(faa_urls["fix"], "FIX_BASE.csv"):
        item = faa_fix_item(row)
        if item is not None:
            store.add(item)
            store.stats.faa_fixes += 1

    for row in csv_rows_from_zip(faa_urls["nav"], "NAV_BASE.csv"):
        item = faa_navaid_item(row)
        if item is not None:
            store.add(item)
            store.stats.faa_navaids += 1

    openaip_urls: Dict[str, str] = {}
    if include_openaip:
        openaip_urls = discover_openaip_urls(country, max_openaip_pages)
        airport_url = openaip_urls.get("apt")
        if airport_url:
            for feature in iter_ndgeojson_features(airport_url):
                item = openaip_airport_item(feature)
                if item is not None:
                    store.add(item)
                    store.stats.openaip_airports += 1
        navaid_url = openaip_urls.get("nav")
        if navaid_url:
            for feature in iter_ndgeojson_features(navaid_url):
                item = openaip_navaid_item(feature)
                if item is not None:
                    store.add(item)
                    store.stats.openaip_navaids += 1

    items = sorted(
        store.items,
        key=lambda item: (
            {"airport": 0, "navaid": 1, "fix": 2}.get(item.get("symbolType", ""), 9),
            item["ident"],
            item.get("name", ""),
        ),
    )

    metadata = {
        "generatedAt": datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "faaCycle": cycle,
        "counts": {
            "total": len(items),
            "faaAirports": store.stats.faa_airports,
            "faaFixes": store.stats.faa_fixes,
            "faaNavaids": store.stats.faa_navaids,
            "openAipAirports": store.stats.openaip_airports,
            "openAipNavaids": store.stats.openaip_navaids,
            "mergedDuplicates": store.stats.merged_duplicates,
        },
        "sources": {
            "faa": faa_urls,
            "openaip": openaip_urls,
        },
    }
    return items, metadata


def main(argv=None) -> int:
    args = parse_args(argv)
    try:
        items, metadata = build_database(
            country=args.country.lower(),
            include_openaip=not args.no_openaip,
            max_openaip_pages=args.max_openaip_pages,
        )
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(items, indent=2))
        if args.metadata_output:
            args.metadata_output.parent.mkdir(parents=True, exist_ok=True)
            args.metadata_output.write_text(json.dumps(metadata, indent=2))
        print(f"Wrote {len(items)} waypoints to {args.output}")
        print(json.dumps(metadata["counts"], indent=2))
        return 0
    except Exception as exc:
        print(f"Waypoint import failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
