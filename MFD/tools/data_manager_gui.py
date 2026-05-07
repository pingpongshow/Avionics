#!/usr/bin/env python3

import json
import os
import queue
import sys
import threading
from pathlib import Path
from tkinter import BOTH, END, LEFT, RIGHT, VERTICAL, W, filedialog, messagebox, simpledialog
import tkinter as tk
from tkinter import ttk

import requests

from import_reference_waypoints import parse_args as parse_waypoint_import_args
from import_reference_waypoints import main as rebuild_reference_waypoints_main
from sync_faa_vfr_charts import (
    discover_current_visual_urls,
    download_if_missing,
    import_archive,
)


ROOT_DIR = Path(__file__).resolve().parents[1]
DEFAULT_CHART_OUTPUT = ROOT_DIR / "charts" / "packages"
DEFAULT_CHART_CACHE = Path.home() / ".cache" / "experimental-pfd" / "faa_vfr"
DEFAULT_REFERENCE_DB = ROOT_DIR / "data" / "reference_waypoints.json"
DEFAULT_REFERENCE_METADATA = ROOT_DIR / "data" / "reference_waypoints.metadata.json"
DEFAULT_DATA_DIR = Path(os.environ.get("PFD_DATA_DIR", str(ROOT_DIR / "data" / "runtime_db")))
FAA_SETS = ("sectional", "terminal", "helicopter")


def human_size(num_bytes: int) -> str:
    if num_bytes <= 0:
        return "0 B"
    step = 1024.0
    units = ["B", "KiB", "MiB", "GiB", "TiB"]
    size = float(num_bytes)
    for unit in units:
        if size < step or unit == units[-1]:
            return f"{size:.2f} {unit}"
        size /= step
    return f"{num_bytes} B"


def http_head_metadata(url: str) -> dict:
    response = requests.head(url, timeout=60, allow_redirects=True)
    response.raise_for_status()
    return {
        "url": response.url,
        "content_length": int(response.headers.get("Content-Length", "0") or "0"),
        "last_modified": response.headers.get("Last-Modified", ""),
    }


def load_json_array(path: Path) -> list:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    return data if isinstance(data, list) else []


def save_json_array(path: Path, items: list) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(items, handle, indent=2, sort_keys=False)


class DataManagerApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Experimental PFD Data Manager")
        self.geometry("1380x920")
        self.minsize(1180, 760)

        self.log_queue: "queue.Queue[str]" = queue.Queue()
        self.reference_index = []
        self.search_results = []

        self.chart_output_var = tk.StringVar(value=str(DEFAULT_CHART_OUTPUT))
        self.chart_cache_var = tk.StringVar(value=str(DEFAULT_CHART_CACHE))
        self.reference_db_var = tk.StringVar(value=str(DEFAULT_REFERENCE_DB))
        self.reference_metadata_var = tk.StringVar(value=str(DEFAULT_REFERENCE_METADATA))
        self.data_dir_var = tk.StringVar(value=str(DEFAULT_DATA_DIR))
        self.country_var = tk.StringVar(value="us")
        self.chart_pattern_var = tk.StringVar(value="")
        self.chart_max_var = tk.StringVar(value="0")
        self.search_var = tk.StringVar(value="")

        self.source_rows = {}
        self.selected_user_waypoint_ident = None

        self._build_ui()
        self.after(150, self._drain_log_queue)
        self.refresh_chart_sources()
        self.reload_user_waypoints()

    def _build_ui(self) -> None:
        notebook = ttk.Notebook(self)
        notebook.pack(fill=BOTH, expand=True, padx=10, pady=10)

        charts_tab = ttk.Frame(notebook)
        waypoints_tab = ttk.Frame(notebook)
        notebook.add(charts_tab, text="FAA Charts")
        notebook.add(waypoints_tab, text="Waypoints")

        self._build_charts_tab(charts_tab)
        self._build_waypoints_tab(waypoints_tab)

    def _build_charts_tab(self, parent: ttk.Frame) -> None:
        options = ttk.LabelFrame(parent, text="FAA VFR Sources")
        options.pack(fill="x", padx=8, pady=8)

        ttk.Label(options, text="Chart package output").grid(row=0, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(options, textvariable=self.chart_output_var, width=72).grid(row=0, column=1, sticky="ew", padx=6, pady=4)
        ttk.Button(options, text="Browse", command=lambda: self._choose_directory(self.chart_output_var)).grid(row=0, column=2, padx=6, pady=4)

        ttk.Label(options, text="Download cache").grid(row=1, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(options, textvariable=self.chart_cache_var, width=72).grid(row=1, column=1, sticky="ew", padx=6, pady=4)
        ttk.Button(options, text="Browse", command=lambda: self._choose_directory(self.chart_cache_var)).grid(row=1, column=2, padx=6, pady=4)

        ttk.Label(options, text="Chart name filter").grid(row=2, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(options, textvariable=self.chart_pattern_var, width=32).grid(row=2, column=1, sticky=W, padx=6, pady=4)

        ttk.Label(options, text="Max charts per set (0 = all)").grid(row=2, column=1, sticky="e", padx=140, pady=4)
        ttk.Entry(options, textvariable=self.chart_max_var, width=8).grid(row=2, column=2, sticky=W, padx=6, pady=4)

        options.columnconfigure(1, weight=1)

        toolbar = ttk.Frame(parent)
        toolbar.pack(fill="x", padx=8, pady=(0, 8))
        ttk.Button(toolbar, text="Refresh Sources", command=self.refresh_chart_sources).pack(side=LEFT, padx=4)
        ttk.Button(toolbar, text="Download Selected", command=self.download_selected_chart_set).pack(side=LEFT, padx=4)
        ttk.Button(toolbar, text="Import Selected", command=self.import_selected_chart_set).pack(side=LEFT, padx=4)
        ttk.Button(toolbar, text="Sync All Visible Sets", command=self.sync_all_chart_sets).pack(side=LEFT, padx=4)

        columns = ("set_name", "size", "last_modified", "status", "url")
        tree_frame = ttk.Frame(parent)
        tree_frame.pack(fill=BOTH, expand=True, padx=8, pady=(0, 8))
        self.chart_tree = ttk.Treeview(tree_frame, columns=columns, show="headings", height=8)
        for column, heading, width in (
            ("set_name", "Chart Set", 110),
            ("size", "Aggregate Size", 120),
            ("last_modified", "Last Modified", 220),
            ("status", "Local Status", 200),
            ("url", "URL", 760),
        ):
            self.chart_tree.heading(column, text=heading)
            self.chart_tree.column(column, width=width, anchor=W)
        self.chart_tree.pack(side=LEFT, fill=BOTH, expand=True)
        tree_scroll = ttk.Scrollbar(tree_frame, orient=VERTICAL, command=self.chart_tree.yview)
        tree_scroll.pack(side=RIGHT, fill="y")
        self.chart_tree.configure(yscrollcommand=tree_scroll.set)

        log_box = ttk.LabelFrame(parent, text="Activity Log")
        log_box.pack(fill=BOTH, expand=False, padx=8, pady=(0, 8))
        self.log_text = tk.Text(log_box, height=14, wrap="word")
        self.log_text.pack(fill=BOTH, expand=True)

    def _build_waypoints_tab(self, parent: ttk.Frame) -> None:
        top = ttk.LabelFrame(parent, text="Waypoint Databases")
        top.pack(fill="x", padx=8, pady=8)

        ttk.Label(top, text="Reference database").grid(row=0, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(top, textvariable=self.reference_db_var, width=72).grid(row=0, column=1, sticky="ew", padx=6, pady=4)
        ttk.Button(top, text="Browse", command=lambda: self._choose_file(self.reference_db_var, save=False)).grid(row=0, column=2, padx=6, pady=4)

        ttk.Label(top, text="Reference metadata").grid(row=1, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(top, textvariable=self.reference_metadata_var, width=72).grid(row=1, column=1, sticky="ew", padx=6, pady=4)
        ttk.Button(top, text="Browse", command=lambda: self._choose_file(self.reference_metadata_var, save=True)).grid(row=1, column=2, padx=6, pady=4)

        ttk.Label(top, text="Runtime data dir").grid(row=2, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(top, textvariable=self.data_dir_var, width=72).grid(row=2, column=1, sticky="ew", padx=6, pady=4)
        ttk.Button(top, text="Browse", command=lambda: self._choose_directory(self.data_dir_var)).grid(row=2, column=2, padx=6, pady=4)

        ttk.Label(top, text="openAIP country").grid(row=3, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(top, textvariable=self.country_var, width=8).grid(row=3, column=1, sticky=W, padx=6, pady=4)
        ttk.Button(top, text="Rebuild FAA + openAIP Database", command=self.rebuild_reference_database).grid(row=3, column=2, padx=6, pady=4)

        top.columnconfigure(1, weight=1)

        search_box = ttk.LabelFrame(parent, text="Search Reference Database")
        search_box.pack(fill=BOTH, expand=False, padx=8, pady=(0, 8))

        ttk.Label(search_box, text="Search").grid(row=0, column=0, sticky=W, padx=6, pady=4)
        ttk.Entry(search_box, textvariable=self.search_var, width=28).grid(row=0, column=1, sticky=W, padx=6, pady=4)
        ttk.Button(search_box, text="Load / Search", command=self.search_reference_database).grid(row=0, column=2, padx=6, pady=4)

        self.search_results_tree = ttk.Treeview(
            search_box,
            columns=("ident", "name", "type", "lat", "lon", "source"),
            show="headings",
            height=8,
        )
        for column, heading, width in (
            ("ident", "Ident", 90),
            ("name", "Name", 320),
            ("type", "Type", 120),
            ("lat", "Lat", 120),
            ("lon", "Lon", 120),
            ("source", "Source", 120),
        ):
            self.search_results_tree.heading(column, text=heading)
            self.search_results_tree.column(column, width=width, anchor=W)
        self.search_results_tree.grid(row=1, column=0, columnspan=3, sticky="nsew", padx=6, pady=6)
        search_box.columnconfigure(1, weight=1)
        search_box.rowconfigure(1, weight=1)

        user_box = ttk.LabelFrame(parent, text="User Waypoints")
        user_box.pack(fill=BOTH, expand=True, padx=8, pady=(0, 8))

        left = ttk.Frame(user_box)
        left.pack(side=LEFT, fill=BOTH, expand=True, padx=6, pady=6)
        self.user_waypoints_tree = ttk.Treeview(
            left,
            columns=("ident", "name", "lat", "lon", "notes"),
            show="headings",
            height=12,
        )
        for column, heading, width in (
            ("ident", "Ident", 90),
            ("name", "Name", 220),
            ("lat", "Lat", 100),
            ("lon", "Lon", 100),
            ("notes", "Notes", 240),
        ):
            self.user_waypoints_tree.heading(column, text=heading)
            self.user_waypoints_tree.column(column, width=width, anchor=W)
        self.user_waypoints_tree.pack(fill=BOTH, expand=True)
        self.user_waypoints_tree.bind("<<TreeviewSelect>>", self.on_user_waypoint_select)

        right = ttk.Frame(user_box)
        right.pack(side=RIGHT, fill="y", padx=6, pady=6)

        self.user_ident_var = tk.StringVar(value="")
        self.user_name_var = tk.StringVar(value="")
        self.user_lat_var = tk.StringVar(value="")
        self.user_lon_var = tk.StringVar(value="")
        self.user_notes_var = tk.StringVar(value="")

        for row, (label, variable) in enumerate((
            ("Ident", self.user_ident_var),
            ("Name", self.user_name_var),
            ("Latitude", self.user_lat_var),
            ("Longitude", self.user_lon_var),
            ("Notes", self.user_notes_var),
        )):
            ttk.Label(right, text=label).grid(row=row, column=0, sticky=W, padx=4, pady=4)
            ttk.Entry(right, textvariable=variable, width=28).grid(row=row, column=1, sticky="ew", padx=4, pady=4)

        ttk.Button(right, text="New", command=self.clear_user_waypoint_form).grid(row=6, column=0, padx=4, pady=8, sticky="ew")
        ttk.Button(right, text="Save / Update", command=self.save_user_waypoint).grid(row=6, column=1, padx=4, pady=8, sticky="ew")
        ttk.Button(right, text="Delete Selected", command=self.delete_user_waypoint).grid(row=7, column=0, padx=4, pady=4, sticky="ew")
        ttk.Button(right, text="Reload", command=self.reload_user_waypoints).grid(row=7, column=1, padx=4, pady=4, sticky="ew")

    def _append_log(self, text: str) -> None:
        self.log_text.insert(END, text.rstrip() + "\n")
        self.log_text.see(END)

    def _drain_log_queue(self) -> None:
        while True:
            try:
                message = self.log_queue.get_nowait()
            except queue.Empty:
                break
            self._append_log(message)
        self.after(150, self._drain_log_queue)

    def log(self, message: str) -> None:
        self.log_queue.put(message)

    def run_background(self, title: str, func) -> None:
        def worker() -> None:
            try:
                self.log(f"[START] {title}")
                func()
                self.log(f"[DONE]  {title}")
            except Exception as exc:  # noqa: BLE001
                self.log(f"[ERROR] {title}: {exc}")

        threading.Thread(target=worker, daemon=True).start()

    def _choose_directory(self, variable: tk.StringVar) -> None:
        initial = Path(variable.get()).expanduser()
        chosen = filedialog.askdirectory(initialdir=str(initial if initial.exists() else ROOT_DIR))
        if chosen:
            variable.set(chosen)

    def _choose_file(self, variable: tk.StringVar, save: bool) -> None:
        initial = Path(variable.get()).expanduser()
        if save:
            chosen = filedialog.asksaveasfilename(initialdir=str(initial.parent if initial.parent.exists() else ROOT_DIR), initialfile=initial.name)
        else:
            chosen = filedialog.askopenfilename(initialdir=str(initial.parent if initial.parent.exists() else ROOT_DIR))
        if chosen:
            variable.set(chosen)

    def refresh_chart_sources(self) -> None:
        def task() -> None:
            urls = discover_current_visual_urls()
            rows = {}
            cache_dir = Path(self.chart_cache_var.get()).expanduser()
            for set_name in FAA_SETS:
                url = urls[set_name]
                metadata = http_head_metadata(url)
                local_path = cache_dir / Path(metadata["url"]).name
                status = "not downloaded"
                if local_path.exists():
                    status = f"cached ({human_size(local_path.stat().st_size)})"
                rows[set_name] = {
                    "url": metadata["url"],
                    "content_length": metadata["content_length"],
                    "last_modified": metadata["last_modified"],
                    "status": status,
                    "local_path": local_path,
                }
                self.log(
                    f"{set_name}: {human_size(metadata['content_length'])}  |  {metadata['last_modified']}  |  {metadata['url']}"
                )

            def apply_rows() -> None:
                self.source_rows = rows
                self.chart_tree.delete(*self.chart_tree.get_children())
                for set_name in FAA_SETS:
                    row = rows[set_name]
                    self.chart_tree.insert(
                        "",
                        END,
                        iid=set_name,
                        values=(
                            set_name.title(),
                            human_size(row["content_length"]),
                            row["last_modified"],
                            row["status"],
                            row["url"],
                        ),
                    )

            self.after(0, apply_rows)

        self.run_background("Refresh FAA chart sources", task)

    def selected_chart_set(self) -> str | None:
        selection = self.chart_tree.selection()
        if not selection:
            messagebox.showinfo("FAA Charts", "Select a chart set first.")
            return None
        return str(selection[0])

    def download_selected_chart_set(self) -> None:
        set_name = self.selected_chart_set()
        if not set_name:
            return

        def task() -> None:
            row = self.source_rows[set_name]
            local_path = download_if_missing(row["url"], row["local_path"])
            self.log(f"{set_name}: downloaded to {local_path}")
            self.after(0, self.refresh_chart_sources)

        self.run_background(f"Download FAA {set_name}", task)

    def import_selected_chart_set(self) -> None:
        set_name = self.selected_chart_set()
        if not set_name:
            return

        def task() -> None:
            row = self.source_rows[set_name]
            local_path = download_if_missing(row["url"], row["local_path"])
            imported = import_archive(
                local_path,
                Path(self.chart_output_var.get()).expanduser(),
                self.chart_pattern_var.get().strip(),
                int(self.chart_max_var.get() or "0"),
                8192,
                512,
                90,
            )
            self.log(f"{set_name}: imported {imported} chart packages from {local_path}")

        self.run_background(f"Import FAA {set_name}", task)

    def sync_all_chart_sets(self) -> None:
        def task() -> None:
            output_dir = Path(self.chart_output_var.get()).expanduser()
            cache_dir = Path(self.chart_cache_var.get()).expanduser()
            pattern = self.chart_pattern_var.get().strip()
            max_charts = int(self.chart_max_var.get() or "0")
            for set_name in FAA_SETS:
                row = self.source_rows[set_name]
                local_path = download_if_missing(row["url"], cache_dir / Path(row["url"]).name)
                imported = import_archive(local_path, output_dir, pattern, max_charts, 8192, 512, 90)
                self.log(f"{set_name}: imported {imported} chart packages")

        self.run_background("Sync all FAA chart sets", task)

    def rebuild_reference_database(self) -> None:
        def task() -> None:
            args = parse_waypoint_import_args([])
            args.output = Path(self.reference_db_var.get()).expanduser()
            args.metadata_output = Path(self.reference_metadata_var.get()).expanduser()
            args.country = self.country_var.get().strip() or "us"
            args.no_openaip = False
            result = rebuild_reference_waypoints_main([
                "--output",
                str(args.output),
                "--metadata-output",
                str(args.metadata_output),
                "--country",
                args.country,
            ])
            if result != 0:
                raise RuntimeError("Reference waypoint rebuild failed")
            self.log(f"Reference database rebuilt: {args.output}")
            self.log(f"Metadata written: {args.metadata_output}")
            self.reference_index = []

        self.run_background("Rebuild FAA/openAIP waypoint database", task)

    def search_reference_database(self) -> None:
        reference_path = Path(self.reference_db_var.get()).expanduser()
        if not self.reference_index:
            try:
                with reference_path.open("r", encoding="utf-8") as handle:
                    data = json.load(handle)
                if not isinstance(data, list):
                    raise ValueError("Reference database is not a JSON array")
                self.reference_index = data
                self.log(f"Loaded reference database: {reference_path} ({len(data)} waypoints)")
            except Exception as exc:  # noqa: BLE001
                messagebox.showerror("Reference DB", f"Could not load {reference_path}\n\n{exc}")
                return

        query = self.search_var.get().strip().upper()
        results = []
        for item in self.reference_index:
            ident = str(item.get("ident", "")).upper()
            name = str(item.get("name", "")).upper()
            aliases = [str(alias).upper() for alias in item.get("aliases", [])]
            if not query or query in ident or query in name or any(query in alias for alias in aliases):
                results.append(item)
            if len(results) >= 250:
                break

        self.search_results = results
        self.search_results_tree.delete(*self.search_results_tree.get_children())
        for index, item in enumerate(results):
            self.search_results_tree.insert(
                "",
                END,
                iid=str(index),
                values=(
                    item.get("ident", ""),
                    item.get("name", ""),
                    item.get("symbolType", item.get("type", "")),
                    item.get("latitudeDeg", ""),
                    item.get("longitudeDeg", ""),
                    ",".join(item.get("sources", [item.get("source", "")])),
                ),
            )

    def user_waypoints_path(self) -> Path:
        return Path(self.data_dir_var.get()).expanduser() / "user_waypoints.json"

    def reload_user_waypoints(self) -> None:
        path = self.user_waypoints_path()
        items = load_json_array(path)
        self.user_waypoints_tree.delete(*self.user_waypoints_tree.get_children())
        for item in items:
            ident = str(item.get("ident", "")).upper()
            self.user_waypoints_tree.insert(
                "",
                END,
                iid=ident,
                values=(
                    ident,
                    item.get("name", ""),
                    item.get("latitudeDeg", ""),
                    item.get("longitudeDeg", ""),
                    item.get("notes", ""),
                ),
            )
        self.clear_user_waypoint_form()
        self.log(f"Loaded user waypoints: {path} ({len(items)} entries)")

    def current_user_waypoints(self) -> list:
        return load_json_array(self.user_waypoints_path())

    def on_user_waypoint_select(self, _event=None) -> None:
        selection = self.user_waypoints_tree.selection()
        if not selection:
            return
        ident = str(selection[0])
        self.selected_user_waypoint_ident = ident
        for item in self.current_user_waypoints():
            if str(item.get("ident", "")).upper() == ident:
                self.user_ident_var.set(str(item.get("ident", "")))
                self.user_name_var.set(str(item.get("name", "")))
                self.user_lat_var.set(str(item.get("latitudeDeg", "")))
                self.user_lon_var.set(str(item.get("longitudeDeg", "")))
                self.user_notes_var.set(str(item.get("notes", "")))
                break

    def clear_user_waypoint_form(self) -> None:
        self.selected_user_waypoint_ident = None
        self.user_ident_var.set("")
        self.user_name_var.set("")
        self.user_lat_var.set("")
        self.user_lon_var.set("")
        self.user_notes_var.set("")

    def save_user_waypoint(self) -> None:
        ident = self.user_ident_var.get().strip().upper()
        name = self.user_name_var.get().strip() or ident
        notes = self.user_notes_var.get().strip()
        if not ident:
            messagebox.showerror("User Waypoint", "Ident is required.")
            return

        try:
            latitude_deg = float(self.user_lat_var.get().strip())
            longitude_deg = float(self.user_lon_var.get().strip())
        except ValueError:
            messagebox.showerror("User Waypoint", "Latitude and Longitude must be valid numbers.")
            return

        items = self.current_user_waypoints()
        replaced = False
        for item in items:
            if str(item.get("ident", "")).upper() == ident:
                item.update({
                    "ident": ident,
                    "name": name,
                    "type": "user_waypoint",
                    "latitudeDeg": latitude_deg,
                    "longitudeDeg": longitude_deg,
                    "notes": notes,
                })
                replaced = True
                break

        if not replaced:
            items.append({
                "ident": ident,
                "name": name,
                "type": "user_waypoint",
                "latitudeDeg": latitude_deg,
                "longitudeDeg": longitude_deg,
                "notes": notes,
            })

        save_json_array(self.user_waypoints_path(), items)
        self.reload_user_waypoints()

    def delete_user_waypoint(self) -> None:
        selection = self.user_waypoints_tree.selection()
        if not selection:
            messagebox.showinfo("User Waypoint", "Select a user waypoint first.")
            return
        ident = str(selection[0]).upper()
        if not messagebox.askyesno("Delete User Waypoint", f"Delete {ident}?"):
            return

        items = [item for item in self.current_user_waypoints() if str(item.get("ident", "")).upper() != ident]
        save_json_array(self.user_waypoints_path(), items)
        self.reload_user_waypoints()


def main() -> int:
    app = DataManagerApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
