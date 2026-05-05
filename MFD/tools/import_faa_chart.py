#!/usr/bin/env python3

import argparse
import html
import json
import math
import re
import shutil
import sys
import zipfile
from pathlib import Path
from tempfile import TemporaryDirectory

from PIL import Image

Image.MAX_IMAGE_PIXELS = None


def slugify(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")


def strip_html(text: str) -> str:
    text = re.sub(r"(?is)<script.*?>.*?</script>", " ", text)
    text = re.sub(r"(?is)<style.*?>.*?</style>", " ", text)
    text = re.sub(r"(?s)<[^>]+>", " ", text)
    return html.unescape(re.sub(r"\s+", " ", text))


def parse_world_file(path: Path) -> dict:
    values = [float(line.strip()) for line in path.read_text().splitlines() if line.strip()]
    if len(values) != 6:
        raise ValueError(f"World file {path} did not contain 6 values")
    return {
        "a": values[0],
        "d": values[1],
        "b": values[2],
        "e": values[3],
        "c": values[4],
        "f": values[5],
    }


def parse_world_file_from_geotiff(path: Path) -> dict:
    with Image.open(path) as image:
        pixel_scale = image.tag_v2.get(33550)
        tie_points = image.tag_v2.get(33922)
        if not pixel_scale or not tie_points or len(pixel_scale) < 2 or len(tie_points) < 6:
            raise ValueError(f"GeoTIFF {path} does not expose enough georeferencing tags")

        scale_x = float(pixel_scale[0])
        scale_y = float(pixel_scale[1])
        tie_x = float(tie_points[3])
        tie_y = float(tie_points[4])

        return {
            "a": scale_x,
            "d": 0.0,
            "b": 0.0,
            "e": -scale_y,
            "c": tie_x + scale_x / 2.0,
            "f": tie_y - scale_y / 2.0,
        }


def parse_metadata(path: Path) -> dict:
    text = strip_html(path.read_text(errors="ignore"))

    def search(pattern: str, default=None, cast=float):
        match = re.search(pattern, text, re.IGNORECASE)
        if not match:
            return default
        return cast(match.group(1))

    def search_all(pattern: str, cast=float):
        return [cast(match) for match in re.findall(pattern, text, re.IGNORECASE)]

    title = search(r"Title:\s*([A-Za-z0-9 .,'()/-]+)", default=path.stem, cast=str)
    west = search(r"West_Bounding_Coordinate:\s*([-\d.]+)")
    east = search(r"East_Bounding_Coordinate:\s*([-\d.]+)")
    north = search(r"North_Bounding_Coordinate:\s*([-\d.]+)")
    south = search(r"South_Bounding_Coordinate:\s*([-\d.]+)")
    std_parallels = search_all(r"Standard_Parallel:\s*([-\d.]+)")
    lon0 = search(r"Longitude_of_Central_Meridian:\s*([-\d.]+)")
    lat0 = search(r"Latitude_of_Projection_Origin:\s*([-\d.]+)")
    false_easting = search(r"False_Easting:\s*([-\d.]+)", default=0.0)
    false_northing = search(r"False_Northing:\s*([-\d.]+)", default=0.0)
    cycle = search(r"Calendar_Date:\s*([A-Za-z]+ \d{1,2}, \d{4})", default="", cast=str)

    if west is None or east is None or north is None or south is None:
        raise ValueError(f"Could not parse bounds from metadata {path}")
    if len(std_parallels) < 2 or lon0 is None or lat0 is None:
        raise ValueError(f"Could not parse Lambert Conformal Conic projection from metadata {path}")

    return {
        "title": title.strip(),
        "bounds": {
            "westLon": west,
            "eastLon": east,
            "northLat": north,
            "southLat": south,
        },
        "projection": {
            "type": "lambert_conformal_conic",
            "stdParallel1": std_parallels[0],
            "stdParallel2": std_parallels[1],
            "lon0": lon0,
            "lat0": lat0,
            "falseEasting": false_easting,
            "falseNorthing": false_northing,
        },
        "cycle": cycle,
    }


def generate_levels(image_path: Path, output_dir: Path, max_dim: int, tile_size: int, jpeg_quality: int):
    with Image.open(image_path) as source_image:
        image = source_image.convert("RGB")
        width, height = image.size
        base_scale = min(1.0, max_dim / float(max(width, height)))

        scales = []
        current_scale = base_scale
        while current_scale >= max(base_scale / 4.0, 0.125):
            scales.append(current_scale)
            current_scale /= 2.0

        scales = sorted(set(round(scale, 6) for scale in scales))
        levels = []

        for index, scale in enumerate(scales):
            scaled_width = max(1, int(round(width * scale)))
            scaled_height = max(1, int(round(height * scale)))
            scaled_image = image if math.isclose(scale, 1.0) else image.resize(
                (scaled_width, scaled_height),
                Image.Resampling.LANCZOS,
            )

            level_id = f"l{index}"
            level_path = output_dir / "tiles" / level_id
            level_path.mkdir(parents=True, exist_ok=True)

            cols = math.ceil(scaled_width / tile_size)
            rows = math.ceil(scaled_height / tile_size)

            for row in range(rows):
                for col in range(cols):
                    left = col * tile_size
                    upper = row * tile_size
                    right = min(left + tile_size, scaled_width)
                    lower = min(upper + tile_size, scaled_height)
                    tile = scaled_image.crop((left, upper, right, lower))
                    tile.save(level_path / f"{col}_{row}.jpg", quality=jpeg_quality, optimize=True)

            levels.append({
                "id": level_id,
                "path": f"tiles/{level_id}",
                "format": "jpg",
                "scale": scale,
                "width": scaled_width,
                "height": scaled_height,
                "tileSize": tile_size,
                "rows": rows,
                "cols": cols,
                "minColumn": 0,
                "maxColumn": cols - 1,
                "minRow": 0,
                "maxRow": rows - 1,
            })

    return levels


def discover_faa_files(root: Path):
    tif = next(iter(sorted(root.rglob("*.tif"))), None)
    tfw = next(iter(sorted(root.rglob("*.tfw"))), None)
    metadata = next(iter(sorted(root.rglob("*.htm"))), None)
    if tif is None or metadata is None:
        raise FileNotFoundError("Expected FAA chart package to contain at least .tif and .htm files")
    return tif, tfw, metadata


def build_manifest(package_id: str, metadata: dict, world_file: dict, levels: list, title: str):
    return {
        "packageId": package_id,
        "title": title,
        "sourceKey": "faa_vfr",
        "sourceName": "FAA VFR",
        "packageType": "lcc_affine_tiles",
        "cycle": metadata.get("cycle", ""),
        "bounds": metadata["bounds"],
        "projection": metadata["projection"],
        "worldFile": world_file,
        "levels": levels,
    }


def import_faa_chart(input_path: Path, output_dir: Path, max_dim: int, tile_size: int, jpeg_quality: int):
    with TemporaryDirectory() as temp_dir:
        temp_root = Path(temp_dir)
        work_root = temp_root

        if input_path.suffix.lower() == ".zip":
            with zipfile.ZipFile(input_path) as archive:
                archive.extractall(work_root)
        else:
            work_root = input_path

        tif_path, tfw_path, metadata_path = discover_faa_files(work_root)
        world_file = parse_world_file(tfw_path) if tfw_path is not None else parse_world_file_from_geotiff(tif_path)
        metadata = parse_metadata(metadata_path)
        title = metadata["title"]
        package_id = slugify(title)
        package_root = output_dir / package_id

        if package_root.exists():
            shutil.rmtree(package_root)
        package_root.mkdir(parents=True, exist_ok=True)

        levels = generate_levels(tif_path, package_root, max_dim=max_dim, tile_size=tile_size, jpeg_quality=jpeg_quality)
        manifest = build_manifest(package_id, metadata, world_file, levels, title)

        (package_root / "manifest.json").write_text(json.dumps(manifest, indent=2))
        print(f"Imported FAA chart package: {package_root}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Import an FAA GeoTIFF chart ZIP into an MFD chart package")
    parser.add_argument("input_path", type=Path, help="FAA ZIP file or extracted directory")
    parser.add_argument("--output-dir", type=Path, default=Path("MFD/charts/packages"))
    parser.add_argument("--max-dim", type=int, default=8192, help="Maximum pixel dimension for the highest generated level")
    parser.add_argument("--tile-size", type=int, default=512)
    parser.add_argument("--jpeg-quality", type=int, default=90)
    args = parser.parse_args(argv)

    try:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        import_faa_chart(args.input_path, args.output_dir, args.max_dim, args.tile_size, args.jpeg_quality)
    except Exception as exc:
        print(f"FAA import failed: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
