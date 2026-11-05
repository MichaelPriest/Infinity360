import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from xtream_export import make_playlist

class XtreamTests(unittest.TestCase):
    def test_live_and_vod(self):
        data, count = make_playlist(
            "https://example.org", "demo", "secret",
            [{"category_id": "10", "category_name": "Educacao"}],
            [{"stream_id": 123, "name": "Canal Aula", "category_id": "10"}],
            [{"category_id": "20", "category_name": "Curtas"}],
            [{"stream_id": 456, "name": "Curta Demo", "category_id": "20", "container_extension": "mp4"}])
        self.assertEqual(count, 2)
        self.assertIn('group-title="Educacao"', data)
        self.assertIn('group-title="Filmes / Curtas"', data)
        self.assertIn("/live/demo/secret/123.ts", data)
        self.assertIn("/movie/demo/secret/456.mp4", data)

if __name__ == "__main__":
    unittest.main()
