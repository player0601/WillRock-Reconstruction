// test_menu.h -- the box test's front end (test_menu.cpp): the 1.2 main menu from ui.tpl / ui.lg + a DEBUG menu
#pragma once
class objOBJ;
enum { TST_MENU_QUIT = 0, TST_MENU_PLAY = 1, TST_MENU_STOP = 2 };
int  tstSndStart(const char *gameDir);                 // (test) msgSystem, gsMsgInit, gsSND_SYSTEM, menu woofer; once
int  tstMenuInit(const char *gameDir);
void tstMenuMarkTex();
const char *tstDrawStats();                            // (test) how many instances / objects the last frame drew                                 // (test) the menu's textures marked used before a level's purge                 // strings, fonts, ui.tpl frames, ui.lg backdrop (needs tstTexInit)
int  tstMenuRun(int inGame);                           // until a choice: quit, play / resume, stop the game
void tstDrawTree(objOBJ *root, const float *inst, unsigned color);
extern const char *tstMenuShot;                        // --menushot <file>: screenshot the menu and quit
extern const char *tstMenuShotFrame;                   // --menuframe <frame>: start on that frame (e.g. debug)
void tstDrawText(const char *s, float x, float y, unsigned color, float scale);  // font_s, y = baseline
void tstDrawWatermark();                               // "WILL ROCK - Decompilation Build 2026", top left (font_s)
enum { TST_MENU_PLAYLG = 3 };                          // DEBUG > PLAY LEVEL: tstMenuLevelPath is the .lg
extern const char *tstMenuLevelPath;
class scnSCENE;
void tstDrawScene(scnSCENE *s, const float *eye);       // a loaded level, lightmapped, from an eye position
extern int tstDrawLayer;   // test_menu.cpp: 0 the world, 1 the first-person buffer
void tstHudDraw();                                     // the game's HUD (wr_ui_hud.cpp) for plrPlayer, over the frame
