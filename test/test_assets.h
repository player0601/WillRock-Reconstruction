// test_assets.h -- the box test's asset side (test_assets.cpp)
#pragma once
class animTPL; class objOBJ; class txmTEXTURE;

extern int tstHaveDevice;
int      tstTexInit(const char *gameDir);               // pctInit + the texture manager (textures under <game>\textures)
animTPL *tstLoadTemplate(const char *path);             // sioReadTemplate + txmManager->LoadAll
objOBJ  *tstFindObj(objOBJ *root, const char *name);
void     tstDumpObj(objOBJ *o, int depth, int maxDepth);
void    *tstTexSurface(txmTEXTURE *t);                  // the D3D surface of a loaded texture (0 if none)
class scnSCENE;
scnSCENE *tstLoadScene(const char *path);              // sioReadScene + txmManager->LoadAll
void tstPoseTemplate(animTPL *t);                      // PLACEHOLDER: animated nodes at their start pose (no playback yet)
void      tstDumpScene(scnSCENE *s, int depth);
int  tstCheckLevel(const char *path, const char *objs); // test_check.cpp: --checklevel (mesh / material sanity)
int tstMsgTest();                                     // (test) --msgtest: the rebuilt message system (test_msg.cpp)
int tstSoundTest(const char *gameDir);                 // (test) --sndtest: the rebuilt DirectSound driver and buffers
