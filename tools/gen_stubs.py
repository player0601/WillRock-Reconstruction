"""Generate stubs for symbols the native test build cannot resolve (not rebuilt yet).

    gen_stubs.py undef.txt stubs.S stubs.txt

Functions get a stub that calls wrStubHit("<demangled name>") (the exe stops and says which function it
needs); everything else (globals, tables) gets zeroed .bss storage. stubs.txt lists both with their kind.
"""
import subprocess, sys, shutil

undef, out_s, out_txt = sys.argv[1:4]
names = [l.strip() for l in open(undef) if l.strip()]
cf = shutil.which("i686-w64-mingw32-c++filt") or shutil.which("c++filt")

def demangle(n):
    if not cf:
        return n
    m = n[1:] if n.startswith("_") else n                 # i686: leading underscore
    try:
        r = subprocess.run([cf, "-n", m], capture_output=True, text=True).stdout.strip()
        return r or n
    except Exception:
        return n

lines = [".text"]
data = []
txt = []
for i, n in enumerate(names):
    if n.startswith("__imp_"):
        continue
    d = demangle(n)
    is_func = "(" in d or n.startswith("@") or ("@" in n and not n.startswith("__Z"))
    if d.startswith(("vtable for", "typeinfo", "VTT for", "guard variable")):
        is_func = False
    if is_func:
        lines += [".globl %s" % n, "%s:" % n, "    pushl $str%d" % i, "    call _wrStubHit", "    hlt"]
        data.append('str%d: .asciz "%s"' % (i, d.replace("\\", "\\\\").replace('"', "'")))
        txt.append("func  %s" % d)
    else:
        lines += [".comm %s, 4096, 16" % n]
        txt.append("data  %s" % d)
lines += [".section .rdata"] + data
open(out_s, "w").write("\n".join(lines) + "\n")
open(out_txt, "w").write("\n".join(txt) + "\n")
print("%d stubs (%d functions)" % (len(txt), sum(1 for t in txt if t.startswith("func"))))
