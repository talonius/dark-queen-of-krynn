# The Dark Queen of Krynn

This project is a source reconstruction of SSI's DOS version of *The Dark Queen of Krynn*. The program has been recovered as Turbo C and assembly source, and `DQK.EXE` can be rebuilt from this repository with Turbo C 2.01, Turbo Assembler 3.1 and Pocket Soft's RTLink/Plus overlay linker under DOSBox-X (see [Building](#building)). The rebuilt executable plays, but it is not a byte-for-byte copy of SSI's: the RTLink/Plus version SSI used has not been found, and the one used here lays the program out slightly differently. See [Status](#status).

I am unable to continue this work. It is published in the hope that someone else can finish it.

## Read this first

- This source was reverse-engineered directly from the original game executable. It is **not** a clean-room reimplementation of the game, its engine or its mechanics.
- *The Dark Queen of Krynn*, *Dragonlance*, *Dungeons & Dragons*, Strategic Simulations, Inc. and all related names, trademarks, copyrights, artwork, writing, game data and other rights belong to their respective owners. I do not claim ownership of them. This project is not affiliated with or endorsed by those owners.
- This repository holds source code only. It includes no game executables and none of SSI's game data. You need to own a copy of *The Dark Queen of Krynn* to work with it. It is sold as part of *Dungeons & Dragons: Krynn Series* on [GOG](https://www.gog.com/en/game/dungeons_dragons_krynn_series) and [Steam](https://store.steampowered.com/app/1904610/Dungeons__Dragons_Krynn_Series/).
- AI coding tools were used throughout the project: Anthropic's Claude Code and OpenAI Codex.
- This is a hobby and preservation project, provided as-is, without warranty or support. Please do not ask GOG, Steam, SNEG or the original rights holders for support with it.
- Because this source is derived from SSI's code, this repository does not make an open-source license grant, except where a file says otherwise (see [Third-party code](#third-party-code)). Making the source visible does not grant rights to the original game or its derived material. Anyone reusing or redistributing material from this repository is responsible for determining what they may legally use.

Laws concerning reverse engineering and derived works vary. You are responsible for complying with the laws and terms that apply to your copy of the game.

## What is here

The `v1.0` and `v1.1` tags hold the reconstruction of each DOS release; `main` holds version 1.1. The target is `DQK.EXE` (version 1.1, 449,664 bytes, SHA-256 `3b41fb06a045568b0c35098a104499d489d8a5c0ef0dd48a039387ba2393ba58`).

| Path | Contents |
|---|---|
| `src/c/` | 86 Turbo C source files, the bulk of the game |
| `src/asm/` | Hand-written assembly for Turbo Assembler, including the startup code and the Miles AIL sound library |
| `src/rtlink/` | Reviewed assembly for the RTLink/Plus overlay manager's startup routines |
| `src/generated/` | NASM source for every byte not yet recovered as C or TASM source, and `manifest.json` |
| `build/` | The DOS build: `build.py` (host driver), `BUILD.BAT`, the RTLink/Plus link script `DQK.LNK` and `RTLINK.CFG` |

`src/generated/manifest.json` describes the whole executable as an ordered list of parts. For each part it records the file offset and size of its bytes in `DQK.EXE` and the source and tool that produce them. For each C or TASM module it also records where the linker placed its segments (`place`) and the address of every external symbol it calls (`syms`). The executable's SHA-256 is recorded as `reference_sha256`.

## Status

By bytes of `DQK.EXE`, the parts break down as:

| Produced by | Parts | Bytes | Share |
|---|---|---|---|
| Turbo C 2.01, from `src/c/` | 159 | 395,675 | 88% |
| Turbo Assembler 3.1, from `src/asm/` | 29 | 16,208 | 3.6% |
| NASM, from `src/generated/` and `src/rtlink/` | the rest | 37,781 | 8% |

Most of the NASM share is not program code: it is the MZ header, the relocation tables and RTLink/Plus's own structures, which a linker writes itself. The rest is RTLink/Plus's overlay manager, which the linker also supplies.

The game was linked with Pocket Soft's **RTLink/Plus**, an overlay linker: `DQK.EXE` carries RTLink/Plus's overlay manager and a table of 13 sections that it loads on demand. The disassembly project proved the sources byte for byte with its own host-side tools, which assembled the NASM parts and placed each compiled C and TASM module at the address the manifest records. Those tools are not part of this repository. What is here instead is a real DOS link, described below.

### The RTLink/Plus build

`build/` compiles every C module with Turbo C 2.01, assembles every assembly module with TASM 3.1, and links them with **RTLink/Plus 4.14** (Pocket Soft, 1991), all under DOSBox-X. `build/DQK.LNK` gives the shipped overlay structure: the resident code, then twelve code sections in four load regions (combat with two sections above it, the adventure with five above it, and spells and views in a second area). RTLink/Plus puts DGROUP in a preloaded data section of its own, as SSI's linker did.

The result, measured against the shipped `DQK.EXE`:

- The link resolves every symbol. Its section table has the shipped shape: all 13 records have the shipped body length, relocation count, parent section and flags.
- The resident code ahead of the overlay manager, 121,226 bytes, has the shipped length and layout. 4,928 of those bytes differ: segment words and DGROUP offsets, moved by the two layout differences below. That was checked by sampling, not proven byte by byte.
- It plays. Under DOSBox-X it shows the title, runs the built-in demo into tactical combat with AI turns and spells, loads a saved game, views a character, enters Palanthas, runs its scripted events, and opens a shop. The demo's screens are identical to the shipped executable's, frame for frame, for its first two minutes. That is as far as the shipped executable repeats itself from run to run, too.
- It is 449,247 bytes, against 449,664.

It is not byte-identical, for two reasons:

1. **The overlay manager is 4.14's, not SSI's.** SSI's manager is from a later RTLink/Plus, most likely 5.02 to 5.10: it adds the file-offset adjustment 5.02 introduced, and lacks the `$$RTL_TERMINATE` code of later versions. Versions 2.03, 4.00, 4.14 and 6.10 were all compared, and none matches it. 4.14's manager, with the same 32-call reload stack (`RELOAD FAR 32.`), is 416 bytes shorter. Everything linked after it, the overlay load regions and DGROUP, sits 26 paragraphs lower, so every relocated segment word that names them differs. `src/asm/rtlerr.asm` and `rtlfind.asm` reconstruct the run-time code beside SSI's manager. They are assembled but not linked, because RTLink/Plus supplies its own.
2. **DGROUP is in a different order.** RTLink/Plus 4.14 lays out each module's `_DATA` and `_BSS` in the order the link script names the modules. The shipped order shows that SSI's link pulled many modules from a library after the named ones, and put four of them, `lineedit`, `dispinit`, `dirnames` and `approach`, into the overlay section that needed them. 4.14 extracts library modules only into the resident code, so that order cannot be reproduced with it. Every DGROUP offset that moves changes the code that uses it.

Three source changes were needed for a real link. Each is additive, and the byte-exact form is kept:

- `src/asm/start.asm` assembles with `/dLINKED` for this build. It then declares the segment order (code, `_DATA`, `_BSS`, an end marker and a 1K stack, as in the shipped image), zeroes `_BSS` from linker-resolved bounds instead of the shipped `a260..db5f`, and names `start` as the entry point.
- `src/asm/globals.asm` defines the 218 global variables that no module defines. Turbo C emits an uninitialised global as a communal variable, which the linker allocates; the reconstructed C declares them `extern` in `dqk.h`. The file keeps their shipped order and spacing.
- The link matches symbols without regard to case. `sound.c` calls `ail_startup`; `ail.asm` exports `AIL_startup`.

The right RTLink/Plus version, about 5.02 from late 1991 or 1992, should close most of the remaining gap: its manager, and very likely its library extraction into sections.

## Building

You need Python 3 and DOSBox-X on the host, and three DOS tools, none of which is included:

- **Turbo C 2.01**: the directory holding `TCC.EXE`, with its `INCLUDE` directory. Its default `TURBOC.CFG` names `D:\INCLUDE`, and the build mounts the compiler as `D:`.
- **Turbo Assembler 3.1**: Borland C++ 3.1's `BIN` directory, holding `TASM.EXE`.
- **RTLink/Plus 4.14**, installed: the directory holding `RTLINK.EXE`.

```
build/build.py --tc /path/to/TC201 --tasm /path/to/BC31/BIN --rtlink /path/to/RTLINK
```

The three directories may also be given as `DQK_TC`, `DQK_TASM` and `DQK_RTLINK`. The build copies the sources to a temporary directory, runs `build/BUILD.BAT` in DOSBox-X without a window, checks the logs, and writes `DQK.EXE`, `DQK.MAP` and the compiler, assembler and linker logs to `obj/`. Copy `DQK.EXE` over the one in an installed copy of the game to play it.

## Compiler and assembler

The compiler has been determined to be **Borland Turbo C 2.01**, run as `TCC.EXE` with its default `TURBOC.CFG`. Every C module is compiled with the same flags:

```
tcc -c -ml -O -Z -d <module>.c
```

- `-c` compiles without linking.
- `-ml` selects the large memory model (far code and far data).
- `-O` optimizes jumps.
- `-Z` suppresses redundant register loads.
- `-d` merges duplicate strings.

The assembly in `src/asm/` is assembled with **Turbo Assembler 3.1** (from Borland C++ 3.1) with `/ml` (case-sensitive symbols). `ail.asm` uses `/m /w+ /ml`: it needs TASM 3.1's multiple passes. For the RTLink/Plus build, `start.asm` adds `/dLINKED`.

The linker was **RTLink/Plus**, from about version 5.02 (see [Status](#status)). This repository's build uses RTLink/Plus 4.14, with the FREEFORMAT link script `build/DQK.LNK`.

## Third-party code

`src/asm/ail.asm`, `ail.inc` and `ail.mac` are the IBM Audio Interface Library (AIL) by John Miles, Copyright (C) 1991, 1992 Miles Design, Inc., from his public AIL2 source release, with edits that make it the version 2.02 this game links. `src/asm/AIL2-README.md` carries that release's README, including John Miles' grant of May 26, 2000, which covers this source.

## Credits and Thanks

This work would not exist without SSI's original game and the people who have spent years documenting and exploring the Gold Box engine.

Special thanks to:

- [Gold Box Companion](https://gbc.zorbus.net/) for its character tools, automapping and years of practical Gold Box knowledge.
- [Gold Box Explorer](https://github.com/bsimser/Gold-Box-Explorer) for making the games' files and resources much easier to inspect and understand.
- Simeon Pilgrim's [CoAB project](https://github.com/simeonpilgrim/coab), an earlier reverse-engineering effort for *Curse of the Azure Bonds* that provided valuable reference work.
- John Miles, for releasing the Audio Interface Library's source.
- GOG and Steam for continuing to sell these games and giving new players a way to find them.
- SNEG for handling the rights work and negotiations that allowed the Gold Box games to return to sale.
