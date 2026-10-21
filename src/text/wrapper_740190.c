/*
 * Ratchet & Clank Future: A Crack in Time (PS3, USA NPUA80966)
 *
 * A one-line wrapper: it calls sub_741F98 and returns nothing.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x740190
 * Extent   : 0x740190 .. 0x7401B8 (40 bytes, 10 instructions)
 * Compiler : SCEI PPC GCC 4.1.1 (SDK270, $Rev: 3019 $), target ppu-lv2
 * Flags    : -O2
 * Result   : **byte-exact**.  The 40 bytes at 0x740190 are reproduced.
 *
 * The proof
 * ---------
 * The candidate is compiled with the compiler that produced the retail
 * build, linked at the measured address with the callee at its retail
 * address, and compared with the retail bytes:
 *
 *     python tools/compare_body.py --elf work/eboot/EBOOT.elf \
 *         --object work/more/gate/final/wrapper_740190/wrapper_740190.elf --address 0x740190
 *     match: 40 bytes at 0x740190
 *
 * What the code does
 * ------------------
 * The function loads the pointer stored at offset 4 of its argument and
 * hands it to sub_741F98 (0x741F98), which returns nothing.  The
 * load is a 32-bit one: the pointers of this ABI are 32 bits (the ILP32
 * model), so the value read is the low half of the word.
 *
 *     stdu r1, -0x70(r1)
 *     mflr r0
 *     std r0, 0x80(r1)
 *     lwz r3, 4(r3)
 *     bl 0x741F98
 *     nop
 *     ld r0, 0x80(r1)
 *     addi r1, r1, 0x70
 *     mtlr r0
 *     blr
 *
 * The two details that make the bytes match
 * ------------------------------------------
 * * **The frame is 0x70 bytes and the link register goes to the
 *   ABI slot** (the caller's frame, at SP + 0x10), not inside the
 *   callee's frame.  The default TOC leaves the frame at 0x70 bytes, with the link
 *   register in the ABI slot of the caller's frame; "-mminimal-toc"
 *   would make the frame 0x90 bytes.
 * * **The arguments are forwarded untouched.**  They are already in the
 *   registers, so the call needs no register set-up: the "bl" is preceded
 *   by nothing at all, and the delay slot holds the "nop" the SDK emits
 *   after a call.
 *
 * What is still a hypothesis
 * --------------------------
 * * The name.  "wrapper_740190" describes the shape of the code, not
 *   the game: the game has no name for it.  None of the functions of this
 *   shape has a call site in the executable's own text: they are reached
 *   from another module.
 * * The parameter.  The wrapper forwards r3 without touching it, so the
 *   bytes do not say what the argument is.  The callee reads it as a
 *   32-bit pointer (the ILP32 model of this ABI), so the source uses
 *   "void *" here; a 32-bit "unsigned int" would produce the same bytes.
 * * The callee's name is its address: sub_741F98 at 0x741F98.
 *
 * Reopening condition
 * -------------------
 * The source is a match; it is promoted.  If the build's compiler or its
 * flags ever change, the comparison has to be run again before the body
 * is counted.
 */

extern void sub_741F98(void *p);

void wrapper_740190(void *p)
{
    sub_741F98(*(void **)((char *)p + 4));
}
