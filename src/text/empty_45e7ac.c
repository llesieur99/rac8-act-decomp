/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function of the retail executable, reproduced byte for byte.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x45E7AC
 * Extent   : 0x45E7AC .. 0x45E7B0 (4 bytes, 1 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -falign-functions=4
 * Result   : **byte-exact**.  The 4 bytes at 0x45E7AC are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/verify.py --src src --address 0x45E7AC
 *     match: 4 bytes at 0x45E7AC
 *
 * What the code does
 * ------------------
 * The function does nothing at all: its body is empty, and the
 * compiler emits a single ``blr``.
 *
 *     0045E7AC  blr
 *
 * The details that make the bytes match
 * -------------------------------------
 * * The body is empty, so the compiler emits the single ``blr``
 *   that ends the function.  No frame is opened, so the link
 *   register is never saved.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "empty_45e7ac" describes the shape of the code, not the game: the
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

void empty_45e7ac(void)
{
}
