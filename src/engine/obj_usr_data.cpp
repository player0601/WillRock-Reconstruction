// obj_usr_data.cpp -- engine.lib per-object user data: up to 5 registered kinds (FourCC), each object keeps an
// array of objUSR_DATA* indexed by kind (Will Rock 1.2, v12.exe; feb obj engine:obj_usr_data.obj)
//
// v12 0x004FFB30..0x004FFCDF, every function an exact twin of the feb code. objUSR_DATA vtable: slot 0 the
// deleting dtor, slot +0x08 Duplicate(mode) (returns the copy, or 0 for none).

#include "engine.h"
#include "../wr_vcall.h"

int objNUsrDataSlots;                       // v12 0x00984DFC (feb 0x00940BBC)
static u32 objUsrDataKind[5];               // v12 0x00984E04 (feb 0x00940BC4)

// v12 0x004FFB30  (feb 0x004ED550)
// The slot of kind `fourcc`, taking a free one the first time.
int objOBJ::RegisterUsrData(u32 fourcc)
{
    int free = -1;
    for (int i = 0; i < 5; i++) {
        if (objUsrDataKind[i] == fourcc)
            return i;
        if (!objUsrDataKind[i] && free == -1)
            free = i;
    }
    objUsrDataKind[free] = fourcc;
    int n = free + 1;
    if (n < objNUsrDataSlots)
        n = objNUsrDataSlots;
    objNUsrDataSlots = n;
    return free;
}

// v12 0x004FFB80  (feb 0x004ED5A0)
void objOBJ::UnregisterUsrData(u32 fourcc)
{
    for (int i = 0; i < 5; i++)
        if (objUsrDataKind[i] == fourcc)
            objUsrDataKind[i] = 0;
}

// v12 0x004FFBA0  (feb 0x004ED5C0)
BOOL32 objOBJ::AddUsrData(int slot, objUSR_DATA *d)
{
    if (!usrData) {
        usrData = (objUSR_DATA_P *)apCalloc(4, objNUsrDataSlots);
        if (!usrData)
            return 0;
    }
    usrData[slot] = d;
    return 1;
}

// v12 0x004FFBF0  (feb 0x004ED610)
// Copies of this object's user data (those whose Duplicate returns one) onto `to`.
BOOL32 objOBJ::DuplicateUsrData(objOBJ *to, int mode)
{
    if (!usrData)
        return 1;
    for (int i = 0; i < objNUsrDataSlots; i++) {
        objUSR_DATA *d = usrData[i];
        if (!d)
            continue;
        objUSR_DATA *c = wrVCall<objUSR_DATA *>(d, 8, mode);
        if (!c)
            continue;
        if (!to->usrData) {
            to->usrData = (objUSR_DATA_P *)apCalloc(4, objNUsrDataSlots);
            if (!to->usrData)
                return 0;
        }
        to->usrData[i] = c;
    }
    return 1;
}

// v12 0x004FFC80  (feb 0x004ED6A0)
void objOBJ::DestroyUsrData()
{
    if (!usrData)
        return;
    for (int i = 0; i < objNUsrDataSlots; i++)
        if (usrData[i])
            wrVCall<void *>(usrData[i], 0, 1u);
    apFree(usrData);
    usrData = 0;
}
