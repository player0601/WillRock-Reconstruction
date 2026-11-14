// test_ents.h -- (test) the entity host of the box test (test_ents.cpp). PLACEHOLDER: see the .cpp.
#pragma once
class scnSCENE; class entENTITY;

void tstLevelObjFlags(scnSCENE *s);                    // SetExtraObjFlags on the level's and templates' objects
void tstServerInit();                                  // (test) single-player gsSysServer / gsSysMP
void tstEntsStart(scnSCENE *s, entENTITY *player);     // doors / buttons of the level: create, INIT, INIT_LEVEL
void tstEntsFrame();                                   // FRAME to each, then their poses
void tstEntsLog(entENTITY *player);
void tstEntsToDomain(entENTITY *player);
void tstEntsToWater(entENTITY *player);               // (test) --watertest: into the first water volume
void tstWaterLog(entENTITY *player);                  // (test) the water volumes, modifiers, under-water test              // (test) --domtest: into the first sound domain                    // door states
extern const char *tstEntsGameDir;                    // the install folder (external templates)
class pwpWEAPON; class plrPLAYER;
pwpWEAPON *tstEntsWeapon(scnSCENE *s, plrPLAYER *p, const char *cls);  // a player weapon from its template
void tstLevelSystemInit(scnSCENE *s);   // the gsSysLevel stand-in (template lookup, load notifies)
void tstLevelSystemSetScene(scnSCENE *s); // its scene (the level now loading)
int  tstEntsCount();
void tstRendDrvEnsure();
int  tstPlayerMpInit();                                // (test) the "player_mp" class (plrInit's)                               // (test) rendDrv with the TermLevel slot (DestroyLevel)
class pteSHADER;
pteSHADER *tstEntVertShader(entENTITY *e, scnSCENE *s); // (test) an entity's vertex shader (the portal's picture)
int  tstPickupCount();                                 // (test) the placeholder pickups (items, vases)
void tstBreakVases();                                 // (test) --picktest: every vase hit once
int  tstPickupBox(int i, m3dV *centre);                // the i-th of the last tstPickupCount's list
struct gsLOAD_SCENE_PARAM;
int  tstTakeTeleport(gsLOAD_SCENE_PARAM *out);         // a GS_MSG_TELEPORT_LEVEL since the last call
void tstEntsToTpt(entENTITY *player);                  // (test) --tpttest: into the first teleport domain
entENTITY *tstEnt(int i);
void tstEntsToActDomain(entENTITY *player, const char *name); // (test) --cintest
void tstCinemaLog();                                           // (test) the cinematics' state
void tstPlayerJoin(entENTITY *player);   // the local player joins the local server (its MP id)
