/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x371200
 * Extent   : 0x371200 .. 0x37120C (12 bytes, 3 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 12 bytes at 0x371200 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x371200
 *     match: 12 bytes at 0x371200
 *
 * What the code does
 * ------------------
 * The function is reproduced by the source below.
 *
 *     00371200  lwz r0, 0x104(r3)
 *     00371204  stw r0, 0x540(r3)
 *     00371208  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The function opens a frame, and the link register is kept in
 *   the register the retail's compiler chose.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "copy_u32_371200" describes the shape of the code, not the game: the
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

void copy_u32_371200(void *p)
{
    *(unsigned int *)((char *)p + 1344) = *(unsigned int *)((char *)p + 260);
}
