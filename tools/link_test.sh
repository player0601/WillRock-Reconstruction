#!/bin/sh
# relink build/test/box_test.exe from the existing objects (after build_quick / single recompiles)
set -e
R=$(cd "$(dirname "$0")/.." && pwd); B=$R/build/test
CXX=i686-w64-mingw32-g++; CC=i686-w64-mingw32-gcc
rm -f $B/obj/zz_stubs.o
LIBS="-static -static-libgcc -static-libstdc++ -lddraw -ldinput -ldxguid -luuid -lole32 -lgdi32 -luser32 -lwinmm -lws2_32 -lcomdlg32 -lstrmiids -loleaut32 -ldsound"
LINK="$CXX -mconsole -o $B/box_test.exe $B/obj/*.o -Wl,--allow-multiple-definition $LIBS"
# pass 1: collect the undefined symbols (raw names)
: > $B/stubs.S
$CXX -mconsole -o $B/box_test.exe $B/obj/*.o -Wl,--allow-multiple-definition -Wl,--no-demangle $LIBS 2>$B/link1.err || true
grep -o "undefined reference to \`[^']*'" $B/link1.err | sed "s/undefined reference to \`//; s/'$//" | sort -u > $B/undef.txt || true
python3 $R/tools/gen_stubs.py $B/undef.txt $B/stubs.S $B/stubs.txt
$CC -c $B/stubs.S -o $B/obj/zz_stubs.o
# pass 2
$CXX -mconsole -o $B/box_test.exe $B/obj/*.o -Wl,--allow-multiple-definition $LIBS 2>$B/link2.err && echo "LINK OK: $B/box_test.exe ($(wc -l < $B/stubs.txt) stubs)" || { echo "LINK FAIL"; head -30 $B/link2.err; exit 1; }
