/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A module link function.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x6FA4F8
 * Extent   : 0x6FA4F8 .. 0x6FA53F (72 bytes, 18 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2
 * Result   : **byte-exact**.  The 72 bytes at 0x6FA4F8 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callees at their retail
 * addresses, and compared with the retail bytes:
 *
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/module_link_6fa4f8.elf --address 0x6FA4F8
 *     match: 72 bytes at 0x6FA4F8
 *
 * The function at 0x6FA6D0 has the same body and is matched the same way.
 *
 * What the code does
 * ------------------
 * The function receives a pointer, hands (pointer + 0x30) truncated to
 * 32 bits and zero to 0x702DC8, then hands the pointer truncated to 32
 * bits to 0x714790.  Both callees therefore take 32-bit arguments: the
 * "clrldi" that follows each computation is the 64 -> 32 bit narrowing.
 * The pointer is kept in r29 across the first call.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name. "module_link" describes the place, not the game: the game
 *   calls this function from five sites between 0x703E38 and 0x709B78.
 *   The game's own name for it is unknown.
 * * The callees. Their names here are addresses; 0x702DC8 reads a table
 *   through the global pointer and 0x714790 starts on the string "fi".
 * * The parameter type.  The retail truncates the pointer to 32 bits, so
 *   the original source very likely used a 32-bit handle rather than a
 *   real pointer; the reconstruction keeps a 64-bit parameter and makes
 *   the truncation explicit, which is what the compiler must emit.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef unsigned int u32;
typedef unsigned char u8;

extern void sub_702DC8(u32 a, u32 b);
extern void sub_714790(u32 a);

void module_link_6FA4F8(void *p)
{
    sub_702DC8((u32)((u8 *)p + 0x30), 0);

    sub_714790((u32)p);
}
