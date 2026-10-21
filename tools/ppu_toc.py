"""Link a candidate against the TOC layout of the retail module.

The retail executable of *Ratchet & Clank Future: A Crack in Time* addresses
its global variables through a TOC (table of contents) that the SDK builds in
``.toc``:

* ``r2`` holds the TOC anchor when the module runs: ``0x00D34770``;
* the anchor sits in the middle of a 64 KiB window of 4-byte entries that
  starts at ``0x00D2C770``;
* an entry holds the **value** of a global variable - the retail code
  compares an entry with zero and reads a float straight out of one, so
  the window holds the data, not the addresses;
* a global is therefore reached with ``lwz rX, offset(r2)`` where ``offset``
  is the signed 16-bit distance between the anchor and the entry.

GNU ld implements that convention natively: the displacement of an
``R_PPC64_TOC16`` relocation is ``slot_address - TOC anchor``, and the anchor
defaults to ``.toc + 0x8000`` -- the same "anchor in the middle of the
window" convention the retail uses.  The SDK linker therefore computes the
right displacement *by itself* as soon as the global lands on the right
entry.

The linker, however, does not know which entry a global belongs to: it lays
the ``.toc`` sections out in the order the compiler emitted them.  This tool
closes that gap:

* it reads ``globals.tsv`` (the catalogue of the retail globals) and turns a
  global name into its retail TOC offset;
* it compiles the candidate, then rewrites the ``R_PPC64_TOC16``
  relocations of the candidate so that every global points at its retail
  entry, and rebuilds the ``.toc`` section so that it really holds the
  entries at their retail addresses;
* it writes the linker script that reproduces the retail TOC geometry
  (``.toc`` at ``0x00D2C770``, anchor ``_GLOBAL_OFFSET_TABLE_`` at
  ``0x00D34770``, ``.text`` at the retail address of the function) and runs
  the SDK linker on the result.

Everything the tool needs is in the standard library; the SDK compiler and
linker are run inside the ``ps3gcc-toolchain`` container.

    python toc_link.py info
    python toc_link.py header  --out toc_globals.h
    python toc_link.py lds     --address 0x103B0 --callee sub=0x1234
    python toc_link.py build   --source cand.c --address 0x103B0 \\
                               --globals g_m7ffc --out cand.elf
"""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile

# ---------------------------------------------------------------------------
# The retail TOC geometry (measured, see the analysis report).
# ---------------------------------------------------------------------------

TOC_BASE = 0x00D34770          # r2 when the module runs
WINDOW = 0x00010000            # the 64 KiB window of entries
TABLE_BASE = TOC_BASE - 0x8000  # 0x00D2C770, first byte of the .toc table
ANCHOR_OFFSET = 0x8000        # the anchor is in the middle of the window

DEFAULT_GLOBALS = ""          # the catalogue of the retail globals
DEFAULT_FUNCS = ""            # the globals each function touches
DEFAULT_ELF = ""              # the retail executable
IMAGE = "ps3gcc-toolchain"    # the container the SDK compiler runs in
PREFIX = "/usr/local/cell/host-linux/ppu"
GCC = f"{PREFIX}/bin/ppu-lv2-gcc"
LD = f"{PREFIX}/bin/ppu-lv2-ld"
TOOLCHAIN_OUT = ""            # where the SDK toolchain is mounted
# The linker script of the harness and the generated header, next to this
# file; a candidate can include the header.
LDS_TEMPLATE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "toc.lds")
HEADER_TEMPLATE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "toc_globals.h")

# ELF constants for the big-endian PowerPC64 objects of the SDK.
SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_NOBITS = 8
R_PPC64_ADDR32 = 1
R_PPC64_TOC16 = 47
R_PPC64_TOC32 = 107


# ---------------------------------------------------------------------------
# The catalogue of globals.
# ---------------------------------------------------------------------------


def global_name(offset: int) -> str:
    """Return the C name the harness gives to the global at ``offset``."""
    if offset < 0:
        return "g_m%x" % -offset
    if offset > 0:
        return "g_p%x" % offset
    return "g_0"


def offset_of_name(name: str) -> int:
    """Inverse of :func:`global_name` (``g_m7ffc`` -> ``-32764``)."""
    if name.startswith("g_m"):
        return -int(name[3:], 16)
    if name.startswith("g_p"):
        return int(name[3:], 16)
    if name == "g_0":
        return 0
    raise ValueError(f"not a harness global name: {name}")


class TocModel:
    """The retail TOC: the catalogue of globals and their geometry."""

    def __init__(self, path: str = DEFAULT_GLOBALS):
        self.path = path
        self.globals: dict[int, dict[str, str]] = {}
        with open(path, encoding="utf-8") as handle:
            header = handle.readline().rstrip("\n").split("\t")
            for line in handle:
                fields = line.rstrip("\n").split("\t")
                if not fields or not fields[0]:
                    continue
                row = dict(zip(header, fields))
                self.globals[int(row["offset"])] = row
    # -- geometry ----------------------------------------------------------
    @staticmethod
    def slot_offset(offset: int) -> int:
        """Byte offset of the entry inside the ``.toc`` table."""
        return offset + ANCHOR_OFFSET

    @staticmethod
    def entry_address(offset: int) -> int:
        """Absolute address of the entry that holds the global."""
        return TOC_BASE + offset

    # -- queries -----------------------------------------------------------
    def knows(self, offset: int) -> bool:
        return offset in self.globals

    def row(self, offset: int) -> dict[str, str]:
        return self.globals[offset]

    def by_name(self, name: str) -> int:
        return offset_of_name(name)

    def functions(self, path: str = DEFAULT_FUNCS) -> dict[str, list[int]]:
        """The globals every catalogued function touches."""
        result: dict[str, list[int]] = {}
        with open(path, encoding="utf-8") as handle:
            handle.readline()
            for line in handle:
                parts = line.rstrip("\n").split("\t")
                if len(parts) < 3:
                    continue
                result[parts[0]] = [int(x) for x in parts[2].split(",") if x]
        return result


# ---------------------------------------------------------------------------
# A small reader/writer for big-endian ELF64 relocatable objects.
# ---------------------------------------------------------------------------


class ElfObject:
    """Just enough of an ELF64 object to move TOC relocations around."""

    def __init__(self, data: bytes):
        self.data = bytearray(data)
        if self.data[:4] != b"\x7fELF":
            raise ValueError("not an ELF file")
        if self.data[5] != 2:
            raise ValueError("not a big-endian ELF file")
        self.e_shoff = struct.unpack_from(">Q", self.data, 0x28)[0]
        self.e_shentsize, self.e_shnum, self.e_shstrndx = struct.unpack_from(">3H", self.data, 0x3A)
        self.sections = []
        for index in range(self.e_shnum):
            off = self.e_shoff + index * self.e_shentsize
            name_off, sh_type = struct.unpack_from(">II", self.data, off)
            flags, addr, sh_offset, size = struct.unpack_from(">4Q", self.data, off + 8)
            link, info = struct.unpack_from(">2I", self.data, off + 40)
            align, entsize = struct.unpack_from(">2Q", self.data, off + 48)
            self.sections.append({
                "index": index, "name_off": name_off, "type": sh_type,
                "flags": flags, "addr": addr, "offset": sh_offset, "size": size,
                "link": link, "info": info, "align": align, "entsize": entsize,
                "header": off,
            })
        self.strtab_off = self.sections[self.e_shstrndx]["offset"]
        for section in self.sections:
            section["name"] = self._string(self.strtab_off, section["name_off"])

    # -- helpers -----------------------------------------------------------
    def _string(self, base: int, offset: int) -> str:
        end = self.data.index(b"\x00", base + offset)
        return self.data[base + offset:end].decode("ascii", "replace")

    def section(self, name: str) -> dict | None:
        for section in self.sections:
            if section["name"] == name:
                return section
        return None

    def content(self, section: dict) -> bytearray:
        return bytearray(self.data[section["offset"]:section["offset"] + section["size"]])

    def symbols(self, symtab: dict) -> list[dict]:
        strtab = self.sections[symtab["link"]]
        out = []
        count = symtab["size"] // 24
        for index in range(count):
            off = symtab["offset"] + index * 24
            name_off, info, other, shndx = struct.unpack_from(">IBBH", self.data, off)
            value, size = struct.unpack_from(">QQ", self.data, off + 8)
            out.append({
                "index": index, "name": self._string(strtab["offset"], name_off),
                "info": info, "shndx": shndx, "value": value, "size": size,
            })
        return out

    def relocs(self, rela: dict) -> list[dict]:
        out = []
        count = rela["size"] // 24
        for index in range(count):
            off = rela["offset"] + index * 24
            r_offset, r_info = struct.unpack_from(">QQ", self.data, off)
            r_addend = struct.unpack_from(">q", self.data, off + 16)[0]
            out.append({
                "index": index, "offset": r_offset, "type": r_info & 0xFFFFFFFF,
                "symbol": r_info >> 32, "addend": r_addend, "header": off,
            })
        return out

    def set_reloc(self, rela: dict, index: int, addend: int) -> None:
        off = rela["offset"] + index * 24
        struct.pack_into(">q", self.data, off + 16, addend)

    # -- writing -----------------------------------------------------------
    def rebuild(self, replacements: dict[int, bytes]) -> bytes:
        """Write the object back, replacing the content of some sections."""
        out = bytearray(self.data[:0x40])
        # The section data are laid out again, in section order.
        new_offsets: dict[int, int] = {}
        new_sizes: dict[int, int] = {}
        for section in self.sections:
            if section["type"] in (0, SHT_NOBITS):
                continue
            payload = replacements.get(section["index"])
            if payload is None:
                payload = self.data[section["offset"]:section["offset"] + section["size"]]
            align = max(section["align"], 1)
            while len(out) % align:
                out.append(0)
            new_offsets[section["index"]] = len(out)
            new_sizes[section["index"]] = len(payload)
            out += payload
        # The section header table goes last.
        while len(out) % 8:
            out.append(0)
        shoff = len(out)
        for section in self.sections:
            header = bytearray(self.data[section["header"]:section["header"] + self.e_shentsize])
            struct.pack_into(">Q", header, 24, new_offsets.get(section["index"], 0))
            struct.pack_into(">Q", header, 32, new_sizes.get(section["index"], section["size"]))
            out += header
        struct.pack_into(">Q", out, 0x28, shoff)
        return bytes(out)


# ---------------------------------------------------------------------------
# The TOC surgery.
# ---------------------------------------------------------------------------


class TocFixer:
    """Move the ``.toc`` entries of an object onto their retail addresses.

    Two shapes are possible for the window the TOC points at:

    ``address``
        the entry holds the address of the global (what the SDK's compiler
        assumes); the candidate then reaches the global with ``&g_global``;

    ``value``
        the entry holds the value of the global, which is what the retail
        does: the window at ``0x00D2C770`` is the small data of the module
        and the entries are the globals themselves.
    """

    def __init__(self, model: TocModel, shape: str = "address"):
        self.model = model
        self.shape = shape

    def fix(self, data: bytes, names: dict[str, int]) -> tuple[bytes, list[str]]:
        """Return the fixed object and the list of globals that were moved.

        ``names`` maps a global name to its retail TOC offset.  Every global
        the object mentions must be listed, otherwise it keeps its
        compiler-chosen entry.
        """
        obj = ElfObject(data)
        toc = obj.section(".toc")
        rela_toc = obj.section(".rela.toc")
        text = obj.section(".text")
        rela_text = obj.section(".rela.text")
        if toc is None or rela_toc is None:
            return data, []
        symtab = obj.sections[rela_toc["link"]]
        symbols = obj.symbols(symtab)

        # entry offset -> global name, read from the object's own .toc
        entries: dict[int, str] = {}
        for reloc in obj.relocs(rela_toc):
            if reloc["type"] != R_PPC64_ADDR32:
                continue
            name = symbols[reloc["symbol"]]["name"]
            entries[reloc["offset"]] = name

        # The retail entry of every global the object mentions.
        wanted: dict[int, str] = {}
        for name in set(entries.values()):
            if name in names:
                wanted[name] = names[name]

        moved: list[str] = []
        if rela_text is not None:
            for reloc in obj.relocs(rela_text):
                if reloc["type"] != R_PPC64_TOC16:
                    continue
                name = entries.get(reloc["addend"])
                if name is None or name not in wanted:
                    continue
                slot = self.model.slot_offset(wanted[name])
                obj.set_reloc(rela_text, reloc["index"], slot)
                moved.append(name)

        # Rebuild the .toc section: the entries move to their retail slots.
        new_toc = bytearray(WINDOW)
        new_relocs: list[tuple[int, int]] = []
        for offset, name in entries.items():
            if name not in wanted:
                continue
            slot = self.model.slot_offset(wanted[name])
            if self.shape == "value":
                # The window holds the globals themselves: the entry carries
                # the value the retail has in the file.
                row = self.model.row(wanted[name])
                try:
                    value = int(row.get("initial_value", "0"), 16)
                except ValueError:
                    value = 0
                new_toc[slot:slot + 4] = struct.pack(">I", value & 0xFFFFFFFF)
            else:
                new_relocs.append((slot, entries_offset_symbol(obj, rela_toc, symbols, offset)))
        new_relocs.sort()
        rela_bytes = bytearray()
        for slot, symbol in new_relocs:
            rela_bytes += struct.pack(">QQq", slot, (symbol << 32) | R_PPC64_ADDR32, 0)

        replacements = {
            toc["index"]: bytes(new_toc),
            rela_toc["index"]: bytes(rela_bytes),
        }
        return obj.rebuild(replacements), sorted(set(moved))


def entries_offset_symbol(obj: ElfObject, rela_toc: dict, symbols: list[dict],
                           offset: int) -> int:
    """The symbol index of the ``.toc`` entry stored at ``offset``."""
    for reloc in obj.relocs(rela_toc):
        if reloc["type"] == R_PPC64_ADDR32 and reloc["offset"] == offset:
            return reloc["symbol"]
    raise KeyError(offset)


# ---------------------------------------------------------------------------
# The linker script.
# ---------------------------------------------------------------------------


def linker_script(address: int, callees: dict[str, int], toc_base: int = TOC_BASE,
                  table_base: int = TABLE_BASE, window: int = WINDOW,
                  entry: str | None = None) -> str:
    """The linker script that reproduces the retail TOC geometry."""
    calls = []
    for name, value in callees.items():
        calls.append(f"  {name} = {value:#010x};")
        calls.append(f"  .{name} = {value:#010x};")
    lines = [
        "/* TOC harness -- the geometry of the retail module. */",
        'OUTPUT_FORMAT("elf64-powerpc-celloslv2")',
        "OUTPUT_ARCH(powerpc:common64)",
        f"ENTRY({entry})" if entry else "ENTRY(_start)",
        "SECTIONS",
        "{",
        f"  . = {address:#010x};",
        "  .text : { *(.text .text.* .stub) }",
    ]
    if calls:
        lines.append("")
        lines += calls
    lines += [
        "",
        f"  . = {table_base:#010x};",
        f"  .toc {table_base:#010x} : {{ *(.toc) }}",
        f"  _GLOBAL_OFFSET_TABLE_ = {toc_base:#010x};",
        f"  . = {table_base + window:#010x};",
        "  .opd : { *(.opd) }",
        f"  . = {table_base + window + 0x100:#010x};",
        "  .data : { *(.data .data.*) }",
        "  .sdata : { *(.sdata .sdata.*) }",
        "  .bss : { *(.bss .bss.* .sbss .sbss.* COMMON) }",
        "  /DISCARD/ : { *(.comment) *(.note*) *(.pdr) *(.PPC*) }",
        "}",
        "",
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Driving the SDK toolchain.
# ---------------------------------------------------------------------------


def run_docker(workdir: str, command: str,
               toolchain: str = TOOLCHAIN_OUT, image: str = IMAGE) -> subprocess.CompletedProcess:
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{workdir}:/w",
        "-v", f"{toolchain}:/usr/local/cell",
        image, "bash", "-c",
        f"export PATH={PREFIX}/bin:$PATH; " + command,
    ]
    return subprocess.run(cmd, capture_output=True, text=True)


def compile_source(source_text: str, workdir: str, flags: list[str], name: str = "c") -> str:
    obj = os.path.join(workdir, name + ".o")
    # -Wa,-mregnames: the SDK's assembler only accepts the "off(r2)" form of
    # a TOC access when it knows the register names; without it, every
    # "lwz rX, off(r2)" is read as a relocation against a symbol named r2 and
    # the assembly fails with "unsupported relocation against r2".
    command = (f"{GCC} -c -nostdinc -fno-builtin -Wa,-mregnames {' '.join(flags)} "
               f"-o /w/{name}.o /w/{name}.c")
    result = run_docker(workdir, command)
    if not os.path.exists(obj):
        raise RuntimeError("compilation failed:\n" + (result.stdout + result.stderr)[-2000:])
    return obj


def link_objects(objs: list[str], workdir: str) -> str:
    linked = os.path.join(workdir, "c.elf")
    command = f"{LD} -T /w/l.lds -o /w/c.elf " + " ".join(f"/w/{o}" for o in objs)
    result = run_docker(workdir, command)
    if not os.path.exists(linked):
        raise RuntimeError("link failed:\n" + (result.stdout + result.stderr)[-2000:])
    return linked


# ---------------------------------------------------------------------------
# The public interface.
# ---------------------------------------------------------------------------


def globals_source(names: list[str]) -> str:
    """The definitions of the globals the candidate refers to.

    The retail declares them in other modules; the harness declares them in
    a companion object so that the ``.toc`` entries of the candidate resolve
    (this is what keeps the SDK linker from reporting "undefined reference
    to g_global in section .toc").
    """
    lines = ["/* The retail globals the candidate refers to. */"]
    for name in names:
        lines.append(f"unsigned int {name};")
    lines.append("")
    return "\n".join(lines)


def build(source_text: str, address: int, names: dict[str, int],
          callees: dict[str, int] | None = None,
          flags: list[str] | None = None,
          model: TocModel | None = None,
          keep: bool = False,
          define_globals: bool = True,
          shape: str = "address") -> bytes:
    """Compile, move the TOC entries and link; return the linked ELF."""
    model = model or TocModel()
    callees = callees or {}
    flags = flags or ["-O1"]
    workdir = tempfile.mkdtemp(prefix="toc-")
    try:
        with open(os.path.join(workdir, "c.c"), "w", encoding="utf-8") as handle:
            handle.write(source_text)
        # the generated header, so that a candidate can include it
        header = HEADER_TEMPLATE
        if os.path.exists(header):
            shutil.copy(header, os.path.join(workdir, "toc_globals.h"))
        with open(os.path.join(workdir, "l.lds"), "w", encoding="utf-8") as handle:
            handle.write(linker_script(address, callees))
        obj = compile_source(source_text, workdir, flags, "c")
        objects = ["c.o"]
        if define_globals and names:
            with open(os.path.join(workdir, "g.c"), "w", encoding="utf-8") as handle:
                handle.write(globals_source(sorted(names)))
            compile_source(globals_source(sorted(names)), workdir, flags, "g")
            objects.append("g.o")
        with open(obj, "rb") as handle:
            raw = handle.read()
        fixer = TocFixer(model, shape)
        fixed, moved = fixer.fix(raw, names)
        with open(obj, "wb") as handle:
            handle.write(fixed)
        linked = link_objects(objects, workdir)
        with open(linked, "rb") as handle:
            return handle.read()
    finally:
        if not keep:
            shutil.rmtree(workdir, ignore_errors=True)


def c_header(model: TocModel | None = None, only: list[int] | None = None) -> str:
    """A C header that declares every catalogued global under its name.

    It also offers the two ways of reaching a global:

    * ``g_m7ffc`` -- the plain declaration; the SDK's compiler reaches the
      global through the TOC on its own;
    * ``TOC_U32(offset)`` -- the access spelled out, which produces the
      retail's instruction exactly.
    """
    model = model or TocModel()
    lines = [
        "/* Generated by toc_link.py -- the retail globals of the module. */",
        "/* Each name is the global at the given TOC offset (r2 = "
        f"{TOC_BASE:#010x}). */",
        "",
        "#ifndef TOC_GLOBALS_H",
        "#define TOC_GLOBALS_H",
        "",
        "typedef unsigned int u32;",
        "typedef unsigned short u16;",
        "typedef float f32;",
        "",
        "/* The access spelled out: the retail's own instruction. */",
        '#define TOC_U32(off) ({ u32 v; __asm__("lwz %0,%1(r2)" : "=r"(v) : "I"(off)); v; })',
        '#define TOC_F32(off) ({ f32 v; __asm__("lfs %0,%1(r2)" : "=f"(v) : "I"(off)); v; })',
        '#define TOC_U16(off) ({ u16 v; __asm__("lhz %0,%1(r2)" : "=r"(v) : "I"(off)); v; })',
        '#define TOC_U8(off)  ({ unsigned char v; __asm__("lbz %0,%1(r2)" : "=r"(v) : "I"(off)); v; })',
        '#define TOC_PTR(off) ((void *)TOC_U32(off))',
        "",
    ]
    offsets = sorted(only if only is not None else model.globals)
    for offset in offsets:
        row = model.row(offset)
        lines.append(
            f"extern u32 {global_name(offset)}; "
            f"/* offset {offset:+d}  address {row.get('address', '?')}  "
            f"type {row.get('type', '?')} */")
    lines += ["", "#endif", ""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--globals-catalogue", default=DEFAULT_GLOBALS,
                        help="the catalogue of the retail globals")
    parser.add_argument("--funcs-catalogue", default=DEFAULT_FUNCS,
                        help="the globals each function touches")
    parser.add_argument("--elf", default=DEFAULT_ELF,
                        help="the retail executable")
    parser.add_argument("--toolchain", default=TOOLCHAIN_OUT,
                        help="where the SDK toolchain is mounted")
    parser.add_argument("--image", default=IMAGE,
                        help="the container the SDK compiler runs in")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("info", help="describe the TOC model")

    header = sub.add_parser("header", help="print the C header of the globals")
    header.add_argument("--out")

    lds = sub.add_parser("lds", help="print the linker script")
    lds.add_argument("--address", default="0x10000")
    lds.add_argument("--callee", action="append", default=[])

    build_cmd = sub.add_parser("build", help="compile, fix the TOC and link")
    build_cmd.add_argument("--source", required=True)
    build_cmd.add_argument("--address", required=True)
    build_cmd.add_argument("--globals", default="", help="comma separated names")
    build_cmd.add_argument("--callee", action="append", default=[])
    build_cmd.add_argument("--flags", default="-O1")
    build_cmd.add_argument("--shape", default="address",
                           choices=["address", "value"],
                           help="what the .toc entries hold")
    build_cmd.add_argument("--out")

    args = parser.parse_args(argv)

    if args.command == "info":
        model = TocModel(args.globals_catalogue)
        print(f"TOC anchor (r2)      : {TOC_BASE:#010x}")
        print(f".toc table starts at : {TABLE_BASE:#010x}")
        print(f"window               : {WINDOW:#x} bytes "
              f"({TABLE_BASE:#010x} .. {TABLE_BASE + WINDOW:#010x})")
        print(f"catalogued globals   : {len(model.globals)}")
        return 0

    if args.command == "header":
        text = c_header(TocModel(args.globals_catalogue))
        if args.out:
            with open(args.out, "w", encoding="utf-8") as handle:
                handle.write(text)
        else:
            print(text)
        return 0

    if args.command == "lds":
        callees = {}
        for item in args.callee:
            name, _, value = item.partition("=")
            callees[name] = int(value, 16)
        print(linker_script(int(args.address, 16), callees), end="")
        return 0

    if args.command == "build":
        with open(args.source, encoding="utf-8") as handle:
            source = handle.read()
        names = {}
        for item in args.globals.split(","):
            item = item.strip()
            if item:
                names[item] = offset_of_name(item)
        callees = {}
        for item in args.callee:
            name, _, value = item.partition("=")
            callees[name] = int(value, 16)
        model = TocModel(args.globals_catalogue)
        elf = build(source, int(args.address, 16), names, callees,
                    args.flags.split(), model, shape=args.shape)
        if args.out:
            with open(args.out, "wb") as handle:
                handle.write(elf)
        else:
            sys.stdout.buffer.write(elf)
        return 0

    return 1


if __name__ == "__main__":
    raise SystemExit(main())
