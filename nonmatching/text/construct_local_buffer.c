/*
 * A Crack in Time (PS3, USA NPUA80966) -- first reconstruction attempt.
 *
 * Target   : USRDIR/EBOOT.BIN, rebuilt PPU ELF
 * Address  : 0x25D98
 * Extent   : 0x25D98 .. 0x25DC4 (44 bytes, 11 instructions)
 * State    : not compiled -- the PPU compiler is not qualified yet
 *             (docs/ROADMAP.md, step 5). This file is a reading of the
 *             code, not a match, and it lives in nonmatching/ for that
 *             reason.
 *
 * Measured
 * --------
 * Compiled with the SDK's compiler (PPC GCC 4.1.1, SDK270) and compared
 * with the retail bytes, the candidate matches up to the fifth byte: the
 * frame size (0x90) and the layout agree, and the difference is the
 * register the link register is moved through — the retail code uses r3
 * where this source uses r0.  A function that *returns* the callee's
 * value keeps r0 free, which is what the retail code does; the next
 * attempt is the same source with a value returned.
 *
 *     difference at 0x25D9D (+5 bytes in)
 *
 * The retail code also stores the link register *after* the first constant
 * is loaded, and loads that constant before the address of the buffer: the
 * order is the register allocator's and the scheduler's, and it is
 * reproduced by the source form and the translation unit, not by hand
 * (docs/ABI.md).
 *
 * What the code does
 * ------------------
 * The function sets up a stack frame, takes the address of a local
 * buffer in it, and calls another function with that address and the
 * value 0. The callee at 0x4F9690 is two instructions long: it stores
 * its second argument at the address its first argument holds.
 *
 * So the function writes a zero at the start of a 32-byte local buffer.
 * The buffer's size comes from the frame: the frame is 0x90 bytes and
 * the buffer starts 0x70 bytes into it, which leaves exactly 0x20 bytes
 * of local space.
 *
 * What is a hypothesis here
 * -------------------------
 * * The local's type. A 32-byte buffer whose first word is set to zero
 *   is consistent with an object under construction, but so is a plain
 *   array.
 * * The call. The compiler did not inline the store, so the source
 *   either calls a function or the store was not inlinable. Which one is
 *   not established, and the two forms do not produce the same bytes.
 * * The argument. The first argument register is not used by the
 *   function; whether the source takes one is not established.
 *
 * Reopening condition
 * -------------------
 * The source can be promoted to src/ when a qualified PPU compiler
 * reproduces the 44 bytes at 0x25D98, and when the local's type is
 * known from the code that uses it.
 */

void sub_4F9690(unsigned int *address, unsigned int value);

void construct_local_buffer(void)
{
    unsigned int buffer[8];

    sub_4F9690(buffer, 0);
}
