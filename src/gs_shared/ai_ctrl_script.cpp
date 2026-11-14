// ai_ctrl_script.cpp -- an NPC's property keys (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_script.obj)
//
// A key is looked for in its spawn area's sheet, then in its spawn domain's, then in the instance's.

#include "../Sources/game.h"

static psSHEET *areaSheet(aiNPC *n)
{
    u8 *a = WR_FIELD(u8 *, n->spawnDomain, 0xDC + n->spawnParam * 4);
    a = a ? a - 0x98 : 0;
    return WR_FIELD(psSHEET *, WR_FIELD(u8 *, a, 0xCC), 0xF0);
}

static psSHEET *domainSheet(aiNPC *n)
{
    return WR_FIELD(psSHEET *, WR_FIELD(u8 *, n->spawnDomain, 0xCC), 0xF0);
}

// v12 0x00553170  (feb 0x00538970)
BOOL aiNPC::GetStr(char *sect, char *key, char *buf, int n)
{
    if (spawnDomain) {
        if (psSHEET *s = areaSheet(this))
            if (s->GetStr(sect, key, buf, n))
                return 1;
        if (psSHEET *s = domainSheet(this))
            if (s->GetStr(sect, key, buf, n))
                return 1;
    }
    return inst->GetStr(sect, key, buf, n);
}

// v12 0x00553220  (feb 0x00538A20)
BOOL aiNPC::GetInt(char *sect, char *key, int *v)
{
    if (spawnDomain) {
        if (psSHEET *s = areaSheet(this))
            if (s->GetInt(sect, key, v))
                return 1;
        if (psSHEET *s = domainSheet(this))
            if (s->GetInt(sect, key, v))
                return 1;
    }
    return inst->GetInt(sect, key, v);
}

// v12 0x005532D0  (feb 0x00538AD0)
BOOL aiNPC::GetFloat(char *sect, char *key, float *v)
{
    if (spawnDomain) {
        if (psSHEET *s = areaSheet(this))
            if (s->GetFloat(sect, key, v))
                return 1;
        if (psSHEET *s = domainSheet(this))
            return s->GetFloat(sect, key, v);
    }
    return inst->GetFloat(sect, key, v);
}

// v12 0x00553370  (feb 0x00538B70)
BOOL aiNPC::GetBool(char *sect, char *key, int *v)
{
    if (spawnDomain) {
        if (psSHEET *s = areaSheet(this))
            if (s->GetBool(sect, key, v))
                return 1;
        if (psSHEET *s = domainSheet(this))
            return s->GetBool(sect, key, v);
    }
    return inst->GetBool(sect, key, v);
}

// v12 0x00553410  (feb 0x00538C10)
char *aiNPC::GetLogableName()
{
    if ((aiFlags & 0x100000) && spawnDomain)
        return (char *)spawnDomain + 0x40;
    return inst->name ? inst->name : (char *)"";
}
