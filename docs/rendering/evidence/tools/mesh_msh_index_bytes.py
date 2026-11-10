import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
def fl(b,o): return struct.unpack_from('<f', b, o)[0]
msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
stats = collections.Counter(); tot = collections.Counter()
examples = []
for f in msh:
    d = open(os.path.join(MESH,f),'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    if infl < 2 or pal < 2 or vc < 1: continue
    off = 32 + pal*64 + pal*4
    nw = infl-1
    ok = [0,0,0,0]
    for v in range(vc):
        base = off + v*sv
        packed = struct.unpack_from('<I', d, base+12+4*nw)[0]
        for k in range(4):
            if ((packed >> (8*k)) & 0xFF) < pal: ok[k] += 1
    key = infl
    tot[key] += vc
    for k in range(4): stats[(key,k)] += ok[k]
    if len(examples) < 6:
        v0 = struct.unpack_from('<I', d, 32 + pal*64 + pal*4 + 12 + 4*nw)[0]
        examples.append((f, infl, pal, hex(v0), [ (v0>>(8*k))&0xFF for k in range(4)]))
print("per influence-count: fraction of vertices whose byte<numPalette, by byte position")
for key in sorted(tot):
    print("  infl=%d  n=%d  " % (key, tot[key]) + "  ".join("byte%d=%.3f" % (k, stats[(key,k)]/tot[key]) for k in range(4)))
print("\nexamples (file, infl, numPalette, packed dword, bytes[0..3]):")
for e in examples: print("   ", e)
