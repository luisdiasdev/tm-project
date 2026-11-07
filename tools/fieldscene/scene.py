"""Read, validate and write WYD ``*Scene2.bin`` control files.

The record layouts mirror ``Projects/TMProject/UIBinary.h``.  Parsing is
strict: every record must be known, the file must end exactly at a record
boundary, and parents are resolved after the whole file has been read.

Keeping each control's original body bytes means a re-save that changes one
integer cannot accidentally re-encode unrelated fields.
"""

from __future__ import annotations

import struct
from pathlib import Path


# CONTROL_TYPE ids used by the 748 field scene.
CTRL_PANEL = 1
CTRL_BUTTON = 2
CTRL_CHECKBOX = 3
CTRL_LISTBOX = 6
CTRL_PROGRESS = 10
CTRL_TEXT = 12
CTRL_EDIT = 13
CTRL_3DOBJ = 15
CTRL_GRID = 16

TYPE_NAMES = {
    CTRL_PANEL: "PANEL",
    CTRL_BUTTON: "BUTTON",
    CTRL_CHECKBOX: "CHECKBOX",
    CTRL_LISTBOX: "LISTBOX",
    CTRL_PROGRESS: "PROGRESS",
    CTRL_TEXT: "TEXT",
    CTRL_EDIT: "EDIT",
    CTRL_3DOBJ: "3DOBJ",
    CTRL_GRID: "GRID",
}

# Ordered field layouts.  ``i`` = int32, ``s128`` = char[128].
_FIELDS = {
    CTRL_PANEL: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("color", "i"), ("fill_type", "i"), ("pickable", "i"),
    ],
    CTRL_BUTTON: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("color", "i"), ("sound", "i"), ("string_index", "i"),
    ],
    CTRL_CHECKBOX: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("color", "i"),
    ],
    CTRL_LISTBOX: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("max_count", "i"), ("visible_count", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("color", "i"), ("fill_type", "i"), ("select", "i"), ("scroll", "i"),
    ],
    CTRL_PROGRESS: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("current", "i"), ("max_value", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("progress_color", "i"), ("color", "i"), ("style", "i"),
    ],
    CTRL_TEXT: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("font_color", "i"), ("border", "i"), ("border_color", "i"),
        ("text_type", "i"), ("align", "i"), ("string_index", "i"),
    ],
    CTRL_EDIT: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("font_color", "i"), ("border", "i"), ("border_color", "i"),
        ("text_type", "i"), ("align", "i"), ("max_length", "i"),
        ("password", "i"), ("value", "s128"),
    ],
    CTRL_3DOBJ: [
        ("id", "i"), ("parent", "i"), ("obj_index", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
    ],
    CTRL_GRID: [
        ("id", "i"), ("parent", "i"), ("tex", "i"),
        ("x", "i"), ("y", "i"), ("w", "i"), ("h", "i"),
        ("rows", "i"), ("columns", "i"), ("item_type", "i"),
    ],
}

TYPE_SIZES = {
    t: sum(4 if kind == "i" else 128 for _, kind in fields)
    for t, fields in _FIELDS.items()
}

# Sanity check against UIBinary.h (documented sizes).
_EXPECTED_SIZES = {
    CTRL_PANEL: 40, CTRL_BUTTON: 40, CTRL_CHECKBOX: 32, CTRL_LISTBOX: 52,
    CTRL_PROGRESS: 48, CTRL_TEXT: 52, CTRL_EDIT: 184, CTRL_3DOBJ: 28,
    CTRL_GRID: 40,
}
assert TYPE_SIZES == _EXPECTED_SIZES, TYPE_SIZES

INT_FIELDS = {
    name for fields in _FIELDS.values() for name, kind in fields if kind == "i"
}
ALL_FIELDS = {
    t: [name for name, _ in fields] for t, fields in _FIELDS.items()
}


class SceneError(Exception):
    pass


class Control:
    """One binary record plus its decoded fields."""

    __slots__ = ("type", "offset", "body", "values", "_dirty", "index")

    def __init__(self, ctype: int, offset: int, body: bytes):
        self.type = ctype
        self.offset = offset
        self.body = body
        self.values = _decode_body(ctype, body)
        self._dirty = False
        self.index = -1

    # Convenience accessors ------------------------------------------------
    @property
    def type_name(self) -> str:
        return TYPE_NAMES.get(self.type, "UNKNOWN(%d)" % self.type)

    @property
    def id(self) -> int:
        return self.values.get("id", 0)

    @property
    def parent(self) -> int:
        return self.values.get("parent", 0)

    @property
    def tex(self) -> int:
        v = self.values.get("tex")
        return 0 if v is None else v

    @property
    def x(self) -> int:
        return self.values.get("x", 0)

    @property
    def y(self) -> int:
        return self.values.get("y", 0)

    @property
    def w(self) -> int:
        return self.values.get("w", 0)

    @property
    def h(self) -> int:
        return self.values.get("h", 0)

    @property
    def string_index(self):
        return self.values.get("string_index")

    def rect(self):
        return (self.x, self.y, self.w, self.h)

    def get(self, name, default=None):
        return self.values.get(name, default)

    def set(self, name, value):
        if name not in self.values:
            raise KeyError("%s has no field %r" % (self.type_name, name))
        if self.values[name] == value:
            return
        self.values[name] = value
        self._dirty = True

    def to_dict(self):
        d = {
            "index": self.index,
            "type": self.type_name,
            "type_id": self.type,
            "offset": self.offset,
            "id": self.id,
            "parent": self.parent,
            "tex": self.tex,
            "rect": [self.x, self.y, self.w, self.h],
        }
        d.update(self.values)
        return d

    def serialize(self) -> bytes:
        if not self._dirty:
            body = self.body
        else:
            body = _encode_body(self.type, self.values)
        return struct.pack("<i", self.type) + body


def _decode_body(ctype: int, body: bytes) -> dict:
    fields = _FIELDS[ctype]
    out = {}
    off = 0
    for name, kind in fields:
        if kind == "i":
            out[name] = struct.unpack_from("<i", body, off)[0]
            off += 4
        else:
            raw = body[off : off + 128]
            out[name] = raw.split(b"\0", 1)[0].decode("latin-1")
            off += 128
    if off != len(body):
        raise SceneError("body length mismatch for type %d" % ctype)
    return out


def _encode_body(ctype: int, values: dict) -> bytes:
    parts = []
    for name, kind in _FIELDS[ctype]:
        if kind == "i":
            parts.append(struct.pack("<i", int(values.get(name, 0))))
        else:
            raw = str(values.get(name, "")).encode("latin-1", "replace")[:127]
            parts.append(raw + b"\0" * (128 - len(raw)))
    return b"".join(parts)


class Scene:
    def __init__(self, path: Path | str, controls: list[Control]):
        self.path = Path(path)
        self.controls = controls
        self.warnings: list[str] = []
        self._by_id = {}
        self._duplicates = []
        for idx, control in enumerate(controls):
            control.index = idx
            if control.id in self._by_id:
                self._duplicates.append(control.id)
            else:
                self._by_id[control.id] = control

    # -- construction -----------------------------------------------------
    @classmethod
    def load(cls, path: Path | str) -> "Scene":
        path = Path(path)
        return cls.load_bytes(path.read_bytes(), path)

    @classmethod
    def load_bytes(cls, data: bytes, path="<memory>") -> "Scene":
        path = Path(path)
        off = 0
        controls = []
        while off < len(data):
            if off + 4 > len(data):
                raise SceneError(
                    "%s: %d trailing bytes at offset %d" % (path.name, len(data) - off, off)
                )
            ctype = struct.unpack_from("<i", data, off)[0]
            if ctype not in TYPE_SIZES:
                raise SceneError(
                    "%s: unknown control type %d at offset %d" % (path.name, ctype, off)
                )
            size = TYPE_SIZES[ctype]
            start = off + 4
            end = start + size
            if end > len(data):
                raise SceneError(
                    "%s: truncated %s at offset %d"
                    % (path.name, TYPE_NAMES[ctype], off)
                )
            controls.append(Control(ctype, off, data[start:end]))
            off = end
        if off != len(data):
            raise SceneError("%s: did not consume file (%d/%d)" % (path.name, off, len(data)))
        scene = cls(path, controls)
        scene._validate()
        return scene

    def _validate(self):
        if self._duplicates:
            uniq = sorted(set(self._duplicates))
            self.warnings.append(
                "duplicate control ids: %s" % ", ".join(map(str, uniq[:12]))
            )
        missing_parents = set()
        for c in self.controls:
            if c.parent and c.parent not in self._by_id:
                missing_parents.add(c.parent)
        if missing_parents:
            self.warnings.append(
                "controls with missing parents: %s"
                % ", ".join(map(str, sorted(missing_parents)[:12]))
            )
        zero = [c.id for c in self.controls if c.w <= 0 or c.h <= 0]
        if zero:
            self.warnings.append(
                "controls with empty size: %s" % ", ".join(map(str, zero[:12]))
            )

    # -- queries ----------------------------------------------------------
    def __len__(self):
        return len(self.controls)

    def get(self, control_id: int):
        return self._by_id.get(control_id)

    @property
    def ids(self) -> set:
        return set(self._by_id)

    @property
    def duplicate_ids(self) -> list:
        return sorted(set(self._duplicates))

    def children(self, control_id: int):
        return [c for c in self.controls if c.parent == control_id]

    def bounds(self):
        if not self.controls:
            return (0, 0, 0, 0)
        x0 = min(c.x for c in self.controls)
        y0 = min(c.y for c in self.controls)
        x1 = max(c.x + max(0, c.w) for c in self.controls)
        y1 = max(c.y + max(0, c.h) for c in self.controls)
        return (x0, y0, x1, y1)

    def type_counts(self):
        counts = {}
        for c in self.controls:
            counts[c.type_name] = counts.get(c.type_name, 0) + 1
        return dict(sorted(counts.items()))

    def to_bytes(self) -> bytes:
        return b"".join(c.serialize() for c in self.controls)

    def save(self, path: Path | str):
        path = Path(path)
        data = self.to_bytes()
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return len(data)


def load_ui_strings(path: Path | str) -> dict:
    """Parse ``UI/UIString.txt`` (``<index> <text>``)."""
    out = {}
    p = Path(path)
    if not p.exists():
        return out
    for line in p.read_text(encoding="latin-1").splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split(None, 1)
        try:
            idx = int(parts[0])
        except ValueError:
            continue
        out[idx] = parts[1] if len(parts) > 1 else ""
    return out


def summarize(scene: Scene) -> dict:
    x0, y0, x1, y1 = scene.bounds()
    return {
        "path": str(scene.path),
        "size": scene.path.stat().st_size if scene.path.exists() else 0,
        "controls": len(scene.controls),
        "unique_ids": len(scene.ids),
        "duplicate_ids": scene.duplicate_ids,
        "types": scene.type_counts(),
        "bounds": [x0, y0, x1, y1],
        "warnings": list(scene.warnings),
    }
