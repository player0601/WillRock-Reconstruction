# Building

The reconstructed sources (`src/<lib>/*.cpp`) plus the host in `test/` build into a stand-alone exe that opens the
game's real main menu and levels from an installed copy of Will Rock 1.2.

## What you need

- **Windows** (32-bit build; DirectX 7, DirectInput, DirectShow).
- **MSYS2** with the 32-bit MinGW toolchain. In the MSYS2 shell:
  `pacman -S mingw-w64-i686-gcc mingw-w64-i686-binutils python`
  The DirectX / DirectShow headers and import libraries come with it.
- **Python 3** for the stub generator (`tools/gen_stubs.py`). The scripts call `python3`; if yours is only
  `python`, put a one-line shim named `python3` on the PATH (`exec python "$@"`).
- **A short path.** Deep folders break GCC's include paths (Windows' 260-character limit). Map the folder to a
  drive letter and build from there, e.g. `subst W: "C:\path\to\parent-of-the-repo"`.
- **The game** (an installed Will Rock 1.2) to run the result. No game files are in this repository.

## Build

From an MSYS2 (or Git Bash) shell in the repository folder:

```sh
export PATH=/c/msys64/mingw32/bin:$PATH
sh tools/build_release.sh      # -> build/rel/WillRock_Decomp.exe   (optimised -O2, stripped, with the icon)
sh tools/build_test.sh         # -> build/test/box_test.exe         (-O1 with debug info: for crash addresses)
```

Each compiles every source (a few minutes), then links twice: the first link lists the functions that are not
rebuilt yet, `tools/gen_stubs.py` turns each into a stub, the second link makes the exe. `stubs.txt` next to it
lists them. A stub that gets called stops the game with a message naming it ("Not rebuilt yet: ...") — that is
missing code, not a crash in rebuilt code.

After one full debug build, `sh tools/build_quick.sh src/<lib>/<file>.cpp [test/<file>.cpp ...]` recompiles only
those files and relinks in seconds. Rebuild in full after changing a header.

The exe needs no Visual C++ runtime: the C++ library is linked statically and it imports only Windows DLLs.

## Run

Copy the exe into the game's `bin` folder (next to `willrock.exe`) and start it, or point it at the game:

```sh
WillRock_Decomp.exe                                         # main menu (from the game's bin folder)
WillRock_Decomp.exe --nomenu --level ../scenes/lev_1_5_atrium.lg
box_test.exe --game "C:/Games/Will Rock"                    # from anywhere
```

Test options: `--windowed`, `--levelshot <file>` (walk forward and save `<file>_0..3.ppm`), `--walktime <s>`,
`--menushot <file>`, `--selftest`. More in [RUN.md](RUN.md).

## Crash addresses

A crash writes `crash_log.txt` (module + offset, registers, stack), `crash_gameplay.txt` and `crash.dmp` next to the
exe. With the debug build, turn an address into a source line:

```sh
addr2line -f -C -e build/test/box_test.exe 0x<address - base + 0x400000>
```

## What is faithful and what is not

`src/` is the reconstruction: each file is one object of the original game, rebuilt from the 1.2 code (addresses in
the comments). `test/` is the host: anything marked PLACEHOLDER there stands in for game code not rebuilt yet (for
example parts of the level loader and the sound start-up) and is removed as the real code arrives.
