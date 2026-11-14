// weapon.cpp -- wpnWEAPON, the base of every player and NPC weapon: ammo, fire rate, damage, the "tip" object
// that gives the shot's origin and direction (Will Rock 1.2, v12.exe; feb obj gs_shared:weapon.obj)
//
// Rebuilt from the 1.2 code (v12 0x00543CC0..0x005440F0, vtable 0x00608620); the feb decompile matches.

#include "../Sources/game.h"

extern float gsElapsedTime;                 // v12 0x0098E9F4 (feb 0x009482DC)

static const float wpnEps = 1e-6f;          // v12 0x0060861C
float wpnEpsSq;                             // v12 0x009A84C0 (static init v12 0x005440C0: wpnEps * wpnEps)
static struct wpnEPS_INIT { wpnEPS_INIT() { wpnEpsSq = wpnEps * wpnEps; } } wpnEpsInit;

// v12 0x00543CC0  (feb 0x0052E210)
wpnWEAPON::wpnWEAPON()
{
    damage = 10.0f;
    ammo = 10.0f;
    priority = 0x4B;
    maxAmmo = 100.0f;
    shootRate = 1.0f;
    cooldown = 0.0f;
    flags = WPNF_ENABLED;
}

// v12 0x00543D50 (the dtor; 0x00543D20 is the deleting one)
wpnWEAPON::~wpnWEAPON()
{
}

// v12 0x00543D60  (feb 0x0052E2B0)
void wpnWEAPON::SetAmmo(float a)
{
    ammo = a;
}

// v12 0x00543D70  (feb 0x0052E2C0)
// Up to maxAmmo; 0 when already full. Weapons with infinite ammo take anything.
int wpnWEAPON::AddAmmo(float a)
{
    if (flags & WPNF_INFINITE_AMMO)
        return 1;
    if (!(ammo < maxAmmo))
        return 0;
    float room = maxAmmo - ammo;
    ammo = (room < a ? room : a) + ammo;
    return 1;
}

// v12 0x00543DD0  (feb 0x0052E320)
// NB (original): only a negative ammo count refuses; at 0 the shot is taken and the count stays 0.
int wpnWEAPON::SubtractAmmo(float a)
{
    if (flags & WPNF_INFINITE_AMMO)
        return 1;
    if (ammo < 0.0f)
        return 0;
    ammo = ammo - a;
    if (ammo < 0.0f)
        ammo = 0.0f;
    return 1;
}

// v12 0x00543E20  (feb 0x0052E370)
// Aim at a point (if given), then shoot if ready and loaded: starts the cooldown of 1 / shootRate.
int wpnWEAPON::Shoot(entENTITY *target, m3dV *aim)
{
    if (aim) {
        shootDir.x = aim->x - shootPos.x;
        shootDir.y = aim->y - shootPos.y;
        shootDir.z = aim->z - shootPos.z;
        m3dNormalize(&shootDir);
    }
    if (!IsReadyShoot())
        return 0;
    if (!(ammo > 0.0f) && !(flags & WPNF_INFINITE_AMMO))
        return 0;
    if (!(shootRate >= wpnEps))
        cooldown = 0.0f;
    else
        cooldown = 1.0f / shootRate;
    return 1;
}

void wpnWEAPON::_slot6C() {}                // v12 0x00403A20
void wpnWEAPON::_slot70() {}                // v12 0x00403A20

// v12 0x00543ED0  (feb 0x0052E420)
// Fill the weapon part of a damage message: amount, hit point, weapon index, and the muzzle (the tip's origin).
void wpnWEAPON::MakeDamage(dmgWEAPON *d, m3dV *hit)
{
    d->amount = GetDamage();
    d->weaponType = wpnIndex;
    d->hitPos = *hit;
    inst->Validate(4);
    tip->GetOrigin(&d->origin);
}

// v12 0x00403AD0  (feb 0x004037E0)
int wpnWEAPON::IsReadyShoot()
{
    return flags & 2;
}

// v12 0x00544090  (feb 0x0052E5E0)
void wpnWEAPON::UpdateShootPosDir()
{
    tip->GetOrigin(&shootPos);
    tip->GetAxisZ(&shootDir);
}

// v12 0x00543F30  (feb 0x0052E480)
int wpnWEAPON::ProcessINIT()
{
    cooldown = 0.0f;
    tip = objFindName(inst->root, (char *)"tip");
    return 1;
}

void wpnWEAPON::ProcessTERM() {}            // v12 0x00401380

// v12 0x00543F60  (feb 0x0052E4B0)
// The cooldown runs down; once below zero the weapon is ready (flag 2). Then the muzzle follows the tip.
void wpnWEAPON::ProcessFRAME()
{
    cooldown = cooldown - gsElapsedTime;
    if (cooldown < 0.0f) {
        cooldown = 0.0f;
        flags |= 2;
    } else
        flags &= ~2u;
    UpdateShootPosDir();
}

int  wpnWEAPON::ProcessINIT_LEVEL() { return 1; }    // v12 0x004031F0
void wpnWEAPON::ProcessTERM_LEVEL() {}               // v12 0x00403A20
void wpnWEAPON::ProcessDESTROY_ADDR(void *) {}       // v12 0x00403950
void wpnWEAPON::ProcessSHOOT(void *) {}              // v12 0x00403950
void wpnWEAPON::ProcessMP_NOTIFY(void *) {}          // v12 0x00403950

// v12 0x00543FA0  (feb 0x0052E500)
// The messages a weapon handles, each to its own slot; always 0 (the chain goes on).
BOOL32 wpnWEAPON::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_INIT:         ProcessINIT(); break;
    case MSG_MSG_TERM:         ProcessTERM(); break;
    case MSG_MSG_DESTROY_ADDR: ProcessDESTROY_ADDR(data); break;
    case 1000:                 ProcessFRAME(); break;               // GS_MSG_FRAME
    case 0x3F4:                ProcessINIT_LEVEL(); break;
    case 0x3F5:                ProcessTERM_LEVEL(); break;
    case 0x40E:                ProcessMP_NOTIFY(data); break;
    case 0x4C7:                ProcessSHOOT(data); break;           // PWP_MSG_SHOOT
    }
    return 0;
}

// v12 0x00544050  (feb 0x0052E5A0)
void wpnWEAPON::GetOrgDirTip(m3dV *org, m3dV *dir)
{
    inst->Validate(4);
    if (org)
        tip->GetOrigin(org);
    if (dir)
        tip->GetAxisZ(dir);
}

// v12 0x005440E0  (feb 0x0052E630)
void wpnWEAPON::SetShootRate(float r)
{
    shootRate = r;
}

// v12 0x005440F0  (feb 0x0052E640) HOOK: global damage scale
float wpnWEAPON::GetDamage()
{
    return damage;
}

// v12 0x00403AE0  (feb 0x004037F0)
void wpnWEAPON::Enable(int on)
{
    if (on)
        flags |= WPNF_ENABLED;
    else
        flags &= ~WPNF_ENABLED;
}
