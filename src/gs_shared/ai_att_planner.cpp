// ai_att_planner.cpp -- aiPLANNER, the attack director (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_att_planner.obj)
//
// Keeps the awake NPCs and each player's breadcrumb trail (1.2: a graph, a crumb every second or 2 m, with its next crumb),
// says where an NPC should go next, and keeps NPCs of one class from walking in step.

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

#define PLN_CRUMBS(p) ((plnTRAIL *)((u8 *)(p) + 0x3DC))
static inline aiNPC *&plnBoss(aiPLANNER *p) { return WR_FIELD(aiNPC *, p, 0x350); }

// v12 0x00552080  a trail: empty
static void plnTrailInit(plnTRAIL *t)
{
    t->owner = -1;
    for (int i = 0; i < 50; i++) {
        t->c[i].pos = m3dVZero;
        t->c[i].id = -1;
        t->c[i].next = -1;
        t->c[i].flags = 0;
    }
    t->timer = 1.0f;
    t->nextId = 1;
    t->firstId = 0;
    t->n = 0;
}

// v12 0x00550DF0  (feb 0x00536760)
int __fastcall aiPlannerInit()
{
    aiAttackPlanner = new aiPLANNER;
    aiAttackPlanner->field04 = 0;
    aiAttackPlanner->field08 = 0;
    aiAttackPlanner->flags = 0;
    aiAttackPlanner->magic = 0x4D414452;
    aiAttackPlanner->classId = 0;
    aiAttackPlanner->unk18 = 0;
    aiAttackPlanner->priority = 0;
    aiAttackPlanner->sign = 0;
    WR_FIELD(int, aiAttackPlanner, 0x24) = 0;
    WR_FIELD(int, aiAttackPlanner, 0x34C) = 2;
    plnBoss(aiAttackPlanner) = 0;
    WR_FIELD(int, aiAttackPlanner, 0x28) = 0;
    memset((u8 *)aiAttackPlanner + 0x2C, 0, 200 * 4);
    WR_FIELD(int, aiAttackPlanner, 0x354) = 0;
    WR_FIELD(int, aiAttackPlanner, 0x398) = 0;
    memset((u8 *)aiAttackPlanner + 0x358, 0, 16 * 4);
    memset((u8 *)aiAttackPlanner + 0x39C, 0, 16 * 4);
    for (int i = 0; i < 16; i++)
        plnTrailInit(&PLN_CRUMBS(aiAttackPlanner)[i]);
    aiAttackPlanner->flags |= 2;
    WR_FIELD(int, aiAttackPlanner, 0x14) = 0x24414924;
    WR_FIELD(int, aiAttackPlanner, 0x1C) = 0x23;
    msgSystem.InsertAddr(aiAttackPlanner, 0x24535953, 0);
    return 1;
}

// v12 0x00550F20  (feb 0x00536830)
void __fastcall aiPlannerTerm()
{
    msgSystem.DestroyAddr(aiAttackPlanner);
}

static inline aiNPC **plnList(aiPLANNER *p) { return (aiNPC **)((u8 *)p + 0x2C); }
static inline int &plnN(aiPLANNER *p) { return WR_FIELD(int, p, 0x28); }

// v12 0x00550F70  (feb 0x00536880)
void aiPLANNER::RegisterNPC(aiNPC *n)
{
    if (plnN(this) < 200)
        plnList(this)[plnN(this)++] = n;
}

// v12 0x00550FE0  (feb 0x005368A0)
void aiPLANNER::UnRegisterNPC(aiNPC *n)
{
    aiNPC **l = plnList(this);
    for (int i = 0; i < plnN(this); i++)
        if (l[i] == n) {
            plnN(this)--;
            l[i] = l[plnN(this)];
            l[plnN(this)] = 0;
        }
    // (1.2 leaves the boss alone here: only the constructor and SetBoss write it -- a deactivated boss stays the
    //  boss; aiNPC::ProcessTERM clears it with SetBoss(0). Clearing it here hid Zeus's health bar.)
}

// v12 0x005510A0  (feb 0x005368F0)
BOOL32 aiPLANNER::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 3) {
        UnRegisterNPC((aiNPC *)data);
    } else if (id == 1000) {
        ProcessFRAME();
    } else if (id == 0x3F4) {
        plnN(this) = 0;
        memset(plnList(this), 0, 200 * 4);
        WR_FIELD(int, this, 0x354) = 0;
        WR_FIELD(int, this, 0x398) = 0;
        memset((u8 *)this + 0x358, 0, 16 * 4);
        memset((u8 *)this + 0x39C, 0, 16 * 4);          // (not the boss: it registered itself before INIT_LEVEL)
        for (int i = 0; i < 16; i++)
            plnTrailInit(&PLN_CRUMBS(this)[i]);
    }
    return 0;
}

// v12 0x005511B0  (feb 0x00536A00)
void aiPLANNER::ProcessFRAME()
{
    aiNPC **l = plnList(this);
    for (int i = 0; i < plnN(this); i++) {
        aiNPC *n = l[i];
        if (!n || !n->inst || (n->aiFlags & 0x210000)) {
            plnN(this)--;
            l[i] = l[plnN(this)];
            l[plnN(this)] = 0;
        }
    }
    GatherPlrInformation();
    DesyncAnimations();
}

// v12 0x00551220  (feb 0x00536A60)
scnDOMAIN *aiPLANNER::GetBanCrossDomain(aiNPC *n)
{
    return (n->aiFlags2 & 0x10) ? (scnDOMAIN *)n->navBanCross : 0;
}

// v12 0x00551240  (feb 0x00536A80)
// Out along its facing; with a nav area, a point of it: not across its banned line if it has one, else far from the enemy.
void aiPLANNER::GetFleeGoalPoint(aiNPC *n, float dist, m3dV *out)
{
    if (n->aiFlags2 & 8) {
        aiAREA *area = (aiAREA *)n->navArea;
        if (n->aiFlags2 & 0x10) {
            aiAREA_GET_POINT_PARAM q;
            q.pos = m3dVUndef;
            q.domain = 0;
            q.cross = (scnDOMAIN *)n->navBanCross;
            area->areaFlags = (area->areaFlags | 0x10) & ~2u;
            n->enemy.GetPos(&q.pos);
            n->GetPosRooted(&q.crossFrom);
            area->GetRandomPos(out, &q);
            return;
        }
        area->areaFlags = (area->areaFlags & ~0x10u) | 2;
        m3dV e;
        n->enemy.GetPos(&e);
        area->GetRandomPos(out, &e);
        return;
    }
    m3dV p, d;
    n->inst->GetPos(&p);
    n->inst->GetDir(&d);
    float k = dist * n->shootRange;
    out->x = d.x * k + p.x;
    out->y = d.y * k + p.y;
    out->z = d.z * k + p.z;
}

// v12 0x00551390  (feb 0x00536BD0)
// Where to meet a moving enemy at this speed (the planner's mode bit 2 leads the target; otherwise its place).
BOOL aiPLANNER::GetEnemyMeetPoint(aiNPC *n, float spd, m3dV *out)
{
    if (!n->enemy.ent || !n->enemy.ent->inst)
        return 0;
    if (!(WR_FIELD(u8, this, 0x34C) & 2)) {
        n->enemy.GetPos(out);
        return 1;
    }
    m3dV p, e, v;
    n->GetPosRooted(&p);
    n->enemy.GetPos(&e);
    v.x = n->enemy.ent->velocity[0];
    v.y = n->enemy.ent->velocity[1];
    v.z = n->enemy.ent->velocity[2];
    if (m3dSqrt(v.x * v.x + v.y * v.y + v.z * v.z) < 1e-6f) {
        *out = e;
        return 1;
    }
    m3dV u = { e.x - p.x, e.y - p.y, e.z - p.z };
    float dist = m3dSqrt(u.x * u.x + u.y * u.y + u.z * u.z);
    float inv = 1.0f / dist;
    u.x *= inv;
    u.y *= inv;
    u.z *= inv;
    float along = u.x * v.x + u.y * v.y + u.z * v.z;
    v.x -= u.x * along;
    v.y -= u.y * along;
    v.z -= u.z * along;
    float rest = spd * spd - (v.x * v.x + v.y * v.y + v.z * v.z);
    if (rest > 0.0f) {
        float s = m3dSqrt(rest);
        if (1e-6f + along <= s) {
            float t = dist / (s - along);
            float k = s * t;
            out->x = v.x * t + u.x * k + p.x;
            out->y = v.y * t + u.y * k + p.y;
            out->z = v.z * t + u.z * k + p.z;
            return 1;
        }
    }
    return 0;
}

// v12 0x005517E0  (feb 0x00536FE0)
// To the enemy, stopping a little short; clipped to a nav area (aiFlags2 0x40).
void aiPLANNER::GetAttackGoalPoint(aiNPC *n, m3dV *out)
{
    m3dV p, d, e, tip, t;
    n->inst->GetPos(&p);
    n->inst->GetDir(&d);
    n->enemy.GetPos(&e);
    n->GetWpnTip(&tip);
    n->GetPlayerTargetPos(&t);
    m3dV to = { p.x - e.x, p.y - e.y, p.z - e.z };
    float dist = m3dSqrt(to.x * to.x + to.y * to.y + to.z * to.z);
    float keep = (n->aiFlags & 1) ? 0.5f : 0.3f;
    if (!(n->aiFlags2 & 1) && fabsf(p.y - e.y) > 5.0f)
        keep *= 0.3f;
    if (keep <= dist) {
        m3dNormalize(&to);
        out->x = to.x * keep + e.x;
        out->y = to.y * keep + e.y;
        out->z = to.z * keep + e.z;
    } else {
        *out = p;
    }
    if ((n->aiFlags2 & 0x40) && (n->aiFlags2 & 8))
        ClipPointByNavArea(n, out);
}

// v12 0x005593E0  (name ours; feb inline)
// The crumb to go to from where it is: the crumb it is on while the trail is old enough and it sees it (up to 3 back), else
// the nearest one.
static plnCRUMB *plnSelectCrumb(aiPLANNER *pl, aiNPC *n)
{
    static plnCRUMB none;
    plnTRAIL *t = (plnTRAIL *)n->enemy.GetBreadCrumb(0);
    if (!t)
        return &none;
    plnCRUMB *c = 0;
    for (int tries = 0; n->crumbID >= t->firstId && tries < 3;) {
        c = 0;
        if (n->crumbID >= 0)
            for (int i = 0; i < t->n; i++)
                if (t->c[i].id == n->crumbID) {
                    c = &t->c[i];
                    break;
                }
        if (!c) {
            n->crumbID--;
            continue;
        }
        if (n->IsPlrVisible(&c->pos)) {
            n->aiFlags &= ~0x4000u;
            return c;
        }
        n->crumbID--;
        tries++;
    }
    if (c)
        return c;
    n->aiFlags &= ~0x4000u;
    m3dV r;
    n->GetPosRooted(&r);
    c = &t->c[0];
    float best = 1e20f;
    for (int i = 0; i < 50; i++) {
        if (t->c[i].id <= 0)
            continue;
        float dx = r.x - t->c[i].pos.x, dy = r.y - t->c[i].pos.y, dz = r.z - t->c[i].pos.z;
        float d = dx * dx + dy * dy + dz * dz;
        if (d < best) {
            best = d;
            c = &t->c[i];
        }
    }
    return c;
}

// v12 0x00559270  (feb 0x00537350, a new trail in 1.2)
void aiPLANNER::GetCrumbGoalPoint(aiNPC *n, m3dV *out)
{
    static plnCRUMB none;
    plnCRUMB *c;
    if (n->aiFlags & 0x4000) {
        c = plnSelectCrumb(this, n);
    } else if (n->aiFlags & 8) {
        if (!(n->aiFlags & 0x41)) {
            *out = n->goalPos;
            return;
        }
        c = &none;
        if (plnTRAIL *t = (plnTRAIL *)n->enemy.GetBreadCrumb(0)) {
            plnCRUMB *cur = 0;
            if (n->crumbID >= 0)
                for (int i = 0; i < t->n; i++)
                    if (t->c[i].id == n->crumbID) {
                        cur = &t->c[i];
                        break;
                    }
            if (cur) {
                for (int i = 0; cur->next >= 0 && i < t->n; i++)
                    if (t->c[i].id == cur->next) {
                        c = &t->c[i];
                        break;
                    }
            } else {
                n->aiFlags |= 0x4000;
                n->crumbID = t->nextId;
                c = plnSelectCrumb(this, n);
            }
        }
        n->aiFlags &= ~0x40u;
    } else {
        plnTRAIL *t = (plnTRAIL *)n->enemy.GetBreadCrumb(0);
        if (!t) {
            c = &none;
        } else {
            n->crumbID = t->nextId;
            n->aiFlags |= 0x4000;
            c = plnSelectCrumb(this, n);
        }
    }
    n->crumbID = c->id;
    *out = c->pos;
    out->y -= 1.0f;
}

// v12 0x00551A10  (feb 0x00537420)
// Square-root the play rates towards 1; two NPCs of a kind in the same sequence and phase play at slightly different rates.
void aiPLANNER::DesyncAnimations()
{
    aiNPC **l = plnList(this);
    int n = plnN(this);
    for (int i = 1; i < n; i++)
        l[i]->unk513 = m3dSqrt(l[i]->unk513);
    int changed = 0;
    for (int i = 1; i < n && changed <= 2; i++) {
        aiNPC *a = l[i];
        animINST *ai = a->inst;
        int s = ai->seq;
        float window = ai->tpl->seq[s].lengthMs * 0.0002f;
        for (int j = i - 1; j >= 0; j--) {
            aiNPC *b = l[j];
            if (b->unk98 == a->unk98 && b->inst->seq == s && fabsf(b->inst->seqTime - ai->seqTime) <= window
                && fabsf(a->unk513 - b->unk513) <= 0.1f) {
                float m = (b->unk513 + a->unk513) * 0.5f;
                if (m < 0.75f)
                    m = 0.75f;
                if (a->unk513 <= b->unk513) {
                    a->unk513 = m - 0.05f;
                    b->unk513 = m + 0.05f;
                } else {
                    a->unk513 = m + 0.05f;
                    b->unk513 = m - 0.05f;
                }
                changed++;
            }
        }
    }
}

// v12 0x00551DB0  (feb 0x00537680)
// The boss's health, its maximum and its kind (entENTITY +0x98: 0x11 cyclop, 0x12 gefest, ...);
// 0 (with 0, 1, -1) when there is no boss or it is dying (aiFlags 0x210000).
BOOL aiPLANNER::GetBossHealth(float *cur, float *max, int *type)
{
    aiNPC *b = plnBoss(this);
    if (b && !(b->aiFlags & 0x210000)) {
        *cur = b->health;
        *max = b->maxHealth;
        *type = b->unk98;
        return 1;
    }
    *cur = 0.0f;
    *max = 1.0f;
    *type = -1;
    return 0;
}

// v12 0x00551FB0  (feb 0x00537870)
// The middle of an NPC's nav area: the mean of its outline's edge ends (at y 0). No area: out untouched.
void aiPLANNER::GetNavAreaCenter(aiNPC *n, m3dV *out)
{
    if (!(n->aiFlags2 & 8))
        return;
    aiAREA *a = (aiAREA *)n->navArea;
    if (!a->nBound)                                     // (native guard: no outline made -- v12 would divide by 0)
        return;
    float x = 0, z = 0;
    for (int i = 0; i < a->nBound; i++) {
        x += a->bound0[i].x + a->bound1[i].x;
        z += a->bound0[i].y + a->bound1[i].y;
    }
    float k = 1.0f / (float)(a->nBound + a->nBound);
    out->y = 0;
    out->x = k * x;
    out->z = k * z;
}

// v12 0x00551E30  (feb 0x005376F0)
void aiPLANNER::SetBoss(aiNPC *n)
{
    plnBoss(this) = n;
}

// v12 0x00551E40  (feb 0x00537700)
void aiPLANNER::ClipPointByNavArea(aiNPC *n, m3dV *p)
{
    if (!(n->aiFlags2 & 8))
        return;
    m3dV o, hit;
    n->inst->GetPos(&o);
    n->navFlags |= 0x10000;
    hit = *p;
    ((scnDOMAIN *)n->navArea)->ISectLSegBoundaryXZ(&o, p, &hit);
    float dx = hit.x - o.x, dz = hit.z - o.z;
    if (m3dSqrt(dx * dx + dz * dz) < n->unk17B) {
        *p = hit;
        return;
    }
    *p = o;
}

// v12 0x00552050  (feb 0x00537910)
void aiPLANNER::UsePredictiveAiming(int on)
{
    if (on)
        WR_FIELD(u32, this, 0x34C) |= 2;
    else
        WR_FIELD(u32, this, 0x34C) &= ~2u;
}

// ---- a player's trail (1.2) ----------------------------------------------------------------------------------------

// v12 0x00558BE0
void *aiPLANNER::GetPlrCrumbs(plgsACTION *a)
{
    plnTRAIL *t = PLN_CRUMBS(this);
    if (!a)
        return &t[16];
    int id = a->mpId;
    for (int i = 0; i < 16; i++)
        if (t[i].owner == id)
            return &t[i];
    for (int i = 0; i < 16; i++)
        if (t[i].owner == -1) {
            t[i].owner = id;
            return &t[i];
        }
    t[16].owner = id;
    return &t[16];
}

// v12 0x00558C40  (name ours)
void aiPLANNER::SaveTarget(void *saved, plgsACTION *a)
{
    plnTRAIL *t = (plnTRAIL *)GetPlrCrumbs(a);
    plnSAVED *s = (plnSAVED *)saved;
    s->owner = t->owner;
    s->nextId = t->nextId;
    s->n = t->n;
    memcpy(s->c, t->c, 49 * sizeof(plnCRUMB));
}

// v12 0x00558CA0  (name ours)
void aiPLANNER::RestoreTarget(void *saved, plgsACTION *a)
{
    plnTRAIL *t = (plnTRAIL *)GetPlrCrumbs(a);
    plnSAVED *s = (plnSAVED *)saved;
    t->owner = s->owner;
    t->nextId = s->nextId;
    t->n = s->n;
    memcpy(t->c, s->c, 49 * sizeof(plnCRUMB));
}

// v12 0x00558FF0
static void plnCrumbSet(plnTRAIL *t, plnCRUMB *c, m3dV *p)
{
    m3dV pos = *p;
    c->pos = pos;
    c->id = t->nextId++;
    c->next = -1;
    c->flags = 0x10000;
}

// v12 0x00559060
// Those a move-marked crumb points at are checked next; then the first marked one is unmarked.
static int plnMarkNext(plnTRAIL *t)
{
    int i;
    for (i = 0; i < t->n; i++)
        if (t->c[i].flags & 0x10000)
            break;
    if (i >= t->n)
        return 0;
    int next = t->c[i].next;
    for (int k = 0; k < t->n; k++)
        if (t->c[k].next == next)
            t->c[k].flags |= 0x1000;
    t->c[i].flags &= ~0x10000u;
    return 1;
}

// v12 0x005590E0
static void plnUpdateFirst(plnTRAIL *t)
{
    t->firstId = t->nextId;
    for (int i = 0; i < t->n; i++)
        if (t->c[i].id > 0 && t->c[i].id < t->firstId)
            t->firstId = t->c[i].id;
}

// v12 0x00559120
static plnCRUMB *plnFindMarked(plnTRAIL *t)
{
    for (int i = 0; i < t->n; i++)
        if (t->c[i].flags & 0x1000)
            return &t->c[i];
    return 0;
}

static plnCRUMB *plnFindId(plnTRAIL *t, int id)
{
    for (int i = 0; i < t->n; i++)
        if (t->c[i].id == id)
            return &t->c[i];
    return 0;
}

// v12 0x00559150
// A crumb whose next-but-one lies almost straight on and in sight skips the one between.
static void plnShorten(plnTRAIL *t, plnCRUMB *c)
{
    c->flags &= ~0x1000u;
    if (c->next < 0)
        return;
    plnCRUMB *b = plnFindId(t, c->next);
    if (!b || b->next < 0)
        return;
    plnCRUMB *d = plnFindId(t, b->next);
    if (!d)
        return;
    m3dV u = { d->pos.x - b->pos.x, d->pos.y - b->pos.y, d->pos.z - b->pos.z };
    m3dV v = { b->pos.x - c->pos.x, b->pos.y - c->pos.y, b->pos.z - c->pos.z };
    if (u.x * v.x + u.y * v.y + u.z * v.z > 0.2f) {
        entCDT_REFINE_EXCL ref(2, 0, 1);
        if (gsScenePtr->IsVisiblePoint(&c->pos, &d->pos, 0, &ref, 0)) {
            c->next = d->id;
            c->flags |= 0x11000;
        }
    }
}

// v12 0x00558D00  (name ours)
// Called by the player each frame: within 2 m of a crumb it is on that one; else every second a new crumb.
void aiPLANNER::NotifyTarget(plgsACTION *a)
{
    if (!a)
        return;
    plnTRAIL *t = (plnTRAIL *)GetPlrCrumbs(a);
    m3dV p;
    wrEVCall<void>(a, 0x64, &p);
    p.y += 1.0f;
    plnCRUMB *c = 0;
    for (int i = 0; i < t->n; i++) {
        if (t->c[i].id <= 0)
            continue;
        float dx = t->c[i].pos.x - p.x, dy = t->c[i].pos.y - p.y, dz = t->c[i].pos.z - p.z;
        if (m3dSqrt(dx * dx + dy * dy + dz * dz) < 2.0f) {
            c = &t->c[i];
            break;
        }
    }
    int newId;
    if (c) {
        int oldId = c->id;
        plnCRUMB *nx = c->next >= 0 ? plnFindId(t, c->next) : 0;
        plnCrumbSet(t, c, &c->pos);
        if (nx) {
            nx->next = c->id;
            nx->flags |= 0x10000;
        }
        for (int i = 0; i < t->n; i++)
            if (t->c[i].next == oldId) {
                t->c[i].next = c->id;
                t->c[i].flags |= 0x10000;
            }
        newId = c->id;
    } else {
        t->timer -= gsElapsedTime;
        if (t->timer > 0.0f)
            return;
        t->timer = 1.0f;
        plnCRUMB *o = &t->c[0];
        int low = t->nextId;
        for (int i = 0; i < 50; i++)
            if (t->c[i].id < low) {
                o = &t->c[i];
                low = t->c[i].id;
            }
        if (t->n < 50)
            t->n++;
        o->pos = p;
        o->id = t->nextId;
        t->nextId++;
        o->next = -1;
        o->flags = 0x10000;
        newId = o->id;
    }
    int prev = -1, prevIdx = -1;
    for (int i = 0; i < t->n; i++) {
        int id = t->c[i].id;
        if (prev <= id && id <= newId - 1) {
            prevIdx = i;
            prev = id;
        }
    }
    if (prevIdx >= 0)
        t->c[prevIdx].next = newId;
    if (plnCRUMB *m = plnFindMarked(t)) {
        plnShorten(t, m);
    } else {
        plnMarkNext(t);
        plnUpdateFirst(t);
    }
}

// ---- which player (feb obj ai_plan_filter.obj) ------------------------------------------------------------------

// v12 0x0055AD90  (feb 0x00540CE0)
// The live players, into the list.
void aiPLANNER::GatherPlrInformation()
{
    int n = 0;
    plgsACTION **l = (plgsACTION **)((u8 *)this + 0x358);
    for (entIACTIVE_OBJ *p = gsSysServer->GetNextPlayer(0); p; p = gsSysServer->GetNextPlayer(p)) {
        plgsACTION *a = (plgsACTION *)p;
        if (!wrEVCall<BOOL32>(a, 0x74) && n < 16)
            l[n++] = a;
    }
    WR_FIELD(int, this, 0x354) = n;
}

// v12 0x0055AEC0  (feb 0x00540DA0)
void aiPLANNER::StartFilteringPlayers()
{
    int n = WR_FIELD(int, this, 0x354);
    WR_FIELD(int, this, 0x398) = n;
    plgsACTION **f = (plgsACTION **)((u8 *)this + 0x39C), **l = (plgsACTION **)((u8 *)this + 0x358);
    for (int i = 0; i < n; i++)
        f[i] = l[i];
    for (int i = n; i < 16; i++)
        f[i] = 0;
}

static void plnFilterRemove(aiPLANNER *pl, int i)
{
    plgsACTION **f = (plgsACTION **)((u8 *)pl + 0x39C);
    int &n = WR_FIELD(int, pl, 0x398);
    n--;
    f[i] = f[n];
    f[n] = 0;
}

// v12 0x0055AF10  (feb 1.2 NEW)
// Out with the players the NPC's cell cannot see (gsSysVis).
void aiPLANNER::RemoveHiddenFromFiltered(aiNPC *npc)
{
    m3dV o, p;
    npc->inst->GetPos(&o);
    plgsACTION **f = (plgsACTION **)((u8 *)this + 0x39C);
    for (int i = 0; i < WR_FIELD(int, this, 0x398) && f[i]; i++) {
        wrEVCall<void>(f[i], 0x64, &p);
        if (!wrEVCall<BOOL32>(gsSysVis, 0x8C, &o, &p))
            plnFilterRemove(this, i--);
    }
}

// v12 0x0055AFB0  (feb 1.2 NEW)
void aiPLANNER::FilterPlayersFor(aiNPC *npc)
{
    plgsACTION **f = (plgsACTION **)((u8 *)this + 0x39C);
    for (int i = 0; i < WR_FIELD(int, this, 0x398) && f[i]; i++)
        if (!npc->Slot66(f[i]))
            plnFilterRemove(this, i--);
}

// v12 0x0055B040  (feb 0x00540DF0)
void aiPLANNER::SortFilteredPlayersByDist(aiNPC *npc)
{
    float d[16];
    plgsACTION **f = (plgsACTION **)((u8 *)this + 0x39C);
    int n = WR_FIELD(int, this, 0x398);
    m3dV o, p;
    npc->inst->GetPos(&o);
    for (int i = 0; i < n; i++) {
        wrEVCall<void>(f[i], 0x64, &p);
        float x = o.x - p.x, y = o.y - p.y, z = o.z - p.z;
        d[i] = m3dSqrt(x * x + y * y + z * z);
    }
    for (int k = 1; k < n; k++)
        for (int i = 0; i < k; i++)
            if (d[i] > d[i + 1]) {
                float t = d[i];
                d[i] = d[i + 1];
                d[i + 1] = t;
                plgsACTION *a = f[i];
                f[i] = f[i + 1];
                f[i + 1] = a;
            }
}

// v12 0x0055B1D0  (feb 0x00540F80)
BOOL aiPLANNER::HaveAChoiceFromFiltered(entENTITY **only)
{
    if (WR_FIELD(int, this, 0x398) >= 2)
        return 1;
    *only = WR_FIELD(entENTITY *, this, 0x39C);
    return 0;
}

// v12 0x0055B200  (feb 0x00540FB0)
plgsACTION *aiPLANNER::GetFilteredByIdx(int i)
{
    int n = WR_FIELD(int, this, 0x398) - 1;
    if (i > n)
        i = n;
    if (i < 0)
        i = 0;
    return WR_FIELD(plgsACTION *, this, 0x39C + i * 4);
}
