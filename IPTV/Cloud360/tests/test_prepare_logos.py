import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from prepare_logos import logo_entries

class LogoPreparationTests(unittest.TestCase):
  def test_playlist_indices(self):
    sample = '#EXTM3U\n#EXTINF:-1 tvg-logo="https://example.org/a.png",Canal A\nhttps://example.org/a.m3u8\n#EXTINF:-1,Canal B\nhttps://example.org/b.ts\n#EXTINF:-1 tvg-logo="https://example.org/c.png",Canal C\nhttps://example.org/c.ts\n'
    self.assertEqual(list(logo_entries(sample)), [(0,"https://example.org/a.png"),(1,""),(2,"https://example.org/c.png")])
if __name__=="__main__": unittest.main()
