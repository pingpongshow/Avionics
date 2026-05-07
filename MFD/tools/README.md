# MFD Tools

## GUI Tool

Launch the desktop data manager:

```bash
python3 MFD/tools/data_manager_gui.py
```

It provides:

- FAA aggregate source discovery
- FAA chart download/import into local chart packages
- national waypoint database rebuild from `FAA + openAIP`
- user waypoint editing in `user_waypoints.json`

## Current FAA Aggregate Sizes

Verified from FAA headers on `May 7, 2026`:

- `Sectional.zip`: `3,286,870,686` bytes (`3.06 GiB`)
- `Terminal.zip`: `848,665,346` bytes (`0.79 GiB`)
- `Helicopter.zip`: `287,262,636` bytes (`0.27 GiB`)

## CLI Tools

### FAA chart sync

```bash
python3 MFD/tools/sync_faa_vfr_charts.py --output-dir MFD/charts/packages
```

### Reference waypoint rebuild

```bash
python3 MFD/tools/import_reference_waypoints.py \
  --output MFD/data/reference_waypoints.json \
  --metadata-output MFD/data/reference_waypoints.metadata.json
```

