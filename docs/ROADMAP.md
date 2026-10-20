# Roadmap

The order of the work, and what each step needs. The steps are ordered by
dependency, not by appeal: nothing below is worth doing before the step
above it.

## 1. The container (done)

Read the package, describe the SELF container, rebuild the executable.
Tools: `tools/pkg.py`, `tools/ppu_elf.py`. Documentation:
[PKG-FORMAT.md](PKG-FORMAT.md), [SELF-FORMAT.md](SELF-FORMAT.md),
[PPU-EXECUTABLE.md](PPU-EXECUTABLE.md).

## 2. The function catalogue

* Run a PPU analysis over the text segment and export the functions it
  finds, with their addresses and sizes.
* Cross-check the catalogue with an independent method: a prologue scanner,
  the call graph, the relocation-like references, the data each function
  touches. A boundary that only one tool sees is a candidate, not a
  function.
* Publish the catalogue as a generated table in `config/`, with the
  evidence for each entry.
* Needs: the local executable; a disassembler; a lot of time.

## 3. The naming

* Attach the strings, the data and the module each function touches, so
  that a name can be justified instead of guessed.
* Keep the aliases explicit: a descriptive name is a name with a stated
  reason, not a fact.
* Needs: step 2.

## 4. The first sources

* Write the first C sources for the functions whose behaviour the analysis
  makes clear, one file per function, in `src/`.
* Record what each source proves and what it does not.
* Needs: steps 2 and 3.

## 5. The compiler

* Identify the compiler that produced the retail build: the PS3 SDK's PPU
  compiler, with its version, its options and its link script.
* Qualify it on a single function: find the source form and the options
  that reproduce the retail bytes.
* Qualify the link layout: the segment order, the alignment, the
  relocation rules, the stub table.
* Needs: a toolchain the contributor supplies. It is proprietary, and the
  project never distributes it.

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
