# Ratchet &amp; Clank Future: A Crack in Time — matching decompilation

An early matching-decompilation workspace for **Ratchet &amp; Clank Future: A
Crack in Time** on the PlayStation 3 (USA release, `NPUA80966`, content id
`UP9000-NPUA80966_00-ACRACKINTIME0001`).

The goal is to rebuild the retail program in **C/C++ compiled**, with a proof:
the bytes the qualified toolchain produces must be **identical** to the retail
bytes, at every applicable placement, and the complete program must still hash
to the pinned retail hash.

This repository holds the **method, the rules, the templates and the tooling**,
and the first measured results. It publishes no game code, no assets and no
console keys.

## What is established

The pinned USA package has been read and measured:

| Fact | Value |
|---|---|
| Content id | `UP9000-NPUA80966_00-ACRACKINTIME0001` |
| Package size | 19,928,976,896 bytes |
| Package entries | 874 (790 files, 84 directories) |
| Main executable | `USRDIR/EBOOT.BIN`, 14,518,160 bytes |
| Executable kind | retail SELF (key revision `0x001C`, NPDRM, local licence) |
| PPU ELF | ELF64, big-endian, 8 program headers, 30 section headers |
| Entry point | `0xCA6B58` |
| Text segment | `0x10000`, 12,626,272 bytes |
| Symbol table | **none** — the retail executable is stripped |

The container, the SELF header and the PPU ELF are all described in
[docs/](docs/): [PKG-FORMAT.md](docs/PKG-FORMAT.md),
[SELF-FORMAT.md](docs/SELF-FORMAT.md), [PPU-EXECUTABLE.md](docs/PPU-EXECUTABLE.md).

Two facts shape the whole project:

1. **The retail executable is stripped.** There is no `.symtab`, no section
   names, and the dynamic section is obfuscated. Function boundaries and names
   have to be recovered by analysis, and every name in this project is a
   hypothesis until the code proves it.
2. **The executable is NPDRM-encrypted.** The package reader
   ([tools/pkg.py](tools/pkg.py)) is published; the SELF decryption needs the
   console key material, so it stays in the contributor's local workspace.
   [docs/SELF-FORMAT.md](docs/SELF-FORMAT.md) documents the scheme and points
   at RPCS3, which does the same thing.

## Start here

| Document | What it gives you |
| --- | --- |
| [PROMPT.md](PROMPT.md) | the starter prompt — what an agent reads, and the prompt to launch a wave |
| [AGENTS.md](AGENTS.md) | the ten working rules, and what must never be committed |
| [docs/METHOD.md](docs/METHOD.md) | the wave protocol, from the foundation lot to the pull request |
| [docs/EVIDENCE.md](docs/EVIDENCE.md) | what counts as a proof, and what does not |
| [docs/GATES.md](docs/GATES.md) | the gates: the body gate, the placement gate, the complete-program gate |
| [docs/CATALOGUE.md](docs/CATALOGUE.md) | the function catalogue: how the boundaries are recovered, and on what evidence |
| [docs/ABI.md](docs/ABI.md) | the Cell OS Lv-2 ABI as the code shows it: frames, the TOC, the nop after a call |
| [docs/STATE.md](docs/STATE.md) | the anchor: where the project stands, and what to do next |
| [docs/ROADMAP.md](docs/ROADMAP.md) | the ordered work, and what each step needs |

## Layout

| Path | Role |
| --- | --- |
| `src/` | accepted authored C/C++ definitions, one per function |
| `nonmatching/` | useful but nonmatching attempts, indexed by function |
| `tools/` | reusable tools, with tests |
| `tests/` | the unit tests of the tools, on synthetic data only |
| `config/` | pinned metadata: target identity, measured tables |
| `docs/` | the method, the evidence standard, the anchor |
| `work/` | the private workspace: package, extracted files, analysis projects (git-ignored) |

## Using the tools

The package reader needs nothing but the Python standard library:

```sh
python tools/pkg.py inspect  "path/to/package.pkg"
python tools/pkg.py list     "path/to/package.pkg" --pattern EBOOT
python tools/pkg.py extract  "path/to/package.pkg" --out work/pkg --only EBOOT.BIN
python tools/pkg.py verify   "path/to/package.pkg" --entries
```

The PPU ELF reader works on a decrypted executable:

```sh
python tools/ppu_elf.py info     work/eboot/EBOOT.elf
python tools/ppu_elf.py sections work/eboot/EBOOT.elf
python tools/ppu_elf.py symbols  work/eboot/EBOOT.elf
```

## Public boundary

This repository publishes **written** source, reusable tools, research
metadata (names, addresses, hashes) and documentation. It never publishes
disc images, retail executables, extracted modules, instruction listings,
disassembly, assets, proprietary toolchains, console keys, SDK sources, or
leaked or NDA source. See [AGENTS.md](AGENTS.md) and [LEGAL.md](LEGAL.md).

Obtain your own lawful copy of the game; the project cannot supply one, and
the local tooling and receipts stay on your machine.

## Licence

The independently authored material here is GPL-3.0, like the sibling
projects; see [LICENSE](LICENSE). That licence does not cover the game, its
assets, any console key material or any proprietary toolchain. Contributions
should be original or compatible with that licence, with attribution where
required.

Ratchet &amp; Clank is a trademark of Sony Interactive Entertainment; the game
was developed by Insomniac Games. This is an unofficial fan project, not
affiliated with or endorsed by either.
