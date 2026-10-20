/*
 * A Crack in Time (PS3, USA NPUA80966) -- first reconstruction attempt.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x10200
 * Extent   : 0x10200 .. 0x1022C (44 bytes, 11 instructions)
 * State    : not compiled -- the PPU compiler is not qualified yet
 *             (docs/ROADMAP.md, step 5). This file is a reading of the
 *             code, not a match, and it lives in nonmatching/ for that
 *             reason.
 *
 * What the code does
 * ------------------
 * This is the first function of the text segment: the section that holds
 * it is the first section of the executable, which is where the PPU
 * loader's module initialisation lives.  The function sets up a frame,
 * calls two other functions in a row, and returns.
 *
 * The first call goes to 0x10638, which reads a table of global data
 * through the global pointer and dispatches through a function pointer.
 * The second goes to 0x4E2518.
 *
 * What is a hypothesis here
 * -------------------------
 * * The return value. The second call's result is left in r3, so the
 *   function returns it, but nothing shows that the loader uses it.
 * * The argument registers. The calls leave r3 alone, so both callees
 *   inherit the caller's arguments. Whether the source passes arguments
 *   on is not established.
 * * The names. "module_init" describes the place, not the game: the
 *   function's real name and owner are unknown.
 *
 * Reopening condition
 * -------------------
 * The source can be promoted to src/ when a qualified PPU compiler
 * reproduces the 44 bytes at 0x10200, and when the two callees are
 * identified well enough to name this function after what it does.
 */

void sub_10638(void);
int sub_4E2518(void);

int module_init(void)
{
    sub_10638();

    return sub_4E2518();
}
