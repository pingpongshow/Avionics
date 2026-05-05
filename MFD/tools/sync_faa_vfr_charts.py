#!/usr/bin/env python3

import argparse
import html
import re
import sys
import zipfile
from pathlib import Path
from tempfile import TemporaryDirectory

import requests

from import_faa_chart import import_faa_chart


FAA_VFR_INDEX = "https://www.faa.gov/air_traffic/flight_info/aeronav/digital_products/vfr/"


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Download and import the FAA national VFR raster chart aggregates into local MFD chart packages."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("MFD/charts/packages"),
        help="Destination chart-package directory.",
    )
    parser.add_argument(
        "--cache-dir",
        type=Path,
        default=Path.home() / ".cache" / "experimental-pfd" / "faa_vfr",
        help="Local cache for the FAA aggregate ZIP downloads.",
    )
    parser.add_argument(
        "--chart-sets",
        nargs="+",
        choices=("sectional", "terminal", "helicopter"),
        default=("sectional", "terminal", "helicopter"),
        help="Which FAA aggregate chart sets to process.",
    )
    parser.add_argument(
        "--pattern",
        default="",
        help="Optional case-insensitive substring filter on chart names.",
    )
    parser.add_argument(
        "--max-charts",
        type=int,
        default=0,
        help="Optional maximum number of charts to import.",
    )
    parser.add_argument(
        "--max-dim",
        type=int,
        default=8192,
        help="Maximum source image dimension for generated top levels.",
    )
    parser.add_argument(
        "--tile-size",
        type=int,
        default=512,
    )
    parser.add_argument(
        "--jpeg-quality",
        type=int,
        default=90,
    )
    return parser.parse_args(argv)


def fetch_text(url: str) -> str:
    response = requests.get(url, timeout=120)
    response.raise_for_status()
    return response.text


def discover_current_visual_urls() -> dict:
    page = fetch_text(FAA_VFR_INDEX)
    urls = {}
    for chart_set, suffix in {
        "sectional": "All_Files/Sectional.zip",
        "terminal": "All_Files/Terminal.zip",
        "helicopter": "All_Files/Helicopter.zip",
    }.items():
        matches = re.findall(rf'https://aeronav\.faa\.gov/visual/[^\s"\']*{re.escape(suffix)}', page)
        if not matches:
            raise RuntimeError(f"Could not discover FAA {chart_set} aggregate ZIP URL")
        urls[chart_set] = html.unescape(matches[-1])
    return urls


def download_if_missing(url: str, destination: Path) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and destination.stat().st_size > 0:
        return destination

    response = requests.get(url, timeout=600, stream=True)
    response.raise_for_status()
    with destination.open("wb") as handle:
        for chunk in response.iter_content(chunk_size=1024 * 1024):
            if chunk:
                handle.write(chunk)
    return destination


def grouped_members(archive: zipfile.ZipFile):
    groups = {}
    for member in archive.namelist():
        if member.endswith("/") or member.startswith("__MACOSX/"):
            continue
        suffix = Path(member).suffix.lower()
        if suffix not in (".tif", ".tfw", ".htm", ".html"):
            continue
        groups.setdefault(Path(member).stem, []).append(member)
    return groups


def import_archive(archive_path: Path, output_dir: Path, pattern: str, max_charts: int, max_dim: int, tile_size: int, jpeg_quality: int) -> int:
    imported = 0
    pattern_lower = pattern.strip().lower()
    with zipfile.ZipFile(archive_path) as archive:
        for stem, members in grouped_members(archive).items():
            if pattern_lower and pattern_lower not in stem.lower():
                continue
            with TemporaryDirectory() as temp_dir:
                temp_root = Path(temp_dir)
                for member in members:
                    archive.extract(member, temp_root)
                import_faa_chart(temp_root, output_dir, max_dim=max_dim, tile_size=tile_size, jpeg_quality=jpeg_quality)
                imported += 1
            if max_charts and imported >= max_charts:
                break
    return imported


def main(argv=None) -> int:
    args = parse_args(argv)
    try:
        urls = discover_current_visual_urls()
        args.output_dir.mkdir(parents=True, exist_ok=True)
        total_imported = 0
        for chart_set in args.chart_sets:
            url = urls[chart_set]
            local_zip = download_if_missing(url, args.cache_dir / Path(url).name)
            imported = import_archive(
                local_zip,
                args.output_dir,
                args.pattern,
                args.max_charts,
                args.max_dim,
                args.tile_size,
                args.jpeg_quality,
            )
            total_imported += imported
            print(f"{chart_set}: imported {imported} charts from {local_zip}")
        print(f"Total imported charts: {total_imported}")
        return 0
    except Exception as exc:
        print(f"FAA VFR sync failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
