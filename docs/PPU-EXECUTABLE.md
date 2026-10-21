# The PPU executable

The PlayStation 3's main processor is the **PPU**: a big-endian PowerPC core
with an SPU coprocessor and an RSX graphics unit. The executable of a PS3
game is a PPU program: a 64-bit big-endian ELF with a few SCE extensions.

This document records what the reader in
[`tools/ppu_elf.py`](../tools/ppu_elf.py) measures on the pinned target.

## The ELF header

| Field | Value on the target |
|---|---|
| class | ELF64 |
| endianness | big |
| `e_ident[EI_OSABI]` | `0x66` (LV2) |
| `e_type` | `2` (executable) |
| `e_machine` | `0x15` |
| `e_entry` | `0xCA6B58` |
| `e_phnum` | 8 |
| `e_shnum` | 30 |
| `e_shstrndx` | 29 |

The program headers are the usual ELF64 ones (56 bytes) with the SCE flag
bits added to `p_flags`:

| Bit | Meaning |
|---|---|
| `0x00100000` | the segment is for the SPU |
| `0x00200000` | the segment is writable by the SPU |
| `0x00400000` | the segment is readable by the SPU |
| `0x01000000` | the segment is for the RSX |
| `0x02000000` | the segment is writable by the RSX |
| `0x04000000` | the segment is readable by the RSX |

The two last program headers are not segments at all: type `0x60000001` is
the SCE dynamic section and type `0x60000002` the SCE segment symbols, both
pointing into the data segment.

## The sections

The retail executable is **stripped**, and it shows:

* there is **no symbol table**: no `.symtab`, no `.dynsym`;
* there are **no section names**: the `sh_name` fields point into the
  string table of the SCE dynamic section, so the names read back as
  fragments of module names such as `libsysutil_np_trophy_stub:p27`. The
  "section names" of a PS3 eboot are an artefact, not information.

The 30 sections are therefore anonymous `PROGBITS` blocks, plus a `NOBITS`
block for the BSS. Their roles have to be recovered from their content:

| Section | Address | Size | Content |
|---|---|---|---|
| 2 | `0x10280` | 10.7 MiB | the PPU text (code) |
| 13 | `0xAD7D00` | 1.3 MiB | read-only data (strings, tables) |
| 21–23 | `0xC5CBA8` … | 0.9 MiB | the data segment's initialised data |
| 26 | `0xD33400` | 0.7 MiB | the rest of the data segment |
| 27 | `0xDE5280` | 3.3 MiB | the BSS (`NOBITS`) |
| 28 | `0xDD527C` | 8,633 | the SCE dynamic section, obfuscated |
| 29 | `0xDD7435` | 291 | the SCE dynamic string table (module names) |

The dynamic string table names the modules the executable links against:
`libsysutil_np_trophy_stub`, `libsysutil_game_stub`, `libpthread`,
`liblv2_stub` and others. It is the only piece of link information the
retail build keeps.

## What the inventory found

A full inventory of the file — sections, segments, strings, import
mechanism — was made on the target and is kept in the contributor's
workspace. Four of its results change the way the file should be read.

**The segment at `0x10000` is not only code.**  It holds the code, the
read-only data and the initialised data in one block:

| Range | Size | Content |
|---|---|---|
| `0x10200` – `0xA49DE4` | 10.7 MB | the game's code |
| `0xA49DE4` – `0xAD7D00` | 581 KB | read-only data (strings, tables) |
| `0xAD7D00` – `0xC09560` | 1.2 MB | initialised data |

There is no cold zone inside the code: every 64 KB of it holds between 0
and 821 function prologues.  What looked like a table inside the text is
valid code (matrix initialisers, leaf functions without a frame).  The
discriminant between code and data is the **number of prologues**, not
the density of calls: the read-only data holds no prologue at all and up
to 3 618 branches per 64 KB.

**`e_entry` does not point at code.**  `0xCA6B58` is the address of a
table of **68 483 pairs** — a function address and the constant
`0xD34770` — kept in the data segment.  The real start of the program is
the module's initialisation function at `0x10200`.

**The link information is richer than the stripped file suggests.**  The
executable links **22 PRX modules** (the studio's own translation units)
with the version prefix `p270001`: `libm`, `libgcm_cmd`,
`libsysutil_stub`, `libhttp_stub`, `libspurs_stub`, `libssl_stub`,
`libpthread`, `crt1`, `liblv` and others; `EBOOT.BIN` carries the
`libc` and `libstdc++` counterpart.  The import mechanism is fully mapped
— 18 SCE module names in the clear, 18 descriptors, 245 `mtctr`/`bctr`
thunks — but the *names of the imported symbols* are compressed in a
245-entry table and cannot be recovered.

**Three SPU programs are embedded in the data.**  They are complete ELF
files with their section names intact: `edgezlib_inflate_task.spu.elf`
(the game's own SPU module), and the SDK's `nextsy` and `lr1_ww`.  The
107 compiler stamps the file carries are spread over the three images,
one group each.

## What this means for the decompilation

Without symbols, a function's name has to be *earned*: it comes from the
code, from its callers, from the strings it touches, or from the module it
belongs to. Every name in `src/` is a hypothesis until the code proves it,
and the project treats it as such (see `AGENTS.md`, rule 4).

The function boundaries are recovered by analysis, not by a table: see
`docs/METHOD.md` for the procedure and `docs/ROADMAP.md` for the order of
the work.

## Reading it

```sh
python tools/ppu_elf.py info     work/eboot/EBOOT.elf
python tools/ppu_elf.py sections work/eboot/EBOOT.elf
python tools/ppu_elf.py symbols  work/eboot/EBOOT.elf
python tools/ppu_elf.py catalogue work/eboot/EBOOT.elf --out work/eboot/functions.tsv
```

`catalogue` writes the named functions; on this target the list is empty,
which is the honest answer for a stripped executable.
