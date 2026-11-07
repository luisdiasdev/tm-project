import unittest
from pathlib import Path

from fieldscene.assets import AssetLibrary
from fieldscene.audit import build_audit, load_known
from fieldscene.controls import Constants
from fieldscene.scene import Scene
from fieldscene.source import scan_source

REPO = Path(__file__).resolve().parents[3]
WORKSPACE = REPO.parent
ASSETS = WORKSPACE / "client_748"
REF = WORKSPACE / "client"
HEADER = REPO / "Projects" / "TMProject" / "ResourceControl.h"
SOURCE = REPO / "Projects" / "TMProject" / "TMFieldScene.cpp"


class AuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        scene_path = ASSETS / "UI" / "FieldScene2.bin"
        if not (scene_path.exists() and HEADER.exists() and SOURCE.exists()):
            raise unittest.SkipTest("assets or sources not available")
        cls.scene = Scene.load(scene_path)
        cls.assets = AssetLibrary(ASSETS)
        cls.constants = Constants(HEADER)
        cls.scan = scan_source(SOURCE, cls.constants)
        ref_scene = ref_assets = None
        if (REF / "UI" / "FieldScene2.bin").exists():
            ref_scene = Scene.load(REF / "UI" / "FieldScene2.bin")
            ref_assets = AssetLibrary(REF)
        cls.audit = build_audit(
            cls.scene,
            cls.assets,
            cls.constants,
            cls.scan,
            known=load_known(),
            ref_scene=ref_scene,
            ref_assets=ref_assets,
            with_candidates=True,
        )

    def test_scene_facts(self):
        self.assertEqual(self.audit.scene_summary["controls"], 709)
        self.assertEqual(self.audit.scene_summary["unique_ids"], 708)

    def test_no_type_mismatches_in_current_port(self):
        self.assertEqual(self.audit.summary["type_mismatches"], 0)

    def test_unnamed_count(self):
        self.assertEqual(self.audit.summary["unnamed_controls"], 16)

    def test_dead_and_orphan_are_substantial(self):
        self.assertGreater(self.audit.summary["dead_values"], 100)
        self.assertGreater(self.audit.summary["orphan_controls"], 300)
        self.assertGreater(self.audit.summary["resolved_references"], 400)

    def test_known_dead_reference_is_classified(self):
        refs = [r for r in self.audit.ref_audits if r.ref.value == 65775]
        self.assertTrue(refs)
        self.assertEqual(refs[0].status, "755-only")

    def test_unknown_constants_are_variables(self):
        self.assertEqual(self.audit.summary["unknown_constants"], 0)

    def test_candidate_matching_finds_party_controls(self):
        if self.audit.ref_scene_summary is None:
            self.skipTest("755 reference scene not available")
        by_value = {g["value"]: g for g in self.audit.dead_references}
        self.assertIn(475138, by_value)
        ids = {c["id"] for c in by_value[475138]["candidates"]}
        self.assertIn(1863, ids)
        self.assertIn(475139, by_value)
        ids = {c["id"] for c in by_value[475139]["candidates"]}
        self.assertIn(1864, ids)

    def test_baseline_keys_are_stable(self):
        keys = self.audit.baseline_keys()
        self.assertEqual(keys, sorted(set(keys)))
        self.assertTrue(any(k.startswith("dead|") for k in keys))
        self.assertTrue(any(k.startswith("unnamed|") for k in keys))


if __name__ == "__main__":
    unittest.main()
