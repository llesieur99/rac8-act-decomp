# Roadmap

The order of the work, and what each step needs. The steps are ordered by
dependency, not by appeal: nothing below is worth doing before the step
above it.

## 1. The container (done)

Read the package, describe the SELF container, rebuild the executable.
Tools: `tools/pkg.py`, `tools/ppu_elf.py`. Documentation:
[PKG-FORMAT.md](PKG-FORMAT.md), [SELF-FORMAT.md](SELF-FORMAT.md),
[PPU-EXECUTABLE.md](PPU-EXECUTABLE.md).

## 2. The function catalogue (done)

* The retail executable keeps its own list of functions: the entry point
  is the module descriptor, not code.  It is read by
  `tools/ppu_bounds.py` and it is the truth the other methods are measured
  against.
* Three methods now recover the boundaries: the descriptor itself, the
  prologue and call scans, and a walk of the control flow.  The flow
  method is 100 % precise and recalls 91 % of the program, where the two
  cheap scans together reach 24 %.
* The catalogue is published in `config/functions.tsv`, with the evidence
  for each address.
* Documentation: [CATALOGUE.md](CATALOGUE.md).

## 3. The naming (done)

* The strings, the globals and the module each function touches are
  attached to it, so that a name can be justified instead of guessed.
* The chain is outilled and checked; eighty functions were named with
  their proof, and the coverage over the whole program was measured.
* Documentation: [NAMING.md](NAMING.md), [GLOBALS.md](GLOBALS.md).

## 4. The first sources (in progress)

* Four functions are byte-exact: the module's initialisation function and
  three more.  Each source says what it proves and what it does not.
* The reading is blocked on one thing: the compiler that built the target
  keeps the link register in a register the rebuilt one never chooses.
  That difference is now measured and located, and it is the subject of
  the next step.
* Documentation: [ABI.md](ABI.md), [CROSS-GAME.md](CROSS-GAME.md).

## 5. The compiler (in progress)

* The compiler that produced the retail build is identified: Sony's PPC
  GCC 4.1.1 from the SDK, and it is built from its own sources — two of
  them, the 2.7.0 and the 3.6.0 — plus the 2.8.0 and the 4.2.0.
* The options are qualified **per link unit**, not per project: `-O1`
  reproduces one function, `-O2` the others.
* What is missing is the compiler's own choice of the register for the
  link register.  The same source, compiled by the two compilers, chooses
  two different registers — measured on the code the two Insomniac games
  share.
* The link layout is not qualified either: the globals are reached
  through a TOC that the candidate's link script has to reproduce.

## 6. The gates

* Open the body gate, then the placement gate, then the
  complete-program gate, as described in [GATES.md](GATES.md).
* Only then publish counts, and only then a percentage.
* Needs: step 5.

## 7. The native port

* Once the program matches, the same approach the sibling projects use
  applies: run the game's own code on a PC, with a renderer, a sound
  backend and the game's own data.
* Needs: step 6.
