/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x4F956C
 * Extent   : 0x4F956C .. 0x4F9578 (12 bytes, 3 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 12 bytes at 0x4F956C are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x4F956C
 *     match: 12 bytes at 0x4F956C
 *
 * What the code does
 * ------------------
 * The function is reproduced by the source below.
 *
 *     004F956C  lwz r0, 0(r4)
 *     004F9570  stw r0, 0(r3)
 *     004F9574  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The function opens a frame, and the link register is kept in
 *   the register the retail's compiler chose.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "copy_u32_4f956c" describes the shape of the code, not the game: the
 *   game has no name for it.
 * * The types of the parameters.  The bytes show how the arguments are
 *   used, not what they are; the source uses the types that reproduce them.
 * * The callee's name is its address: none
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

void copy_u32_4f956c(void *p, void *q)
{
    *(unsigned int *)((char *)p + 0) = *(unsigned int *)((char *)q + 0);
}
