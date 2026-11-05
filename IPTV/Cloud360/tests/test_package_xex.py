import sys,tempfile,unittest
from pathlib import Path
from zipfile import ZipFile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from package_xex import package
class Packaging(unittest.TestCase):
    def test_bad_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); xex=root/"bad.xex"; xex.write_bytes(b"NOEX"*32)
            playlist=root/"demo.m3u";playlist.write_text("#EXTM3U\n")
            with self.assertRaises(ValueError):package(xex,root/"out.zip",playlist)
    def test_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);xex=root/"fixture.xex";xex.write_bytes(b"XEX2"+bytes(100))
            playlist=root/"demo.m3u";playlist.write_text("#EXTM3U\n")
            sample=root/"sample.h264";sample.write_bytes(b"\0\0\0\1\x67")
            out=package(xex,root/"out.zip",playlist,sample)
            with ZipFile(out) as z:self.assertEqual(set(z.namelist()),
                {"default.xex","playlist.m3u","sample.h264","LEIA-ME-TESTE.txt"})
if __name__=="__main__":unittest.main()
