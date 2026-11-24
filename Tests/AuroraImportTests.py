import hashlib
import sqlite3
import subprocess
import sys
import tempfile
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "Platform/Xbox360/tools/import_aurora.py"
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    src, dest = root / "Aurora", root / "AuroraX"
    dbdir = src / "Data/Databases"
    dbdir.mkdir(parents=True)
    content, settings = dbdir / "content.db", dbdir / "settings.db"
    with sqlite3.connect(content) as db:
        db.execute("CREATE TABLE ContentItems(Id INTEGER,TitleId INTEGER,TitleName TEXT)")
        db.executemany("INSERT INTO ContentItems VALUES(?,?,?)",
                       [(1, 0x4D5307E6, "Halo 3"), (2, 0x4D5307E6, "Duplicate"),
                        (3, -1, "Café\nGame"), (4, 0, "Invalid")])
    with sqlite3.connect(settings) as db:
        db.execute("CREATE TABLE ScanPaths(Id INTEGER,Path TEXT,DeviceId TEXT)")
        db.executemany("INSERT INTO ScanPaths VALUES(?,?,?)",
                       [(1, "\\Games\\", "disk"), (2, "\\games", "disk"),
                        (3, "\\..\\bad", "disk"), (4, "\\Other", "unknown")])
    with sqlite3.connect(content) as db:
        db.execute("ALTER TABLE ContentItems ADD COLUMN TestFavorite INTEGER")
        db.execute("ALTER TABLE ContentItems ADD COLUMN TestLast INTEGER")
        db.execute("UPDATE ContentItems SET TestFavorite=1, TestLast=100 WHERE Id=1")
        db.execute("UPDATE ContentItems SET TestFavorite=0, TestLast=300 WHERE Id=2")
        db.execute("UPDATE ContentItems SET TestFavorite=1, TestLast=200 WHERE Id=3")
        db.execute("UPDATE ContentItems SET TestFavorite=1, TestLast=999 WHERE Id=4")
    original = [hashlib.sha256(p.read_bytes()).digest() for p in (content, settings)]
    cmd = [sys.executable, str(script), str(src), str(dest), "--mount", "disk=HDD1"]
    preview = subprocess.run(cmd, check=True, capture_output=True, text=True)
    assert "unknown" in preview.stdout and not dest.exists()
    subprocess.run(cmd + ["--apply"], check=True)
    names = (dest / "aurorax-aurora-names.tsv").read_text(encoding="utf-8")
    assert names == "4D5307E6\tHalo 3\nFFFFFFFF\tCafé Game\n"
    assert (dest / "aurorax-aurora-paths.txt").read_text() == "HDD1:\\Games\n"
    assert subprocess.run(cmd + ["--apply"], capture_output=True).returncode != 0
    subprocess.run(cmd + ["--apply", "--replace"], check=True)
    assert not (dest / "aurorax-aurora-profile.tsv").exists()
    profile_cmd = cmd + ["--favorite-column", "TestFavorite", "--recent-column", "TestLast"]
    preview = subprocess.run(profile_cmd, check=True, capture_output=True, text=True)
    assert '"favorites": 2' in preview.stdout and not (dest / "aurorax-aurora-profile.tsv").exists()
    subprocess.run(profile_cmd + ["--apply", "--replace"], check=True)
    assert (dest / "aurorax-aurora-profile.tsv").read_text() == (
        "F\t4D5307E6\nF\tFFFFFFFF\nR\t4D5307E6\nR\tFFFFFFFF\n")
    assert subprocess.run(cmd + ["--favorite-column", "not_a_column"], capture_output=True).returncode != 0
    assert original == [hashlib.sha256(p.read_bytes()).digest() for p in (content, settings)]
