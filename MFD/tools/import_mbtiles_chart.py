#!/usr/bin/env python3

import argparse
import json
import shutil
import sqlite3
import sys
from pathlib import Path


def slugify(value: str) -> str:
    import re
    return re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")


def read_metadata(connection):
    return {name: value for name, value in connection.execute("select name, value from metadata")}


def parse_bounds(bounds_text: str):
    west, south, east, north = [float(part) for part in bounds_text.split(",")]
    return {
        "westLon": west,
        "southLat": south,
        "eastLon": east,
        "northLat": north,
    }


def export_level(connection, zoom: int, level_dir: Path, tile_format: str):
    rows = list(connection.execute(
        "select tile_column, tile_row, tile_data from tiles where tile_zoom_level = ? order by tile_column, tile_row",
        (zoom,),
    ))
    if not rows:
        return None

    min_column = None
    max_column = None
    min_row = None
    max_row = None

    for tile_column, tile_row_tms, tile_data in rows:
        tile_row_xyz = (1 << zoom) - 1 - tile_row_tms
        column_dir = level_dir / str(tile_column)
        column_dir.mkdir(parents=True, exist_ok=True)
        tile_path = column_dir / f"{tile_row_xyz}.{tile_format}"
        tile_path.write_bytes(tile_data)

        min_column = tile_column if min_column is None else min(min_column, tile_column)
        max_column = tile_column if max_column is None else max(max_column, tile_column)
        min_row = tile_row_xyz if min_row is None else min(min_row, tile_row_xyz)
        max_row = tile_row_xyz if max_row is None else max(max_row, tile_row_xyz)

    tile_size = 256
    return {
        "id": f"z{zoom}",
        "path": f"tiles/z{zoom}",
        "format": tile_format,
        "scale": 1.0,
        "zoom": zoom,
        "tileSize": tile_size,
        "cols": max_column + 1,
        "rows": max_row + 1,
        "minColumn": min_column,
        "maxColumn": max_column,
        "minRow": min_row,
        "maxRow": max_row,
        "width": tile_size * (1 << zoom),
        "height": tile_size * (1 << zoom),
    }


def import_mbtiles(input_path: Path, output_dir: Path, min_zoom=None, max_zoom=None):
    with sqlite3.connect(input_path) as connection:
        metadata = read_metadata(connection)
        title = metadata.get("name", input_path.stem)
        package_id = slugify(title)
        package_root = output_dir / package_id
        tile_format = metadata.get("format", "png")
        metadata_min_zoom = int(metadata.get("minzoom", 0))
        metadata_max_zoom = int(metadata.get("maxzoom", metadata_min_zoom))
        selected_min_zoom = metadata_min_zoom if min_zoom is None else max(min_zoom, metadata_min_zoom)
        selected_max_zoom = metadata_max_zoom if max_zoom is None else min(max_zoom, metadata_max_zoom)
        bounds = parse_bounds(metadata.get("bounds", "-180,-85,180,85"))

        if package_root.exists():
            shutil.rmtree(package_root)
        package_root.mkdir(parents=True, exist_ok=True)

        levels = []
        for zoom in range(selected_min_zoom, selected_max_zoom + 1):
            level_dir = package_root / "tiles" / f"z{zoom}"
            level = export_level(connection, zoom, level_dir, tile_format)
            if level is not None:
                levels.append(level)

        if not levels:
            raise RuntimeError("No tiles were exported from the MBTiles package")

        manifest = {
            "packageId": package_id,
            "title": title,
            "sourceKey": "openflightmaps",
            "sourceName": "OpenFlightMaps",
            "packageType": "mercator_tiles",
            "cycle": metadata.get("version", ""),
            "bounds": bounds,
            "defaultZoom": levels[-1]["zoom"],
            "levels": levels,
        }

        (package_root / "manifest.json").write_text(json.dumps(manifest, indent=2))
        print(f"Imported MBTiles chart package: {package_root}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Import an MBTiles chart package into the MFD chart-package format")
    parser.add_argument("input_path", type=Path, help="Path to an MBTiles database")
    parser.add_argument("--output-dir", type=Path, default=Path("MFD/charts/packages"))
    parser.add_argument("--min-zoom", type=int, default=None)
    parser.add_argument("--max-zoom", type=int, default=None)
    args = parser.parse_args(argv)

    try:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        import_mbtiles(args.input_path, args.output_dir, args.min_zoom, args.max_zoom)
    except Exception as exc:
        print(f"MBTiles import failed: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
