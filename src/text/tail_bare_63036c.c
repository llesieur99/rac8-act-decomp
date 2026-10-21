/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x63036C
 * Extent   : 0x63036C .. 0x630370 (4 bytes, 1 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mno-toc
 * Result   : **byte-exact**.  The 4 bytes at 0x63036C are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x63036C
 *     match: 4 bytes at 0x63036C
 *
 * What the code does
 * ------------------
 * The function is a tail call: it adjusts its argument and jumps
 * to another function, which does the work.
 *
 *     0063036C  b 0x62cfa4
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
 * * The name.  "tail_bare_63036c" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_62CFA4 at 0x62CFA4
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_62CFA4(void *);

void tail_bare_63036c(void *p)
{
    sub_62CFA4(p);
}
