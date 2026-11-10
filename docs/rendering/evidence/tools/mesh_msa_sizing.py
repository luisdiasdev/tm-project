import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
msa = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msa')])
bad_range = 0; bad_face = 0; bad_ib = 0; nonterm = 0; maxnamelen = 0
namelen = collections.Counter()
att_total = 0
multi_example = None
for f in msa:
    b = open(os.path.join(MESH,f),'rb').read()
    fvf, sv, att = struct.unpack_from('<3I', b, 0)
    off = 12
    ranges = [struct.unpack_from('<5I', b, off+20*i) for i in range(att)]
    off += 20*att
    names = []
    for i in range(att):
        raw = b[off:off+11]; off += 11
        s = raw.split(b'\x00')[0]
        namelen[len(s)] += 1; maxnamelen = max(maxnamelen, len(s))
        if len(s) == 11: nonterm += 1
        names.append(s.decode('latin1'))
    ibsize = struct.unpack_from('<I', b, off)[0]; off += 4
    ib = b[off:off+ibsize]; off += ibsize
    vbsize = struct.unpack_from('<I', b, off)[0]; off += 4
    off += vbsize
    att_total += att
    # consistency
    if max(r[4] for r in ranges) > vbsize//sv: bad_range += 1
    if sum(r[2] for r in ranges)*3 != ibsize//2: bad_face += 1
    idx = struct.unpack_from('<%dH' % (len(ib)//2), ib, 0)
    if max(idx) >= vbsize//sv: bad_ib += 1
    if att >= 4 and multi_example is None:
        multi_example = (f, fvf, sv, att, vbsize, ibsize, ranges, names)
print("files:", len(msa), " total attribute ranges:", att_total)
print("files where max(VertexStart+VertexCount) > vertexCount:", bad_range)
print("files where sum(FaceCount)*3 != indexCount:", bad_face)
print("files where max index >= vertexCount:", bad_ib)
print("texture names exactly 11 bytes (no NUL):", nonterm, " max name length:", maxnamelen)
print("name length histogram:", dict(sorted(namelen.items())))
if multi_example:
    f, fvf, sv, att, vb, ib, ranges, names = multi_example
    print("\nexample multi-submesh file:", f, "FVF=0x%X sizeVertex=%d att=%d vbSize=%d ibSize=%d" % (fvf, sv, att, vb, ib))
    for i,(aid,fs,fc,vs,vc) in enumerate(ranges):
        print("   [%d] attribId=%d FaceStart=%d FaceCount=%d VertexStart=%d VertexCount=%d  tex=%s" % (i,aid,fs,fc,vs,vc,names[i]))
# appended: full per-file table for .msa
import struct, os
MESH = r"E:\wyd\test-setup\client_748\mesh"
print("\n--- per-file size proof (.msa): file, size, FVF, sizeVertex, attribs, ibBytes, vbBytes, computed, ok")
ok = 0; n = 0
for f in sorted(os.listdir(MESH)):
    if not f.lower().endswith('.msa'): continue
    d = open(os.path.join(MESH,f),'rb').read()
    fvf, sv, att = struct.unpack_from('<3I', d, 0)
    off = 12 + 20*att + 11*att
    ib = struct.unpack_from('<I', d, off)[0]; off += 4 + ib
    vb = struct.unpack_from('<I', d, off)[0]; off += 4 + vb
    print("%-16s %8d fvf=0x%03X sv=%2d att=%2d ib=%6d vb=%7d calc=%8d %s" %
          (f, len(d), fvf, sv, att, ib, vb, off, "OK" if off == len(d) else "MISMATCH"))
    ok += (off == len(d)); n += 1
print("files with exact size match: %d / %d" % (ok, n))
