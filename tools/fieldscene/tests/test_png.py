import tempfile
import unittest
from pathlib import Path

from fieldscene.png import Image, draw_text, encode_png, read_png

REPO = Path(__file__).resolve().parents[3]


class PngTests(unittest.TestCase):
    def test_roundtrip(self):
        img = Image(7, 5)
        img.put(0, 0, (255, 0, 0, 255))
        img.put(6, 4, (0, 0, 255, 128))
        img.fill_rect(2, 1, 3, 3, (10, 20, 30, 255))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "x.png"
            path.write_bytes(encode_png(img))
            back = read_png(path)
        self.assertEqual((back.width, back.height), (7, 5))
        self.assertEqual(back.get(0, 0), (255, 0, 0, 255))
        self.assertEqual(back.get(6, 4), (0, 0, 255, 128))
        self.assertEqual(back.get(2, 1), (10, 20, 30, 255))

    def test_scaled_blit_and_text(self):
        img = Image(40, 20)
        src = Image(2, 2, (255, 255, 255, 255))
        img.blit_scaled(src, 0, 0, 8, 8)
        draw_text(img, 10, 5, "#123", (255, 255, 0, 255), 1)
        self.assertEqual(img.get(7, 7), (255, 255, 255, 255))
        self.assertEqual(img.get(39, 19), (0, 0, 0, 0))

    def test_screenshot_decoder_if_present(self):
        shot = REPO / "artifacts" / "field-verified.png"
        if not shot.exists():
            self.skipTest("no screenshot artifact")
        img = read_png(shot)
        self.assertGreater(img.width, 800)
        self.assertGreater(img.height, 600)


if __name__ == "__main__":
    unittest.main()
