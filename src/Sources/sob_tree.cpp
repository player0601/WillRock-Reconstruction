// sob_tree.cpp -- trees (Will Rock 1.2, v12.exe; feb obj sob_tree.obj)
//
// The "leaves_root..." objects' children are leaves turning to the viewer (billboards, about Y when the root's name has
// "&v"). The instance has its own collision grid.

#include "game.h"

// v12 0x0048ADC0  (feb 0x00480060)
int __fastcall wrsobInitTree()
{
    return entRegisterClass((char *)"sob_tree", 0x54524545, 0x28, wrsobTREE::Create, 0, 1) != 0;
}

// v12 0x0048ADF0  (feb 0x00480090)
void wrsobTREE::InitLeaves(objOBJ *o, int vertical)
{
    inst->Validate(4);
    objOBJ *first = o->child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1) {
        if (!strncmp(c->name, "leaves_root", strlen("leaves_root"))) {
            InitLeaves(c, strstr(c->name, "&v") != 0);
            continue;
        }
        objMOD_BILLBOARD *m = new objMOD_BILLBOARD;
        m->flags |= 0x4000;
        m->AddObj(c);
        if (vertical)
            m->flags |= 0x2000;
        c->SetStateProcYes(0x40000, 0);
        c->state |= 0x20;
        c->state |= 4;
    }
}

// v12 0x0048AF20  (feb 0x004801C0)
void wrsobTREE::InitLeavesRoot(objOBJ *o)
{
    if (!o)
        return;
    if (o->name && !strncmp(o->name, "leaves_root", strlen("leaves_root"))) {
        InitLeaves(o, strstr(o->name, "&v") != 0);
        return;
    }
    objOBJ *first = o->child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        InitLeavesRoot(c);
}

// v12 0x0048AFD0  (feb 0x00480270)
BOOL32 wrsobTREE::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 1) {
        InitLeavesRoot(inst->root);
        m3dV vox = { 1.0f, 1.0f, 1.0f };
        int maxVox[3] = { 10, 10, 10 };
        inst->CDTInit(&vox, (struct m3dVINT *)maxVox, 0, 0);
    }
    return 0;
}

// v12 0x0048B060  (feb 0x00480300)
entENTITY *__fastcall wrsobTREE::Create(animINST *i)
{
    void *mem = operator new(0x16B);
    if (!mem)
        return 0;
    return new (mem) wrsobTREE;
}
