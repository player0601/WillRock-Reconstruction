// wr_vcall.h -- slot-exact virtual calls shared by the reconstructed libraries
#pragma once
// Call vtable slot 'off' (bytes) of obj as the original does (__thiscall). Used where the SDK class does not
// declare the slot, or where GCC's vtable layout would differ from MSVC's; works on game objects too.
template <class R, class... A>
inline R wrVCall(const void *obj, int off, A... a)
{
    typedef R (__thiscall *FN)(const void *, A...);
    return (*(FN *)(*(const char *const *)obj + off))(obj, a...);
}

// Slot 'off' (the original's byte offset) of an object of a class derived from msgADDR: every entity, the player,
// the MP client and server. msgADDR has a virtual destructor: MSVC gives it one vtable slot (the deleting
// destructor), GCC's Itanium ABI two (complete and deleting), so in this build every later slot of these classes
// is 4 bytes further on. Slot 0 is not mapped (the destructor differs in kind, not just in place).
#if defined(__GNUC__)
#define WR_MSGADDR_VTBL_SHIFT 4
#else
#define WR_MSGADDR_VTBL_SHIFT 0
#endif
template <class R, class... A>
inline R wrEVCall(const void *obj, int off, A... a)
{
    return wrVCall<R>(obj, off + WR_MSGADDR_VTBL_SHIFT, a...);
}
