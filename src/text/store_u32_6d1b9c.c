/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x6D1B9C
 * Extent   : 0x6D1B9C .. 0x6D1BA4 (8 bytes, 2 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 8 bytes at 0x6D1B9C are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x6D1B9C
 *     match: 8 bytes at 0x6D1B9C
 *
 * What the code does
 * ------------------
 * The function writes a value through its argument and returns
 * nothing.
 *
 *     006D1B9C  stw r4, 0x268(r3)
 *     006D1BA0  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The value is already in the register the ABI gives it, so the
 *   store needs no set-up at all.
 * * The function opens no frame.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "store_u32_6d1b9c" describes the shape of the code, not the game: the
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

void store_u32_6d1b9c(void *p, unsigned int v)
{
    *(unsigned int *)((char *)p + 616) = v;
}
