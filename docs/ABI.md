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
