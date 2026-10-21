/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x56D6A0
 * Extent   : 0x56D6A0 .. 0x56D6A8 (8 bytes, 2 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 8 bytes at 0x56D6A0 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x56D6A0
 *     match: 8 bytes at 0x56D6A0
 *
 * What the code does
 * ------------------
 * The function reads a value through its argument and returns
 * it.  The load is a 32 bit one: the pointers of this ABI are
 * 32 bits (the ILP32 model).
 *
 *     0056D6A0  lbz r3, 0x2f0(r3)
 *     0056D6A4  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The load is a 32 bit one, and the offset is folded into the
 *   instruction: the compiler knows the pointer is 32 bits wide.
 * * The function opens no frame.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "load_u8_56d6a0" describes the shape of the code, not the game: the
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

unsigned char load_u8_56d6a0(void *p)
{
    return *(unsigned char *)((char *)p + 752);
}
