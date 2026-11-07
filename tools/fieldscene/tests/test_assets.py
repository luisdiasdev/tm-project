import unittest
from pathlib import Path

from fieldscene.assets import AssetLibrary, decode_wyt

REPO = Path(__file__).resolve().parents[3]
WORKSPACE = REPO.parent
ASSETS = WORKSPACE / "client_748"


class AssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (ASSETS / "UI" / "UITextureList.bin").exists():
            raise unittest.SkipTest("client_748 assets not available")
        cls.lib = AssetLibrary(ASSETS)

    def test_texture_list(self):
        self.assertEqual(len(self.lib.texture_list), 512)
        self.assertTrue(self.lib.texture_list.filename(0).lower().endswith(".wyt"))

    def test_texture_sets(self):
        self.assertEqual(len(self.lib.set_list), 446)
        self.assertIn(456, self.lib.set_list.sets)
        self.assertTrue(any("198" in w for w in self.lib.set_list.warnings))

    def test_wyt_dimensions(self):
        img = self.lib.load_texture("UI/MainBox2.wyt")
        self.assertIsNotNone(img)
        self.assertEqual((img.width, img.height), (512, 256))
        self.assertEqual(len(img.data), 512 * 256 * 4)

    def test_frame_decode(self):
        # Set 0 is a real 748 set; its first frame must decode.
        info = self.lib.frame_info(0, 0, decode=True)
        self.assertIsNotNone(info)
        self.assertIsNotNone(info["image"])
        self.assertGreater(info["image"].width, 0)
        self.assertGreater(info["image"].height, 0)

    def test_negative_sets(self):
        self.assertIsNone(self.lib.resolved_set_index(-1))
        self.assertIsNone(self.lib.resolved_set_index(-2))
        self.assertEqual(self.lib.resolved_set_index(-456), 456)

    def test_bad_wyt_magic(self):
        with self.assertRaises(ValueError):
            decode_wyt(b"XXXX" + b"\0" * 64)


if __name__ == "__main__":
    unittest.main()
