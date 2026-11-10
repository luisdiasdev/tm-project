import os, struct, collections, math

MESH = r"E:\wyd\test-setup\client_748\mesh"

def fl(b, o):  return struct.unpack_from('<f', b, o)[0]

msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
pal_hist = collections.Counter()
maxidx_over_pal = 0
index_oob = 0
weight_bad = 0
pos_min = [1e9]*3; pos_max=[-1e9]*3
weight_sum_bad = 0
first_vertex_examples = {}
bone_matrix_first_examples = {}
for f in msh:
    p = os.path.join(MESH, f)
    d = open(p,'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    pal_hist[(pal, infl)] += 1
    off = 32 + pal*64 + pal*4
    # vertices
    nw = infl-1          # number of float blend weights
    idx_off = 12 + 4*nw  # packed bone indices dword
    for v in range(vc):
        base = off + v*sv
        w = [fl(d, base+12+4*k) for k in range(nw)]
        packed = struct.unpack_from('<I', d, base+idx_off)[0]
        ids = [ (packed >> (8*k)) & 0xFF for k in range(4) ]
        used = ids[:infl] if infl<=4 else ids
        if used and max(used) >= pal:
            maxidx_over_pal += 1
        if nw:
            s = sum(w)
            if abs(s-1.0) > 0.02: weight_sum_bad += 1
        elif nw == 0:
            pass
        for k in range(3):
            c = fl(d, base+4*k)
            pos_min[k] = min(pos_min[k], c); pos_max[k]=max(pos_max[k], c)
    if f in ('ch010264.msh','ch010101.msh','or020101.msh','ch0101115.msh'):
        first_vertex_examples[f] = dict(parent=parent, id=mid, fvf=hex(fvf), sv=sv,
            infl=infl, pal=pal, vc=vc, fc=fc,
            v0=[round(fl(d, off+4*k),4) for k in range(3)],
            v0_tail=d[off+12:off+sv].hex(' '),
            v1=[round(fl(d, off+sv+4*k),4) for k in range(3)],
            m0=[round(fl(d, 32+4*k),5) for k in range(12)],
            names=[struct.unpack_from('<I', d, 32+pal*64+4*k)[0] for k in range(pal)][:8])
    # indices
    ib = 32 + pal*64 + pal*4 + vc*sv
    idx = struct.unpack_from('<%dH'%fc, d, ib)
    if max(idx) >= vc: index_oob += 1

print("numPalette/influence histogram:", dict(sorted(pal_hist.items())))
print("vertices whose blend index >= numPalette:", maxidx_over_pal)
print("files with index >= vertexCount:", index_oob)
print("vertices with |sum(weights)-1| > 0.02:", weight_sum_bad)
print("global position range:", [round(x,3) for x in pos_min], [round(x,3) for x in pos_max])
for k,v in first_vertex_examples.items():
    print("\n---", k)
    for kk,vv in v.items(): print("   ", kk, "=", vv)
