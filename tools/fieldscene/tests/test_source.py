import unittest
from pathlib import Path

from fieldscene.controls import Constants
from fieldscene.source import scan_source

REPO = Path(__file__).resolve().parents[3]
HEADER = REPO / "Projects" / "TMProject" / "ResourceControl.h"
SOURCE = REPO / "Projects" / "TMProject" / "TMFieldScene.cpp"


class SourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (HEADER.exists() and SOURCE.exists()):
            raise unittest.SkipTest("port sources not available")
        cls.constants = Constants(HEADER)
        cls.scan = scan_source(SOURCE, cls.constants)

    def test_non_utf8_roundtrip(self):
        self.assertEqual(self.scan.roundtrip_bytes(), SOURCE.read_bytes())
        self.assertIn("\xA8", self.scan.text)

    def test_known_binding_is_resolved(self):
        refs = [r for r in self.scan.refs if r.token == "TML_CHATLIST"]
        self.assertTrue(refs)
        self.assertEqual(refs[0].value, 5377)
        self.assertEqual(refs[0].expected_type, "LISTBOX")

    def test_known_missing_binding(self):
        refs = [r for r in self.scan.refs if r.value == 65775]
        self.assertTrue(refs)
        self.assertTrue(all(r.kind == "findcontrol" for r in refs))

    def test_computed_ranges_are_detected(self):
        computed = [r for r in self.scan.refs if r.computed]
        self.assertTrue(computed)
        bases = {r.computed_base for r in computed}
        self.assertIn(65574, bases)
        self.assertIn(94208, bases)

    def test_function_context(self):
        refs = [r for r in self.scan.refs if r.function == "TMFieldScene::InitializeScene"]
        self.assertGreater(len(refs), 100)

    def test_constants(self):
        self.assertEqual(self.constants.resolve("TMB_CHAR"), 293)
        self.assertEqual(self.constants.resolve("TMG_INV_GRID"), 337)
        self.assertGreater(len(self.constants), 2000)


if __name__ == "__main__":
    unittest.main()
