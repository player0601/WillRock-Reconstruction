// wr_cc.h -- calling-convention shims so the SDK parses with GCC/Clang (Will Rock unofficial SDK)
#pragma once
// ---- toolchain shims: let the headers parse with GCC/Clang (syntax checks, tooling, mingw mods) -------
#if !defined(_MSC_VER)
#  if defined(__i386__)
#    ifndef __fastcall
#      define __fastcall __attribute__((fastcall))
#    endif
#    ifndef __cdecl
#      define __cdecl    __attribute__((cdecl))
#    endif
#    ifndef __stdcall
#      define __stdcall  __attribute__((stdcall))
#    endif
#    ifndef __thiscall
#      define __thiscall __attribute__((thiscall))
#    endif
#  else          // 64-bit host: documentation / syntax check only, the layouts are NOT valid there
#    ifndef __fastcall
#      define __fastcall
#    endif
#    ifndef __cdecl
#      define __cdecl
#    endif
#    ifndef __stdcall
#      define __stdcall
#    endif
#    ifndef __thiscall
#      define __thiscall
#    endif
#  endif
#endif

