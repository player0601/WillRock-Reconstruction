// m3d_ctrl.h -- m3dCTRL_* time controllers (m3d_ctrl.obj). Ctors / SetValue / Enable are inline (header).
#pragma once
#include "m3d_lib.h"

// flags at +0x04
enum m3dCTRL_FLAGS {
    M3D_CTRL_RUNNING     = 1,   // Update advances t
    M3D_CTRL_STOP_AT_END = 2,   // Enable(0) when t passes length
    M3D_CTRL_CLAMP       = 4    // do not wrap t (no loop)
};

// vtable feb 0x005CE674 (m3dCTRL_TIME): 0 dtor, 1 Update, 2 Enable
class m3dCTRL_TIME_ {
public:
    virtual void *__vecDelDtor(unsigned f) { if (f & 1) operator delete(this); return this; } // slot +0x00 (feb 0x00469EC0 shared)
    virtual void  Update(float dt);                            // slot +0x04 v12 0x005691C0
    virtual void  Enable(int on)                               // slot +0x08 inline (feb 0x00403470)
    { if (on) flags |= M3D_CTRL_RUNNING; else flags &= ~M3D_CTRL_RUNNING; }
    unsigned flags;     // +0x04
    float    t;         // +0x08 current time
    float    length;    // +0x0C period (negative = reversed for the SPL controllers)
    float    speed;     // +0x10 time scale
    int      _14;       // +0x14 ?
};

class m3dCTRL_1D : public m3dCTRL_TIME_ {
public:
    virtual void  GetValue(float *out) = 0;                    // slot +0x0C
    virtual void  SetValue(float *v) { value = *v; }           // slot +0x10 inline (feb 0x00403490)
    virtual float GetMaxValue() = 0;                           // slot +0x14
    float value;        // +0x18
};
class m3dCTRL_1D_SPL : public m3dCTRL_1D {                     // vtable feb 0x005CE65C
public:
    void  GetValue(float *out);                                // v12 0x00569220
    float GetMaxValue();                                       // v12 0x00569330
    m3dSPL *spl;        // +0x1C
};
class m3dCTRL_1D_NOISE : public m3dCTRL_1D {                   // vtable feb 0x005DBB30
public:
    void  GetValue(float *out);                                // v12 0x005694B0
    float GetMaxValue();                                       // v12 0x00569510
    void *noise;        // +0x1C m3dNOISE*
    float vMin;         // +0x20
    float vMax;         // +0x24
    float freq;         // +0x28
};

// m3dCTRL_3D: a point (animCTRL_3D_FOLLOW: an instance's / object's position, kind +0x14 = 2)
class m3dCTRL_3D : public m3dCTRL_TIME_ {
public:
    virtual void GetValue(m3dV *out) = 0;                      // slot +0x0C
    virtual void SetValue(m3dV *v) { value = *v; }             // slot +0x10
    m3dV value;         // +0x18
};
// m3dCTRL_TIME ctor (feb 0x00403440, inline copies): running, t 0, length 1, speed 1
inline void m3dCtrlTimeInit(m3dCTRL_TIME_ *c) { c->flags = 0; c->t = 0; c->length = 1.0f; c->speed = 1.0f; c->_14 = 0; c->flags |= M3D_CTRL_RUNNING; }

class m3dCTRL_COLOR : public m3dCTRL_TIME_ {
public:
    virtual void GetValue(m3dCOLOR *out) = 0;                  // slot +0x0C
    virtual void SetValue(m3dCOLOR *c) { value = *c; }         // slot +0x10 inline (feb 0x00410760)
    m3dCOLOR value;     // +0x18
};
class m3dCTRL_COLOR_SPL : public m3dCTRL_COLOR {               // vtable feb 0x005CFB40
public:
    void GetValue(m3dCOLOR *out);                              // v12 0x005693A0
    m3dSPL *spl;        // +0x28 (m3dSPL_COLOR)
};
class m3dCTRL_COLOR_NOISE : public m3dCTRL_COLOR {             // vtable feb 0x005D6820
public:
    void GetValue(m3dCOLOR *out);                              // v12 0x00569520
    void    *noise;     // +0x28
    float    vMin;      // +0x2C
    float    vMax;      // +0x30
    float    freq;      // +0x34
    m3dCOLOR color;     // +0x38 rgb scaled by the noise value, alpha copied
};
