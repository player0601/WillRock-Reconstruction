// gs_domain.cpp -- gs.lib domains (Will Rock 1.2, v12.exe; feb obj gs:gs_domain.obj)
//
// A domain list collects the level objects tagged "&dom_..." of its kind at level load (Collect -> IsAddObj ->
// MakeDomain) and every frame runs Enter / Leave / Update on its domains for the update points (the players).
// A domain is an active object and a scnDOMAIN volume (the tagged object's outline) with up to 0x13 sub-volumes:
// its child objects and the objects its sheet names ([DOMAIN] subDom1..n), shared through the list by name.

#include <stdio.h>
#include "../Sources/game.h"
#include "gs_mp_server.h"

// ---------------------------------------------------------------------------------------------- gsDOMAIN

// v12 0x00527A20  (feb 0x00512570)
gsDOMAIN::gsDOMAIN()
{
    domFlags = 0;
    nSub = 0;
    list = 0;
    classId = 0x24444F4D;                    // '$DOM'
    priority = 0x53;
}

// v12 0x00527AF0  (feb 0x00512640)  the bases' (scnDOMAIN, entIACTIVE_OBJ) only
gsDOMAIN::~gsDOMAIN()
{
}

// v12 0x005284A0  the scnDOMAIN side's deleting destructor (this - 0x98, then v12 0x00527AA0)
void *gsDOMAIN::__vecDelDtor(unsigned flags)
{
    if (flags & 1)
        delete this;
    else
        this->~gsDOMAIN();
    return this;
}

// v12 0x00527B30  (feb 0x00512680)
// Once: the outline, the IActions of the object's sheet, [DOMAIN] height (0: flat) and isExactIncl, then the
// sub-volumes: the object's children, then the objects named by [DOMAIN] subDom1, subDom2, ... (each made once
// by the list and shared).
BOOL32 gsDOMAIN::Init()
{
    if (domFlags & DOMF_INITED)
        return 1;
    if (!scnDOMAIN::Init())
        return 0;
    ParseIActions(obj->props);
    float height = 0.0f;
    int exact = 0;
    if (obj->props) {
        obj->props->GetFloat((char *)"DOMAIN", (char *)"height", &height);
        obj->props->GetBool((char *)"DOMAIN", (char *)"isExactIncl", &exact);
    }
    SetHeight(height);
    inside = exact;
    objOBJ *objs[0x14];
    int n = objMakePtrList(&obj, 1, objs, 0x14);
    for (int i = 1; i < n; i++) {
        scnDOMAIN *s = list->AddSubDomain(this, objs[i]);
        if (!s || nSub >= 0x13)
            break;
        sub[nSub++] = s;
    }
    if (obj->props) {
        char key[0x40], name[0x40];
        sprintf(key, "subDom%d", 1);
        int i = 1;
        while (obj->props->GetStr((char *)"DOMAIN", key, name, 0x40)) {
            scnDOMAIN *s = list->AddSubDomain(this, name);
            if (!s || nSub >= 0x13)
                break;
            sub[nSub++] = s;
            i++;
            sprintf(key, "subDom%d", i);
        }
    }
    domFlags |= DOMF_INITED;
    return 1;
}

// v12 0x00527D10  (feb 0x00512860)
void gsDOMAIN::Term()
{
    scnDOMAIN::Term();
    nSub = 0;
    list = 0;
}

// v12 0x00527D30  (feb 0x00512880)
int gsDOMAIN::AddSubDomain(scnDOMAIN *d)
{
    if (nSub >= 0x13)
        return 0;
    sub[nSub++] = d;
    return 1;
}

// v12 0x00527D60  (feb 0x005128B0)
BOOL32 gsDOMAIN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == MSG_MSG_INIT)
        Init();
    return 0;
}

// v12 0x00403950 / 0x00403A20 (shared empty functions)
void gsDOMAIN::Update(m3dV *pos)
{
}

void gsDOMAIN::Enter()
{
}

void gsDOMAIN::Leave()
{
}

// ---------------------------------------------------------------------------------------------- gsDOMAIN_LIST

// v12 0x00527D80  (feb 0x005128D0)
gsDOMAIN_LIST::gsDOMAIN_LIST()
{
    curDomain = -1;
    _9c = 0;
    nDomains = 0;
    nSub = 0;
    classId = 0x24444F4C;                    // '$DOL'
    priority = 0x52;
}

// v12 0x00527DE0  (feb 0x00512940)
int gsDOMAIN_LIST::ProcessINIT_LEVEL()
{
    for (int i = 0; i < nDomains; i++)
        if (!domain[i]->Init())
            apLogErr((char *)"Cannot initialize domain %s", domain[i]->obj->name);
    return 1;
}

// v12 0x00527E40  (feb 0x005129A0)
// Every domain ends and is destroyed (deleted at the end of the frame), then the sub-volumes.
void gsDOMAIN_LIST::ProcessTERM_LEVEL()
{
    for (int i = 0; i < nDomains; i++) {
        domain[i]->Term();
        msgSystem.DestroyAddr(domain[i]);
        domain[i] = 0;
    }
    nDomains = 0;
    for (int i = 0; i < nSub; i++) {
        sub[i]->Term();
        DeleteSubDomain(sub[i]);
        sub[i] = 0;
    }
    nSub = 0;
}

// v12 0x00527EE0  (feb 0x00512A40)
int gsDOMAIN_LIST::GetNUpdatePoint()
{
    return gsSysServer->GetNPlayer();
}

// v12 0x00527EF0  (feb 0x00512A50)
// Player i's position raised 1.6 (the eyes).
int gsDOMAIN_LIST::GetUpdatePoint(int i, m3dV *p)
{
    entENTITY *e = gsSysServer->GetPlayer(i);
    if (!e)
        return 0;
    e->inst->GetPos(p);
    p->y += 1.6f;
    return 1;
}

// v12 0x00527F30  (feb 0x00512A90)
// Each domain: a point inside that was not -> Enter, none inside that had one -> Leave; then Update with each point.
// The list itself: Enter when a point is in some domain and was not, Leave when none is and was.
void gsDOMAIN_LIST::Update()
{
    int n = GetNUpdatePoint();
    int any = 0;
    m3dV p;
    _9c &= ~4;
    for (int i = 0; i < nDomains; i++) {
        gsDOMAIN *d = domain[i];
        int k;
        for (k = 0; k < n; k++)
            if (GetUpdatePoint(k, &p) && d->IsBelongPoint(&p))
                break;
        if (k < n) {
            if (!(d->domFlags & DOMF_ACTIVE)) {
                d->domFlags |= DOMF_ACTIVE;
                d->Enter();
                _9c |= 4;
            }
            any = 1;
        } else if (d->domFlags & DOMF_ACTIVE) {
            d->domFlags &= ~DOMF_ACTIVE;
            d->Leave();
            _9c |= 4;
        }
        for (curDomain = 0; curDomain < n; curDomain++)
            if (GetUpdatePoint(curDomain, &p))
                d->Update(&p);
        curDomain = -1;
    }
    if (any) {
        if (!(_9c & 1))
            Enter();
        _9c = (_9c & ~2) | 1;
    } else {
        if (!(_9c & 2))
            Leave();
        _9c = (_9c & ~1) | 2;
    }
}

// v12 0x00403A20 (shared empty function)
void gsDOMAIN_LIST::Enter()
{
}

void gsDOMAIN_LIST::Leave()
{
}

// v12 0x005280F0  (feb 0x00512C50)
gsDOMAIN *gsDOMAIN_LIST::FindDomain(char *name)
{
    for (int i = 0; i < nDomains; i++)
        if (!_stricmp(domain[i]->obj->name, name))
            return domain[i];
    return 0;
}

// v12 0x00528150  (feb 0x00512CB0)
scnDOMAIN *gsDOMAIN_LIST::AddSubDomain(gsDOMAIN *d, char *name)
{
    objOBJ *o = objFindName(gsScenePtr->root, name);
    if (!o)
        return 0;
    return AddSubDomain(d, o);
}

// v12 0x00528180  (feb 0x00512CE0)
// The sub-volume of the object o: the one already made for an object of that name, else a new one with o's
// [DOMAIN] height (default: its own) and isExactIncl.
scnDOMAIN *gsDOMAIN_LIST::AddSubDomain(gsDOMAIN *d, objOBJ *o)
{
    for (int i = 0; i < nSub; i++)
        if (!_stricmp(sub[i]->obj->name, o->name))
            return sub[i];
    if (nSub >= 0x63)
        return 0;
    sub[nSub] = MakeSubDomain(d);
    if (!sub[nSub])
        return 0;
    sub[nSub]->obj = o;
    if (!sub[nSub]->Init())
        return 0;
    float height = sub[nSub]->yMax - sub[nSub]->yMin;
    int exact = 0;
    if (o->props) {
        o->props->GetFloat((char *)"DOMAIN", (char *)"height", &height);
        o->props->GetBool((char *)"DOMAIN", (char *)"isExactIncl", &exact);
    }
    sub[nSub]->inside = exact;
    sub[nSub]->SetHeight(height);
    nSub++;
    return sub[nSub - 1];
}

// v12 0x004ACF70
scnDOMAIN *gsDOMAIN_LIST::MakeSubDomain(gsDOMAIN *d)
{
    return new scnDOMAIN;
}

// v12 0x004ACFB0
void gsDOMAIN_LIST::DeleteSubDomain(scnDOMAIN *s)
{
    if (s)
        s->__vecDelDtor(1);
}

// v12 0x005282E0  (name ours)
int gsDOMAIN_LIST::FindDomainIndex(gsDOMAIN *d)
{
    for (int i = 0; i < nDomains; i++)
        if (domain[i] == d)
            return i;
    return -1;
}

// v12 0x00528310  (feb 0x00512E70)
// o and its descendants: each one of this kind (IsAddObj) becomes a domain, named as the object and registered
// with the message system (at most 100; past that nothing more is collected).
int gsDOMAIN_LIST::Collect(objOBJ *o)
{
    if (o->name && IsAddObj(o)) {
        if (nDomains >= 100)
            return 1;
        domain[nDomains] = MakeDomain();
        if (!domain[nDomains])
            return 0;
        domain[nDomains]->obj = o;
        domain[nDomains]->list = this;
        domain[nDomains]->name.SetName(o->name);
        msgSystem.InsertAddr(domain[nDomains], 0, 0);
        nDomains++;
    }
    objOBJ *first = o->child;
    int started = 0;
    for (objOBJ *c = first; c && !(started && c == first); c = c->next) {
        if (!Collect(c))
            return 0;
        started = 1;
    }
    return 1;
}

// v12 0x00528420  (feb 0x00512F80)
void gsDOMAIN_LIST::ProcessLEVEL_LOADED()
{
    if (gsScenePtr->root)
        Collect(gsScenePtr->root);
}

// v12 0x00528440  (feb 0x00512FA0)
BOOL32 gsDOMAIN_LIST::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_TERM:
        return 1;
    case GS_MSG_FRAME:
        Update();
        break;
    case GS_MSG_INIT_LEVEL:
        ProcessINIT_LEVEL();
        break;
    case GS_MSG_TERM_LEVEL:
        ProcessTERM_LEVEL();
        break;
    case GS_MSG_LEVEL_LOADED:
        ProcessLEVEL_LOADED();
        break;
    }
    return 0;
}

static_assert(sizeof(gsDOMAIN) == 0x130, "gsDOMAIN size (v12 0x130)");
static_assert(sizeof(gsDOMAIN_LIST) == 0x3C8, "gsDOMAIN_LIST size (v12 0x3C8)");
