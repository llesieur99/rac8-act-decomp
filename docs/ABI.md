# The Cell OS Lv-2 ABI, as the code shows it

What the retail executable teaches about how the SDK's compiler lays code
out.  Every statement here was measured: either by reading the compiler's
own sources (the SDK's `gcc/config/rs6000/`) or by compiling a candidate and
comparing the bytes with the retail ones.

## The two options that are not in the upstream GCC

The SDK adds two target options to the rs6000 backend
(`gcc/config/rs6000/lv2.opt`):

| Option | Effect |
|---|---|
| `-mno-nop-after-bl` | does not generate a `nop` after a `bl` |
| `-mno-save-restore-tocbase` | does not save or restore the TOC base (r2) |

A third one, `-mcellos-kernel`, turns both of them on and adds
`-falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1`.

**The second one is inert in this fork.**  The variable it sets is written
once in the backend and never read: the option is accepted, it sets a
flag, and nothing changes.  This was measured, not assumed: the functions
that are byte-exact reproduce the retail bytes with the option, without it,
and with it added to a set that already worked.

The option that *does* change the code is **`-mminimal-toc`**, which is
one of the generic rs6000 options: it makes the module's TOC hold one
entry per procedure instead of one entry per global, it moves the
secondary table to a section of its own, and it changes the size of the
frame.  The retail's code uses it in some files and not in others — the
frame of the function at 0x25D98 is 0x90 bytes, the default TOC, while the
frame of the function at 0x10854 is 0x80 bytes, `-mminimal-toc` — and the
options are a property of the file, not of the project.

The retail code keeps the `nop` after most calls **and** restores the TOC
base in some functions, so neither option was turned on by default.

## The `nop` after a call depends on the callee

This is the most surprising result, and it was found by comparing a
candidate with the retail bytes:

* a call to a function the compiler **knows** (same translation unit) is
  followed by no `nop`;
* a call to a function the compiler does **not** know is followed by a
  `nop`.

The rule is the one the PowerPC ELF ABI imposes: the caller's stack must be
16-byte aligned when the callee starts, so a `nop` is inserted when the
callee's prologue needs it — and the compiler can only know that when the
callee is in the same translation unit.

In the retail executable, 2,575 of the 2,580 catalogue functions open with
no `nop` after their prologue, four with one, and a single one with two.
The two are a property of that one function's source, not of the build.

The rule was confirmed by experiment: a candidate that puts the callee in
the same translation unit loses the `nop` after the call to it, and a
candidate that leaves it external keeps it.  The retail function at
`0x10378` calls `0x102F0` with no `nop` and `0xA48E04` with no `nop`
either, so **both callees are in the same translation unit as the caller** —
while the function at `0x10200`, whose calls do carry a `nop`, calls
functions the compiler could not see.

**Consequence for the project:** the grouping of the functions into
translation units is part of what has to be recovered.  A candidate that
calls a function the compiler cannot see will not match, however good its
source is.

The executable keeps the grouping itself.  Its entry point is not code
but the **module descriptor**: a table of `(function address, module
base)` pairs that lists what every linked module exports, in link order.
The functions of one link unit are contiguous in it, so the table says
which functions were compiled together.  `tools/ppu_modules.py` reads it
and checks the rule against the code:

```
$ python tools/ppu_modules.py work/eboot/EBOOT.elf --check
module descriptor at 0xCA6B58: 68490 functions
  16121 link units, the largest ones holding 1041, 1022, 1014, 711, 588, 561, 448, 273 functions
  8 distinct module bases
  call to a function of the same unit : no nop 737, nop 2080
  call to a function of another unit  : no nop 264, nop 20570
```

98.7 % of the calls that cross a link unit carry the `nop`, and the calls
inside a unit are the ones that do not: the grouping is real, and it is
the grouping the compiler reacted to.  The remaining difference inside a
unit is what one expects — the unit of the descriptor is the module, and
the compiler only knows a callee that the *same object file* defines.

## The TOC base

`r2` holds the TOC (the table of contents) in the Cell OS Lv-2 ABI.  A
function that uses globals gets its TOC pointer from the chain in the
caller's frame (`ld r2, 0x28(r1)`), and a function that calls another one
restores it afterwards, because the callee may have changed it.

That is why some retail functions end with a `ld r2, 0x28(r1)` and others
do not: it depends on whether the calls they make can change `r2`.

The compiler's own source says exactly when the restore is emitted.  The
function `uses_TOC()` walks the function's RTL looking for a
`use (unspec ... UNSPEC_TOC)`, and that use is only generated where the
function *loads the TOC base* — that is, where it references a global:

```c
/* This returns nonzero if the current function uses the TOC.  This is
   determined by the presence of (use (unspec ... UNSPEC_TOC)), which
   is generated by the ABI_V4 load_toc_* patterns.  */
```

So the restore is not a property of the calls a function makes: it is a
property of the *function*, and a function that never names a global does
not restore the TOC, whatever it calls.

### How the globals are addressed, and how a candidate reproduces it

The module keeps its TOC base in `r2`, and a global is a signed 16-bit
displacement from it.  The geometry was measured:

| | |
|---|---|
| the TOC base (`r2`) | `0x00D34770` |
| the table of entries | `0x00D2C770` |
| the window | 64 KiB, `0x00D2C770` … `0x00D3C76F` |

The base sits **at the exact middle of the window**, which is the
convention GNU ld applies by itself: the anchor of a `.toc` section
defaults to `.toc + 0x8000`, and the displacement of an `R_PPC64_TOC16`
relocation is the distance from the anchor to the entry.  The SDK's
linker therefore computes the right displacement by itself, as soon as the
global lands on the right entry.

Two things had to be measured to make that work:

* **the entries hold the values of the globals, not their addresses.**  The
  retail code compares an entry with zero, and the entry at `0x00D34774`
  holds `0x40000000`, which is 2.0f — a table of addresses would hold
  neither.
* **the SDK's compiler cannot produce the retail's single
  `lwz rX, off(r2)`.**  Its model of the TOC is the address one, which
  takes two instructions.  The harness therefore rewrites the
  `R_PPC64_TOC16` relocations of the candidate so that every global
  points at its retail entry, and rebuilds the `.toc` section so that it
  really holds the entries at their retail addresses.

`tools/ppu_toc.py` is that harness.  Measured on the target, **eleven of
eleven** TOC accesses come out with the retail's displacement, and **six
of eleven** real functions are reproduced byte for byte.  The five that
are not are accounted for: the order of the floating-point operands, the
choice of a register, and one structural case — GCC 4.1.1 does not fold a
*negative* displacement into a load.

### The `nop` after a call has nothing to do with the TOC

It is the stack alignment rule of the ABI, and the compiler removes it
only when the callee is in the same translation unit.  In the 17,851
functions of the target that use the TOC, **88,590 of the 95,327 calls
(92 %)** are followed by one.

That is directly visible between the two Insomniac games.  The engine's
initialisation function is at `0x10200` in *A Crack in Time* and at
`0x10210` in *Tools of Destruction*, with the same frame, the same two
calls and the same epilogue — but the older one restores the TOC base in
the delay slot of its second call:

```
0x10210:  stdu   r1, -0x70(r1)
           mflr   r0
           std    r0, 0x80(r1)
           bl     0x146ac
           nop
           bl     0x10200
           ld     r2, 0x28(r1)      <- the TOC base, restored
           ld     r0, 0x80(r1)
           mtlr   r0
           addi   r1, r1, 0x70
           blr
```

while the newer one has a `nop` there.  The older engine's initialisation
named a global; the newer one's does not.  It is one more sign that *A
Crack in Time* was recompiled against a newer engine — see
[CROSS-GAME.md](CROSS-GAME.md).

The option `-mno-save-restore-tocbase` removes that restore as well as the
prologue's save.  The retail executable has functions with the restore and
functions without, so **the flags are not the same everywhere**: the file
that holds the function at `0x10200` was built with the option, and the
file that holds the function at `0x10378` was not.  Qualifying the build
means qualifying it *file by file*.

### The delay slot of a call that leaves the module

The SDK's linker ships a fix-up tool, `ppu-lv2-prx-fixup`, that the
public toolchain sources do not carry.  What it does was measured on the
retail executable: it visits every `bl` whose target is **outside** the
module's descriptor — every call that leaves the module — and replaces the
`nop` the compiler put in the branch's delay slot with
`ld r2, 0x28(r1)`, which restores the TOC base from the caller's frame.

Of the **161,564** calls that leave the module, **999** carry that
instruction, and every one of them follows a call the descriptor does not
name.  The three exceptions the rule cannot see are the calls to the
module's own entry point, which is not exported, and the calls to the
stub area.

`tools/ppu_prx.py` does the fix-up, and applying it to the retail
executable reproduces it byte for byte.

## The model the compiler works in

Three properties of the SDK's code generation were measured while
reproducing the target's functions:

* **the model is ILP32**: pointers are 32 bits — every narrowing is a
  `clrldi` — while `long` is 64 bits, so an `int` passed to a `long`
  parameter produces an `extsw`; an `unsigned int` passed to a `long`
  produces one as well, and an `int` does not.  That is what forces the
  type of a parameter in a candidate;
* **an address-taken parameter is copied into the parameter area** at
  `SP + 0x30` and read back on the way out;
* **`-mminimal-toc` changes the frame**: 0x80 bytes instead of 0x70, and
  the link register goes to the ABI slot of the *caller's* frame.  The
  options are not interchangeable — a function with a 0x70-byte frame
  needs the default TOC and forbids `-mminimal-toc`.

## The link register's register

The SDK's prologue moves the link register into a general register before
it stores it (`mflr rX` / `std rX, offset(r1)`), and the epilogue always
reads it back into `r0` (`ld r0, offset(r1)` / `mtlr r0`).  Which register
the prologue uses is the register allocator's choice, and it is worth
measuring, because it is where the rebuilt toolchain and the retail build
disagree.

Measured over every function of the catalogue of each executable
(`python tools/ppu_lr.py <eboot.elf>` does the measure):

| Executable | Compiler | `r0` | `r4` | `r5` | `r6` | `r3` |
|---|---|---|---|---|---|---|
| A Crack in Time | SDK270 rev 3019 | 37 % | 25 % | 22 % | 6 % | 3 % |
| Tools of Destruction | CELL 4.1.2.7 rev 1826 | 48 % | 28 % | 9 % | 5 % | — |
| Jak 2 | SDK360 rev 3455 | **100 %** | — | — | — | — |
| Jak 3 | SDK360 rev 3455 | **100 %** | — | — | — | — |
| Jak and Daxter: The Precursor | SDK360 rev 3455 | **100 %** | — | — | — | — |
| LittleBigPlanet | SDK310 rev 3320 | **100 %** | — | — | — | — |
| Uncharted: Drake's Fortune | CELL 4.1.2.7 rev 1826 | **100 %** | — | — | — | — |
| Assassin's Creed | CELL 4.1.28 rev 1855 | **100 %** | — | — | — | — |

Two families appear.  In most executables the link register is **always**
kept in `r0`.  In the two newest Insomniac games it is spread over `r0`,
`r4`, `r5` and `r6` — and there the rule is visible: in every function that
calls something, the register the link register is kept in is the one the
first argument is passed in.

It is not the age of the engine: *Jak and Daxter: The Precursor* is the
newest Insomniac engine (the one *Rift Apart* is built on) and it is built
with the same SDK as *Jak 2* and *Jak 3* — and it keeps the link register
in `r0` in every function, like they do.  It is not the SDK either: the two
games that mix the registers were built with two different SDKs, and
*Uncharted*, built with the same one as *Tools of Destruction*, keeps the
link register in `r0` in every function.  What is left is the code itself:
the sources of those two games hold patterns the other games do not.

The rebuilt toolchains do not reproduce the second family: both the SDK
2.7.0 and the SDK 3.6.0 compiler built here keep the link register in `r0`
in every case, in C and in C++, at every optimisation level, with and
without `-mno-save-restore-tocbase`, and with the TOC options
(`-mminimal-toc`, `-mno-toc`, `-mbase-toc`) turned on and off.  Nor does
the SDK 2.8.0 or the SDK 4.2.0 branch of the same repository.

### The proof: the same source, two compilers, two choices

*A Crack in Time* and *Tools of Destruction* share large blocks of code —
the studio's third-party libraries and its engine module — which the
signature search finds across the two executables.  In those blocks, the
same source is compiled by two different compilers, and the two builds do
not choose the same register:

| Shared block | A Crack in Time (SDK270) | Tools of Destruction (CELL 4.1.2.7) |
|---|---|---|
| the third-party library | r5 48 %, r0 37 %, r6 5 % | **r0 100 %** |
| the engine module | r0 51 %, r5 15 %, r4 13 % | **r0 99 %** |

The choice follows the compiler, not the source: the same code, compiled
twice, keeps the link register in `r0` in one build and spreads it in the
other.  And since the compiler built here always chooses `r0`, **the
compiler that built the target is not the one the public sources
describe**.

The epilogue says where the difference is.  In the compiler's own source, the link
register is read back into `r0` and nowhere else:

```c
/* Get the old lr if we saved it.  */
if (info->lr_save_p && (info->lr_save_inline_p || sibcall))
  {
    ...
    emit_move_insn (gen_rtx_REG (Pmode, 0), mem);
  }
```

The retail's epilogues use `r3`, `r4`, `r31` and others — 43 % of the
target's epilogues do not use `r0`.  Every branch of the public toolchain
repository (1.8.0, 1.9.0, 2.2.0, 2.4.0, 2.5.0, 2.7.0, 2.8.0, 3.0.0 to 3.7.0
and 4.2.0) has that same line, and the compilers built from them (2.7.0,
2.8.0, 3.6.0 and 4.2.0) all keep the link register in `r0`.

The two games that mix the registers were built by two different
compilers, and neither of them is in that repository:

| Game | Stamp | In the public repository? |
|---|---|---|
| A Crack in Time | `SDK270 rev 3019` | the branch exists, but its epilogue is the one above |
| Tools of Destruction | `CELL 4.1.2.7 rev 1826` | **no**: no branch of the repository carries that stamp |

The `CELL 4.1.2.7` wording belongs to the PlayStation 4's compiler, and
the repository stops at the PlayStation 3's SDKs (3.7.0 is the last one
before it, and it stamps `SDK370 rev 3509`).  So the two games that mix the
registers are the two whose compiler's register allocation was never
published, and the games whose compiler *was* published - Jak 2, Jak 3,
Jak and Daxter, LittleBigPlanet, Uncharted, Assassin's Creed - keep the
link register in `r0` in every single function.

That is the reason three of the readings are stuck (§ *The measurements* in
[STATE.md](STATE.md)): the bytes agree with the retail ones right up to the
`mflr`, and the register is the first thing that differs.  The
compiler's identity, the flags and the linking model are all confirmed
there (the same source reproduces forty-four bytes in three games); what is
not reproduced is this register choice, and that is a property of the
compiler build, not of the source.

**What it means for the work:** the functions whose retail code keeps the
link register in `r4` or `r5` cannot be matched byte for byte with the
toolchain built here.  Either the compiler that built the game is not the
one the public sources describe, or the difference lies in a build option
that has not been found yet.  Both are worth another look before the
readings are closed as "not matchable".

### How much of the game that covers

The target's 7,214 functions whose prologue and epilogue can both be
identified break down like this:

| Prologue | Epilogue | Functions |
|---|---|---|
| `r0` | `r0` | 1,465 (20 %) |
| `r0` | another | 1,652 |
| another | `r0` | 2,188 |
| another | another | 1,909 |

So one function in five is written the way the rebuilt compiler writes it,
and those are the ones the body gate can be run on.  The other four in
five differ from the compiler built here in *which register* the link
register is kept in — in the prologue, in the epilogue, or in both.

That is a statement about the toolchain, not about the project: the source
form, the flags, the linking model and the compiler's identity are all
confirmed (the same source reproduces forty-four bytes in three games),
and what is missing is a compiler that reproduces this one choice.

## The frame

| Layout | Value |
|---|---|
| frame | a multiple of 0x10, reserved by the function itself |
| link register | saved in the **caller's** frame, at `0x10(r1)` of the caller's stack pointer — in the function's own frame that is `0x80(r1)` for a 0x70-byte frame |
| locals | between the stack pointer and the link register's slot |
| parameter save area | reserved when the function or one of its callees needs it |

The retail code saves the link register with `mflr` into a register and
then stores it; which register it uses is the register allocator's choice,
and it differs from function to function.

## Where the link register's save lands

The `mflr` is almost always the second instruction of the function, right
after the `stdu`: **21,801 times out of 21,824** ✓.  In nineteen functions
it comes later in the body, and in four it is not in the first
instructions at all.

The **store** is a different matter.  It is rarely the third instruction.
Measured over the 19,723 functions that have a frame and a save:

| Position of the store | Functions |
|---|---|
| 2 | 950 (4,8 %) |
| 3 | 4,501 (22,8 %) |
| 4 | 3,461 (17,5 %) |
| 5 | 3,589 (18,2 %) |
| 6 | 2,446 (12,4 %) |
| 7 | 1,637 (8,3 %) |
| 8 | 936 (4,7 %) |
| 9 | 783 (4,0 %) |
| 10 | 836 (4,2 %) |
| 11 | 584 (3,0 %) |
| further | 2,101 (10,7 %) |

**So 95 % of the retail's functions save the link register deeper in the
body** ✓✓✓.  The prologue holds the `stdu`, the `mflr` and the
callee-saved registers; the save of the link register joins the body:

```
00315188  stdu   r1, -0x80(r1)
0031518C  mflr   r5
00315190  std    r30, 0x70(r1)      ; a callee-saved register
00315194  lwz    r30, -0x700c(r2)   ; the load of a global
00315198  mr     r9, r3             ; the copy of an argument
0031519C  std    r5, 0x90(r1)       ; the save of the link register, here
```

The published reconstruction keeps the save in the prologue.  That is
what still stands between a candidate and the retail's functions that
read a global.  The cause was measured; it is the scheduler, below.

**Two explanations were measured and ruled out.**  The store is marked
*frame related*, which is what forbids the scheduler to move it — but
clearing that mark changes nothing: the objects produced with and without
it are identical, and the 2,505 byte-exact functions verify the same way
with either.  And the save is **not** placed just before the first call:
the distance between the store and the first call is spread from one
instruction to twenty, so this is not a save that waits for the link
register to be needed.

What remains is the scheduler.  Three mechanisms were measured on the
SDK 2.7.0 compiler at `-O2`, with the link register kept in `r5`.

**The hoist.**  A load of the TOC does not use the stack, so the second
scheduling pass moves it *before* the `stdu`.  The `mflr` follows it,
and ends up before the `stdu` as well.  The retail never does either:
the `stdu` is first and the `mflr` is second in 21,801 of 21,824
functions.  Grouping the `mflr` with the `stdu` and placing a barrier
after that pair stops the hoist.  With that barrier switched off, the
same four functions compile to the same bytes as the compiler that
produced the 2,505 matches.  The catalogue itself was not re-run.

**The alias set.**  The store of the link register is emitted in alias
set 0, which conflicts with every memory operation whose address is not
a known distinct base.  A load through a pointer is therefore a true
dependence of the store, and the store cannot sink past it.  A load
from `r2` is disambiguated and is not a dependence.  The epilogue
already reloads the slot with `rs6000_sr_alias_set`.  Giving the store
that same set removes the dependence: in the sched2 log, the pointer
load no longer lists the store.  The store then sinks past an `addi`
and past the save of a callee-saved register.

**The CELL bypass.**  It does not sink past the load of the TOC, and
the priority sort is not the reason.  In the same log the load's
priority is 16 and the store's is 7, so the sort would issue the load
first.  The barrier used for the measurement above is `gen_blockage`.
That instruction has the default type `integer`, and the CELL
reservation `cell-integer` applies to it.  The pipeline description in
`dd3.md` gives a true dependence from `cell-integer` to `cell-load` a
cost of 5 cycles, with no guard.  The barrier is a true dependence of
every later instruction, so the load is queued for 5 cycles.  The same
description's bypass to a store is guarded by `rs6000_use_address_p`,
and the barrier sets no register, so the guard does not fire.  The
store waits only the barrier's own latency, 2 cycles, and is issued
while the load is still queued.

A barrier that the recognizer does not classify as an integer
instruction has no such bypass.  The load and the store then become
ready together, and the sort issues the load first.  Measured on a
function that adds one to its argument and passes a global:

```
stdu 1,-112(1)
mflr 5
lwz  9,.LC0@toc(2)
addi 3,3,1
std  5,128(1)
```

A function that only reads the global does the same: the load of the
TOC, then the store, then the load through the pointer.  That second
load waits 5 cycles on the first by the same bypass (`cell-load` to
`cell-load`), so it is not ready when the store is issued.

The frame-related flag plays no part in any of the three.  The
scheduler never reads it.

The barrier is off by default and is not published.  The one that
reproduces the order above emits no instruction.  What is left is an
instruction pattern of its own, of a type other than `integer`, `load`
or `load_ext`, so the 5-cycle bypass cannot apply, and a re-run of the
2,505 with the option off.

## The epilogue's two shapes

The retail code ends its functions in two ways:

```
ld r0, 0x80(r1)     ; the link register
mtlr r0
addi r1, r1, 0x70   ; the stack
blr
```

```
ld r0, 0x80(r1)
addi r1, r1, 0x70
mtlr r0
blr
```

Both occur in the same executable, so the order is a property of the
function, not of the build flags.

**And the compiler that produced it agrees, function by function.**  The
reconstruction reproduces the retail's order in **66 functions out of 66**
that were compiled from known sources, and in all the sources the project
publishes.  Both orders appear in the same sample: the initialisation
function at `0x10200` ends with the `mtlr` first, like the retail's, and
the one-line wrappers end with the stack restore first, like the retail's.

What decides it is the **source's shape**, and the rule was measured:

* the backend emits the link register's restore *before* the other
  registers and dereferences the stack *last*, the way the ABI wants it;
* but the prologue and the epilogue are ordinary RTL in Sony's backend, so
  the instruction scheduler sees them and may permute them;
* `sched2` then places the `mtlr` two cycles after the reload, and the
  stack restore at the last load of the frame.

So the order follows the number of frame loads after the link register's
reload:

| Loads after the reload | `mtlr` first |
|---|---|
| none | 27 % |
| one | 74 % |
| two or more | 99,8 % |

and the source's shape is what feeds that number: a function whose
epilogue ends with the `mtlr` first restores at least one callee-saved
register, and one that ends with the stack restore first restores only
one.

Of the target's functions, 87 % end with the `mtlr` first (69 % in *Tools
of Destruction*), and the zone of the executable decides nothing — the
proportion varies inside each module.

`-fno-schedule-insns2` is the only option that changes the order — it
puts the `mtlr` first everywhere — which is the direct proof that the
scheduler is what decides.  It is not a repair: it breaks every function
that is byte-exact.

**There is no compiler difference here.**  What the project still had to
reproduce was the *source's* shape, and it now can.

## What this means for a wave

1. Recover the *grouping* of the functions, not only the functions: the
   translation unit changes the bytes.
2. Recover the *flags* of each group.  They are not necessarily the same
   everywhere: one file's functions keep the link register in `r0` and
   another's in `r4`, and one file was built with `-mminimal-toc` while
   another was not.
3. Recover the *shape* of each function: whether it restores a
   callee-saved register decides the order of its epilogue.
4. Compare the bytes, not the intent.  The register allocator's choices
   are part of what has to be reproduced, and they are the project's
   evidence.
