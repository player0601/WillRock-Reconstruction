#!/bin/sh
# The shareable build of the box test: every source at -O2 (no debug info), the Will Rock icon, stripped.
#   sh tools/build_release.sh   -> build/rel/WillRock_Decomp.exe
set -e
R=$(cd "$(dirname "$0")/.." && pwd); B=$R/build/rel; mkdir -p $B/obj
rm -f $B/obj/*.o
CXX=i686-w64-mingw32-g++; CC=i686-w64-mingw32-gcc
FLAGS="-O2 -fno-strict-aliasing -DWR_NATIVE"
ok=0; fail=0
for f in $R/src/*/*.cpp; do
  o=$B/obj/$(basename $(dirname $f))_$(basename ${f%.cpp}).o
  if sh $R/tools/check.sh $f $o $FLAGS 2>$B/obj/$(basename ${f%.cpp}).err; then ok=$((ok+1)); else fail=$((fail+1)); rm -f $o; echo "compile FAIL $f"; fi
done
for f in $R/test/*.cpp; do
  sh $R/tools/check.sh $f $B/obj/test_$(basename ${f%.cpp}).o $FLAGS -I$R/test || { echo "compile FAIL $f"; exit 1; }
done
$CC -c -O2 $R/test/stubhit.c -o $B/obj/test_stubhit.o
(cd $R/test && windres -F pe-i386 willrock.rc -O coff -o $B/obj/zz_icon.o)
echo "compiled $ok, failed $fail"
LIBS="-static -static-libgcc -static-libstdc++ -lddraw -ldinput -ldxguid -luuid -lole32 -lgdi32 -luser32 -lwinmm -lws2_32 -lcomdlg32 -lstrmiids -loleaut32 -ldsound"
: > $B/stubs.S
$CXX -mconsole -o $B/WillRock_Decomp.exe $B/obj/*.o -Wl,--allow-multiple-definition -Wl,--no-demangle $LIBS 2>$B/link1.err || true
grep -o "undefined reference to \`[^']*'" $B/link1.err | sed "s/undefined reference to \`//; s/'$//" | sort -u > $B/undef.txt || true
python3 $R/tools/gen_stubs.py $B/undef.txt $B/stubs.S $B/stubs.txt
$CC -c $B/stubs.S -o $B/obj/zz_stubs.o
$CXX -mconsole -s -o $B/WillRock_Decomp.exe $B/obj/*.o -Wl,--allow-multiple-definition $LIBS 2>$B/link2.err && echo "LINK OK: $B/WillRock_Decomp.exe ($(wc -l < $B/stubs.txt) stubs)" || { echo "LINK FAIL"; head -30 $B/link2.err; exit 1; }
