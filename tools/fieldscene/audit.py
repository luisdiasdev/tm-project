"""Join the scene binary, the header constants and the C++ source."""

from __future__ import annotations

import json
import math
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from .assets import AssetLibrary
from .controls import Constants
from .scene import Scene
from .source import Ref, SourceScan

KNOWN_PATH = Path(__file__).resolve().parent / "data" / "known_bindings.json"


def load_known(path: Path | None = None) -> dict:
    p = Path(path) if path else KNOWN_PATH
    if p.exists():
        return json.loads(p.read_text(encoding="utf-8"))
    return {"dynamic": {}, "known_absent": {}, "unmapped_scene_ids": {}}


@dataclass
class RefAudit:
    ref: Ref
    status: str
    control: object | None
    note: str = ""

    def to_dict(self, control_dict=None):
        d = self.ref.to_dict()
        d["status"] = self.status
        d["note"] = self.note
        d["control"] = control_dict
        return d


class Audit:
    def __init__(self, **data):
        self.__dict__.update(data)

    # -- serialization ----------------------------------------------------
    def to_dict(self):
        return {
            "generated": self.generated,
            "paths": self.paths,
            "scene": self.scene_summary,
            "reference_scene": self.ref_scene_summary,
            "summary": self.summary,
            "references": [a.to_dict() for a in self.ref_audits],
            "dead_references": self.dead_references,
            "type_mismatches": self.type_mismatches,
            "computed_references": self.computed_references,
            "orphan_controls": self.orphan_controls,
            "unnamed_controls": self.unnamed_controls,
            "duplicate_constants": self.duplicate_constants,
            "texture_problems": self.texture_problems,
            "scene_warnings": self.scene_warnings,
            "asset_warnings": self.asset_warnings,
            "source_warnings": self.source_warnings,
        }

    def baseline_keys(self):
        keys = []
        for dead in self.dead_references:
            keys.append(
                "dead|%s|%s"
                % (dead["category"], dead.get("value") if dead.get("value") is not None else dead["token"])
            )
        for item in self.type_mismatches:
            keys.append("mismatch|%s|%s" % (item["line"], item["token"]))
        for item in self.unnamed_controls:
            keys.append("unnamed|%s" % item["id"])
        for item in self.texture_problems:
            keys.append("texture|%s" % item["message"])
        for warning in self.scene_warnings:
            keys.append("scene|%s" % warning)
        for warning in self.source_warnings:
            keys.append("source|%s" % warning)
        return sorted(set(keys))

    # -- markdown ---------------------------------------------------------
    def to_markdown(self) -> str:
        s = self.summary
        out = []
        out.append("# FieldScene audit")
        out.append("")
        out.append("Generated: `%s`" % self.generated)
        out.append("")
        out.append("| Input | Path |")
        out.append("|---|---|")
        for key, value in self.paths.items():
            out.append("| %s | `%s` |" % (key, value))
        out.append("")
        out.append("## Scene")
        out.append("")
        sc = self.scene_summary
        out.append(
            "%d controls, %d unique ids, bounds `%s`, types `%s`."
            % (
                sc["controls"],
                sc["unique_ids"],
                sc["bounds"],
                ", ".join("%s=%d" % kv for kv in sc["types"].items()),
            )
        )
        out.append("")
        out.append("## Summary")
        out.append("")
        out.append("| Metric | Count |")
        out.append("|---|---:|")
        for key in (
            "references",
            "resolved_references",
            "dead_values",
            "dead_references",
            "type_mismatches",
            "unknown_constants",
            "computed_references",
            "orphan_controls",
            "unnamed_controls",
            "duplicate_values",
            "dead_values_with_candidates",
        ):
            out.append("| %s | %d |" % (key.replace("_", " "), s.get(key, 0)))
        out.append("")

        if self.dead_references:
            out.append("## Dead references")
            out.append("")
            out.append(
                "IDs the port looks up that do not exist in this scene. "
                "Candidates are derived from the 755 `FieldScene2.bin` by "
                "type + size + texture-file similarity."
            )
            out.append("")
            out.append("| ID / token | Category | Refs | Lines | Candidates |")
            out.append("|---|---|---:|---|---|")
            for dead in self.dead_references:
                ident = dead.get("value") if dead.get("value") is not None else dead["token"]
                lines = ", ".join(str(r["line"]) for r in dead["references"][:4])
                cands = ", ".join(
                    "%s(%d)" % (c["id"], c["score"]) for c in dead.get("candidates", [])[:3]
                )
                out.append(
                    "| `%s` | %s | %d | %s | %s |"
                    % (ident, dead["category"], len(dead["references"]), lines, cands or "—")
                )
            out.append("")

        if self.type_mismatches:
            out.append("## Type mismatches")
            out.append("")
            out.append("| Line | Token | Expected | Scene type | Function |")
            out.append("|---:|---|---|---|---|")
            for item in self.type_mismatches:
                out.append(
                    "| %d | `%s` | %s | %s | `%s` |"
                    % (
                        item["line"],
                        item["token"],
                        item["expected"],
                        item["actual"],
                        item["function"],
                    )
                )
            out.append("")

        if self.computed_references:
            out.append("## Computed references")
            out.append("")
            out.append("| Line | Expression | Base | Step | Count | Missing | Function |")
            out.append("|---:|---|---:|---:|---:|---:|---|")
            for item in self.computed_references:
                out.append(
                    "| %d | `%s` | %s | %s | %s | %s | `%s` |"
                    % (
                        item["line"],
                        item["expr"],
                        item["base"],
                        item["step"],
                        item["count"],
                        item["missing"],
                        item["function"],
                    )
                )
            out.append("")

        if self.unnamed_controls:
            out.append("## Scene ids without a header name")
            out.append("")
            out.append("| ID | Type | Rect | Texture set | Proposed name |")
            out.append("|---:|---|---|---|---|")
            for item in self.unnamed_controls:
                out.append(
                    "| %d | %s | `%s` | %s | %s |"
                    % (
                        item["id"],
                        item["type"],
                        item["rect"],
                        item["texture_set"],
                        item.get("proposed_name") or "—",
                    )
                )
            out.append("")

        if self.orphan_controls:
            out.append("## Controls the source never references")
            out.append("")
            out.append(
                "%d controls.  The first %d are listed; use the viewer or JSON "
                "for the full set." % (len(self.orphan_controls), min(80, len(self.orphan_controls)))
            )
            out.append("")
            out.append("| ID | Type | Rect | Texture | Name |")
            out.append("|---:|---|---|---|---|")
            for item in self.orphan_controls[:80]:
                out.append(
                    "| %d | %s | `%s` | %s | %s |"
                    % (item["id"], item["type"], item["rect"], item["texture_set"], item["name"] or "—")
                )
            out.append("")

        if self.duplicate_constants:
            out.append("## Constants sharing a numeric value")
            out.append("")
            out.append("| Value | Names |")
            out.append("|---:|---|")
            for item in self.duplicate_constants:
                out.append("| %d | %s |" % (item["value"], ", ".join(item["names"])))
            out.append("")

        if self.texture_problems:
            out.append("## Texture problems")
            out.append("")
            for item in self.texture_problems:
                out.append("- `%s`: %s" % (item["id"], item["message"]))
            out.append("")

        if self.scene_warnings or self.asset_warnings or self.source_warnings:
            out.append("## Warnings")
            out.append("")
            for warning in self.scene_warnings:
                out.append("- scene: %s" % warning)
            for warning in self.asset_warnings:
                out.append("- asset: %s" % warning)
            for warning in self.source_warnings:
                out.append("- source: %s" % warning)
            out.append("")

        return "\n".join(out)


def _control_dict(c, constants: Constants, known: dict):
    names = constants.names_for(c.id)
    proposed = None
    if not names:
        info = known.get("unmapped_scene_ids", {}).get(str(c.id))
        if info:
            proposed = info.get("name")
    return {
        "id": c.id,
        "type": c.type_name,
        "rect": [c.x, c.y, c.w, c.h],
        "tex": c.tex,
        "texture_set": c.tex,
        "names": names,
        "proposed_name": proposed,
        "name": (names[0] if names else proposed),
    }


_CONTROL_PREFIXES = {
    "TMB", "TMP", "TMT", "TML", "TMG", "TME", "TMR",
    "B", "P", "T", "L", "G", "E", "M",
}


def _is_control_name(name: str) -> bool:
    return bool(name) and name.split("_")[0] in _CONTROL_PREFIXES


def _expand_ref(ref: Ref):
    if ref.computed and ref.computed_base is not None:
        return [
            ref.computed_base + i * max(1, ref.computed_step)
            for i in range(max(0, ref.computed_count))
        ]
    if ref.value is not None:
        return [ref.value]
    return []


def _signature(control, assets: AssetLibrary):
    return (
        control.type_name,
        control.w,
        control.h,
        assets.set_basename(control.tex),
    )


_NAME_PREFIXES = {
    "TMB", "TMP", "TMT", "TML", "TMG", "TME", "TMR",
    "B", "P", "T", "L", "G", "E", "M", "S", "C",
}


def _core_name(name: str) -> str:
    if not name:
        return ""
    parts = name.split("_")
    while parts and parts[0] in _NAME_PREFIXES:
        parts.pop(0)
    return "_".join(parts)


def _name_score(old_name: str, cand_name: str) -> int:
    a, b = _core_name(old_name), _core_name(cand_name)
    if not a or not b:
        return 0
    if a == b:
        return 60
    ta, tb = set(a.split("_")), set(b.split("_"))
    if ta and tb:
        if ta <= tb or tb <= ta:
            return 40
        j = len(ta & tb) / float(len(ta | tb))
        if j >= 0.5:
            return int(j * 30)
    if a in b or b in a:
        return 25
    return 0


def _candidate_score(old, cand, target_assets: AssetLibrary, used_ids, old_name, cand_name):
    if old.type_name != cand.type_name:
        return None
    old_file = _old_file_cache.get(id(old), "")
    file_match = bool(old_file) and old_file == target_assets.set_basename(cand.tex)
    size_match = old.w == cand.w and old.h == cand.h
    string_match = (
        old.string_index is not None
        and old.string_index != 0
        and old.string_index == cand.string_index
    )
    parent_match = bool(old.parent) and old.parent == cand.parent
    name_match = _name_score(old_name, cand_name)
    if not (file_match or name_match or (size_match and (string_match or parent_match))):
        return None
    score = 0
    score += name_match
    if file_match:
        score += 40
    if size_match:
        score += 15
    if string_match:
        score += 8
    if parent_match:
        score += 5
    ox, oy = old.x + old.w / 2.0, old.y + old.h / 2.0
    cx, cy = cand.x + cand.w / 2.0, cand.y + cand.h / 2.0
    dist = math.hypot(ox - cx, oy - cy)
    score += max(0, int(15 - dist / 50.0))
    if cand.id in used_ids:
        score -= 35
    if score <= 0:
        return None
    return score, int(dist)


_old_file_cache: dict[int, str] = {}


def build_audit(
    scene: Scene,
    assets: AssetLibrary,
    constants: Constants,
    scan: SourceScan,
    known: dict | None = None,
    ref_scene: Scene | None = None,
    ref_assets: AssetLibrary | None = None,
    with_candidates: bool = True,
) -> Audit:
    known = known or load_known()
    dynamic = {int(k): v for k, v in known.get("dynamic", {}).items()}
    absent = {int(k): v for k, v in known.get("known_absent", {}).items()}
    unmapped = {int(k): v for k, v in known.get("unmapped_scene_ids", {}).items()}

    global _old_file_cache
    _old_file_cache = {}
    if ref_scene is not None and ref_assets is not None:
        for c in ref_scene.controls:
            _old_file_cache[id(c)] = ref_assets.set_basename(c.tex)

    ids = scene.ids
    referenced = set()
    ref_audits: list[RefAudit] = []
    computed_references = []
    unknown_constants = []
    type_mismatches = []

    for ref in scan.refs:
        if ref.kind == "case":
            # ``case 40027:`` is usually an enum, not a control id.  Keep only
            # values that exist in the scene, are a named control constant, or
            # are explicitly known to be absent.
            named = constants.names_for(ref.value or 0)
            control_named = any(_is_control_name(n) for n in named)
            if (
                ref.value not in ids
                and not control_named
                and ref.value not in dynamic
                and ref.value not in absent
            ):
                continue
        expanded = _expand_ref(ref)
        referenced.update(expanded)
        if ref.computed:
            missing = [v for v in expanded if v not in ids]
            computed_references.append(
                {
                    "line": ref.line,
                    "expr": ref.expr,
                    "function": ref.function,
                    "base": ref.computed_base,
                    "step": ref.computed_step,
                    "count": len(expanded),
                    "missing": len(missing),
                    "missing_ids": missing[:24],
                }
            )
            ref_audits.append(RefAudit(ref, "computed", None))
            continue
        if ref.value is None:
            # A lowercase identifier is a local variable / parameter
            # (``idwControlID``, ``nMixPanelID``), not an unresolved constant.
            if any(ch.islower() for ch in ref.token):
                ref_audits.append(RefAudit(ref, "variable", None))
            else:
                unknown_constants.append(
                    {
                        "line": ref.line,
                        "token": ref.token,
                        "function": ref.function,
                        "snippet": ref.snippet,
                    }
                )
                ref_audits.append(RefAudit(ref, "unknown-constant", None))
            continue
        control = scene.get(ref.value)
        if control is not None:
            if ref.expected_type and control.type_name != ref.expected_type:
                type_mismatches.append(
                    {
                        "line": ref.line,
                        "token": ref.token,
                        "value": ref.value,
                        "expected": ref.expected_type,
                        "actual": control.type_name,
                        "function": ref.function,
                        "snippet": ref.snippet,
                    }
                )
                ref_audits.append(RefAudit(ref, "type-mismatch", control))
            else:
                ref_audits.append(RefAudit(ref, "resolved", control))
            continue
        if ref.value in dynamic:
            ref_audits.append(RefAudit(ref, "dynamic", None, dynamic[ref.value]))
        elif ref.value in absent:
            ref_audits.append(RefAudit(ref, "755-only", None, absent[ref.value]))
        elif ref.value == 0:
            ref_audits.append(RefAudit(ref, "none", None))
        else:
            ref_audits.append(RefAudit(ref, "unresolved", None))

    # Dead reference groups -------------------------------------------------
    dead: dict[tuple, dict] = {}
    for audit in ref_audits:
        if audit.status not in ("unresolved", "755-only", "dynamic", "unknown-constant"):
            continue
        if audit.ref.computed:
            continue
        key = (
            "value" if audit.ref.value is not None else "token",
            audit.ref.value if audit.ref.value is not None else audit.ref.token,
        )
        group = dead.setdefault(
            key,
            {
                "value": audit.ref.value,
                "token": audit.ref.token,
                "category": audit.status,
                "note": audit.note,
                "references": [],
                "candidates": [],
            },
        )
        group["references"].append(
            {
                "line": audit.ref.line,
                "function": audit.ref.function,
                "snippet": audit.ref.snippet,
                "kind": audit.ref.kind,
                "token": audit.ref.token,
            }
        )

    # Candidate matching against the 755 scene ------------------------------
    if with_candidates and ref_scene is not None and ref_assets is not None:
        for group in dead.values():
            value = group.get("value")
            if value is None:
                continue
            old = ref_scene.get(value)
            if old is None:
                continue
            old_name = constants.primary_name(value)
            candidates = []
            for cand in scene.controls:
                names = constants.names_for(cand.id)
                cand_name = (
                    constants.primary_name(cand.id)
                    or unmapped.get(cand.id, {}).get("name", "")
                )
                scored = _candidate_score(
                    old, cand, assets, referenced, old_name, cand_name
                )
                if scored is None:
                    continue
                score, dist = scored
                candidates.append(
                    {
                        "id": cand.id,
                        "type": cand.type_name,
                        "rect": [cand.x, cand.y, cand.w, cand.h],
                        "score": score,
                        "distance": dist,
                        "used": cand.id in referenced,
                        "name": cand_name or None,
                        "texture_file": assets.set_basename(cand.tex),
                    }
                )
            candidates.sort(key=lambda c: (-c["score"], c["distance"], c["id"]))
            group["candidates"] = candidates[:6]

    dead_references = sorted(
        dead.values(),
        key=lambda g: (-len(g["references"]), str(g.get("value")), g["token"]),
    )

    # Scene-side findings ---------------------------------------------------
    orphan_controls = []
    for c in scene.controls:
        if c.id == 0 or c.id in referenced:
            continue
        orphan_controls.append(_control_dict(c, constants, known))

    unnamed_controls = []
    for c in scene.controls:
        if c.id in constants.by_value:
            continue
        info = unmapped.get(c.id)
        item = _control_dict(c, constants, known)
        item["proposed_name"] = info.get("name") if info else None
        item["proposed_note"] = info.get("note") if info else None
        unnamed_controls.append(item)

    duplicate_constants = []
    referenced_values = referenced | {a.ref.value for a in ref_audits if a.ref.value is not None}
    for value, names in sorted(constants.duplicate_values().items()):
        if value not in ids and value not in referenced_values:
            continue
        duplicate_constants.append({"value": value, "names": names})

    texture_problems = []
    seen_textures = set()
    for c in scene.controls:
        if "tex" not in c.values:
            continue
        tex = c.tex
        if tex in seen_textures:
            continue
        seen_textures.add(tex)
        if -2 <= tex <= -1:
            continue
        info = assets.frame_info(tex, 0, decode=True)
        if info is None:
            texture_problems.append(
                {"id": c.id, "tex": tex, "message": "missing texture set %d" % abs(tex)}
            )
        elif info.get("solid") is None and info.get("image") is None:
            texture_problems.append(
                {
                    "id": c.id,
                    "tex": tex,
                    "message": "no drawable frame for set %s (%s)"
                    % (info.get("set"), info.get("file") or "?"),
                }
            )

    summary = {
        "references": len(scan.refs),
        "resolved_references": sum(1 for a in ref_audits if a.status == "resolved"),
        "dead_values": len(dead_references),
        "dead_references": sum(len(g["references"]) for g in dead_references),
        "type_mismatches": len(type_mismatches),
        "unknown_constants": len(unknown_constants),
        "computed_references": len(computed_references),
        "orphan_controls": len(orphan_controls),
        "unnamed_controls": len(unnamed_controls),
        "duplicate_values": len(duplicate_constants),
        "dead_values_with_candidates": sum(1 for g in dead_references if g["candidates"]),
        "texture_problems": len(texture_problems),
    }

    scene_summary = {
        "path": str(scene.path),
        "size": scene.path.stat().st_size if scene.path.exists() else 0,
        "controls": len(scene.controls),
        "unique_ids": len(ids),
        "duplicate_ids": scene.duplicate_ids,
        "types": scene.type_counts(),
        "bounds": list(scene.bounds()),
    }
    ref_scene_summary = None
    if ref_scene is not None:
        ref_scene_summary = {
            "path": str(ref_scene.path),
            "controls": len(ref_scene.controls),
            "unique_ids": len(ref_scene.ids),
        }

    return Audit(
        referenced_ids=referenced,
        generated=datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%SZ"),
        paths={
            "scene": str(scene.path),
            "reference_scene": str(ref_scene.path) if ref_scene else "",
            "constants": str(constants.path),
            "source": str(scan.path),
            "assets": str(assets.root),
        },
        scene_summary=scene_summary,
        ref_scene_summary=ref_scene_summary,
        summary=summary,
        ref_audits=ref_audits,
        dead_references=dead_references,
        type_mismatches=type_mismatches,
        computed_references=computed_references,
        orphan_controls=orphan_controls,
        unnamed_controls=unnamed_controls,
        duplicate_constants=duplicate_constants,
        texture_problems=texture_problems,
        scene_warnings=list(scene.warnings),
        asset_warnings=assets.unique_warnings(),
        source_warnings=list(scan.warnings),
    )


# ---------------------------------------------------------------------------
# Baseline helpers
# ---------------------------------------------------------------------------


def read_baseline(path: Path | str):
    p = Path(path)
    if not p.exists():
        return None
    data = json.loads(p.read_text(encoding="utf-8"))
    return set(data.get("keys", []))


def write_baseline(path: Path | str, audit: Audit):
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "version": 1,
        "generated": audit.generated,
        "keys": audit.baseline_keys(),
    }
    p.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def compare_baseline(audit: Audit, baseline_keys):
    current = set(audit.baseline_keys())
    base = set(baseline_keys or ())
    return sorted(current - base), sorted(base - current)
