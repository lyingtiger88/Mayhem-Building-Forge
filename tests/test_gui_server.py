import importlib.util
from pathlib import Path
import unittest

MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "mbf_gui_server.py"
SPEC = importlib.util.spec_from_file_location("mbf_gui_server", MODULE_PATH)
SERVER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SERVER)


class ParametricLabInputTests(unittest.TestCase):
    def setUp(self):
        self.brief = {
            "width": 20, "depth": 15, "south": 3, "north": 2,
            "east": 1, "west": 2, "floors": 2, "bedrooms": 1,
            "seed": 42, "bathrooms": 1, "entrance": "south", "profile": "all",
            "living_area": 24, "kitchen_area": 12, "bedroom_area": 14,
            "bathroom_area": 6, "hallway_area": 8,
        }

    def test_gui_loads_external_application_script(self):
        root = Path(__file__).resolve().parents[1]
        html = (root / "gui" / "index.html").read_text(encoding="utf-8")
        script = root / "gui" / "app.js"
        self.assertTrue(script.is_file())
        self.assertIn('<script src="/app.js" defer></script>', html)
        self.assertIn('<link rel="stylesheet" href="/studio.css">', html)
        self.assertTrue((root / "gui" / "studio.css").is_file())

    def test_valid_brief_and_all_profiles(self):
        values, profile = SERVER.parse_brief(self.brief)
        self.assertEqual(profile, "all")
        self.assertEqual(values["width"], 20.0)
        self.assertEqual(values["floors"], 2)
        self.assertEqual(values["entrance"], "south")
        self.assertEqual(values["bathrooms"], 1)
        self.assertIsInstance(values["bathrooms"], int)
        self.assertEqual(values["bedroom_area"], 14.0)

    def test_rejects_setbacks_that_consume_width(self):
        brief = dict(self.brief, east=10, west=10)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_setbacks_that_consume_depth(self):
        brief = dict(self.brief, north=8, south=7)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_fractional_floor_count(self):
        brief = dict(self.brief, floors=1.5)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_room_target_below_minimum(self):
        brief = dict(self.brief, bathroom_area=3.0)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_unsupported_brief_schema(self):
        brief = dict(self.brief, schema_version=2)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_unknown_entrance_facade(self):
        brief = dict(self.brief, entrance="northwest")
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_unsupported_bathroom_count(self):
        brief = dict(self.brief, bathrooms=5)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_boolean_bathroom_count(self):
        brief = dict(self.brief, bathrooms=True)
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_unknown_profile(self):
        brief = dict(self.brief, profile="random")
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)

    def test_rejects_non_finite_dimensions(self):
        brief = dict(self.brief, width=float("nan"))
        with self.assertRaises(ValueError):
            SERVER.parse_brief(brief)


if __name__ == "__main__":
    unittest.main()
