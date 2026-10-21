/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A module link function, twin of the one at 0x6FA4F8.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x6FA6D0
 * Extent   : 0x6FA6D0 .. 0x6FA717 (72 bytes, 18 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2
 * Result   : **byte-exact**.  The 72 bytes at 0x6FA6D0 are reproduced.
 *
 * The proof
 * ---------
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/gate/module_link_6fa6d0.elf --address 0x6FA6D0
 *     match: 72 bytes at 0x6FA6D0
 *
 * What the code does
 * ------------------
 * The same body as the function at 0x6FA4F8: the pointer's field at
 * +0x30 and a zero go to 0x702DC8, then the pointer goes to 0x714790.
 * Two functions with the same body, one link unit apart, is what a
 * generated pair of module links looks like.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name, for the same reason as its twin's.
 * * Why the engine has two of them: the retail code does not say.
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

void module_link_6FA6D0(void *p)
{
    sub_702DC8((u32)((u8 *)p + 0x30), 0);

    sub_714790((u32)p);
}
