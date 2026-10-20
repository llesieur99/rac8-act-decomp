/*
 * A Crack in Time (PS3, USA NPUA80966) -- first reconstruction attempt.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x3B704
 * Extent   : 0x3B704 .. 0x3B738 (52 bytes, 13 instructions)
 * State    : not compiled -- the PPU compiler is not qualified yet
 *             (docs/ROADMAP.md, step 5). This file is a reading of the
 *             code, not a match, and it lives in nonmatching/ for that
 *             reason.
 *
 * What the code does
 * ------------------
 * The function reads a word out of a global table, reads another word
 * out of that table, calls a function with it, and returns the call's
 * result sign-extended to 32 bits.
 *
 * The two reads are the way the PPU reaches global data: the global
 * pointer arrives in r2, the first read is at a fixed offset from it, and
 * the second is at a fixed offset from the word that was read.  The
 * offsets are large and negative, which is what a table of thousands of
 * entries looks like.
 *
 * What is a hypothesis here
 * -------------------------
 * * The types. The first argument is not used by the function itself: the
 *   call is made with a value read from the table, and the argument
 *   register of the caller is simply left alone. Whether the source
 *   takes an argument at all is not established.
 * * The globals. Which table the first read names, and what the second
 *   read selects, is unknown.
 * * The names. "read_global_and_call" describes the shape, not the game.
 *
 * Reopening condition
 * -------------------
 * The source can be promoted to src/ when a qualified PPU compiler
 * reproduces the 52 bytes at 0x3B704, and when the table's layout gives
 * the two globals a name.
 */

int sub_3BFC4(int value);

/* The PPU's global pointer, in the register the loader passes it in. */
extern char global_pointer[];

int read_global_and_call(void)
{
    unsigned int table = *(unsigned int *)(global_pointer - 0x7E30);
    int value = *(int *)(table - 0x7FD8);

    return sub_3BFC4(value);
}
