/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A function that reads a global of a linked module and calls a routine
 * with its address.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x3B704
 * Extent   : 0x3B704 .. 0x3B73B (56 bytes, 14 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2 -mminimal-toc -mps3-lr-reg=3 -falign-functions=2
 * Result   : **byte-exact**.  The 56 bytes at 0x3B704 are reproduced.
 *
 * The proof
 * ---------
 * The candidate needs the module's TOC layout, so it is compiled and
 * linked with the harness that reproduces it (tools/ppu_toc.py):
 *
 *     python tools/ppu_toc.py --globals-catalogue <globals.tsv> \
 *         --toolchain <toolchain> build --source src/text/read_global_and_call.c \
 *         --address 0x3B704 --globals g_m7e30 --callee sub_3BFC4=0x3BFC4 \
 *         --shape value --flags "-O2 -mminimal-toc -mps3-lr-reg=3 -falign-functions=2" \
 *         --out candidate.elf
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object candidate.elf --address 0x3B704
 *     match: 56 bytes at 0x3B704
 *
 * What the code does
 * ------------------
 * The function reads a global of a *linked* module, takes the address of
 * a member of it, and hands that address to 0x3BFC4, which reads a
 * 32-bit member at +0x28 of the object it is given and returns it.
 *
 * The five details that make the bytes match
 * ------------------------------------------
 * * **The global is reached through the secondary TOC.**  The entry of the
 *   table holds the *anchor* of the linked module, and the global sits at
 *   a fixed offset from that anchor (-0x7fd8, the eleventh slot of the
 *   secondary TOC).  Ten other globals of the same translation unit take
 *   the ten first slots, which is why the source below names them: the
 *   displacement the assembler computes depends on their order.
 * * **-mminimal-toc** is what gives that layout: the table holds one
 *   entry per procedure instead of one entry per global.
 * * **The link register is kept in r3**, and the frame is 0x80 bytes
 *   because the function has to keep a non-volatile register.
 * * **-falign-functions=2**: the function is not aligned on eight bytes.
 * * **The return value is a 32-bit int**, sign extended with "extsw"; the
 *   C, C++ and method forms all give the same bytes.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "read_global_and_call" describes the code, not the game:
 *   the game has no name for it.
 * * The ten globals that take the first slots are the real ones of the
 *   game, and the original translation unit uses them in other functions;
 *   the function below only names them so that the layout comes out
 *   right.
 * * The callee 0x3BFC4: its name is its address.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

typedef unsigned int u32;
typedef int s32;

struct Object {
    u32 pad[10];
    s32 value;
};

/* the global the retail reads: the offset -32304 of the catalogue */
extern struct Object g_m7e30;

/* the ten globals that take the first slots of the secondary TOC */
extern u32 g_m7ffc, g_m7fe6, g_m7fd0, g_m7fcc, g_m7fc8;
extern u32 g_m7fc4, g_m7fc0, g_m7fbc, g_m7fb8, g_m7fb4;

/* the callee, at its retail address */
extern s32 sub_3BFC4(struct Object *object);

/* The other globals of the unit: they take the ten first slots of the
 * secondary TOC, and the displacement of the second load depends on
 * them. */
s32 other_globals(void)
{
    return (s32)(g_m7ffc + g_m7fe6 + g_m7fd0 + g_m7fcc + g_m7fc8 +
                  g_m7fc4 + g_m7fc0 + g_m7fbc + g_m7fb8 + g_m7fb4);
}

__attribute__((section(".text.f")))
s32 read_global_and_call(void)
{
    return sub_3BFC4(&g_m7e30);
}
