/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A method that calls a routine with two constants.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x10854
 * Extent   : 0x10854 .. 0x1087F (44 bytes, 11 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O1 -mminimal-toc -fschedule-insns2 -mps3-lr-reg=4
 * Result   : **byte-exact**.  The 44 bytes at 0x10854 are reproduced.
 *
 * The proof
 * ---------
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/call_with_49_and_1.elf --address 0x10854
 *     match: 44 bytes at 0x10854
 *
 * What the code does
 * ------------------
 * The function calls 0x48F410 with 0x31 and 1, and returns.  The game
 * calls it from 0x2607C.
 *
 * The four details that make the bytes match
 * -------------------------------------------
 * * **It is a C++ method.**  The callee receives its first argument in
 *   r4, not in r3: r3 holds the hidden ``this``.  A plain C function
 *   would pass its first argument in r3, and the bytes would differ.
 * * **The frame is 0x80 bytes**, which is what ``-mminimal-toc`` gives:
 *   the default TOC would make it 0x90.  The options are a property of
 *   the file, not of the project.
 * * **The link register is kept in r4**, the register the first argument
 *   uses; the reconstruction keeps it in r0 in every function.
 * * **The arguments are evaluated from the last to the first**, so the
 *   "li r5, 1" comes before the "li r4, 0x31".
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  It describes the code, not the game: the game has no name
 *   for it.
 * * The class the method belongs to.  The bytes show that the function
 *   is called with a hidden first argument; they do not say what it is.
 * * The parameter of the method: the game passes one argument in r3 at
 *   0x2607C, and the bytes do not say what it is.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef unsigned int u32;

extern "C" void sub_48F410(void *self, u32 a, u32 b);

struct Object
{
    void call_with_49_and_1(u32 x);
};

void Object::call_with_49_and_1(u32 x)
{
    sub_48F410(this, 0x31, 1);
}
