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

One flag turned out to matter more than the optimisation level:
**`-mno-save-restore-tocbase`**.  Without it, the compiler keeps a TOC
base and a back chain in the frame, which the retail code does not have.
With it, the prologue and the epilogue of the first function came out
byte for byte identical to the retail ones, and the whole function
matched:

| Function | Address | Bytes | Flags | Result |
|---|---|---|---|---|
| `module_init` | `0x10200` | 44 | `-O1 -mno-save-restore-tocbase` | **match** |

At `-O2` the same function differs by two instructions: the compiler
restores the stack pointer before the link register, where the retail
code does the opposite.  The flags are therefore recorded in
[`config/compiler.json`](../config/compiler.json) as *provisional* — one
function is not enough to qualify a build.

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
