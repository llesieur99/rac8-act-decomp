# Where the project stands

**Date:** 2026-10-21

## What exists

| Item | State |
|---|---|
| the package is read and measured | done |
| the package reader is published, with tests | done |
| the SELF container is described | done |
| the retail executable is decrypted and rebuilt into an ELF | done, in the contributor's local workspace |
| the PPU ELF reader is published, with tests | done |
| the first function catalogue | **done**: 8,271 addresses where two independent methods agree ([CATALOGUE.md](CATALOGUE.md)) |
| the third method (a disassembler's analysis) | **done**: 105 of its 106 functions are call targets of the published scan |
| the compiler | **identified and built**: SCEI PPC GCC 4.1.1 (SDK270), compiled from its own sources ([COMPILER.md](COMPILER.md)) |
| the first byte-exact body | **done**: `module_init` at 0x10200, 44 bytes reproduced ([src/text/module_init.c](../src/text/module_init.c)) |
| the other readings | in progress: three functions are a few bytes away |
| a qualified PPU compiler and its options | **provisional**: `-O1 -mno-save-restore-tocbase` reproduces one function |
| the same engine in other games | **done**: the matched function is in Jak 2 and Jak 3, at the same address ([CROSS-GAME.md](CROSS-GAME.md)) |
| a second SDK's toolchain | **done**: SDK 3.6.0 built from its own sources; the same source matches 44 of 44 bytes in Jak 3 and in Jak 2 |

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

## The measurements

With the SDK's compiler and the flags `-O1 -mno-save-restore-tocbase`:

| Function | Address | Result |
|---|---|---|
| `module_init` | `0x10200` | **match**, 44 bytes |
| `construct_local_buffer` | `0x25D98` | matches up to the fifth byte; the frame and the layout agree |
| `call_with_49_and_1` | `0x10854` | matches up to the third byte; the retail frame is 0x10 larger |
| `read_global_and_call` | `0x3B704` | matches up to the first byte |
| the block at `0x21B5C8` | — | turned out **not to be a function**: no prologue, no caller, and it ends another function |

The last line is the kind of result the catalogue is for: the address was a
candidate, and the evidence says it is a block inside a bigger function.

The three that do not match yet all fail the same way: the compiler keeps
the link register in `r0` where the retail code keeps it in `r3`, `r4` or
`r5`.  The register is the allocator's choice, and the retail's choice
follows the register the first argument uses — so the source form that is
still missing is the one that makes the compiler reuse that register.
What is measured across the whole executable:

| Link register | Functions |
|---|---|
| `r0` | 8,323 (37 %) |
| `r4` | 5,681 (25 %) |
| `r5` | 4,900 (22 %) |
| `r3` | 627 (3 %) |
| the others | the rest |

## What the measurements taught about the ABI

Three things came out of the comparisons and are written up in
[ABI.md](ABI.md):

* the `nop` after a call depends on whether the **callee is known** to the
  compiler — so the grouping of the functions into translation units is part
  of what has to be recovered;
* the TOC base (r2) is restored by some functions and not by others, so the
  build's flags are not the same everywhere;
* the epilogue comes in two shapes (`mtlr` before or after the stack
  restore) and the shape is a property of the function.

A candidate that reproduces the retail bytes therefore has to reproduce the
*structure* of the original build, not only the instructions of one
function.

## The next step

1. Close the three other readings: each is a few bytes away, and the
   difference says what the source form still gets wrong — in all three,
   the register the compiler keeps the link register in.
2. Work the catalogue from the top: the functions whose behaviour the
   analysis makes clear, one at a time, each one ending in a comparison.
3. Qualify the flags on more than one function, so that
   `config/compiler.json` stops being provisional.
4. Extend the cross-game check: the same source, the same options and the
   same bytes in three games already; the next check is a function that is
   *not* the engine's initialisation.
