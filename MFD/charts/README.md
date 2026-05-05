# MFD Chart Packages

This directory holds local offline chart packages used by the `MFD` runtime.

The application searches this directory first during local development:

- `MFD/charts/packages/`

On Pi 5 / CM5 you can also override the charts directory with:

- `PFD_CHARTS_DIR=/path/to/charts/packages`

## Package Sources

The first supported import paths are:

- `FAA` VFR `GeoTIFF` ZIP packages
- `OpenFlightMaps` `MBTiles` packages

The runtime does not load raw FAA ZIPs or raw MBTiles files directly. The import scripts convert source files into a local chart-package structure that is stable for the embedded application.

## Local Workflow

1. Import an FAA ZIP:

```bash
python3 MFD/tools/import_faa_chart.py /path/to/faa_chart.zip --output-dir MFD/charts/packages
```

2. Import an OpenFlightMaps MBTiles package:

```bash
python3 MFD/tools/import_mbtiles_chart.py /path/to/ofm_chart.mbtiles --output-dir MFD/charts/packages
```

3. Launch the MFD app and select the chart source in `Setup`.

If the chart bounds cover the current ownship position, the map pane will switch from the placeholder preview to the imported chart package automatically.
