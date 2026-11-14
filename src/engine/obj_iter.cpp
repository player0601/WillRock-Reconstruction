// obj_iter.cpp -- engine.lib objOBJ_ITER_SAFE: a snapshot of an object's direct children, so a loop over them
// survives the children being re-linked (objOBJ::Duplicate uses it) (Will Rock 1.2, v12.exe; feb obj
// engine:obj_iter.obj, its only function)
//
// v12 0x00503A00, an exact twin of the feb code. At most 128 children are taken.

#include "engine.h"

// v12 0x00503A00  (feb 0x004EEFE0)
objOBJ_ITER_SAFE::objOBJ_ITER_SAFE(objOBJ *parent)
{
    n = 0;
    cur = 0;
    if (!parent || !parent->child)
        return;
    objOBJ *first = parent->child, *c = first;
    do {
        if (n > 0x7F)
            return;
        obj[n++] = c;
        c = c->next;
    } while (c != first);
}
