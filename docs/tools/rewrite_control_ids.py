#!/usr/bin/env python3
"""Rewrite 755/760 control-ID references to the 748 TM* namespace.

This is the scripted pass used on TMFieldScene.cpp. It is byte-preserving:
the source is read/written as latin-1 so the non-UTF-8 bytes in
TMFieldScene.cpp are not replaced with U+FFFD.

Dry-run (default) prints what would change:

    python docs/tools/rewrite_control_ids.py \
        --source Projects/TMProject/TMFieldScene.cpp \
        --scene-bin ../client_748/UI/FieldScene2.bin

Apply only after reviewing the diff:

    python docs/tools/rewrite_control_ids.py ... --write

Rules:
  * only rewrite a numeric reference if that value is ABSENT from the target
    scene and a TM+name alias exists and its value is present;
  * only rewrite a symbolic plain constant under the same conditions;
  * handle FindControl(<n>), idwControlID/dwControlID == <n>, and case <n>:;
  * drop the C++ `u` suffix when it sits after the replaced number.
"""

import argparse
import collections
import os
import re
import struct
import sys

CONTROL_SIZE = {1: 40, 2: 40, 3: 32, 6: 52, 10: 48,
                12: 52, 13: 184, 15: 28, 16: 40}


def parse_header(path):
    text = open(path, encoding="utf-8", errors="ignore").read()
    return {n: int(v) for n, v in
            re.findall(r"constexpr auto (\w+) = (-?\d+);", text)}


def scene_ids(path):
    data = open(path, "rb").read()
    off = 0
    ids = set()
    while off + 4 <= len(data):
        ctype = struct.unpack_from("<i", data, off)[0]
        off += 4
        if ctype not in CONTROL_SIZE:
            raise SystemExit("bad control type %d at %d" % (ctype, off - 4))
        ids.add(struct.unpack_from("<i", data, off)[0])
        off += CONTROL_SIZE[ctype]
    if off != len(data):
        raise SystemExit("scene did not consume to EOF: %d/%d" % (off, len(data)))
    return ids


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--scene-bin", required=True)
    ap.add_argument("--header", default="Projects/TMProject/ResourceControl.h")
    ap.add_argument("--write", action="store_true",
                    help="actually write the file (default: dry-run)")
    args = ap.parse_args()

    constants = parse_header(args.header)
    values = collections.defaultdict(list)
    for name, value in constants.items():
        values[value].append(name)
    present = scene_ids(args.scene_bin)

    def alias_for_value(value):
        names = values.get(value, [])
        plain = [n for n in names if not n.startswith("TM")] or names
        for name in sorted(plain, key=len):
            candidate = "TM" + name
            if candidate in constants and constants[candidate] in present:
                return candidate
        for name in names:
            if name.startswith("TM") and constants[name] in present:
                return name
        return None

    src = open(args.source, encoding="latin-1").read()
    changes = []
    current = [0]

    def repl(match, kind):
        value = int(match.group("num"))
        if value in present:
            return match.group(0)
        alias = alias_for_value(value)
        if alias is None:
            return match.group(0)
        changes.append((current[0], kind, value, alias))
        post = match.group("post")
        if post.startswith("u"):
            post = post[1:]
        return match.group("pre") + alias + post

    pat_find = re.compile(r"(?P<pre>FindControl\(\s*)(?P<num>-?\d+)(?P<post>u?\s*\))")
    pat_cmp = re.compile(r"(?P<pre>\b(?:idwControlID|dwControlID)\s*==\s*)"
                         r"(?P<num>-?\d+)(?P<post>u?\b)")
    pat_case = re.compile(r"(?P<pre>\bcase\s+)(?P<num>-?\d+)(?P<post>u?\s*:)")

    out = []
    for line in src.splitlines(keepends=True):
        current[0] += 1
        line = pat_find.sub(lambda m: repl(m, "FindControl"), line)
        line = pat_cmp.sub(lambda m: repl(m, "compare"), line)
        line = pat_case.sub(
            lambda m: repl(m, "case") if int(m.group("num")) >= 10000 else m.group(0),
            line)
        out.append(line)
    new = "".join(out)

    # Symbolic plain constants that are absent from the scene.
    used = set(re.findall(r"\b[A-Z][A-Z0-9_]{2,}\b", src))
    ident_changes = []
    for name in sorted(used):
        if name not in constants or name.startswith("TM"):
            continue
        if constants[name] in present:
            continue
        candidate = "TM" + name
        if candidate in constants and constants[candidate] in present:
            ident_changes.append((name, candidate))
            new = re.sub(r"\b" + re.escape(name) + r"\b", candidate, new)

    print("numeric rewrites:", len(changes))
    for line, kind, value, alias in changes:
        print("  L%-6d %-11s %-8d -> %s" % (line, kind, value, alias))
    print("symbolic rewrites:", len(ident_changes))
    for old, new_name in ident_changes:
        print("  %-32s -> %s" % (old, new_name))

    if args.write:
        open(args.source, "w", encoding="latin-1", newline="").write(new)
        print("written:", args.source)
    else:
        print("dry-run; pass --write to apply")


if __name__ == "__main__":
    main()
