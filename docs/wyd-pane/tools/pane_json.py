#!/usr/bin/env python3
"""
pane_json.py - parse WYD .pane files into ordered JSON and rebuild them.

The .pane file is UTF-16LE text (BOM) obfuscated by the delta transform in
WYD.exe::FUN_00407580. This tool handles the container transparently: pass it
either the encrypted .pane or the decoded UTF-16LE text.

Commands
--------
  to-json   <input.pane|input.txt> [output.json]
  from-json <input.json> [output.pane] [--text]
  validate  <input.pane|input.txt>

JSON shape
----------
{
  "format": "wyd.pane",
  "source": "...",
  "comments": ["// ...", ...],
  "panel": {
    "attributes": [ {"key": "Resource", "value": "ui/default.guimat"}, ... ]
  },
  "elements": [
    {
      "type": "Image",
      "name": "bg",
      "attributes": [ {"key": "Area", "value": "0 0 500 25"}, ... ]
    }
  ]
}

Attributes are kept as an ordered list so unknown/duplicate keys survive a
round-trip. Values are strings; parse them in your runtime.
"""

import json
import os
import re
import sys
from typing import Any

# ---------------------------------------------------------------------------
# Container
# ---------------------------------------------------------------------------

def decrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    for i in range(n - 1, 0, -1):
        b[i] = (b[i] - b[i - 1] - (i % 14)) & 0xFF
    if n:
        b[0] = (b[0] - 14) & 0xFF
    return bytes(b)


def encrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    if n:
        b[0] = (b[0] + 14) & 0xFF
    for i in range(1, n):
        b[i] = (b[i] + b[i - 1] + (i % 14)) & 0xFF
    return bytes(b)


def to_text(raw: bytes) -> str:
    """Accept encrypted .pane or already-decoded UTF-16LE text."""
    if raw.startswith(b"\xff\xfe"):
        return raw.decode("utf-16-le").lstrip("\ufeff")
    dec = decrypt(raw)
    if not dec.startswith(b"\xff\xfe"):
        raise ValueError("not a WYD .pane (no BOM after decryption)")
    return dec.decode("utf-16-le").lstrip("\ufeff")


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

_ELEMENT_RE = re.compile(r"^\s*element\s+([^\s(]+)\s*(?:\(\s*([^)]*?)\s*\))?\s*$", re.I)
_PANEL_RE = re.compile(r"^\s*panel\s*$", re.I)


def parse_text(text: str, source: str = "") -> dict[str, Any]:
    model: dict[str, Any] = {
        "format": "wyd.pane",
        "source": source,
        "comments": [],
        "panel": {"attributes": []},
        "elements": [],
    }

    block = None          # "panel" | "element" | None
    current: dict[str, Any] | None = None
    depth = 0

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("//"):
            model["comments"].append(line)
            continue
        if line == "{":
            depth += 1
            continue
        if line == "}":
            depth = max(0, depth - 1)
            if depth == 0:
                block = None
                current = None
            continue

        if _PANEL_RE.match(line):
            block = "panel"
            current = model["panel"]
            continue

        m = _ELEMENT_RE.match(line)
        if m and line.lower().startswith("element"):
            block = "element"
            current = {
                "type": m.group(1),
                "name": m.group(2) or "",
                "attributes": [],
            }
            model["elements"].append(current)
            continue

        if block and current is not None:
            parts = line.split(None, 1)
            key = parts[0]
            value = parts[1].strip() if len(parts) > 1 else ""
            current["attributes"].append({"key": key, "value": value})

    return model


# ---------------------------------------------------------------------------
# Emitter
# ---------------------------------------------------------------------------

def _fmt_attr(a: dict[str, str]) -> str:
    # Keep the trailing space for empty values: shipped files use "\tKey " and
    # the parser treats it as an empty value either way.
    return f"\t{a['key']} {a.get('value', '')}"


def emit_text(model: dict[str, Any]) -> str:
    out: list[str] = []
    for c in model.get("comments", []):
        out.append(c)

    out.append("Panel")
    out.append("{")
    for a in model["panel"].get("attributes", []):
        out.append(_fmt_attr(a))
    out.append("}")

    for e in model.get("elements", []):
        name = e.get("name", "")
        header = f"element {e['type']}"
        if name:
            header += f"({name})"
        out.append("")
        out.append(header)
        out.append("{")
        for a in e.get("attributes", []):
            out.append(_fmt_attr(a))
        out.append("}")

    return "\r\n".join(out) + "\r\n"


def emit_binary(model: dict[str, Any]) -> bytes:
    text = emit_text(model)
    return encrypt(b"\xff\xfe" + text.encode("utf-16-le"))


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def load_schema(path: str | None = None) -> dict[str, Any] | None:
    if path is None:
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "pane_schema.json")
    if not os.path.isfile(path):
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


_INDEXED_RE = re.compile(r"\[\d+\]\s*$")


def _norm_key(key: str) -> str:
    """Normalize indexed keys so TextureAlias[0] matches TextureAlias[n]."""
    return _INDEXED_RE.sub("[n]", key.strip()).lower()


def _type_keys(schema: dict[str, Any], type_name: str) -> tuple[set[str] | None, str | None]:
    """Return the allowed key set for a type, following the fallback chain."""
    types = schema.get("element_types", {})
    info = types.get(type_name)
    if info is None:
        return None, None
    allowed: set[str] = {_norm_key(k) for k in schema.get("common_element_keys", {})}
    seen: set[str] = set()
    current = type_name
    while current in types and current not in seen:
        seen.add(current)
        cur = types[current]
        allowed |= {_norm_key(k) for k in cur.get("keys", {})}
        current = cur.get("fallback", "")
    return allowed, info.get("class", type_name)


def validate(model: dict[str, Any], schema: dict[str, Any] | None) -> list[str]:
    problems: list[str] = []
    if schema is None:
        problems.append("schema not found; skipping key validation")
        return problems

    panel_keys = {_norm_key(k) for k in schema.get("panel_keys", {})}
    types = schema.get("element_types", {})

    for a in model["panel"].get("attributes", []):
        if _norm_key(a["key"]) not in panel_keys:
            problems.append(f"panel: unknown key {a['key']!r}")

    for e in model.get("elements", []):
        t = e["type"].lower()
        info = types.get(t)
        if info is None:
            problems.append(f"element {e['name'] or e['type']}: unknown type {e['type']!r}")
            continue
        allowed, _ = _type_keys(schema, t)
        assert allowed is not None
        for a in e.get("attributes", []):
            if _norm_key(a["key"]) not in allowed:
                problems.append(
                    f"element {e['name'] or e['type']} ({e['type']}): unknown key {a['key']!r}"
                )
    return problems


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str]) -> int:
    if len(argv) < 3:
        print(__doc__)
        return 2
    cmd = argv[1]
    src = argv[2]

    if cmd == "to-json":
        raw = open(src, "rb").read()
        model = parse_text(to_text(raw), source=os.path.abspath(src))
        dst = argv[3] if len(argv) > 3 and not argv[3].startswith("-") else src + ".json"
        with open(dst, "w", encoding="utf-8") as f:
            json.dump(model, f, ensure_ascii=False, indent=2)
        print(f"wrote {dst}")
        return 0

    if cmd == "from-json":
        with open(src, "r", encoding="utf-8") as f:
            model = json.load(f)
        want_text = "--text" in argv
        dst = next((a for a in argv[3:] if not a.startswith("-")), None) or (
            src[:-5] + (".txt" if want_text else ".pane")
        )
        if want_text:
            data = b"\xff\xfe" + emit_text(model).encode("utf-16-le")
        else:
            data = emit_binary(model)
        with open(dst, "wb") as f:
            f.write(data)
        print(f"wrote {dst} ({len(data)} bytes)")
        return 0

    if cmd == "validate":
        schema = load_schema()
        raw = open(src, "rb").read()
        model = parse_text(to_text(raw), source=os.path.abspath(src))
        problems = validate(model, schema)
        if not problems:
            print(f"OK: {len(model['panel']['attributes'])} panel keys, "
                  f"{len(model['elements'])} elements")
            return 0
        for p in problems:
            print("WARN:", p)
        return 1

    print("unknown command:", cmd)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
