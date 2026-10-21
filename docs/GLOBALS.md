# The globals of the target

The retail executable is stripped, so the game's global variables have no
names.  They do have *places*, and the places are recoverable: the
PlayStation 3's compilers address them through a **TOC** (table of
contents) held in `r2`, and the executable keeps both the TOC and the
window of globals it points at.

This document records what was measured on the target.  The catalogue
itself, with every global, its type and its access sites, is kept in the
contributor's workspace.

## How the globals are reached

The program's own initialisation stub computes the TOC base once:

```
0x10280:  li    r2, 0
          oris  r2, r2, 0xca
          ori   r2, r2, 0x6b58
          lwz   r2, 4(r2)      ; r2 = *(0x00CA6B5C)
```

`0x00CA6B58` is the **module descriptor** — the executable's entry point
— and the word at `0x00CA6B5C` gives `0x00D34770`, the TOC base.

From then on, a global is a signed 16-bit displacement from `r2`:

```
lwz   r30, -0x7e30(r2)     ; a global
lwz   r3, -0x7fd8(r30)     ; a member of the structure it points at
```

The layout that follows from it:

| Range | Size | Content |
|---|---|---|
| `0x00CA6B48` – `0x00D2C76F` | 547 KB | the module descriptor: 68 485 entries of 8 bytes, `{function address, TOC base}` |
| `0x00D2C770` – `0x00D3C76F` | 64 KB | the window of globals, addressed from `r2` |

The base `0x00D34770` sits at the exact middle of that 64 KB window, and
the window starts right after the descriptor: the layout is the
PowerPC64 **small data** convention, with the TOC holding the base of a
64 KB window instead of a pointer per global.

The window holds **the values of the globals, not their addresses**: the
retail code compares an entry with zero, and the entry at `0x00D34774`
holds `0x40000000`, which is 2.0f.  The anchor in the middle of the
window is the convention GNU ld applies by itself — the anchor of a
`.toc` section defaults to `.toc + 0x8000` — and the harness that
reproduces it is `tools/ppu_toc.py` (see [ABI.md](ABI.md)).

## What was counted

| Measure | Value |
|---|---|
| distinct globals of the main module | **5 032** |
| access sites through the TOC | **23 919** (23 424 reads, 495 writes) |
| functions that touch a global | **17 851** |
| accesses to the members of pointed-at structures | **163 319** |

The types are deduced from the way each global is used: a `lwz` alone
means a 32-bit unsigned value, a `lbz` a byte, a `lhz` a halfword, an
access of eight bytes a 64-bit value, a use followed by `extsw` a signed
integer, and a global that is dereferenced before being used a pointer.
Twenty-seven globals stay unclassified.

Most of the member offsets are negative — 91 % — which says the pointers
the game keeps point at the *end* of an object, the way a C++ compiler
lays out a class with a base at the end.

## The dispatch tables

Some globals hold tables of `{function pointer, TOC base}`: the address
of a function in another module and the TOC base to call it with.  They
are how the engine calls across modules without a direct link.

| Global | Table | Use |
|---|---|---|
| `TOC-0x4F34` | `0x00CACAA8` | 84 sites, 77 functions — the most used table of the game |
| `TOC-0xB7B4` | `0x00DDD804` | a render state table, 20 sites in 17 functions |
| `TOC-0xADB8` | `0x00CA8C68` | 32 exported functions of a linked module |
| `TOC-0xB78C` | `0x00CAEAB0` | 32 exported functions of a linked module |
| `TOC-0x83C0` | `0x00C765F8` | render constants and parameters |

The names here are placeholders built from the TOC offset; the game's own
names are lost with the symbols.  The *values* are not always the ones
the program sees: the loader relocates them, so the types come from the
usage and not from the raw value.

## What this means for the decompilation

A global that the analysis can place is a global that can be given a
type, and a type is what makes a call's prototype recoverable.  It is
also what tells apart the two halves of a structure: the members at
negative offsets and the ones at positive offsets belong to different
objects, and the 163 319 member accesses are the raw material for
reconstructing the structures the engine passes around.
