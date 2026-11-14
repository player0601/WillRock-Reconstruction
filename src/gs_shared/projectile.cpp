// projectile.cpp -- the projectiles' shared helpers (Will Rock 1.2, v12.exe). pjlPJL<SFX> itself is a template
// (include/game_shared/projectile.h): each projectile's file instantiates it.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/game_shared/projectile.h"
#include "../../include/player/player.h"

// v12 0x00547450
// The nearest live player to the entity (its position raised by yOff): its distance, the point (out) and who (may
// be 0). No player in the MP list: the local one, else a point 50 ahead of the entity (who = 0). The first player
// counts even when dead, until a live one is found.
float __fastcall entNearestPlayerDist(entENTITY *self, float yOff, m3dV *out, entENTITY **who)
{
    m3dV p;
    self->inst->GetPos(&p);
    entENTITY *pl = gsSysMP->GetNextPlayer(0);
    if (!pl) {
        if (plgsPlayer) {
            plgsPlayer->inst->GetPos(out);              // (plrPLAYER::GetPos, vtable +0x64)
            if (who)
                *who = plgsPlayer;
        } else {
            self->inst->GetDir(out);
            out->x = out->x * 50.0f + p.x;
            out->y = out->y * 50.0f + p.y;
            out->z = out->z * 50.0f + p.z;
            if (who)
                *who = 0;
        }
        out->y += yOff;
        m3dV d = p - *out;
        return m3dLength(d);
    }
    m3dV q;
    pl->inst->GetPos(&q);
    q.y += yOff;
    float best = m3dLength(p - q);
    *out = q;
    if (who)
        *who = pl;
    int bestDead = ((plrPLAYER_COMMON *)pl)->moveFlags & 2;    // (IsDead, vtable +0x74)
    while ((pl = gsSysMP->GetNextPlayer(pl)) != 0) {
        if (((plrPLAYER_COMMON *)pl)->moveFlags & 2)
            continue;
        pl->inst->GetPos(&q);
        q.y += yOff;
        float d = m3dLength(p - q);
        if (d < best || bestDead) {
            best = d;
            *out = q;
            if (who)
                *who = pl;
            bestDead = 0;
        }
    }
    return best;
}
