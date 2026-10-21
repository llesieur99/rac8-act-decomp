"""Tests for the link-time fix-up, on synthetic code."""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools"))

import ppu_prx  # noqa: E402


def bl(target_offset, base=0x10000, at=0):
    """A ``bl`` whose displacement reaches ``target_offset``."""
    displacement = (target_offset - (base + at)) & 0x03FFFFFC
    if displacement & 0x02000000:
        displacement -= 0x04000000
    return 0x48000001 | (displacement & 0x03FFFFFC)


class PrxFixupTest(unittest.TestCase):
    def test_the_delay_slot_of_an_outgoing_call_is_replaced(self):
        code = bytearray()
        code += struct.pack(">I", bl(0x20000, at=0))
        code += struct.pack(">I", ppu_prx.NOP)
        code += struct.pack(">I", 0x4E800020)
        calls, patched = ppu_prx.patch(code, 0x10000, known=set())
        self.assertEqual(calls, 1)
        self.assertEqual(patched, 1)
        self.assertEqual(struct.unpack_from(">I", code, 4)[0], ppu_prx.LD_R2_28)

    def test_a_call_inside_the_module_is_left_alone(self):
        code = bytearray()
        code += struct.pack(">I", bl(0x10010, at=0))
        code += struct.pack(">I", ppu_prx.NOP)
        code += struct.pack(">I", 0x4E800020)
        calls, patched = ppu_prx.patch(code, 0x10000, known={0x10010})
        self.assertEqual(calls, 1)
        self.assertEqual(patched, 0)
        self.assertEqual(struct.unpack_from(">I", code, 4)[0], ppu_prx.NOP)

    def test_a_delay_slot_that_is_not_a_nop_is_left_alone(self):
        code = bytearray()
        code += struct.pack(">I", bl(0x20000, at=0))
        code += struct.pack(">I", 0x7C0803A6)
        code += struct.pack(">I", 0x4E800020)
        calls, patched = ppu_prx.patch(code, 0x10000, known=set())
        self.assertEqual(patched, 0)
        self.assertEqual(struct.unpack_from(">I", code, 4)[0], 0x7C0803A6)


if __name__ == "__main__":
    unittest.main()
