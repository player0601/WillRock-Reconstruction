# Will Rock: Source Reconstruction

A rebuild of the source code of **Will Rock** (Saber Interactive, 2003), patch 1.2, reconstructed function by
function from the 1.2 executable and checked against it. The project contains **no game
data**, so you need your own copy of the game to be able to run it.

## What works

- Engine: scenes, models, animation, collision, particles, shaders (fog), sound, HUD and menus.
- The player, movement, weapons (colt, shotgun, minigun, bazooka, bomb) and some of their effects.
- Enemies: rats, skeletons (sword, spear, kamik), satyr, minotaur (and the atrium one), cyclops, gladiators,
  crocodile, birds, amur, centaur and skeleton centaur, tiger and lion, cerberus, gefest and zevs.
- Doors, buttons, traps, water and lava, destructible objects, spawn domains, teleports, level scripts.

Not rebuilt yet: medusa, the statues (sphinx, atlas, diskobolus, perseus), Zeus' bolt, the crossbow, acid gun, medusa gun and atomic gun,
multiplayer play, saving/loading. About 240 functions are still stubs: a stub stops the game and names itself.

## When it crashes

Three files appear next to the exe, attach them to an issue:

- `crash_log.txt` - where it crashed (module + offset), registers, stack, free disk space
- `crash_gameplay.txt` - the last events before the crash: level, player position, health, weapon, enemies alive
- `crash.dmp` - a minidump (Visual Studio / WinDbg)

## Layout

- `src/engine`, `src/m3d`, `src/gs`, `src/gs_shared`, `src/ap`, `src/drv` - engine and game systems
- `src/Sources` - the game itself (player, weapons, enemies, effects)
- `include/` - class layouts; every field and function with its address in the 1.2 exe
- `test/` - the host (renderer, level loader, menus) that runs the rebuilt code
- `tools/` - build scripts

Building: [BUILDING.md](BUILDING.md) (quick: `sh tools/build_release.sh` → `build/rel/WillRock_Decomp.exe`).

## Credits

Will Rock © Saber Interactive / Ubisoft. Non-commercial preservation project.
