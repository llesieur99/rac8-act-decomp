/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0xA17198
 * Extent   : 0xA17198 .. 0xA171A0 (8 bytes, 2 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mno-toc
 * Result   : **byte-exact**.  The 8 bytes at 0xA17198 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0xA17198
 *     match: 8 bytes at 0xA17198
 *
 * What the code does
 * ------------------
 * The function is a tail call: it adjusts its argument and jumps
 * to another function, which does the work.
 *
 *     00A17198  addi r3, r3, -0x10
 *     00A1719C  b 0xa171a0
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
 * * The name.  "tail_add64_a17198" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_A171A0 at 0xA171A0
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef long long l64;
extern void sub_A171A0(l64);

void tail_add64_a17198(l64 p)
{
    sub_A171A0(p - 16);
}
