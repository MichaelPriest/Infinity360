#!/usr/bin/env python3
"""Export Aurora metadata and scan roots to AuroraX, without modifying Aurora."""
import argparse
import json
import re
import sqlite3
from pathlib import Path

DEVICES = {"HDD1", "USB0", "USB1", "MU0", "MU1"}
MAX_NAMES = 4096
MAX_ROOTS = 32

def open_readonly(path):
    if not path.is_file():
        raise ValueError(f"Missing database: {path}")
    db = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    db.execute("PRAGMA query_only=ON")
    return db

def title_id(value):
    if isinstance(value, int) and -(2**31) <= value <= 0xffffffff:
        return value & 0xffffffff
    return 0

def clean_name(value):
    if not isinstance(value, str):
        return ""
    value = " ".join(value.split())
    value = "".join(c for c in value if ord(c) >= 32 and ord(c) != 127)
    return value.encode("utf-8")[:240].decode("utf-8", errors="ignore").strip()

def mount_path(device, path):
    if not isinstance(path, str) or not path or device not in DEVICES:
        return None
    path = path.replace("/", "\\").strip("\\")
    parts = path.split("\\")
    if any(not p or p in (".", "..") or p != p.strip() or
           any(ord(c) < 32 or ord(c) == 127 or c in ':*?"|<>' for c in p)
           for p in parts):
        return None
    result = device + ":\\" + "\\".join(parts)
    return result if len(result.encode("utf-8")) <= 220 else None

def export_profile(aurora, favorite_column, recent_column):
    """Only use columns explicitly confirmed by the operator. No schema guesses."""
    with open_readonly(aurora / "Data/Databases/content.db") as db:
        columns = [row[1] for row in db.execute("PRAGMA table_info(ContentItems)")]
        selected = [c for c in (favorite_column, recent_column) if c]
        for col in selected:
            if col not in columns:
                raise ValueError(f"Unknown ContentItems column: {col}; available: {columns}")
        if not selected:
            return columns, [], []
        # Identifier quoting after exact schema membership validation.
        def quoted(col):
            return '"' + col.replace('"', '""') + '"'
        fav_expr = quoted(favorite_column) if favorite_column else "NULL"
        recent_expr = quoted(recent_column) if recent_column else "NULL"
        favorites, recent = set(), {}
        for tid, fav, last in db.execute(
            f"SELECT TitleId,{fav_expr},{recent_expr} FROM ContentItems ORDER BY Id LIMIT 20000"
        ):
            tid = title_id(tid)
            if not tid:
                continue
            # Caller confirms an integer Boolean and a positive integer
            # monotonic timestamp; unknown or unsupported values are ignored.
            if type(fav) is int and fav == 1:
                favorites.add(tid)
            if type(last) is int and last > 0:
                recent[tid] = max(last, recent.get(tid, 0))
        return columns, sorted(favorites)[:4096], sorted(recent, key=lambda tid: (-recent[tid], tid))[:4096]

def export(aurora, mounts):
    dbdir = aurora / "Data" / "Databases"
    names, paths, skipped = {}, {}, set()
    with open_readonly(dbdir / "content.db") as db:
        for tid, name in db.execute("SELECT TitleId, TitleName FROM ContentItems ORDER BY Id LIMIT 20000"):
            tid, name = title_id(tid), clean_name(name)
            if tid and name and len(names) < MAX_NAMES:
                names.setdefault(f"{tid:08X}", name)
    with open_readonly(dbdir / "settings.db") as db:
        for path, device_id in db.execute("SELECT Path, DeviceId FROM ScanPaths ORDER BY Id LIMIT 4096"):
            device = mounts.get(str(device_id))
            if not device:
                skipped.add(str(device_id))
                continue
            root = mount_path(device, path)
            if root and len(paths) < MAX_ROOTS:
                paths.setdefault(root.casefold(), root)
    return names, list(paths.values()), sorted(skipped)

def write_new(path, data, replace):
    # Export into separate files; replacement always explicit.
    if not replace:
        with path.open("x", encoding="utf-8", newline="\n") as out:
            out.write(data)
    else:
        temp = path.with_name(path.name + ".import-tmp")
        with temp.open("x", encoding="utf-8", newline="\n") as out:
            out.write(data)
        temp.replace(path)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("aurora", type=Path, help="Local copy of original Aurora directory")
    p.add_argument("aurorax", type=Path, help="Separate AuroraX directory")
    p.add_argument("--mount", action="append", default=[], metavar="DEVICE_ID=HDD1")
    p.add_argument("--favorite-column", help="Confirmed ContentItems integer boolean column (1=favorite)")
    p.add_argument("--recent-column", help="Confirmed ContentItems integer timestamp column (positive, higher=newer)")
    p.add_argument("--apply", action="store_true")
    p.add_argument("--replace", action="store_true", help="Replace previous exports only")
    args = p.parse_args()
    if args.aurora.resolve() == args.aurorax.resolve():
        p.error("Aurora and AuroraX must be different directories")
    mounts = {}
    for entry in args.mount:
        key, sep, val = entry.rpartition("=")
        val = val.upper()
        if not sep or not key or val not in DEVICES:
            p.error("Use --mount DEVICE_ID=HDD1 (or USB0, USB1, MU0, MU1)")
        mounts[key] = val
    try:
        names, roots, skipped = export(args.aurora, mounts)
        columns, favorites, recent = export_profile(args.aurora, args.favorite_column, args.recent_column)
        print(json.dumps({"titles": len(names), "roots": roots,
                          "content_columns": columns, "favorites": len(favorites),
                          "history": len(recent),
                          "unmapped_device_ids": skipped, "apply": args.apply},
                         ensure_ascii=False, indent=2))
        if args.apply:
            args.aurorax.mkdir(parents=True, exist_ok=True)
            outputs = {
                "aurorax-aurora-names.tsv": "".join(f"{k}\t{v}\n" for k, v in sorted(names.items())),
                "aurorax-aurora-paths.txt": "".join(f"{r}\n" for r in roots),
            }
            if args.favorite_column or args.recent_column:
                outputs["aurorax-aurora-profile.tsv"] = (
                    "".join(f"F\t{tid:08X}\n" for tid in favorites)
                    + "".join(f"R\t{tid:08X}\n" for tid in recent)
                )
            if not args.replace and any((args.aurorax / f).exists() for f in outputs):
                p.error("Export already exists; use --replace explicitly")
            for file, data in outputs.items():
                write_new(args.aurorax / file, data, args.replace)
    except (OSError, ValueError, sqlite3.Error) as exc:
        p.error(str(exc))

if __name__ == "__main__":
    main()
