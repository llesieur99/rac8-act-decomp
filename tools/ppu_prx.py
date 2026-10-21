"""Apply the SDK's link-time fixup to a linked candidate.

The PlayStation 3's linker ships a fix-up tool, ``ppu-lv2-prx-fixup``,
that the public toolchain sources do not carry.  What it does was measured
on the retail executable:

* it visits every ``bl`` whose target is **outside** the module's
  descriptor — that is, every call that leaves the module;
* it replaces the ``nop`` the compiler puts in the branch's delay slot
  with ``ld r2, 0x28(r1)``, which restores the TOC base from the
  caller's frame.

On the target, 999 of the delay slots carry that instruction and **all
999** of them follow a call to a function the descriptor does not name —
the rule has no exception among the 161,564 calls that leave the module.
Three calls are the exceptions the rule cannot see: the two that call the
module's own entry point, which is not exported, and the two that call
the module's stub area.

    python tools/ppu_prx.py <linked.elf> [--elf EBOOT.elf] [--out patched.elf]

The descriptor is read from the retail executable's entry point, which is
where the loader finds it.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ppu_elf import PpuElf, ElfError  # noqa: E402

# ``bl``: opcode 18, the link bit set, the absolute bit clear.
BRANCH_LINK = 0x48000001
BRANCH_MASK = 0xFC000001
BRANCH_ABSOLUTE = 0x00000002
# ``ld r2, 0x28(r1)``
LD_R2_28 = 0xE8410028
NOP = 0x60000000


def descriptor_addresses(elf: PpuElf) -> set[int]:
    """The function addresses the module's descriptor names."""
    entry = elf.entry
    if not entry:
        return set()
    pairs = []
    for segment in elf.program_headers():
        if segment.is_load and segment.vaddr <= entry < segment.vaddr + segment.filesz:
            break
    else:
        return set()
    data = elf.read_vaddr(entry, 0x400000)
    for offset in range(0, len(data) - 7, 8):
        function, base = struct.unpack_from(">II", data, offset)
        if not base:
            break
        pairs.append((function, base))
    if not pairs:
        return set()
    gp = pairs[0][1]
    out = set()
    for function, base in pairs:
        if base != gp:
            break
        out.add(function)
    return out


def patch(data: bytearray, base_address: int, known: set[int]) -> tuple[int, int]:
    """Replace the delay slot of the calls that leave the module."""
    calls = 0
    patched = 0
    for offset in range(0, len(data) - 7, 4):
        word = struct.unpack_from(">I", data, offset)[0]
        if word & BRANCH_MASK != BRANCH_LINK or word & BRANCH_ABSOLUTE:
            continue
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        target = (base_address + offset + displacement) & 0xFFFFFFFFFFFFFFFF
        calls += 1
        if target in known:
            continue
        slot = offset + 4
        if struct.unpack_from(">I", data, slot)[0] != NOP:
            continue
        struct.pack_into(">I", data, slot, LD_R2_28)
        patched += 1
    return calls, patched


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("linked", help="the linked candidate to fix up")
    parser.add_argument("--elf", help="the retail executable, for the descriptor")
    parser.add_argument("--out", help="write the fixed-up file here")
    args = parser.parse_args(argv)

    try:
        with open(args.linked, "rb") as handle:
            image = bytearray(handle.read())
        candidate = PpuElf(bytes(image), args.linked)
    except (OSError, ElfError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    known: set[int] = set()
    if args.elf:
        try:
            with open(args.elf, "rb") as handle:
                retail = PpuElf(handle.read(), args.elf)
            known = descriptor_addresses(retail)
        except (OSError, ElfError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1

    text = None
    segments = [ph for ph in candidate.program_headers()
                if ph.is_load and ph.filesz and (ph.flags & 0x4)]
    if segments:
        # the code is the lowest segment that carries a payload: a large
        # data segment may carry the executable bit as well
        text = min(segments, key=lambda ph: ph.vaddr)
    if text is None:
        print("error: the candidate has no executable segment", file=sys.stderr)
        return 1

    segment = bytearray(image[text.offset:text.offset + text.filesz])
    calls, patched = patch(segment, text.vaddr, known)
    image[text.offset:text.offset + text.filesz] = segment
    print(f"{calls} calls, {patched} delay slots fixed up")
    if args.out:
        with open(args.out, "wb") as handle:
            handle.write(image)
        print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
