# Running the reconstructed build

1. Copy `bin/box_test.exe` into the `bin` folder of your Will Rock install (next to `willrock.exe`).
2. Double-click it: it finds the install itself and opens the real main menu.
3. Or start a level directly from a command prompt in that `bin` folder:

       box_test.exe --nomenu --level ..\scenes\lev_1_1_outdoor.lg

Keys: WASD move, mouse look, Enter / left mouse shoot, 1 shovel, 2 colt, Q next weapon, E use, Esc menu, F11 full screen.
Options: `--windowed`, `--cull none`, `WR_FLASH=1` (muzzle flash), `WR_NODYNAMIC=1` (level actors without their "dynamic" entities).
Building from source: see BUILDING.md. What is rebuilt and what is a test stand-in: README.md.
