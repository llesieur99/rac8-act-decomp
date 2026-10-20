"""Recover the function boundaries of a PPU executable.

The retail build of a PlayStation 3 game is stripped: there is no symbol
table and no section name.  The boundaries have to come from the code
itself, and a boundary that only one method sees is a candidate, not a
function.

This tool offers the two cheap, independent methods:

* **prologue scan** — the PS3 SDK's PPU functions open with a fixed
  pattern: the stack pointer is moved down with ``stdu r1, -N(r1)`` and the
  link register is saved with ``mflr r0`` / ``stw r0, 0x10(r1)``;
* **call scan** — every ``bl`` (branch and link) names an address that the
  program calls, and a called address is a function.

Both sets are published separately, so that they can be compared.  Only the
tool needs the Python standard library: the few opcodes that matter are
decoded directly.

    python tools/ppu_funcs.py prologue <eboot.elf> [--out FILE]
    python tools/ppu_funcs.py calls    <eboot.elf> [--out FILE]
    python tools/ppu_funcs.py catalogue <eboot.elf> [--out FILE]
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from dataclasses import dataclass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ppu_elf import PpuElf, ElfError  # noqa: E402

# PPU instruction patterns, measured on the target's code.
#
# The PPU uses the 64-bit PowerPC encoding.  A function opens with
# ``stdu r1, -N(r1)`` (opcode 62, rD = rS = 1, rA = 1, signed 16-bit
# offset), and the SDK saves the link register right after with
# ``mflr rX`` (the mfspr/mflr family, extended opcode 0x2A6).
STDU_R1 = 0xF8210000
# ``mflr rX`` is the mfspr family with the extended opcode 0x2A6; the
# destination register sits in bits 21-25.
MFLR_MASK = 0xFF1FFCFF
MFLR_VALUE = 0x7C0800A6

# The call instruction: the unconditional branch with the link bit set
# (opcode 18, LK at bit 0; AA at bit 1).
BRANCH_MASK = 0xFC000001
BRANCH_LINK = 0x48000001


@dataclass(frozen=True)
class Candidate:
    address: int
    kind: str


def _text_range(elf: PpuElf) -> tuple[int, int]:
    """Return the (address, size) of the segment that holds the code."""
    best = None
    for ph in elf.program_headers():
        if not ph.is_load or (ph.flags & 0x4) == 0:
            continue
        if best is None or ph.filesz > best.filesz:
            best = ph
    if best is None:
        raise ElfError("no executable segment found")
    return best.vaddr, best.filesz


def _words(elf: PpuElf) -> Iterator[int]:
    """Yield the instruction words of the code segment, with their addresses."""
    vaddr, size = _text_range(elf)
    data = elf.read_vaddr(vaddr, size)
    for offset in range(0, len(data) - 3, 4):
        yield vaddr + offset, struct.unpack_from(">I", data, offset)[0]


def prologue_candidates(elf: PpuElf) -> list[Candidate]:
    """The addresses where the SDK's function prologue appears.

    Two shapes were measured on the target's code:

    * ``stdu r1, -N(r1)`` followed by ``mflr rX``;
    * ``mflr rX`` followed by ``stdu r1, -N(r1)`` (the compiler's other
      order, seen when the frame is set up before the link register is
      saved).
    """
    out = []
    words = list(_words(elf))
    for index, (address, word) in enumerate(words):
        if word & 0xFFFF0000 != STDU_R1:
            continue
        offset = word & 0xFFFF
        if not 0x8000 <= offset:
            # The offset is signed: a prologue moves the stack *down*.
            continue
        following = words[index + 1][1] if index + 1 < len(words) else 0
        preceding = words[index - 1][1] if index > 0 else 0
        if following & MFLR_MASK == MFLR_VALUE or preceding & MFLR_MASK == MFLR_VALUE:
            out.append(Candidate(address, "prologue"))
    return out


def call_targets(elf: PpuElf) -> list[Candidate]:
    """The addresses the program calls."""
    seen = set()
    out = []
    for address, word in _words(elf):
        if word & BRANCH_MASK != BRANCH_LINK:
            continue
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        target = (address + displacement) & 0xFFFFFFFFFFFFFFFF
        if target in seen:
            continue
        seen.add(target)
        out.append(Candidate(target, "call"))
    return out


def build_catalogue(elf: PpuElf) -> list[tuple[int, str, str]]:
    """The union of both methods, with the evidence for each address."""
    prologues = {c.address for c in prologue_candidates(elf)}
    calls = {c.address for c in call_targets(elf)}
    rows = []
    for address in sorted(prologues | calls):
        evidence = []
        if address in prologues:
            evidence.append("prologue")
        if address in calls:
            evidence.append("call")
        rows.append((address, "+".join(evidence), "both" if len(evidence) == 2 else evidence[0]))
    return rows


def cmd_prologue(elf: PpuElf, args: argparse.Namespace) -> int:
    rows = [(c.address, "", "prologue") for c in prologue_candidates(elf)]
    return _write(rows, args)


def cmd_calls(elf: PpuElf, args: argparse.Namespace) -> int:
    rows = [(c.address, "", "call") for c in call_targets(elf)]
    return _write(rows, args)


def cmd_catalogue(elf: PpuElf, args: argparse.Namespace) -> int:
    rows = [(address, kind, evidence) for address, kind, evidence in build_catalogue(elf)]
    return _write(rows, args)


def _write(rows: list[tuple[int, str, str]], args: argparse.Namespace) -> int:
    text = "address\tkind\tevidence\n" + "".join(
        f"0x{address:08X}\t{kind}\t{evidence}\n" for address, kind, evidence in rows)
    if args.out:
        with open(args.out, "w", encoding="utf-8") as handle:
            handle.write(text)
        print(f"wrote {args.out}: {len(rows)} entries")
    else:
        sys.stdout.write(text)
    return 0


def main(argv: list[str] | None = None) -> int:
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):  # pragma: no cover
            pass
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    def add(name: str, help_text: str) -> argparse.ArgumentParser:
        p = sub.add_parser(name, help=help_text)
        p.add_argument("elf", help="path to the rebuilt PPU ELF")
        p.add_argument("--out", help="write a TSV file instead of stdout")
        return p

    add("prologue", "the addresses with the SDK function prologue").set_defaults(func=cmd_prologue)
    add("calls", "the addresses the program calls").set_defaults(func=cmd_calls)
    add("catalogue", "both methods, with their evidence").set_defaults(func=cmd_catalogue)

    args = parser.parse_args(argv)
    try:
        with open(args.elf, "rb") as handle:
            elf = PpuElf(handle.read(), args.elf)
    except (OSError, ElfError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return args.func(elf, args)


if __name__ == "__main__":
    raise SystemExit(main())
