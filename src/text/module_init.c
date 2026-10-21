/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * The module's initialisation function.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x10200
 * Extent   : 0x10200 .. 0x1022C (44 bytes, 11 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -mno-save-restore-tocbase
 * Result   : **byte-exact**.  The 44 bytes at 0x10200 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callees at their retail
 * addresses, and compared with the retail bytes:
 *
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/module_init.elf --address 0x10200
 *     match: 44 bytes at 0x10200
 *
 * What the code does
 * ------------------
 * It calls two functions in a row and returns nothing.  The first
 * (0x10638) reads a table of global data through the global pointer and
 * dispatches through a function pointer; the second (0x4E2518) is a
 * callee this project has not identified yet.
 *
 * The prologue and the epilogue are the SDK's: the frame is 0x70 bytes,
 * the link register is saved in the caller's frame at 0x10(r1), and the
 * stack pointer is restored with an immediate add.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name. "module_init" describes the place: this is the first
 *   function of the text segment, the section the loader runs at start-up.
 *   The game's own name for it is unknown.
 * * The two callees. Their names here are addresses; what they do is
 *   still to be read.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

void sub_10638(void);
void sub_4E2518(void);

void module_init(void)
{
    sub_10638();

    sub_4E2518();
}
