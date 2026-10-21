/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function that builds a local buffer and hands it to a callee.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x25D98
 * Extent   : 0x25D98 .. 0x25DC3 (44 bytes, 11 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -fschedule-insns2 -mps3-lr-reg=3
 * Result   : **byte-exact**.  The 44 bytes at 0x25D98 are reproduced.
 *
 * The proof
 * ---------
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/construct_local_buffer.elf --address 0x25D98
 *     match: 44 bytes at 0x25D98
 *
 * What the code does
 * ------------------
 * The function takes three arguments and uses none of them.  It builds a
 * 32-byte buffer in its own frame and calls 0x4F9690 with it and with
 * zero.  The game calls it from five sites between 0x27414C and others.
 *
 * The three details that make the bytes match
 * --------------------------------------------
 * * **The link register is kept in r3.**  The reconstruction keeps it in
 *   r0 in every function; the retail's compiler lets the register
 *   allocator choose, and here it chose r3.  The option that reproduces
 *   the choice is described in [COMPILER.md](../COMPILER.md).
 * * **The frame is 0x90 bytes and the link register goes to the ABI
 *   slot** of the caller's frame, at 0xa0(r1) — the default TOC does
 *   that; ``-mminimal-toc`` would make the frame 0x80 bytes.
 * * **The second scheduler is on.**  The retail's code puts the "li r4, 0"
 *   between the "mflr" and the "std", which is the scheduler's choice.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "construct_local_buffer" describes the code, not the game:
 *   the game has no name for it.
 * * The three parameters.  The bytes show that they are unused; their
 *   types are not shown by the bytes.
 * * The callee 0x4F9690: its name is its address.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef unsigned int u32;

extern void sub_4F9690(u32 *address, u32 value);

void construct_local_buffer(u32 *a, u32 *b, u32 *c)
{
    u32 local[8];

    sub_4F9690(local, 0);
}
