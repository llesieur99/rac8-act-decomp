# The compiler

A matching decompilation is only possible with the compiler that produced
the retail bytes. This document records which one that is, how the answer
was obtained, and what can be done with it.

## The answer

The retail executable identifies its compiler itself, once per linked
object, in the `.comment` section:

| Stamps | Compiler |
|---|---|
| 106 | `GCC: (GNU) 4.1.1 (SDK270, $Rev: 3019 $)` |
| 1 | `GCC: (GNU) 4.1.1 (SDK250, $Rev: 2824 $)` |

So the game was built with **Sony's PPC GCC 4.1.1**, from the PlayStation 3
SDK, revision `SDK270` (the SDK's own revision `3019`). One object — a
prebuilt library, most likely — was compiled a bit earlier with `SDK250`
(revision `2824`).

The game's `PARAM.SFO` asks for system version `04.3000`, which is the
firmware that shipped with SDK270: the two agree, and the executable is a
`4.1.1` build.

The stamp is read with the published tool, so the answer is reproducible:

```sh
python tools/compiler_stamp.py work/eboot/EBOOT.elf --libraries
```

### Where the stamp comes from

The stamp is not the game's doing: the *compiler* writes it. Sony's
toolchain sources carry it in `gcc/gcc/version.c`:

```c
#define VERSUFFIX " (SDK270, $Rev: 3019 $)"   /* the SDK 2.7.0 toolchain */
#define VERSUFFIX " (SDK250, $Rev: 2824 $)"   /* the SDK 2.5.0 toolchain */
```

So every object a toolchain compiles carries its own stamp, and the stamps
found in an executable are a list of the toolchains that produced it. A
stamp that appears **once** in an executable, and that also appears in
other games, is a *prebuilt library* the SDK froze and reused.

### The one odd object

Comparing the stamps of other games makes the pattern clear. Each
executable was read the same way (package → executable → SELF → stamps):

| Game | Stamps |
|---|---|
| Ratchet & Clank: A Crack in Time (2009) | 106 × `SDK270 rev 3019`, 1 × `SDK250 rev 2824` |
| Ratchet & Clank: Tools of Destruction (2011) | 24 × `CELL 4.1.2.7 rev 1826`, 8 × `CELL 4.1.2.7 rev 1830` |
| Uncharted: Drake's Fortune (2007) | 24 × `CELL 4.1.2.7 rev 1826`, 8 × `CELL 4.1.2.7 rev 1830` |
| LittleBigPlanet (2008) | 205 × `SDK310 rev 3320`, 9 × `CELL 4.1.2.7 rev 2184` |
| Jak 3 | 6 × `SDK360 rev 3455` |
| Assassin's Creed (2007) | 172 × `CELL 4.1.28 rev 1855`, 65 × `CELL rev 1826`, 60 × `CELL rev 1757`, 14 × `CELL rev 1830` |

The `CELL 4.1.2.7` revisions (1757, 1826, 1830, 2184) show up in *several*
titles with the *same* revision numbers: they are the SDK's prebuilt
libraries, compiled once and linked into every game. Later SDKs changed the
stamp's wording, from `SDK<version>` to `CELL <gcc version>, RTTI`.

By that rule, the single `SDK250 rev 2824` object in *A Crack in Time* is
**a prebuilt SDK library frozen at SDK 2.5.0** — the same kind of artefact
as the runtime library, not the studio's code. It is out of the matching
scope, exactly like a libc is out of the scope of a matching
decompilation.

That leaves a clean statement of the matching scope:

* the game's own code: **106 objects, built with the SDK 2.7.0 toolchain**;
* one prebuilt SDK library object: not the game's, not matched;
* the rest of the executable: the SDK's runtime, the assets, and the
  engine's data.

## What else the executable says

The same strings name the libraries the game was linked against:

| Library | Identification |
|---|---|
| the C++ library | Dinkumware (the SDK's `libstdc++`, "Copyright (c) 1992-2003 by P.J. Plauger") |
| the script engine | `Lua 5.1.2 (Insomniac)`, the studio's own fork |
| compression | `zlib 1.2.3` |

The Lua fork matters for the port: the game's logic is partly scripted, and
the scripts are data the project will have to read.

## The two routes

### Route A — the byte-exact route

Byte-exact matching needs Sony's PPC GCC 4.1.1 (SDK270) itself, plus the
SDK's libraries, its linker script and its C++ headers.

* The SDK was never sold outside the developer programme, and that
  programme closed years ago. There is no commercial channel for it today.
* The toolchain is a **tool**, like Sony's SN ProDG in the sibling
  PlayStation 2 projects: the contributor brings their own copy, uses it
  locally, and publishes only the written result. `LEGAL.md` states the
  boundary — the repository never ships a proprietary toolchain, a
  confidential source package, an SDK header or an SDK library.
* The project does not describe how to obtain the SDK, and it does not
  publish or mirror any part of it.
* The toolchain's own sources are what make the route practical: the Cell
  OS Lv-2 toolchain (SDK 2.7.0, GCC 4.1.1) is GCC 4.1.1 with the Cell
  target, and it builds on an ordinary Linux system. The build takes a
  container with a period-appropriate host compiler, which is what the
  contributor's workspace does.
* Whatever the toolchain, the *evidence* stays the same: the bytes are
  compared, and the gate is the complete-program hash.

The first experiment with this route is the `body gate` on one of the
functions read so far: compile the source with the SDK's compiler, link
it at the measured address, and compare every byte
([tools/compare_body.py](../tools/compare_body.py) does the comparison).

### The first result

The compiler is built from its own sources, in a container, with a host
toolchain old enough to build GCC 4.1.1.  It reports itself as

```
ppu-lv2-gcc (GCC) 4.1.1 (SDK270, $Rev: 3019 $)
```

which is the very stamp the retail executable carries.

The target's own runtime comes with it: the build produces the PPU
`libgcc.a` (499,648 bytes) in the SDK's three multilib variants (the
default one, `fno-exceptions` and `fno-exceptions/fno-rtti`), which is the
library the retail executable links against.  The SDK's C++ library is a
*separate* prebuilt archive that is not part of the toolchain sources, so
C++ can be compiled to an object file but not linked yet; that does not
affect the matching, which compares object code.

That runtime is checked the same way as the game: the retail executable
links libgcc statically, so the library's own code is in the text, and the
functions the game uses are **byte-identical** to the ones our build
produces:

| Function | Bytes | In the retail executable at |
|---|---|---|
| `frame_downheap` | 320 | `0x918B48` |
| `read_sleb128` | 88 | `0x9155A0` |
| `read_uleb128` | 56 | `0x915568` |
| `_Unwind_DeleteException` | 80 | `0x915630` |
| `fde_unencoded_compare` | 40 | `0x918B20` |

The other functions of the library are simply not linked into the game:
the linker only pulls what the program needs, so their absence from the
retail executable is not a difference.

One option turned out to matter more than the optimisation level:
**`-mminimal-toc`**.  It makes the module's TOC hold one entry per
procedure instead of one entry per global, and it changes the size of the
frame — 0x80 bytes instead of 0x90 in the functions that use it.  The
retail's files use it or not, so the options are a property of the *file*,
not of the build.

**`-mno-save-restore-tocbase`, which the SDK adds, is inert in this
fork.**  The variable it sets is written once in the backend and never
read: the option is accepted, it sets a flag, and nothing changes.  That
was measured rather than assumed — the functions that are byte-exact
reproduce the retail bytes with the option, without it, and with it added
to a set that already worked.

The first functions were reproduced like this:

| Function | Address | Bytes | Flags | Result |
|---|---|---|---|---|
| `module_init` | `0x10200` | 44 | `-O1` | **match** |
| `module_link_6FA4F8` | `0x6FA4F8` | 72 | `-O2` | **match** |
| `module_link_6FA6D0` | `0x6FA6D0` | 72 | `-O2` | **match** |
| `float_to_short_6E1C78` | `0x6E1C78` | 80 | `-O2` | **match** |
| `construct_local_buffer` | `0x25D98` | 44 | `-O1 -fschedule-insns2` | **match** |
| `Object::call_with_49_and_1` | `0x10854` | 44 | `-O1 -mminimal-toc -fschedule-insns2` | **match** |
| `read_global_and_call` | `0x3B704` | 56 | `-O2 -mminimal-toc -falign-functions=2` | **match** |

The flags are therefore recorded in
[`config/compiler.json`](../config/compiler.json) per function, and they
are re-verified by `tools/verify_sources.py`.

### The second toolchain

The SDK 3.6.0 toolchain (GCC 4.1.1, revision 3455) was built the same way,
from the same sources, with the same recipe.  It is the compiler **Jak 2
and Jak 3** were built with, so it is the one that can check a source
against a second game:

```
ppu-lv2-gcc (GCC) 4.1.1 (SDK360, $Rev: 3455 $)
```

With it, the source of `module_init` reproduces **44 of 44 bytes** in Jak 3
and in Jak 2 — the same source, the same options, and the same bytes as in
*A Crack in Time* (see [CROSS-GAME.md](CROSS-GAME.md)).  Two toolchains two
SDK revisions apart agreeing on forty-four bytes is what turns the flags
from a guess into a measurement.

### Where the rebuilt compiler is not the retail one

The toolchains built here are not a perfect reproduction of the compilers
that built the games.  The difference is measured and documented in
[ABI.md](ABI.md): the rebuilt compiler always keeps the link register in
`r0`, while two of the retail executables spread it over `r0`, `r4`, `r5`
and `r6`.

The cause is now located.  In the compiler's own source, the register that
holds the link register is written in four places, all of them hard-coded
to `r0`:

* the prologue's `mflr` — the move that takes the link register into a
  general register;
* the prologue's `std` — the store of that register in the frame;
* the epilogue's `ld` — the reload of the saved link register;
* the epilogue's `mtlr` — the move that puts it back.

The retail's compiler does not do that: it lets the register allocator
choose, and the proof is in the binary — the register it chooses is dead
on entry and dead on exit in **23,333 of 23,333** cases, and it is reused
right after the save (`mflr r5` / `std r5, …` / `li r5, 0`), which is the
signature of an allocator and not of a fixed register.

**The patch.**  A change to those four places that takes the register from
a target option reproduces the retail's shape:

| Option | Effect |
|---|---|
| `-mps3-lr-reg=N` | the prologue keeps the link register in `rN` |
| `-mps3-lr-restore-reg=N` | the epilogue reloads it into `rN` |

The default is the current behaviour, so the change is backwards
compatible.  **The patch is published** — it touches the two files of the
backend that are upstream GCC and that carry the Free Software
Foundation's GPL notice, and nothing else; see
[`patches/README.md`](../patches/README.md) for the licence and the
details.

**What it buys.**  14,520 of the target's 23,333 functions with a frame
are blocked on that choice; the option reaches 11,541 of them with
`r4`, `r5` and `r6` alone, and 12,310 with `r7`.  The faithful repair
would move the save into the RTL so that the allocator assigns the
register as the retail's compiler does; that is the next thing to try.

**And the option is not the whole answer.**  The register is only half of
it: the *order* of the epilogue's `mtlr` and the stack restore is not
fixed by the source either, and the reconstruction puts the stack restore
first where the retail keeps the `mtlr` first in 87 % of its functions.


### Route B — the open route

The PPU is an open target, and there are open toolchains for it:

| Project | What it gives |
|---|---|
| [ps3dev/ps3toolchain](https://github.com/ps3dev/ps3toolchain) | GCC 9.5.0 or 13.2.0 with the PPU patches, plus binutils and newlib; it builds from source in Linux (WSL or Docker) |
| [ps3dev/ps3libraries](https://github.com/ps3dev/ps3libraries) | open implementations of the console's runtime libraries |
| [rpcs3/PS3DK](https://github.com/rpcs3/PS3DK) | a modern open SDK: newer GCC, binutils and newlib, C++20 on the PPU |

These are legal, and they build here (the machine has WSL 2 with Ubuntu and
Docker). But they are **not** the compiler that produced the retail bytes:
GCC 9 or 13 lays out the code differently from GCC 4.1.1, so:

* the **body gate** cannot be closed with them — the bytes will differ;
* a **semantic** reconstruction of the game in C++ can be built, which is
  what the native port needs, and which the recompilation projects
  ([ps3recomp](https://github.com/sp00nznet/ps3recomp) and the others listed
  in [REFERENCES.md](REFERENCES.md)) already show is possible;
* the two routes are not exclusive: the sources written for Route B are the
  same sources Route A would have to match, and the project's rules
  (authored form, no assembly, original behaviour) are the same in both.

## What the project does with this

1. The compiler identity is published, with the tool that reads it, so
   that the fact is reproducible and not hearsay.
2. The sources are written to be *authentic* — the form the compiler would
   have produced — so that Route A stays open if a contributor has the
   toolchain, and so that Route B produces readable code.
3. The gates stay as they are. With Route B, no byte-exact claim is made,
   and the project's counts remain what the gates accept: nothing.

## Building the open toolchain here

```sh
git clone --recursive https://github.com/ps3dev/ps3toolchain.git
cd ps3toolchain
./download.sh          # fetches the compiler sources
./toolchain.sh 001     # binutils for the PPU
./toolchain.sh 002     # GCC and newlib for the PPU
```

The build wants Linux, `autoconf`, `bison`, `flex`, `libelf`, `texinfo` and
a few more; WSL 2 with Ubuntu or the project's own Docker image both work.
The result lands in `/usr/local/ps3dev`, with the PPU compiler in
`/usr/local/ps3dev/ppu/bin`.
