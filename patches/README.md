# The link register patches

The SDK 2.7.0 compiler, rebuilt from its own sources, keeps the link
register in `r0` in every function.  The compiler that built the retail
executable does not: it lets the register allocator choose, and the
result is spread over `r0`, `r4`, `r5` and `r6`.

That difference is what stopped three of the readings from matching, and
the two patches here deal with it: one **reproduces** the choice, the
other **explains** it.

## `ps3-lr-reg.patch` — the patch that reproduces the choice

In the backend, the register that holds the link register is written in
four places, and all four are hard-coded to `r0`:

| Place | What it does |
|---|---|
| the prologue's `mflr` | takes the link register into a general register |
| the prologue's `std` | stores that register in the frame |
| the epilogue's `ld` | reads the saved link register back |
| the epilogue's `mtlr` | puts it back in the link register |

The patch replaces those four hard-coded registers with the value of two
new target options:

| Option | Effect |
|---|---|
| `-mps3-lr-reg=N` | the prologue keeps the link register in `rN` |
| `-mps3-lr-restore-reg=N` | the epilogue reloads it into `rN` |

The default is the current behaviour, so the patch is backwards
compatible: a build without the options produces the same code as before.

## How to apply it

The patch applies to the GCC 4.1.1 sources of the SDK's toolchain, in the
`toolchain-src-2.7.0-GCC411` branch of the public toolchain repository:

```sh
git apply --directory=gcc patches/ps3-lr-reg.patch
```

and the compiler is then rebuilt as usual.  The same code is present in the
2.8.0, 3.6.0 and 4.2.0 branches, so the patch applies to them as well.

**The four patches of the 2.7.0 branch apply in this order** — the second,
the third and the fourth are written against the sources the previous ones
leave:

```sh
git apply --directory=gcc patches/ps3-lr-reg.patch
git apply --directory=gcc patches/ps3-toc-neg-disp.patch
git apply --directory=gcc patches/ps3-neg-disp-completion.patch
git apply --directory=gcc patches/ps3-lr-sink.patch
```

Each of them adds its own options, and each of the options is off by
default except the sinking, which is the retail's behaviour.

## What it buys

14,520 of the target's 23,333 functions with a frame were blocked on the
choice of the register.  The option reaches **11,541** of them with `r4`,
`r5` and `r6` alone, and 12,310 with `r7`.  Two readings that were blocked
now match byte for byte:

| Function | Options |
|---|---|
| `0x25D98` | `-O1 -fschedule-insns2 -mps3-lr-reg=3` |
| `0x10854` | `-O1 -mminimal-toc -fschedule-insns2 -mps3-lr-reg=4` |

## `ps3-lr-alloc.patch` — the patch that explains it

The second patch does the thing properly: it moves the save of the link
register into the function's RTL, **before** the allocation, with two
pseudo-registers, so that the allocator chooses the hard registers — the
way the retail's compiler does.  The prologue and the epilogue, which
Sony's backend emits *after* the allocation, then use the register the
allocator chose.

It works mechanically, and it reproduces the retail's bytes whenever the
allocator agrees: given the register the retail chose, the two compilers
emit **the same bytes**.

It does not, however, reproduce the retail's *choice*: on 2,674 measured
functions, the faithful mechanism lands on the retail's register in 23
cases (0.9 %), where the option reaches 100 %.  The reason is precise and
worth stating: in this compiler the allocator picks `r0` whenever `r0` is
free at the entry, while the retail's build picked `r4`, `r5`, `r6`, `r3`
or `r7` **for the same source**.

**That choice is a property of the compiler build that produced those
modules, not of the function.**  No patch of the SDK 2.7.0 sources can
recover it, because the difference lies in the build's register
preferences, which the public sources do not carry.  The option is
therefore what the project uses; the faithful patch is the explanation,
and the groundwork for the day the shipped compiler is available.

## The licence, and why the patches touch only GPL files

The files the two patches touch are **upstream GCC**, and each of them
carries the Free Software Foundation's notice:

> This file is part of GCC.  GCC is free software; you can redistribute it
> and/or modify it under the terms of the GNU General Public License as
> published by the Free Software Foundation; either version 2, or (at your
> option) any later version.

A modification of a GPL file is itself under the GPL, so the patches are
published here under the same terms, with the notices kept.

**No other file of the SDK is touched.**  The two options could have been
declared in `lv2.opt`, which is a file Sony added to GCC; they are
declared in the upstream `rs6000.opt` instead, so that every line of the
patches comes from code that is under the GNU General Public License.
The patches quote no header, no library and no sample of the SDK.

## `ps3-lr-reg-1.9.0.diff` — the same patch, for the 1.9.0 branch

The PlayStation 4's compiler has the same four hard-coded registers in
the same places, but its blocks around them carry `lto_info` calls, the
condition of the epilogue's reload is written differently, and its option
file ends differently — so the patch needs its context adjusted.  The
variant is published next to this one, and applies cleanly.

## `ps3-toc-neg-disp.patch` — the TOC and the negative displacements

Two more rules of the retail's code, both of them in the SDK's compiler:

* **the TOC entry is read in one instruction.**  The SDK's backend copies
  the base into `r30` in minimal-TOC mode, so reading an entry takes two
  instructions; the retail addresses the entries from `r2` and reads the
  value in one.
* **a negative displacement is folded into the load.**  The D-form's
  displacement is tested as *unsigned* (`offset += 0x8000; offset < 0x10000`),
  so `-4096` is rejected and the address is computed in 64 bits and then
  truncated; a positive offset is folded, a negative one never is.

The patch adds two options, both **off by default**: `-mps3-toc-direct` and
`-mps3-neg-disp`.  It touches four files of upstream GCC — `rs6000.c`,
`rs6000.h`, `rs6000.opt` and `tree-ssa-address.c` — all under the GPL, and
nothing that Sony added.  It applies **on top of** the first patch: both
declare options in the same option file, so the two are applied in that
order.

### What it buys, and what it does not

The first half works: with `-mps3-toc-direct` the entry is read in one
instruction from `r2` and `r30` is freed.

The second half is diagnosed but not finished.  The rules are found — the
D-form's displacement is tested as unsigned, and the sign is also lost
when the tree layer decomposes a `MEM_REF` address into base and offset,
because only the low word of a 64-bit constant is kept.  The decomposition
rule itself is sign-sensitive and is not patched yet, and there is a third
rule: the PPU's `lwz` zero-extends the register, so the retail needs no
`clrldi` where the SDK's `POINTERS_EXTEND_UNSIGNED = -1` model demands one.

**The gain is zero, and that is the most useful thing the exercise
produced.**  Of 18,660 functions that reach the TOC and are not yet
reproduced, all of a sample of two hundred need *both* fixes — but the
campaign that searched for them never wrote a source form that *reads a
global*: its twenty-nine forms read a member **through the argument**, so
the TOC path was never exercised at all.  Those functions were not one
instruction away; a source form was missing.  That is invisible without
writing the patch.

With the patch, all 2,505 byte-exact functions still verify byte for byte.

## `ps3-lr-sink.patch` — letting the scheduler sink the save

The retail's compiler lets the instruction scheduler sink the **save of
the link register into the body** of the function.  The store is not
necessarily the third instruction: it comes after the load of a global,
after the copy of an argument, just before the call it belongs to.

GCC 4.1 marks that store as *frame related*, and a frame-related
instruction may not be moved — so the store stays in the prologue and the
candidate stops matching.

The patch clears the flag on that one store.  It adds the option
`-mps3-lr-sink`, **on by default**, because that is what the retail does;
`-mno-ps3-lr-sink` keeps GCC 4.1's behaviour.  It touches two files of
upstream GCC — `rs6000.c` and `rs6000.opt` — both under the GPL.

**Every one of the 2,505 byte-exact functions still reproduces the retail
bytes with it** ✓✓✓.  The sinking changes nothing that was already
right, and it removes one of the two things that stood between a
candidate and the retail's functions that read a global.

## The validation on a second game and a second compiler

The patch was checked on *Tools of Destruction*, the other Insomniac game,
whose code was compiled by the PlayStation 4's compiler — the one with the
`CELL 4.1.2.7` stamp.  Both the negative and the positive control were
run on 208 functions whose source can be derived mechanically:

| | Functions byte-exact |
|---|---|
| the 1.9.0 compiler, unpatched | **32 of 208** (15 %) — exactly the functions whose link register is in `r0`, and none of the others |
| the 1.9.0 compiler, patched | **188 of 208** (90 %) |

and on a random sample of 500 functions of that game:

| | Prologues byte-identical |
|---|---|
| unpatched | 126 of 500 (25 %) |
| patched | **492 of 500 (98 %)** |

The functions that the patch frees are exactly those whose link register
is in `r4` or `r5`: they go from 5 matching bytes out of 36 to 36 out of
36.

On the twenty pairs of functions the two games share, the patched compiler
reproduces **twenty out of twenty in each game**, where the unpatched one
reproduces none in *A Crack in Time* and three in *Tools of Destruction*.

Two facts came out of the exercise:

* the retail aligns its functions on **four** bytes and not eight, so
  `-falign-functions=4` belongs to any byte-for-byte comparison;
* the 1.9.0 branch does not have the SDK 2.7.0's TOC options, so it
  cannot produce the restoration of the TOC base — a difference between
  the two SDKs' ABIs, not a difference of register.

## What the patches do not fix

The order of the epilogue's `mtlr` and the stack restore is **not** a
compiler difference: the reconstruction reproduces the retail's order in
the sixty-six functions whose source is known, and the order follows the
shape of the source — a function whose epilogue ends with the `mtlr`
first restores at least one callee-saved register.  See
[ABI.md](../docs/ABI.md).

What the patches do not fix is the **source's shape**: the retail's
functions read a global through the TOC in one instruction, the SDK's
compiler knows only the address model, and the two forms have to be
recovered from the bytes.
