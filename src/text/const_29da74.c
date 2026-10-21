/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x29DA74
 * Extent   : 0x29DA74 .. 0x29DA7C (8 bytes, 2 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 8 bytes at 0x29DA74 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x29DA74
 *     match: 8 bytes at 0x29DA74
 *
 * What the code does
 * ------------------
 * The function returns the constant 0 and does nothing
 * else: the compiler puts the value in ``r3`` and returns.
 *
 *     0029DA74  li r3, 0
 *     0029DA78  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The value is small enough for the ``li`` instruction, which
 *   puts it in ``r3`` in one go.
 * * The function opens no frame, so the option that moves the
 *   link register has nothing to do here.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "const_29da74" describes the shape of the code, not the game: the
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

unsigned int const_29da74(void)
{
    return 0;
}
