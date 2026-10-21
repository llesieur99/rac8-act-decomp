/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x38F41C
 * Extent   : 0x38F41C .. 0x38F428 (12 bytes, 3 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 12 bytes at 0x38F41C are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x38F41C
 *     match: 12 bytes at 0x38F41C
 *
 * What the code does
 * ------------------
 * The function returns its argument, moved along by a constant.
 *
 *     0038F41C  addi r3, r3, 0x1860
 *     0038F420  clrldi r3, r3, 0x20
 *     0038F424  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The arithmetic is folded into a single ``addi``.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "add_ptr_38f41c" describes the shape of the code, not the game: the
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

void *add_ptr_38f41c(void *p)
{
    return (void *)((char *)p + 6240);
}
