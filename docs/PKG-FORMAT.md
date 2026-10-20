# The PS3 package container (`.pkg`)

A PlayStation 3 package is a flat container: a header, a content table, a
file table and a data area. The whole payload is XORed with a keystream, so
the reader must decrypt as it goes.

Everything below was measured on the pinned USA package of *A Crack in Time*
(`UP9000-NPUA80966_00-ACRACKINTIME0001`) and implemented in
[`tools/pkg.py`](../tools/pkg.py).

## Header (0x00 … 0xFF)

All the integers are big-endian.

| Offset | Size | Field | Value on the target |
|---|---|---|---|
| `0x00` | 4 | magic `7F 50 4B 47` (`\x7FPKG`) | ✓ |
| `0x04` | 2 | revision | `0x8000` |
| `0x06` | 2 | platform | `0x0001` (PS3) |
| `0x08` | 4 | content table offset | `0xC0` |
| `0x0C` | 4 | content count | 8 |
| `0x10` | 4 | header size | `0xC0` |
| `0x14` | 4 | file count | 874 |
| `0x18` | 8 | total size | 19,928,976,896 |
| `0x20` | 8 | data area offset | `0x180` |
| `0x28` | 8 | data area size | 19,928,976,416 |
| `0x30` | 48 | content id, NUL padded | `UP9000-NPUA80966_00-ACRACKINTIME0001` |
| `0x60` | 16 | digest | `618f4c2a9f20c6e61eaec1b5ee196567` |
| `0x70` | 16 | data RIV (the CTR initial counter) | `48ad2c004daba6c15e5904784f8d8712` |
| `0xC0` | 8×8 | content table: (index, type) pairs | see below |

The content table of a retail game holds eight entries: the package info,
the package data, the package metadata, the content, the DRM block, the
encrypted content and two more for the large-content variants. The reader
exposes them but does not need them: the file table is enough to extract.

## The file table

The file table starts at the data area offset (`0x180` here) and holds one
32-byte record per entry:

| Offset | Size | Field |
|---|---|---|
| `0x00` | 4 | name offset, relative to the data area |
| `0x04` | 4 | name length |
| `0x08` | 8 | payload offset, relative to the data area |
| `0x10` | 8 | payload size |
| `0x18` | 4 | flags: the low byte is the type (`0x03` file, `0x04` directory) |
| `0x1C` | 4 | padding |

The names live in the data area too, right after the table, and the
directories are entries with type `0x04` and a size of zero.

## The keystream

* **Retail packages** (revision `0x8000`) use AES-128 in counter mode. The
  counter is the 128-bit big-endian value of the data RIV at `0x70`, and the
  key is the platform key (`0x2E 7B 71 D7 C9 C9 A1 4E A3 22 1F 18 88 28 B8 F8`
  for PS3). The counter is incremented once per 16-byte block, starting at
  the RIV.
* **Debug packages** (revision `0x0000`) use a SHA-1 based keystream derived
  from the digest at `0x60`.

The keystream position is the offset **inside the data area**, and every
part of the container is encrypted at its own position: the file table at
`0`, each name at its name offset, each payload at its payload offset.
Getting this wrong is the classic reason for a package that "decrypts" into
garbage.

## Reading the pinned target

```sh
python tools/pkg.py inspect  package.pkg
python tools/pkg.py list     package.pkg --pattern EBOOT
python tools/pkg.py extract  package.pkg --out work/pkg --only EBOOT.BIN
python tools/pkg.py verify   package.pkg --entries
```

The target's layout:

| Entry | Size | Note |
|---|---|---|
| `PS3LOGO.DAT` | 5,120 | |
| `PARAM.SFO` | 1,040 | the title metadata |
| `ICON0.PNG`, `ICON0_01..03.PNG` | 91,369 each | |
| `PIC0..2.PNG`, `SND0.AT3` | | |
| `USRDIR/EBOOT.BIN` | 14,518,160 | the main executable, see [SELF-FORMAT.md](SELF-FORMAT.md) |
| `USRDIR/built/…` | 18.6 GiB in total | the game's assets, movies and audio |

The package holds the retail disc's content, so the working copy needs a lot
of room; extract only what a step needs.
