# Where the project stands

**Date:** 2026-10-20

## What exists

| Item | State |
|---|---|
| the package is read and measured | done |
| the package reader is published, with tests | done |
| the SELF container is described | done |
| the retail executable is decrypted and rebuilt into an ELF | done, in the contributor's local workspace |
| the PPU ELF reader is published, with tests | done |
| the first function catalogue | **done**: 8,271 addresses where two independent methods agree ([CATALOGUE.md](CATALOGUE.md)) |
| the third method (a disassembler's analysis) | running |
| the first C readings | **started**: five functions read and written up in `nonmatching/text/`, none compiled yet |
| the compiler that produced the retail bytes | **identified**: SCEI PPC GCC 4.1.1, SDK270 revision 3019 ([COMPILER.md](COMPILER.md)) |
| a qualified PPU compiler and its options | **not started** |

## The measured facts

* The target is the USA retail release: content id
  `UP9000-NPUA80966_00-ACRACKINTIME0001`, package 19,928,976,896 bytes,
  874 entries, 18.6 GiB of game data.
* The main executable is `USRDIR/EBOOT.BIN`, 14,518,160 bytes, a retail
  SELF with key revision `0x001C` and a local NPDRM licence.
* The rebuilt executable is a 14,515,416-byte ELF64 big-endian PPU program:
  entry `0xCA6B58`, 8 program headers, 30 section headers, a 10.7 MiB text
  segment.
* **The executable is stripped.** No symbol table, no section names, and the
  dynamic section is obfuscated. The only link information left is the list
  of modules in the dynamic string table.

## What this changes

The sibling projects start from a boot executable that still carries a
symbol table, so their function boundaries and their names come for free.
Here they do not: the boundaries have to be recovered by analysis, and
every name has to be earned from the code. That makes the *analysis* the
first real work of the project, not the compilation.

It also means the compiler question comes later than in the sibling
projects: until a PPU compiler and its options are qualified, no
byte-exact claim is possible, and the complete-program gate stays closed.
No percentage is published for that reason.

## The next step

1. Let the disassembler's analysis finish and compare it with the
   published catalogue: the addresses where the three methods agree become
   the function candidates, the others stay candidates.
2. Continue the C readings: the five functions in `nonmatching/text/` are
   the beginning, and the next ones should be picked from the catalogue
   the same way — a function whose behaviour the code makes clear.
3. Identify the compiler that produced the retail build, and qualify it on
   a single function.
