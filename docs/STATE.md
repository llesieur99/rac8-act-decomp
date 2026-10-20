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
| the function boundaries | **not started** |
| the first C source | **not started** |
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

1. Run a PPU analysis over the text segment (Ghidra's `PowerPC:BE:64`
   language reads it well) and produce a first catalogue of function
   boundaries.
2. Cross-check the boundaries with an independent method (a prologue
   scanner, the call graph, the data the code touches) so that the
   catalogue is not a single tool's opinion.
3. Start the first C sources on the functions the analysis names with
   confidence, and record what each one proves.
