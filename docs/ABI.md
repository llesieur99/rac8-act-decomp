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

The retail code keeps the `nop` after most calls **and** restores the TOC
base in some functions, so the game was built with neither option *by
default*; `-mno-save-restore-tocbase` is what reproduces the prologue of
the first matched function (see [COMPILER.md](COMPILER.md)).

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

## The TOC base

`r2` holds the TOC (the table of contents) in the Cell OS Lv-2 ABI.  A
function that uses globals gets its TOC pointer from the chain in the
caller's frame (`ld r2, 0x28(r1)`), and a function that calls another one
restores it afterwards, because the callee may have changed it.

That is why some retail functions end with a `ld r2, 0x28(r1)` and others
do not: it depends on whether the calls they make can change `r2`.

The option `-mno-save-restore-tocbase` removes that restore as well as the
prologue's save.  The retail executable has functions with the restore and
functions without, so **the flags are not the same everywhere**: the file
that holds the function at `0x10200` was built with the option, and the
file that holds the function at `0x10378` was not.  Qualifying the build
means qualifying it *file by file*.

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
the SDK 2.8.0 branch of the same repository.

And the epilogue says why.  In the compiler's own source, the link
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
and 4.2.0) has that same line.  **The compiler that built the target is
therefore not the one the public sources describe** — the difference is in
the parts of the SDK's compiler that were never published.

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
function, not of the build flags.  A candidate that always produces one of
them will match half the functions and no more.

## What this means for a wave

1. Recover the *grouping* of the functions, not only the functions: the
   translation unit changes the bytes.
2. Recover the *flags* of each group.  They are not necessarily the same
   everywhere: the prologue of one function may say the TOC base is not
   saved while another's says it is.
3. Compare the bytes, not the intent.  The register allocator's choices are
   part of what has to be reproduced, and they are the project's evidence.
