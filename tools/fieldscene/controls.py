"""Parse ``Projects/TMProject/ResourceControl.h`` control constants."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

# Supports ``constexpr auto NAME = 123;`` and ``static constexpr int NAME = -1;``.
_CONST_RE = re.compile(
    r"(?:constexpr|#define)\s+"
    r"(?:(?:static\s+)?(?:const\s+)?(?:auto|int|unsigned(?:\s+int)?|DWORD|long|uint32_t)\s+)?"
    r"([A-Za-z_]\w*)\s*(?:=|\s)\s*(-?(?:0[xX][0-9A-Fa-f]+|\d+))"
)


class Constants:
    def __init__(self, path: Path | str):
        self.path = Path(path)
        self.names: dict[str, int] = {}
        self.by_value: dict[int, list[str]] = defaultdict(list)
        if self.path.exists():
            self._load(self.path.read_text(encoding="latin-1", errors="ignore"))

    def _load(self, text: str):
        for line in text.splitlines():
            line = line.strip()
            if not line or line.startswith("//"):
                continue
            m = _CONST_RE.search(line)
            if not m:
                continue
            raw = m.group(2)
            value = int(raw, 16) if raw.lower().lstrip("-").startswith("0x") else int(raw, 10)
            name = m.group(1)
            if name in self.names and self.names[name] != value:
                continue
            self.names[name] = value
        for name, value in self.names.items():
            self.by_value[value].append(name)
        for value in self.by_value:
            self.by_value[value].sort()

    # -- lookups ----------------------------------------------------------
    def resolve(self, token: str):
        return self.names.get(token)

    def names_for(self, value: int) -> list:
        return list(self.by_value.get(value, ()))

    def tm_name_for(self, value: int):
        for name in self.by_value.get(value, ()):
            if name.startswith("TM"):
                return name
        return None

    def primary_name(self, value: int) -> str:
        names = self.by_value.get(value, ())
        if not names:
            return ""
        tm = [n for n in names if n.startswith("TM")]
        return (tm or list(names))[0]

    def plain_name(self, value: int):
        for name in self.by_value.get(value, ()):
            if not name.startswith("TM"):
                return name
        return None

    def alias_for(self, plain_name: str):
        if not plain_name or plain_name.startswith("TM"):
            return None
        candidate = "TM" + plain_name
        return candidate if candidate in self.names else None

    def duplicate_values(self):
        return {
            value: names
            for value, names in self.by_value.items()
            if len(names) > 1
        }

    def __len__(self):
        return len(self.names)
