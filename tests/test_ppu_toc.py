"""Tests for the TOC harness, on a synthetic catalogue."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools"))

import ppu_toc  # noqa: E402


CATALOGUE = """offset\taddress\ttype\tconfidence
-18508\t0x00D2FF24\tpointer\televee
-21064\t0x00D2F528\tpointer\televee
-31808\t0x00D2CB30\tpointer\televee
"""


class TocHarnessTest(unittest.TestCase):
    def setUp(self):
        import tempfile
        self.handle = tempfile.NamedTemporaryFile("w", suffix=".tsv", delete=False,
                                                  encoding="utf-8")
        self.handle.write(CATALOGUE)
        self.handle.close()
        self.path = self.handle.name

    def tearDown(self):
        os.unlink(self.path)

    def test_the_geometry_is_the_retail_one(self):
        self.assertEqual(ppu_toc.TOC_BASE, 0x00D34770)
        self.assertEqual(ppu_toc.TABLE_BASE, 0x00D2C770)
        self.assertEqual(ppu_toc.ANCHOR_OFFSET, 0x8000)
        self.assertEqual(ppu_toc.TABLE_BASE + ppu_toc.ANCHOR_OFFSET, ppu_toc.TOC_BASE)

    def test_the_catalogue_is_read(self):
        model = ppu_toc.TocModel(self.path)
        self.assertEqual(len(model.globals), 3)
        self.assertIn(-18508, model.globals)

    def test_the_linker_script_reproduces_the_geometry(self):
        script = ppu_toc.linker_script(0x103B0, {"sub": 0x1234})
        self.assertIn(". = 0x000103b0;", script)
        self.assertIn("_GLOBAL_OFFSET_TABLE_ = 0x00d34770;", script)
        self.assertIn(".toc 0x00d2c770 : { *(.toc) }", script)
        self.assertIn("sub = 0x00001234;", script)


if __name__ == "__main__":
    unittest.main()
