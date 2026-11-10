import os, struct, sys, collections

MESH = r"E:\wyd\test-setup\client_748\mesh"
ROOT = r"E:\wyd\test-setup\client_748"

def read_msh(path):
    d = open(path,'rb').read()
    if len(d) < 32: return None
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    off = 32
    bones = pal*64
    names = pal*4
    verts = vc*sv
    idx = fc*2
    total = 32 + bones + names + verts + idx
    return dict(file=path, size=len(d), parent=parent, id=mid, fvf=fvf, sv=sv,
                infl=infl, pal=pal, vc=vc, fc=fc, bones=bones, names=names,
                verts=verts, idx=idx, calc=total, data=d)

def read_msa(path):
    d = open(path,'rb').read()
    if len(d) < 12: return None
    fvf, sv, att = struct.unpack_from('<3I', d, 0)
    off = 12
    ranges = []
    for i in range(att):
        ranges.append(struct.unpack_from('<5I', d, off)); off += 20
    texnames = []
    for i in range(att):
        texnames.append(d[off:off+11]); off += 11
    ibsize = struct.unpack_from('<I', d, off)[0]; off += 4
    off += ibsize
    vbsize = struct.unpack_from('<I', d, off)[0]; off += 4
    off += vbsize
    return dict(file=path, size=len(d), fvf=fvf, sv=sv, att=att, ranges=ranges,
                tex=texnames, ibsize=ibsize, vbsize=vbsize, calc=off, data=d)

def fmt_fvf(f):
    parts = []
    base = f & 0xF
    names = {0x2:'XYZ',0x4:'XYZRHW',0x6:'XYZB1',0x8:'XYZB2',0xA:'XYZB3',0xC:'XYZB4',0xE:'XYZB5'}
    parts.append(names.get(base, hex(base)))
    if f & 0x10: parts.append('NORMAL')
    if f & 0x20: parts.append('PSIZE')
    if f & 0x40: parts.append('DIFFUSE')
    if f & 0x80: parts.append('SPECULAR')
    ntex = (f >> 8) & 0xF
    parts.append('TEX%d' % ntex)
    for i in range(ntex):
        sz = (f >> (16 + 2*i)) & 0x3
        parts.append('TEXCOORDSIZE%d=%d' % (i, sz+1))
    return '|'.join(parts)

msh = sorted([os.path.join(MESH,f) for f in os.listdir(MESH) if f.lower().endswith('.msh')])
msa = sorted([os.path.join(MESH,f) for f in os.listdir(MESH) if f.lower().endswith('.msa')])

bad = []
fvf_count = collections.Counter()
sv_by_fvf = collections.defaultdict(collections.Counter)
infl_count = collections.Counter()
msh_rows = []
for p in msh:
    r = read_msh(p)
    if r is None: continue
    fvf_count[r['fvf']] += 1
    sv_by_fvf[r['fvf']][r['sv']] += 1
    infl_count[(r['infl'], r['sv'], r['fvf'])] += 1
    msh_rows.append(r)
    if r['calc'] != r['size']:
        bad.append((os.path.basename(p), r['size'], r['calc']))

print("MSH files:", len(msh), " parsed:", len(msh_rows))
print("MSH size mismatches:", len(bad))
for b in bad[:20]: print("   MISMATCH", b)
print("\nMSH FVF histogram (value, count, decoded):")
for f,c in sorted(fvf_count.items()):
    print("   0x%03X %5d  %-30s sizeVertex=%s" % (f, c, fmt_fvf(f), dict(sv_by_fvf[f])))
print("\nMSH (influences, sizeVertex, fvf) histogram:")
for k,c in sorted(infl_count.items()): print("   ", k, c)

badm = []
fvf2 = collections.Counter(); sv2 = collections.defaultdict(collections.Counter)
attc = collections.Counter()
texset = collections.Counter()
for p in msa:
    r = read_msa(p)
    if r is None: continue
    fvf2[r['fvf']] += 1; sv2[r['fvf']][r['sv']] += 1; attc[r['att']] += 1
    for t in r['tex']: texset[t] += 1
    if r['calc'] != r['size']: badm.append((os.path.basename(p), r['size'], r['calc'], r['att'], r['fvf'], r['sv'], r['ibsize'], r['vbsize']))
print("\nMSA files:", len(msa), "size mismatches:", len(badm))
for b in badm[:15]: print("   MISMATCH", b)
print("\nMSA FVF histogram:")
for f,c in sorted(fvf2.items()):
    print("   0x%03X %5d  %-30s sizeVertex=%s" % (f, c, fmt_fvf(f), dict(sv2[f])))
print("\nMSA attrib-count histogram:", dict(attc))
print("\nMSA texture-name samples:", [t for t,_ in texset.most_common(12)])
# appended: full per-file table
import struct, os
MESH = r"E:\wyd\test-setup\client_748\mesh"
print("\n--- per-file size proof (.msh): file, size, numPalette, influences, verts, indices, FVF, sizeVertex, computed, ok")
ok = 0
for f in sorted(os.listdir(MESH)):
    if not f.lower().endswith('.msh'): continue
    d = open(os.path.join(MESH,f),'rb').read()
    p,i,fvf,sv,infl,pal,vc,fc = struct.unpack_from('<8I', d, 0)
    calc = 32 + 64*pal + 4*pal + vc*sv + fc*2
    print("%-20s %7d  pal=%2d infl=%d vc=%5d ic=%6d fvf=0x%03X sv=%2d calc=%7d %s" %
          (f, len(d), pal, infl, vc, fc, fvf, sv, calc, "OK" if calc == len(d) else "MISMATCH"))
    ok += (calc == len(d))
print("files with exact size match: %d / 657" % ok)
