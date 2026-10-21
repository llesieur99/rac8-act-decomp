/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x6B9D2C
 * Extent   : 0x6B9D2C .. 0x6B9D38 (12 bytes, 3 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 12 bytes at 0x6B9D2C are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x6B9D2C
 *     match: 12 bytes at 0x6B9D2C
 *
 * What the code does
 * ------------------
 * The function returns its argument, moved along by a constant.
 *
 *     006B9D2C  addi r3, r3, 0xa0
 *     006B9D30  clrldi r3, r3, 0x20
 *     006B9D34  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The arithmetic is folded into a single ``addi``.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "add_ptr_6b9d2c" describes the shape of the code, not the game: the
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

void *add_ptr_6b9d2c(void *p)
{
    return (void *)((char *)p + 160);
}
