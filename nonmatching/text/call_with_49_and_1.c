/*
 * A Crack in Time (PS3, USA NPUA80966) -- first reconstruction attempt.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x10854
 * Extent   : 0x10854 .. 0x10880 (44 bytes, 11 instructions)
 * State    : not compiled -- the PPU compiler is not qualified yet
 *             (docs/ROADMAP.md, step 5). This file is a reading of the
 *             code, not a match, and it lives in nonmatching/ for that
 *             reason.
 *
 * Measured
 * --------
 * Compiled with the SDK's compiler (PPC GCC 4.1.1, SDK270) and compared
 * with the retail bytes, the candidate agrees on the frame's *shape* and
 * differs on its size: the retail frame is 0x80 bytes, this one is 0x70.
 * The retail code also moves the link register through r4 rather than r0,
 * and stores it after the first constant is loaded.
 *
 *     difference at 0x10857 (+3 bytes in)
 *
 * The extra 0x10 of frame is what the compiler reserves for arguments it
 * has to keep, and nothing tried so far reproduces it: a callee with more
 * arguments, a function with more parameters, and a local variable all
 * leave the frame at 0x70.  The next attempt has to find what the retail
 * function keeps on the stack.
 *
 * What the code does
 * ------------------
 * The function forwards to another one with two constant arguments: 49
 * and 1. The first argument is passed through untouched.
 *
 * The callee at 0x48F410 is large: it looks up an entry in a table
 * indexed by its second argument, then dispatches through a function
 * pointer taken from that entry. The constant 49 is therefore most
 * likely an index or a kind, and 1 a flag, but the table's layout is not
 * known and the constant's meaning is not established.
 *
 * What is a hypothesis here
 * -------------------------
 * * The types of the arguments. The first is 32-bit in the register the
 *   callee reads as a 32-bit value, but the callee also treats its
 *   second argument as a 64-bit quantity, so the widths of the source
 *   parameters are not settled.
 * * The names. "call_with_49_and_1" describes the call, not the game.
 *
 * Reopening condition
 * -------------------
 * The source can be promoted to src/ when a qualified PPU compiler
 * reproduces the 44 bytes at 0x10854, and when the table the callee
 * indexes gives the constant 49 a name.
 */

int sub_48F410(int first, int kind, int flag);

void call_with_49_and_1(int first)
{
    sub_48F410(first, 49, 1);
}
