/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x169E18
 * Extent   : 0x169E18 .. 0x169E40 (40 bytes, 10 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mminimal-toc -mps3-lr-reg=5
 * Result   : **byte-exact**.  The 40 bytes at 0x169E18 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x169E18
 *     match: 40 bytes at 0x169E18
 *
 * What the code does
 * ------------------
 * The function does nothing but call one function and return: the
 * argument it received is forwarded untouched.
 *
 *     00169E18  stdu r1, -0x80(r1)
 *     00169E1C  mflr r5
 *     00169E20  std r5, 0x90(r1)
 *     00169E24  lwz r3, 0x68(r3)
 *     00169E28  bl 0x4d1c94
 *     00169E2C  nop
 *     00169E30  ld r0, 0x90(r1)
 *     00169E34  addi r1, r1, 0x80
 *     00169E38  mtlr r0
 *     00169E3C  blr
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
 * * The name.  "wrapper_field_169e18" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_4D1C94 at 0x4D1C94
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_4D1C94(void *);

void wrapper_field_169e18(void *p)
{
    sub_4D1C94(*(void **)((char *)p + 104));
}
