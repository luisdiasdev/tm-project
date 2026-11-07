#!/usr/bin/env python3
"""Audit WYD 748 UI scene binaries and control-id usage.

Run from the tmproject repo root:

    python docs/tools/ui_inventory.py --assets ../client_748
    python docs/tools/ui_inventory.py --assets ../client_748 --source-refs

The script is read-only. It uses the current UIBinary.h record sizes and the
current ResourceControl.h constants, so it also tells you when a binding no
longer exists in the target client.
"""

import argparse
import collections
import os
import re
import struct
import sys

CONTROL_SIZE = {1: 40, 2: 40, 3: 32, 6: 52, 10: 48,
                12: 52, 13: 184, 15: 28, 16: 40}
CONTROL_NAME = {1: "PANEL", 2: "BUTTON", 3: "CHECKBOX", 6: "LISTBOX",
                10: "PROGRESS", 12: "TEXT", 13: "EDIT", 15: "3DOBJ",
                16: "GRID"}

SCENES = [
    "SelServerScene2.bin",
    "SelCharScene2.bin",
    "FieldScene2.bin",
]

SOURCES = {
    "FieldScene2.bin": "Projects/TMProject/TMFieldScene.cpp",
    "SelCharScene2.bin": "Projects/TMProject/TMSelectCharScene.cpp",
    "SelServerScene2.bin": "Projects/TMProject/TMSelectServerScene.cpp",
}


def parse_header(path):
    text = open(path, encoding="utf-8", errors="ignore").read()
    out = {}
    for name, value in re.findall(r"constexpr auto (\w+) = (-?\d+);", text):
        out[name] = int(value)
    return out


def walk_scene(path):
    data = open(path, "rb").read()
    off = 0
    controls = []
    while off + 4 <= len(data):
        ctype = struct.unpack_from("<i", data, off)[0]
        start = off
        off += 4
        if ctype not in CONTROL_SIZE:
            raise ValueError(
                "bad control type %d at offset %d (file size %d)"
                % (ctype, start, len(data)))
        size = CONTROL_SIZE[ctype]
        if off + size > len(data):
            raise ValueError("truncated %s at offset %d"
                             % (CONTROL_NAME[ctype], start))
        cid, parent, tex = struct.unpack_from("<iii", data, off)
        controls.append({
            "type": CONTROL_NAME[ctype], "id": cid, "parent": parent,
            "tex": tex, "offset": start,
            "x": struct.unpack_from("<i", data, off + 12)[0],
            "y": struct.unpack_from("<i", data, off + 16)[0],
            "w": struct.unpack_from("<i", data, off + 20)[0],
            "h": struct.unpack_from("<i", data, off + 24)[0],
        })
        off += size
    if off != len(data):
        raise ValueError("consumed %d of %d bytes" % (off, len(data)))
    return controls


def parse_sets(path):
    lines = [l.strip() for l in open(path, encoding="latin-1") if l.strip()]
    sets = {}
    names = {}
    i = 0
    while i < len(lines):
        m = re.match(r"SetIndex:\s*(-?\d+)", lines[i])
        if not m:
            i += 1
            continue
        idx = int(m.group(1))
        count = 0
        if i + 1 < len(lines):
            mc = re.match(r"ItemCount:\s*(\d+)", lines[i + 1])
            if mc:
                count = int(mc.group(1))
        coords = []
        j = i + 2
        while j < len(lines) and re.match(r"^-?\d+,", lines[j]):
            coords.append(tuple(int(x) for x in lines[j].split(",")))
            j += 1
        if len(coords) != count:
            print("WARN: set %d says %d items, found %d"
                  % (idx, count, len(coords)), file=sys.stderr)
        sets[idx] = coords
        names[idx] = lines[i - 1].strip("[]") if i > 0 else "?"
        i = j
    return sets, names


def resolve_set(tex):
    if tex >= 0:
        return tex
    if tex < -2:
        return -tex
    return None


def resolve_token(raw, constants):
    """Map a FindControl()/comparison token to a control id.

    Handles numeric literals with C++ suffixes (``66432u``), decimals, and
    ``ResourceControl.h`` constant names.
    """
    token = raw
    if re.fullmatch(r"\d+[uUlL]*", raw):
        token = re.sub(r"[uUlL]+$", "", raw)
    if token.isdigit():
        return token, int(token)
    return token, constants.get(token)


def scan_source(path, constants):
    if not os.path.exists(path):
        return []
    text = open(path, encoding="latin-1").read()
    refs = []
    for lineno, line in enumerate(text.splitlines(), 1):
        for m in re.finditer(r"FindControl\(\s*(\w+)\s*\)", line):
            token, value = resolve_token(m.group(1), constants)
            if value is not None:
                refs.append((lineno, token, value, "FindControl"))
        for m in re.finditer(r"\b(?:idwControlID|dwControlID)\s*==\s*(\w+)", line):
            token, value = resolve_token(m.group(1), constants)
            if value is not None:
                refs.append((lineno, token, value, "compare"))
    return refs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--assets", default="../client_748",
                    help="path to client_748 (default: ../client_748)")
    ap.add_argument("--header", default="Projects/TMProject/ResourceControl.h")
    ap.add_argument("--source-refs", action="store_true",
                    help="also report source references missing from each scene")
    args = ap.parse_args()

    constants = parse_header(args.header)
    values = set(constants.values())
    repo = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(args.header))))
    set_list_path = os.path.join(args.assets, "UI", "UITextureSetList.txt")
    sets, set_names = parse_sets(set_list_path)
    print("texture sets: %d, max index %d" % (len(sets), max(sets)))
    print()

    for scene in SCENES:
        path = os.path.join(args.assets, "UI", scene)
        if not os.path.exists(path):
            print("MISSING", path)
            continue
        controls = walk_scene(path)
        counts = collections.Counter(c["type"] for c in controls)
        ids = {c["id"] for c in controls}
        undefined = sorted(i for i in ids if i not in values and i != 0)
        tex_sets = {resolve_set(c["tex"]) for c in controls}
        missing_sets = sorted(t for t in tex_sets
                              if t is not None and t not in sets)
        print("== %s: %d controls, EOF exact" % (scene, len(controls)))
        print("   types:", dict(sorted(counts.items())))
        print("   IDs not in ResourceControl.h:", undefined or "none")
        print("   texture sets used:", len(tex_sets),
              "missing:", missing_sets or "none")

        if args.source_refs and scene in SOURCES:
            refs = scan_source(os.path.join(repo, SOURCES[scene]), constants)
            missing = collections.Counter()
            examples = collections.defaultdict(list)
            for lineno, token, value, kind in refs:
                if value not in ids and value != 0:
                    missing[(token, value)] += 1
                    if len(examples[(token, value)]) < 3:
                        examples[(token, value)].append(lineno)
            print("   source references not in this scene: %d" % len(missing))
            for (token, value), count in sorted(missing.items(),
                                                key=lambda kv: -kv[1]):
                print("      %-32s value=%-8d refs=%d lines=%s"
                      % (token, value, count, examples[(token, value)]))
        print()

    print("note: -1/-2 are special texture sets; values < -2 mean abs(value).")


if __name__ == "__main__":
    main()
