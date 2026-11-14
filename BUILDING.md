# Building the decompilation build (box_test.exe)

`box_test.exe` is the stand-alone build of the reconstructed sources: every `src/<lib>/*.cpp` plus the test host
in `test/`. It opens the game's real main menu and levels from an installed copy of Will Rock.

## What you need

- **Windows** (the build and the exe are 32-bit Windows; DirectX 7 rendering, DirectInput, DirectShow).
- **MSYS2** with the 32-bit MinGW GCC: install MSYS2, then in its shell
  `pacman -S mingw-w64-i686-gcc`. The compiler is `C:\msys64\mingw32\bin\i686-w64-mingw32-g++.exe`.
  The DirectX / DirectShow headers and import libraries (ddraw, dinput, dsound, strmiids...) come with it.
- **Python 3** (the stub generator `tools/gen_stubs.py`, `tools/packet.py`). The scripts call `python3`; if
  your Python is only `python`, put a one-line shim named `python3` on the PATH (`exec python "$@"`).
- **A short path**: deep folders break GCC's include paths (Windows MAX_PATH). Map the project folder to a
  drive letter and build from there: `subst W: <the folder that holds WillRock_RE>`.
- **The game data** (an installed Will Rock 1.2: `bin\willrock.ini`, `scenes\`, `textures\`, `sounds\`,
  `texts\`) to run the exe.

## Build

From a bash shell (MSYS2 or Git Bash):

```
export PATH=/c/msys64/mingw32/bin:$PATH
cd /w/WillRock_RE/src_recon
sh tools/build_test.sh
```

It compiles every source (about 2 minutes), then links twice: the first link lists the functions that are not
rebuilt yet, `gen_stubs.py` turns each into a stub, the second link makes `build/test/box_test.exe`.
`build/test/stubs.txt` lists the stubs. A stub that is called stops the exe with a message box naming it
("Not rebuilt yet: ..."): that is the next function to rebuild, not a crash in rebuilt code.

After one full build, `sh tools/build_quick.sh src/<lib>/<file>.cpp [test/<file>.cpp ...]` recompiles only
those files and relinks (a few seconds). Rebuild in full after changing a header.

## Run

```
build/test/box_test.exe --game "C:/Games/Will Rock"                      # main menu (keys: arrows, Enter, Esc)
build/test/box_test.exe --game "C:/Games/Will Rock" --nomenu --level "C:/Games/Will Rock/scenes/lev_1_5_atrium.lg"
```

Test options: `--levelshot <file>` (walk forward and save `<file>_0..3.ppm`), `--walktime <s>`, `--menushot
<file>`, `--selftest` (physics / mouse scripts on the box level).

The exe needs no Visual C++ runtime: the C++ library is linked statically and it imports only Windows DLLs.

## What is faithful and what is not

`src/` is the reconstruction: each file is one object of the original game, rebuilt from the 1.2 code
(addresses in the comments). `test/` is the host: anything marked PLACEHOLDER there stands in for game code that
is not rebuilt yet (for example the level loader's entity creation and the sound start-up), and is removed as
the real code arrives.
