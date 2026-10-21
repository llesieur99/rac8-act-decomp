# The same engine, in other games

The engine a studio writes once is shared between the studio's titles, and
that is worth more than it looks: a function that has been reconstructed once
can be *looked for* in the other games, and a match there is an independent
confirmation of the reconstruction.

Four Insomniac titles were taken apart for this:

| Game | Year | Compiler stamp | The init function at `0x10200` |
|---|---|---|---|
| Jak 2 | 2006 | `GCC: (GNU) 4.1.1 (SDK360, $Rev: 3455 $)` | the same code |
| Jak 3 | 2004 | `GCC: (GNU) 4.1.1 (SDK360, $Rev: 3455 $)` | the same code |
| Ratchet & Clank: A Crack in Time | 2009 | `GCC: (GNU) 4.1.1 (SDK270, $Rev: 3019 $)` | **the target**, byte for byte |
| Ratchet & Clank: Tools of Destruction | 2011 | `GCC: (GNU) 4.1.1 (CELL 4.1.2.7, RTTI, $Rev: 1826 $)` | a different one |

The three first games were built with **two different SDKs** — and the
function is at the **same address** in all three, with the same shape:

```
stdu r1, -0x70(r1)     ; the frame
mflr r0                ; the link register
std r0, 0x80(r1)       ; ... saved in the caller's frame
bl  <a>                ; the first call
nop
bl  <b>                ; the second call
nop
ld r0, 0x80(r1)
mtlr r0
addi r1, r1, 0x70
blr
```

Only the *targets* of the two calls differ from one game to the next
(`0x10638`/`0x4E2518` in A Crack in Time, `0x105A8`/`0x67AF8` in Jak 3,
`0x105A8`/`0x67AE8` in Jak 2): the engine's initialisation calls live
somewhere else in each build.

Tools of Destruction no longer has that function: the engine was rewritten
between the two games, so it is a different program at that address.

## The same source, four builds

The point of building the second toolchain is that it makes the check
independent.  The source of `module_init` was compiled with the SDK 3.6.0
compiler and linked at `0x10200` with **Jak 3's** and **Jak 2's** call
targets, then compared with those games' own bytes:

| Game | Compiler | Callee targets | Result |
|---|---|---|---|
| A Crack in Time | SDK 2.7.0 (rev 3019) | `0x10638`, `0x4E2518` | **44 of 44 bytes** |
| Jak 3 | SDK 3.6.0 (rev 3455) | `0x105A8`, `0x67AF8` | **44 of 44 bytes** |
| Jak 2 | SDK 3.6.0 (rev 3455) | `0x105A8`, `0x67AE8` | **44 of 44 bytes** |
| Jak and Daxter: The Precursor | SDK 3.6.0 (rev 3455) | `0x105A8`, `0x59BB8` | **44 of 44 bytes** |

One source, four games, two SDKs, and the same forty-four bytes every
time.  What that rules out:

* the flags are not a coincidence of one build — the same options reproduce
  the function in a compiler two SDK revisions apart;
* the source is not a shape that happens to fit — it fits three different
  call targets, so the calls really are calls and the layout really is the
  compiler's;
* the linker model is right — a candidate linked at the retail address with
  the retail callees lands on the retail bytes, in every game.

This is the strongest evidence the project has that the method is sound.
It is still not a match: the body gate is open, the complete-program gate
is closed, and the counts stay what they are (see [GATES.md](GATES.md)).

## How the search is done

The addresses are not the same from one game to the next, so the search
compares the *shape* of the code: the displacement of every branch is
blanked, and what is left is compared.  `tools/ppu_signature.py` does this:

```
python tools/ppu_signature.py <reference.elf> <address> <length> \
    --search <other.elf> --search <another.elf>
```

```
$ python tools/ppu_signature.py eboot.elf 10200 44 \
      --search jak3.elf --search jak2.elf --search tod.elf
jak3.elf: found at 0x10200
jak2.elf: found at 0x10200
tod.elf: not found
```

This is what turned the first byte-exact body into a cross-check: the 44
bytes reproduced from source in A Crack in Time are the same 44 bytes that
Jak 2 and Jak 3 carry, so the source is not a coincidence that happens to
fit one build.

## The shared code of the two Insomniac games

*A Crack in Time* and *Tools of Destruction* are the same studio's engine,
one generation apart, and the signature search finds **3.19 %** of the
target's functions in the other game: 721 matches over 22 592 functions.
The matches are not spread out — they form blocks:

| Shared block | In A Crack in Time | In Tools of Destruction |
|---|---|---|
| the third-party library | `0x8F32E0` – `0x925F78` | `0x83F348` – `0x85FC88` |
| the engine module | `0x6DB0AC` – `0x6E6738` | `0x6EE0D0` – `0x6FCC44` |

Inside those blocks the two builds agree on the *structure* — on twenty
pairs, 73.8 % of the instructions are identical byte for byte, and some
pairs run for 258 instructions without a difference — but not on the
registers.  That is what the [link register's register](ABI.md) section is
about, and it is measured on this shared code: **the same source,
compiled by two compilers, keeps the link register in `r0` in one build
and spreads it in the other.**

The initialisation function of the engine is in both games, at `0x10200`
in the target and at `0x10210` in the older one — the same frame, the
same two calls, the same epilogue.  Only one thing differs: the older
game uses the delay slot of its second call to restore the TOC base
(`ld r2, 0x28(r1)`) where the target has a `nop`.  The older game also
stacks 1,036 four-instruction stubs (`std r2, 0x28(r1)` / `addis` /
`addi` / `b`) between `0x10280` and `0x14330` to register the routines,
where the target calls the registration routine directly.

**What that means for the project:** the older game's code is a *reading*
of the same engine, compiled by a compiler whose behaviour the public
sources reproduce.  It is the closest thing to a reference for what the
engine does, and the naming work has a second source to work from.

## The PlayStation 4 was looked at, and there is nothing to read

The share holds thirty-one PlayStation 4 packages, and the obvious
question was whether one of them would show whether the PlayStation 4's
chain of compilers mixes the registers as well.  It does not answer it:
**every one of those packages is an "extra package"** (`\x7FCNT`, the
format Sony's PlayGo uses), not a `\x7FPKG`, and their volumes are
encrypted — a full entropy map of the largest one measures 7.997 bits per
byte, and a byte-for-byte search of two of them finds no executable at
all.

So no PlayStation 4 code was readable, and the question was answered
without it: *Uncharted*, built with the same compiler as *Tools of
Destruction*, keeps the link register in `r0` in every function.  The
PlayStation 4's chain was never needed to tell a compiler difference
from a code difference.

The reader that would do it — it walks the container, looks for any clear
ELF64 big-endian PowerPC executable and hands it to the tools of this
repository — was written and kept in the contributor's workspace, ready
for a source that is not encrypted.

## What the other games teach about the analysis

**The code segment cannot be recognised by its size.**  The PPU executables
carry the SCE flags next to the usual permission bits, and in Jak 3 a 64 MiB
*data* segment has the executable bit set.  Choosing the largest executable
segment therefore scans the wrong memory in that game (and in Tools of
Destruction).  The code is the *lowest* segment that carries a payload, and
both tools now use that rule.  On the target it makes no difference — the
published catalogue of 8,271 addresses is unchanged — but on the other games
it is the difference between a catalogue and a list of addresses in the data.

**The prologue pattern is a property of the SDK version.**  The scan is
calibrated on the target's SDK, and it is weaker on another one:

| Game | prologue candidates | call targets | both |
|---|---|---|---|
| A Crack in Time (SDK 2.7.0) | 22,592 | 26,280 | 8,271 |
| Jak 3 (SDK 3.6.0) | 17,551 | 3,853 | 2,191 |

The target's code calls a lot of different functions (26,280 of them), so
the two methods agree often (37 % of the candidates).  Jak 3's code calls a
much smaller set of functions (3,853), so the calls say much less about
where the functions are, and the agreement falls to 12 %.  A catalogue built
with one SDK's pattern is not a catalogue for another SDK, and the two have
to be kept apart.

**The two SDKs agree on this function's frame.**  In all three games the
link register is saved in `r0` and stored in the caller's frame at
`0x80(r1)`, and the TOC base (`r2`) is not restored — even in the 3.6.0
build.  The prologue rules measured on the target therefore hold for the
other SDK as well, at least for this function.
