import os, collections, re
MESH = r"E:\wyd\test-setup\client_748\mesh"
ROOT = r"E:\wyd\test-setup\client_748"

def load_list(path):
    rows = []
    for ln in open(path, 'r', encoding='latin1'):
        ln = ln.rstrip('\r\n')
        if not ln.strip(): continue
        m = re.match(r'^\s*(\d+)\s+(\S+)\s*$', ln)
        if m: rows.append((int(m.group(1)), m.group(2)))
        else: rows.append((None, ln))
    return rows

for name in ('MeshList.txt','CommonMeshList.txt','MeshTextureList.txt'):
    p = os.path.join(MESH,name)
    rows = load_list(p)
    bad = [r for r in rows if r[0] is None]
    idxs = [r[0] for r in rows if r[0] is not None]
    print("== %s : %d lines, %d parse as 'idx path', %d others" % (name, len(rows), len(idxs), len(bad)))
    if idxs:
        print("   idx range %d..%d, unique=%d, duplicates=%s" % (min(idxs), max(idxs), len(set(idxs)),
              [k for k,v in collections.Counter(idxs).items() if v>1][:5]))
        gaps = sorted(set(range(min(idxs), max(idxs)+1)) - set(idxs))
        print("   gaps in range: %d (first 20: %s)" % (len(gaps), gaps[:20]))
    if bad: print("   non-index lines (first 3):", bad[:3])

rows = load_list(os.path.join(MESH,'MeshList.txt'))
rows = [r for r in rows if r[0] is not None]
ext = collections.Counter(os.path.splitext(r[1])[1].lower() for r in rows)
print("\nMeshList.txt extensions:", dict(ext))
dirs = collections.Counter(os.path.dirname(r[1]).lower() for r in rows)
print("MeshList.txt directories:", dict(dirs))
missing = []
for i,p in rows:
    q = os.path.join(ROOT, p.replace('\\\\','\\'))
    if not os.path.exists(q): missing.append((i,p))
print("MeshList.txt entries whose file is absent: %d ; first 10 %s" % (len(missing), missing[:10]))
listed = set(os.path.normcase(r[1].replace('\\\\','\\')) for r in rows)
on_disk = set(os.path.normcase('mesh\\'+f) for f in os.listdir(MESH) if f.lower().endswith('.msa'))
unlisted = sorted(on_disk - listed)
print(".msa files present but not listed in MeshList.txt: %d ; first 10 %s" % (len(unlisted), unlisted[:10]))
# CommonMeshList overlap
cm = [r for r in load_list(os.path.join(MESH,'CommonMeshList.txt')) if r[0] is not None]
print("\nCommonMeshList.txt idx range %d..%d, %d entries" % (min(r[0] for r in cm), max(r[0] for r in cm), len(cm)))
diff = [(i,p) for i,p in cm if (i, dict((a,b) for a,b in rows).get(i)) != (i,p)]
print("CommonMeshList entries that differ from MeshList at same index: %d ; first 5 %s" % (len(diff), diff[:5]))
