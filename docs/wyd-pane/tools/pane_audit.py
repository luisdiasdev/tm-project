#!/usr/bin/env python3
"""
pane_audit.py - audit a shipped WYD/`.pane` data set.

Checks the things that silently break a port:

  * every panel parses and validates against pane_schema.json;
  * every TextureAlias used by a panel exists in the guimat;
  * every texture row used by an alias points at a file that exists;
  * every `uu_` string key exists in the language table;
  * duplicate element names per panel (they make name binding ambiguous).

It also emits a JSON manifest of the panels/assets, which is the input for the
C++ binding generator described in docs/ui-modernization/02-architecture.md.

Usage:
    python pane_audit.py --client ../../wydglobal2026
    python pane_audit.py --client ../../wydglobal2026 --json manifest.json
    python pane_audit.py --client ../../wydglobal2026 --strict
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from collections import Counter, defaultdict

# Same-directory import; pane_json owns the container + parser.
import pane_json


ALIAS_RE = re.compile(r"^\s*(\S+)\s+<(\d+)>\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s*$")
TEXTURE_RE = re.compile(r"^\s*(\d+)\s+(-?\d+)\s+(.+?)\s*$")
FONT_RE = re.compile(r"^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*$")
UU_RE = re.compile(r"\buu_(\d+)\b", re.I)


def read_guimat(path: str) -> dict:
    """Parse FontList / TextureList / TextureAlias from a plain UTF-16 guimat."""
    raw = open(path, "rb").read()
    if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff"):
        text = raw.decode("utf-16")
    elif raw.startswith(b"\xef\xbb\xbf"):
        text = raw.decode("utf-8-sig")
    else:
        text = raw.decode("utf-8", errors="replace")
    if text.startswith("\ufeff"):
        text = text.lstrip("\ufeff")

    fonts: list[dict] = []
    textures: dict[int, dict] = {}
    aliases: dict[str, dict] = {}

    section = ""
    for line in text.splitlines():
        s = line.strip()
        if not s or s.startswith("//"):
            continue
        low = s.lower()
        if low.startswith("fontlist"):
            section = "fonts"
            continue
        if low.startswith("texturelist"):
            section = "textures"
            continue
        if low.startswith("texturealias"):
            section = "aliases"
            continue
        if s == "{" or s == "}":
            continue
        if section == "fonts":
            m = FONT_RE.match(s)
            if m:
                fonts.append({
                    "index": int(m.group(1)),
                    "face": int(m.group(2)),
                    "size": int(m.group(3)),
                    "flags": int(m.group(4)),
                    "bold": m.group(5).lower() == "true",
                    "italic": m.group(6).lower() == "true",
                })
        elif section == "textures":
            m = TEXTURE_RE.match(s)
            if m:
                textures[int(m.group(1))] = {
                    "index": int(m.group(1)),
                    "colorkey": int(m.group(2)),
                    "path": m.group(3).strip(),
                }
        elif section == "aliases":
            m = ALIAS_RE.match(s)
            if m:
                aliases[m.group(1)] = {
                    "name": m.group(1),
                    "texture": int(m.group(2)),
                    "rect": [int(m.group(i)) for i in range(3, 7)],
                }
    return {"fonts": fonts, "textures": textures, "aliases": aliases}


def read_strings(path: str) -> dict[int, str]:
    if not os.path.exists(path):
        return {}
    raw = open(path, "rb").read()
    if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff"):
        text = raw.decode("utf-16")
    else:
        text = raw.decode("cp1252", errors="replace")
    out: dict[int, str] = {}
    for line in text.splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2 and parts[0].isdigit():
            out[int(parts[0])] = parts[1].strip()
    return out


def iter_panels(folder: str):
    for root, _dirs, files in os.walk(folder):
        for name in sorted(files):
            if name.lower().endswith((".pane", ".panel")):
                yield os.path.join(root, name)


def audit(client: str, panels_dir: str, guimat_path: str, strings_path: str) -> dict:
    schema = pane_json.load_schema()
    guimat = read_guimat(guimat_path)
    strings = read_strings(strings_path)

    panels = []
    type_counts: Counter[str] = Counter()
    total_elements = 0
    used_aliases: Counter[str] = Counter()
    uu_keys: set[int] = set()
    issues: list[str] = []
    duplicates: dict[str, list[str]] = {}

    for path in iter_panels(panels_dir):
        raw = open(path, "rb").read()
        rel = os.path.relpath(path, client).replace("\\", "/")
        model = pane_json.parse_text(pane_json.to_text(raw), rel)
        problems = pane_json.validate(model, schema)
        for p in problems:
            issues.append(f"{rel}: {p}")

        elements = model.get("elements", [])
        total_elements += len(elements)
        names: Counter[str] = Counter()
        for e in elements:
            type_counts[e["type"].lower()] += 1
            if e.get("name"):
                names[e["name"]] += 1
            for a in e.get("attributes", []):
                key = a["key"].lower()
                if key.startswith("texturealias"):
                    value = a["value"].strip()
                    if value:
                        used_aliases[value] += 1
                for n in UU_RE.findall(a["value"]):
                    uu_keys.add(int(n))
        for a in model.get("panel", {}).get("attributes", []):
            key = a["key"].lower()
            if key.startswith("texturealias"):
                value = a["value"].strip()
                if value:
                    used_aliases[value] += 1
            for n in UU_RE.findall(a["value"]):
                uu_keys.add(int(n))

        dup = sorted(n for n, c in names.items() if c > 1)
        if dup:
            duplicates[rel] = dup

        panels.append({
            "path": rel,
            "sha256": hashlib.sha256(raw).hexdigest(),
            "elements": len(elements),
            "types": sorted({e["type"] for e in elements}),
            "schema_issues": len(problems),
        })

    # Alias -> texture -> file.
    alias_issues = []
    texture_used: dict[int, dict] = {}
    for name in sorted(used_aliases):
        a = guimat["aliases"].get(name)
        if not a:
            alias_issues.append(f"alias '{name}' is not defined in the guimat")
            continue
        texture_used.setdefault(a["texture"], {"aliases": []})["aliases"].append(name)
    for idx, info in texture_used.items():
        t = guimat["textures"].get(idx)
        if not t:
            alias_issues.append(f"texture index {idx} is not in TextureList")
            continue
        info.update(t)
        full = os.path.join(client, t["path"])
        info["exists"] = os.path.exists(full)
        info["bytes"] = os.path.getsize(full) if info["exists"] else 0
        if not info["exists"]:
            alias_issues.append(f"texture {idx} '{t['path']}' is missing")

    missing_strings = sorted(k for k in uu_keys if k not in strings)

    return {
        "client": os.path.abspath(client),
        "panels_dir": os.path.abspath(panels_dir),
        "guimat": os.path.abspath(guimat_path),
        "strings": os.path.abspath(strings_path),
        "panel_count": len(panels),
        "element_records": total_elements,
        "types": dict(type_counts.most_common()),
        "aliases_defined": len(guimat["aliases"]),
        "aliases_used": len(used_aliases),
        "textures_defined": len(guimat["textures"]),
        "textures_used": len(texture_used),
        "fonts": len(guimat["fonts"]),
        "uu_keys_used": len(uu_keys),
        "uu_keys_missing": missing_strings,
        "duplicate_names": duplicates,
        "issues": issues + alias_issues,
        "textures": {str(k): v for k, v in sorted(texture_used.items())},
        "panels": panels,
    }


def print_summary(r: dict) -> None:
    print(f"client        : {r['client']}")
    print(f"panels        : {r['panel_count']} files, {r['element_records']} element records")
    print(f"types         : {', '.join(f'{k} {v}' for k, v in r['types'].items())}")
    print(f"aliases       : {r['aliases_used']} used / {r['aliases_defined']} defined")
    print(f"textures      : {r['textures_used']} used / {r['textures_defined']} defined")
    print(f"fonts         : {r['fonts']}")
    print(f"uu_ keys      : {r['uu_keys_used']} used, {len(r['uu_keys_missing'])} missing")
    print(f"issues        : {len(r['issues'])}")
    for line in r["issues"][:40]:
        print("  -", line)
    if len(r["issues"]) > 40:
        print(f"  ... {len(r['issues']) - 40} more")
    if r["duplicate_names"]:
        print("duplicate element names (binding ambiguity):")
        for panel, names in r["duplicate_names"].items():
            print(f"  - {panel}: {', '.join(names)}")
    missing = [t for t in r["textures"].values() if not t.get("exists")]
    if missing:
        print("missing texture files:")
        for t in missing:
            print(f"  - {t.get('index')} {t.get('path')}")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client", required=True, help="2026 client root (contains UI/ and Lang/)")
    ap.add_argument("--panels", help="panel folder (default: <client>/UI/Panel)")
    ap.add_argument("--guimat", help="guimat (default: <client>/UI/Default.guimat)")
    ap.add_argument("--strings", help="language table (default: <client>/Lang/EN/UIString.txt)")
    ap.add_argument("--json", help="write the full manifest to this path")
    ap.add_argument("--strict", action="store_true", help="exit 1 if any issue is found")
    args = ap.parse_args(argv[1:])

    client = os.path.abspath(args.client)
    panels_dir = args.panels or os.path.join(client, "UI", "Panel")
    guimat = args.guimat or os.path.join(client, "UI", "Default.guimat")
    strings = args.strings or os.path.join(client, "Lang", "EN", "UIString.txt")

    if not os.path.isdir(panels_dir):
        print(f"panel folder not found: {panels_dir}", file=sys.stderr)
        return 2
    if not os.path.exists(guimat):
        print(f"guimat not found: {guimat}", file=sys.stderr)
        return 2

    result = audit(client, panels_dir, guimat, strings)
    print_summary(result)
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(result, fh, indent=2, ensure_ascii=False)
        print(f"manifest written: {args.json}")

    if args.strict and result["issues"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
