#!/bin/sh
# compile one reconstructed source file to a real 32-bit Windows object
#   check.sh file.cpp [out.o [extra flags]]
R=$(cd "$(dirname "$0")/.." && pwd)
f="$1"; d=$(dirname "$f"); out="${2:-/dev/null}"; shift; [ $# -gt 0 ] && shift
i686-w64-mingw32-g++ -std=gnu++17 -c -fpermissive -w -fno-exceptions -fno-delete-null-pointer-checks -DWIN32_LEAN_AND_MEAN -DDIRECTINPUT_VERSION=0x0800 \
  -I"$R/include" -I"$R/include/core" -I"$R/src/gs" -I"$R/src/drv" -I"$R/src/Sources" -I"$d" "$@" "$f" -o "$out"
