"""Portable tests for packaging a real XDK-generated XEX (no compiler required)."""
import importlib.util
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

SCRIPT = Path(__file__).resolve().parents[1] / "Platform" / "Xbox360" / "tools" / "package_alpha.py"
SPEC = importlib.util.spec_from_file_location("package_alpha", SCRIPT)
PACKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKER)

class PackageAlphaTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.xex = self.root / "default.xex"
        self.art = self.root / "home.tga"
        self.out = self.root / "AuroraX-Alpha.zip"
        # Packaging fixture only; this is NOT a working Xbox executable.
        self.xex.write_bytes(b"XEX2" + bytes(8192))
        head = bytearray(18)
        head[2] = 2    # TGA uncompressed true color.
        head[12:14] = (1280).to_bytes(2, "little")
        head[14:16] = (720).to_bytes(2, "little")
        head[16] = 32
        self.art.write_bytes(bytes(head) + bytes(128))

    def test_package_contains_required_files(self):
        PACKER.package(self.xex, self.art, self.out)
        with ZipFile(self.out) as archive:
            self.assertEqual(set(archive.namelist()), {
                "default.xex", "home.tga", "LEIA-ME-ALPHA.txt", "MANIFEST.sha256", "VERSION.txt"
            })
            self.assertIsNone(archive.testzip())
            self.assertEqual(archive.read("VERSION.txt").decode().strip(), PACKER.VERSION)
            self.assertIn(PACKER.VERSION, archive.read("LEIA-ME-ALPHA.txt").decode())
            self.assertIn(PACKER.VERSION, PACKER.ARCHIVE_NAME)
            self.assertIn(PACKER.sha256(self.xex), archive.read("MANIFEST.sha256").decode())
            self.assertIn(PACKER.sha256(self.art), archive.read("MANIFEST.sha256").decode())

    def test_rejects_non_xex(self):
        self.xex.write_bytes(b"ELF!" + bytes(8192))
        with self.assertRaisesRegex(ValueError, "XEX2"):
            PACKER.package(self.xex, self.art, self.out)
        self.assertFalse(self.out.exists())

    def test_rejects_missing_or_invalid_atlas(self):
        self.art.write_bytes(b"not TGA")
        with self.assertRaises(ValueError):
            PACKER.package(self.xex, self.art, self.out)
        self.assertFalse(self.out.exists())

if __name__ == "__main__":
    unittest.main()
