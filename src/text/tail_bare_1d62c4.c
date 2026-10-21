/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x1D62C4
 * Extent   : 0x1D62C4 .. 0x1D62C8 (4 bytes, 1 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mno-toc
 * Result   : **byte-exact**.  The 4 bytes at 0x1D62C4 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x1D62C4
 *     match: 4 bytes at 0x1D62C4
 *
 * What the code does
 * ------------------
 * The function is a tail call: it adjusts its argument and jumps
 * to another function, which does the work.
 *
 *     001D62C4  b 0x1d61e8
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The compiler turns the call into a branch, so the function has
 *   no frame of its own.
 * * The delay slot of the branch is empty, and the branch is
 *   relative: the candidate is linked at the retail address, so
 *   the displacement is the retail one.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "tail_bare_1d62c4" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_1D61E8 at 0x1D61E8
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_1D61E8(void *);

void tail_bare_1d62c4(void *p)
{
    sub_1D61E8(p);
}
