/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x62BAA8
 * Extent   : 0x62BAA8 .. 0x62BAAC (4 bytes, 1 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mno-toc
 * Result   : **byte-exact**.  The 4 bytes at 0x62BAA8 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x62BAA8
 *     match: 4 bytes at 0x62BAA8
 *
 * What the code does
 * ------------------
 * The function is a tail call: it adjusts its argument and jumps
 * to another function, which does the work.
 *
 *     0062BAA8  b 0x62b9cc
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
 * * The name.  "tail_bare_62baa8" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_62B9CC at 0x62B9CC
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_62B9CC(void *);

void tail_bare_62baa8(void *p)
{
    sub_62B9CC(p);
}
