# FieldScene Inspector

Self-contained, read-only tooling for `client_748/UI/FieldScene2.bin`.

It exists to answer one question quickly and repeatably:

> For every control in the 748 field scene, does the port actually look it up —
> and for every id the port looks up, does that control exist, with the right
> type and the right neighbours?

Nothing outside `tools/fieldscene/` is created or modified.  The tool only
reads:

- `../client_748/UI/FieldScene2.bin`, `UITextureList.bin`, `UITextureSetList.txt`,
  `UIString.txt` and `UI/*.wyt`
- `../client/UI/FieldScene2.bin` (755 reference, used for candidate matching)
- `Projects/TMProject/ResourceControl.h`
- `Projects/TMProject/TMFieldScene.cpp`

## Quick start

Run from the repository root:

```powershell
# human + machine report, with problem counts
python tools\fieldscene\fieldscene.py audit

# textured layout snapshot (real WYT art, real rects)
python tools\fieldscene\fieldscene.py render --png tools\fieldscene\out\layout.png --labels ids

# overlay the scene on a captured client screenshot
python tools\fieldscene\fieldscene.py overlay `
  --screenshot artifacts\field-verified.png `
  --png tools\fieldscene\out\overlay.png `
  --mode both --labels ids

# interactive viewer (opens http://127.0.0.1:<random>/)
python tools\fieldscene\fieldscene.py serve
```

`audit` writes `tools/fieldscene/out/fieldscene-audit.{md,json}`.
The viewer is read-only: no request can write a file.

## What the audit reports

| Category | Meaning |
|---|---|
| `resolved` | The id exists in the scene and the cast type matches the record type. |
| `unresolved` | The id is absent and is not in the curated known-absent list. |
| `755-only` | Documented as absent from 748 (`data/known_bindings.json`). |
| `dynamic` | Created at runtime (message box, generic input dialog). |
| `computed` | `FindControl(i + 65574)` style ranges; expanded with the enclosing loop bound. |
| `variable` | `FindControl(someLocal)`; not a constant, reported but not an error. |
| `type-mismatch` | Id exists but the scene record type disagrees with the cast. |

Inverse findings:

- **orphan controls** — scene ids no source reference points at.  These are the
  controls that exist but are not wired.
- **unnamed ids** — scene ids with no constant in `ResourceControl.h`
  (currently 16: `318/319`, `802..811`, `1857`, `1862..1864`).
- **candidate matches** — for a dead 755/760 id, the closest 748 controls,
  ranked by type, header-name similarity, WYT file family, size, string index
  and screen position.  Candidates are hints, never applied automatically.

The 755 reference scene is used only for these candidates.  `--no-reference`
skips it; `--no-candidates` keeps a faster audit.

## Baseline

`data/baseline.json` records the current set of problems.  A normal `audit`
prints only **new** and **fixed** keys, which makes regressions visible:

```powershell
python tools\fieldscene\fieldscene.py audit --fail-on-new
python tools\fieldscene\fieldscene.py audit --write-baseline   # after a real fix
```

## Viewer

- Canvas at the authored 800×600 size, wheel zoom, drag pan, click to inspect.
- Filters for problems, orphans, unnamed ids, referenced controls and each
  control type.
- **Dead refs** tab: every unresolved id, its call sites (`file:line`,
  enclosing function, snippet) and ranked 748 candidates.  Clicking a candidate
  selects and focuses it.
- **Problems** tab: type mismatches, computed ranges, unnamed ids, duplicate
  constant values, texture warnings and the orphan list.
- Status colours: green resolved, blue unbound/orphan, orange unnamed,
  red type mismatch.

Keyboard: `F` fit, `L` labels, `O` outlines, `/` search, `Esc` clear.

## Rendering caveats

- Layouts are drawn in file order with absolute scene coordinates, which is
  what the client uses for these controls.  Panels hidden at runtime still
  appear in the snapshot; the viewer has filters and the client screenshot
  overlay for context.
- `.wyt` is `WT10` + a TGA image; 24/32-bit frames decode exactly.  `.wys`
  (DDS/DXT) is not decoded because exactly one field control uses it (id
  `1377`); it renders as a placeholder and produces one asset warning.
- Text in the offline PNG uses a 5×7 bitmap font; the browser viewer uses the
  system font and the real `UIString.txt` values.

## Tests

```powershell
python -m unittest discover -s tools\fieldscene\tests -t tools -v
```

The suite covers byte-identical scene round-trips, the 755 reference parse,
non-UTF-8 source preservation, computed-range detection, WYT decoding, PNG
round-trips and the current audit invariants.

## Layout

```
fieldscene.py      launcher
cli.py             audit / render / overlay / serve / inspect
scene.py           binary parse, validation, byte-preserving write
assets.py          UITextureList.bin, UITextureSetList.txt, WYT/TGA decode
controls.py        ResourceControl.h parsing and duplicate-value detection
source.py          TMFieldScene.cpp scan (latin-1)
audit.py           classification, 755↔748 candidates, baseline diff
render.py          layout composition and screenshot overlay
server.py          local HTTP API
web/               vanilla canvas viewer
data/              curated known-absent list + audit baseline
out/               generated reports and images (git-ignored)
```
