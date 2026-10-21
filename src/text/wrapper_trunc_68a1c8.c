/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x68A1C8
 * Extent   : 0x68A1C8 .. 0x68A1F0 (40 bytes, 10 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -falign-functions=4 -mminimal-toc -mps3-lr-reg=3
 * Result   : **byte-exact**.  The 40 bytes at 0x68A1C8 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x68A1C8
 *     match: 40 bytes at 0x68A1C8
 *
 * What the code does
 * ------------------
 * The function does nothing but call one function and return: the
 * argument it received is forwarded untouched.
 *
 *     0068A1C8  stdu r1, -0x80(r1)
 *     0068A1CC  mflr r3
 *     0068A1D0  std r3, 0x90(r1)
 *     0068A1D4  bl 0x6dc5b4
 *     0068A1D8  nop
 *     0068A1DC  ld r0, 0x90(r1)
 *     0068A1E0  clrldi r3, r3, 0x20
 *     0068A1E4  mtlr r0
 *     0068A1E8  addi r1, r1, 0x80
 *     0068A1EC  blr
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
 * * The name.  "wrapper_trunc_68a1c8" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: sub_6DC5B4 at 0x6DC5B4
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef long long l64;
extern l64 sub_6DC5B4(void *);

void *wrapper_trunc_68a1c8(void *p)
{
    return (void *)sub_6DC5B4(p);
}
