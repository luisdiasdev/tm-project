"""Scan the port's C++ source for control-id references.

The scan is intentionally conservative: it reports what it sees (including
computed expressions) and never guesses silently.  The file is read as
latin-1 because ``TMFieldScene.cpp`` deliberately contains raw non-UTF-8
bytes; a round-trip through the same codec is byte-identical.
"""

from __future__ import annotations

import bisect
import re
from dataclasses import dataclass, field
from pathlib import Path

from .controls import Constants

TYPE_BY_CLASS = {
    "SPanel": "PANEL",
    "SButton": "BUTTON",
    "SText": "TEXT",
    "SEdit": "EDIT",
    "SListBox": "LISTBOX",
    "SGridControl": "GRID",
    "SProgress": "PROGRESS",
    "SCheckBox": "CHECKBOX",
    "S3DObj": "3DOBJ",
}

_CAST_RE = re.compile(
    r"(?:static_cast\s*<\s*(S\w+)\s*\*>|\(\s*(S\w+)\s*\*\s*\))"
)
_IDENT_RE = re.compile(r"\b([A-Za-z_]\w*)\b")
_INT_RE = re.compile(r"-?\d+[uUlL]*")
_FUNC_RE = re.compile(
    r"^\s*(?:[\w:<>,*&\s]+?\s+)?([A-Za-z_]\w*::[~A-Za-z_]\w*)\s*\("
)


@dataclass
class Ref:
    kind: str                 # findcart | compare | case
    token: str                # literal or constant name as written
    expr: str                 # full expression (FindControl argument)
    value: int | None
    line: int
    function: str
    snippet: str
    expected_type: str | None = None
    computed: bool = False
    computed_base: int | None = None
    computed_step: int = 1
    computed_count: int = 0
    extra: dict = field(default_factory=dict)

    def key(self):
        return (self.kind, self.token, self.value)

    def to_dict(self):
        return {
            "kind": self.kind,
            "token": self.token,
            "expr": self.expr,
            "value": self.value,
            "line": self.line,
            "function": self.function,
            "snippet": self.snippet,
            "expected_type": self.expected_type,
            "computed": self.computed,
            "computed_base": self.computed_base,
            "computed_step": self.computed_step,
            "computed_count": self.computed_count,
        }


class SourceScan:
    def __init__(self, path: Path, text: str, refs: list[Ref], warnings: list[str]):
        self.path = path
        self.text = text
        self.refs = refs
        self.warnings = warnings
        self.lines = text.splitlines()
        self._line_starts = [0]
        for line in text.splitlines(keepends=True):
            self._line_starts.append(self._line_starts[-1] + len(line))

    def line_of(self, pos: int) -> int:
        return bisect.bisect_right(self._line_starts, pos)

    def line_text(self, line: int) -> str:
        return self.lines[line - 1] if 0 < line <= len(self.lines) else ""

    def roundtrip_bytes(self) -> bytes:
        return self.text.encode("latin-1")


def scan_source(path: Path | str, constants: Constants) -> SourceScan:
    path = Path(path)
    raw = path.read_bytes()
    text = raw.decode("latin-1")
    lines = text.splitlines()
    warnings = []

    function_starts = []
    for idx, line in enumerate(lines, 1):
        m = _FUNC_RE.match(line)
        if m:
            name = m.group(1)
            if name.split("::")[0] and not name.startswith(("if", "for", "while", "switch")):
                function_starts.append((idx, name))
    functions = [f[0] for f in function_starts]

    def enclosing_function(line: int) -> str:
        i = bisect.bisect_right(functions, line) - 1
        return function_starts[i][1] if i >= 0 else ""

    line_starts = [0]
    for line in text.splitlines(keepends=True):
        line_starts.append(line_starts[-1] + len(line))

    def line_of(pos: int) -> int:
        return bisect.bisect_right(line_starts, pos)

    def snippet_at(line: int) -> str:
        s = lines[line - 1].strip() if 0 < line <= len(lines) else ""
        return s[:200]

    def expected_type(line: int):
        text_line = lines[line - 1] if 0 < line <= len(lines) else ""
        m = _CAST_RE.search(text_line)
        if not m:
            return None
        cls = m.group(1) or m.group(2)
        return TYPE_BY_CLASS.get(cls)

    def loop_count(line: int) -> int:
        for back in range(line - 1, max(0, line - 25), -1):
            t = lines[back - 1]
            m = re.search(r"for\s*\([^;)]*?<=?\s*(\d+)", t)
            if m:
                try:
                    return max(1, min(64, int(m.group(1))))
                except ValueError:
                    pass
        return 12

    refs: list[Ref] = []

    def classify_expr(expr: str):
        e = expr.strip()
        m = re.fullmatch(r"(-?\d+)[uUlL]*", e)
        if m:
            return int(m.group(1)), e, False, None, 1
        if re.fullmatch(r"[A-Za-z_]\w*", e):
            value = constants.resolve(e)
            return value, e, False, None, 1
        # Computed expression.  Use the largest literal/constant as the base
        # and the smallest positive literal as the step (if any).
        bases = []
        for m in re.finditer(r"(-?\d+)[uUlL]*", e):
            bases.append(int(m.group(1)))
        for m in re.finditer(r"\b([A-Za-z_]\w*)\b", e):
            v = constants.resolve(m.group(1))
            if v is not None:
                bases.append(v)
        base = max(bases) if bases else None
        # ``i + 65574`` walks by 1; ``32 * j + ii + 475152`` walks by 32.
        step_match = re.search(r"(\d+)\s*\*\s*[A-Za-z_]", e) or re.search(
            r"[A-Za-z_]\w*\s*\*\s*(\d+)", e
        )
        step = int(step_match.group(1)) if step_match else 1
        return None, e, True, base, step

    # FindControl(<expr>) with balanced parentheses
    for m in re.finditer(r"\bFindControl\s*\(", text):
        start = m.end()
        depth = 1
        i = start
        while i < len(text) and depth:
            ch = text[i]
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            i += 1
        if depth != 0:
            warnings.append("unbalanced FindControl( at offset %d" % m.start())
            continue
        expr = text[start : i - 1]
        value, token, computed, base, step = classify_expr(expr)
        line = line_of(m.start())
        count = loop_count(line) if computed else 0
        refs.append(
            Ref(
                kind="findcontrol",
                token=token,
                expr=expr.strip(),
                value=value,
                line=line,
                function=enclosing_function(line),
                snippet=snippet_at(line),
                expected_type=expected_type(line),
                computed=computed,
                computed_base=base,
                computed_step=step,
                computed_count=count,
            )
        )

    # idwControlID / dwControlID comparisons
    for m in re.finditer(
        r"\b(?:idwControlID|dwControlID)\s*==\s*([A-Za-z_]\w*|-?\d+[uUlL]*)", text
    ):
        token = m.group(1)
        value = constants.resolve(token)
        if value is None and re.fullmatch(r"-?\d+[uUlL]*", token):
            value = int(re.sub(r"[uUlL]+$", "", token))
        line = line_of(m.start())
        refs.append(
            Ref(
                kind="compare",
                token=token,
                expr=token,
                value=value,
                line=line,
                function=enclosing_function(line),
                snippet=snippet_at(line),
                expected_type=None,
            )
        )

    # case <id>: only large ids (control ids, not enum states)
    for m in re.finditer(r"\bcase\s+(-?\d+)\s*:", text):
        value = int(m.group(1))
        if value < 10000:
            continue
        line = line_of(m.start())
        refs.append(
            Ref(
                kind="case",
                token=str(value),
                expr=str(value),
                value=value,
                line=line,
                function=enclosing_function(line),
                snippet=snippet_at(line),
            )
        )

    refs.sort(key=lambda r: (r.line, r.kind, r.expr))
    return SourceScan(path, text, refs, warnings)
