/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A floating point conversion.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x6E1C78
 * Extent   : 0x6E1C78 .. 0x6E1CC7 (80 bytes, 20 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2
 * Result   : **byte-exact**.  The 80 bytes at 0x6E1C78 are reproduced.
 *
 * The proof
 * ---------
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/float_to_short_6e1c78.elf --address 0x6E1C78
 *     match: 80 bytes at 0x6E1C78
 *
 * What the code does
 * ------------------
 * The function takes a double and a pointer to an int.  It calls
 * 0x903748 with the address of a local short and the address of the
 * double, stores the (sign extended) short into the caller's int, and
 * returns the double in f1.
 *
 * The two details that make the bytes match
 * ------------------------------------------
 * * The double is passed by address, so the compiler has to keep a copy
 *   of it in memory (0xd0(r1), the caller's frame) and reload it into
 *   f1 after the call: the function returns the double it was given.
 *   Without the "return x", the reload is dead and the compiler drops
 *   it, and the function is 4 bytes short.
 * * The short is reloaded with "lhz" then sign extended with "extsh",
 *   and stored with "stw" into an int.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name. "float_to_short" describes the shape of the code, not the
 *   game: the game has no name for it.
 * * The signature of 0x903748.  It is called with (short *, double *).
 *   Its own return value is discarded.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef short s16;

extern int sub_903748(s16 *out, double *d);

double float_to_short_6E1C78(double x, int *out)
{
    s16 v;

    sub_903748(&v, &x);

    *out = v;

    return x;
}
