/*
 * A Crack in Time (PS3, USA NPUA80966) -- first reconstruction attempt.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x21B5C8
 * Extent   : 0x21B5C8 .. 0x21B5EC (32 bytes, 8 instructions)
 * State    : not compiled -- the PPU compiler is not qualified yet
 *             (docs/ROADMAP.md, step 5). This file is a reading of the
 *             code, not a match, and it lives in nonmatching/ for that
 *             reason.
 *
 * Measured
 * --------
 * Compiled with the SDK's compiler (PPC GCC 4.1.1, SDK270), the boolean
 * test comes out as the *same sequence of instructions* as the retail
 * code — the absolute-value-and-shift idiom, then the sign extension —
 * but the registers differ: the retail code takes its two operands from
 * r8 and r10, this source from r3 and r4.
 *
 *     difference at 0x21B5C8 (+0 bytes in)
 *
 * The reason is now known, and it is not the source: the retail code at
 * 0x21B5C8 is **not a function**.  It has no prologue of its own, no
 * caller names it, and it ends the function it belongs to; the two
 * operands are the enclosing function's registers.  The address is in the
 * catalogue as a block, not as a function, and this file is kept only as
 * the record of what the block does.
 *
 * What the code does
 * ------------------
 * The function reads a 32-bit word at a computed address and returns a
 * boolean: 1 when the word is not zero, 0 when it is. The test is
 * branchless; the arithmetic is the classic absolute-value-and-shift
 * idiom, and the final shift is a logical one, which is what turns
 * "non-zero" into 1.
 *
 * The address is the sum of two of the function's arguments, plus a
 * constant of 8.
 *
 * What is a hypothesis here
 * -------------------------
 * * The argument registers. The retail build has no symbol table, so the
 *   mapping from the registers that carry the operands to the source's
 *   parameters is inferred from the PPU calling convention: the operands
 *   live in the argument registers r3..r10, and the two used here are r8
 *   and r10. Which source-level parameters they are, and their types, is
 *   not established.
 * * The width of the field. The load is a 32-bit zero-extending load, so
 *   the field is 32 bits wide; the surrounding structure's layout is not
 *   known.
 * * The name. "field_is_set" describes the behaviour, not the game: the
 *   function's real name and owner are unknown.
 *
 * Reopening condition
 * -------------------
 * The source can be promoted to src/ when a qualified PPU compiler
 * reproduces the 32 bytes at 0x21B5C8 with the parameter mapping
 * confirmed by a caller, and when the surrounding structure's layout
 * gives the field a name.
 */

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

int field_is_set(u64 base, u64 offset)
{
    u32 value = *(const u32 *)(base + offset + 8);

    /* Branchless "is not zero": the absolute value of the word, negated,
       then shifted right logically, which is 1 for any non-zero word. */
    u32 sign = (u32)((int)value >> 31);
    u32 magnitude = sign ^ value;
    u32 negated = sign - magnitude;

    return (int)(negated >> 31);
}
