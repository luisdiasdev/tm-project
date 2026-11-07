import unittest
from pathlib import Path

from fieldscene.scene import Scene, SceneError

REPO = Path(__file__).resolve().parents[3]
WORKSPACE = REPO.parent
ASSETS = WORKSPACE / "client_748"
REF = WORKSPACE / "client"


class SceneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.path = ASSETS / "UI" / "FieldScene2.bin"
        if not cls.path.exists():
            raise unittest.SkipTest("client_748 assets not available")
        cls.scene = Scene.load(cls.path)

    def test_counts_and_eof(self):
        self.assertEqual(len(self.scene), 709)
        self.assertEqual(len(self.scene.ids), 708)
        self.assertEqual(self.scene.path.read_bytes(), self.scene.to_bytes())

    def test_types(self):
        counts = self.scene.type_counts()
        self.assertEqual(counts["BUTTON"], 170)
        self.assertEqual(counts["TEXT"], 234)
        self.assertEqual(counts["PANEL"], 138)
        self.assertEqual(counts["GRID"], 125)

    def test_bounds(self):
        self.assertEqual(self.scene.bounds(), (-120, -60, 804, 600))

    def test_roundtrip_is_byte_identical(self):
        self.assertEqual(self.scene.to_bytes(), self.path.read_bytes())

    def test_edit_only_touches_serialized_body(self):
        import copy

        scene = Scene.load(self.path)
        target = scene.controls[0]
        original = target.x
        target.set("x", original + 7)
        data = scene.to_bytes()
        self.assertEqual(len(data), self.path.stat().st_size)
        reloaded = Scene.load_bytes(data, self.path)
        self.assertEqual(reloaded.controls[0].x, original + 7)
        # all other controls keep their original bytes
        for before, after in zip(scene.controls[1:], reloaded.controls[1:]):
            self.assertEqual(before.body, after.body)

    def test_755_scene_parses(self):
        ref = REF / "UI" / "FieldScene2.bin"
        if not ref.exists():
            self.skipTest("755 reference client not available")
        scene = Scene.load(ref)
        self.assertEqual(len(scene), 1338)

    def test_malformed_scene_is_rejected(self):
        data = self.path.read_bytes()[:20]
        with self.assertRaises(SceneError):
            Scene.load_bytes(data, self.path)


if __name__ == "__main__":
    unittest.main()
