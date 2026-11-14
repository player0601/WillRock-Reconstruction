// wr_game_opt.cpp -- wrGO_SYSTEM, the game options and rules (Will Rock 1.2, v12.exe; feb obj wr_game_opt.obj)
//
// Only GetPlrDmgScale so far (the MP part of the game rules is in wr_go_mp.cpp).

#include "game.h"

// v12 0x004A7B00  (feb 0x00499C20)
// The damage the player takes: easy 0.5, hard 1.5 (feb 2.0), else 1.0.
float wrGO_SYSTEM::GetPlrDmgScale()
{
    switch (difficulty) {
    case 0:
        return 0.5f;
    case 2:
        return 1.5f;
    }
    return 1.0f;
}
