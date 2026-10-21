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

## What the patches do not fix

The order of the epilogue's `mtlr` and the stack restore is not fixed by
the source: the reconstruction restores the stack first, and the retail
code keeps the `mtlr` first in 87 % of its functions.  Two experiments on
the frame's size did not change it, so the cause is still open.
