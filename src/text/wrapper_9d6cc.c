/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x9D6CC
 * Extent   : 0x9D6CC .. 0x9D6F0 (36 bytes, 9 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mminimal-toc -mps3-lr-reg=4
 * Result   : **byte-exact**.  The 36 bytes at 0x9D6CC are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x9D6CC
 *     match: 36 bytes at 0x9D6CC
 *
 * What the code does
 * ------------------
 * The function does nothing but call one function and return: the
 * argument it received is forwarded untouched.
 *
 *     0009D6CC  stdu r1, -0x80(r1)
 *     0009D6D0  mflr r4
 *     0009D6D4  std r4, 0x90(r1)
 *     0009D6D8  bl 0x99654
 *     0009D6DC  nop
 *     0009D6E0  ld r0, 0x90(r1)
 *     0009D6E4  addi r1, r1, 0x80
 *     0009D6E8  mtlr r0
 *     0009D6EC  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * **The frame and the link register.**  The frame is opened by
 *   the prologue and the link register is kept in the register the
 *   retail's compiler chose; the option ``-mps3-lr-reg`` names
 *   it.  The link register goes to the ABI slot (the caller's
 *   frame, at SP + 0x10), not inside the callee's frame.
 * * **The arguments are forwarded untouched.**  They are already in
 *   the registers, so the call needs no register set-up, and the
 *   delay slot holds the ``nop`` the SDK emits after a call.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "wrapper_9d6cc" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_99654 at 0x99654
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_99654(void *);

void wrapper_9d6cc(void *p)
{
    sub_99654(p);
}
