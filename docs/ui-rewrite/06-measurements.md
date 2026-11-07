# 06 — Measurements and How to Reproduce Them

All numbers in this folder come from the current `test/748` tree and the
`client_748` assets. Nothing here is estimated unless marked.

Environment: Windows, PowerShell, Python 3.10, assets at
`E:\wyd\test-setup\client_748`.

## 1. Scene binaries

Parsed with the record sizes in `Projects/TMProject/UIBinary.h`.

| Scene | Bytes | Controls | Unique IDs | Roots | Max depth |
|---|---:|---:|---:|---:|---:|
| `SelServerScene2.bin` | 1,352 | 22 | 22 | 8 | 1 |
| `SelCharScene2.bin` | 4,820 | 85 | 84 | 7 | 1 |
| `FieldScene2.bin` | 35,404 | 709 | 708 | 48 | 3 |

Control type counts:

| Scene | PANEL | BUTTON | CHECK | LISTBOX | PROGRESS | TEXT | EDIT | 3DOBJ | GRID |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Server | 7 | 5 | 0 | 2 | 0 | 6 | 2 | 0 | 0 |
| Char | 17 | 11 | 0 | 1 | 0 | 53 | 3 | 0 | 0 |
| Field | 138 | 170 | 0 | 28 | 7 | 234 | 7 | 0 | 125 |

Duplicate IDs:

| Scene | Duplicate ID | Count |
|---|---:|---:|
| `SelCharScene2.bin` | 631 | 2 |
| `FieldScene2.bin` | 6192 | 2 |

Parent-before-child violations: **0** in all three scenes.

Negative texture sets are used heavily in the field scene (58 distinct values,
including `-455`/`-456` for the auto-attack buttons).

### Reproduce

```powershell
python - <<'PY'
import struct, collections, os
SIZE = {1:40,2:40,3:32,6:52,10:48,12:52,13:184,15:28,16:40}
NAME = {1:'PANEL',2:'BUTTON',3:'CHECKBOX',6:'LISTBOX',10:'PROGRESS',12:'TEXT',13:'EDIT',15:'3DOBJ',16:'GRID'}
UI = r'E:\wyd\test-setup\client_748\UI'
for f in ['SelServerScene2.bin','SelCharScene2.bin','FieldScene2.bin']:
    data = open(os.path.join(UI,f),'rb').read(); off=0; c=[]
    while off+4 <= len(data):
        ct = struct.unpack_from('<i',data,off)[0]; off += 4
        cid,par = struct.unpack_from('<ii',data,off)
        c.append((ct,cid,par)); off += SIZE[ct]
    assert off == len(data)
    print(f, len(c), dict(sorted(collections.Counter(NAME[t] for t,_,_ in c).items())))
    print('  dup ids', {k:v for k,v in collections.Counter(i for _,i,_ in c).items() if v>1})
PY
```

(PowerShell here-strings with Python are awkward; save the snippet to a `.py`
file if your shell mangles it.)

## 2. Source-reference audit

The existing audit output lives in `tools/fieldscene/out/`. Note: `tools/` is
present in the working tree but **untracked** (`git status` shows `?? tools/`),
and the `.py` sources are missing — only `__pycache__` and `out/` remain. Treat
the output as a snapshot, not as a reproducible tool until the sources are
restored.

From `fieldscene-audit.json` (generated 2026-11-07 20:37 UTC):

| Metric | Count |
|---|---:|
| Source references | 699 |
| Resolved references | 465 |
| Dead values (distinct IDs) | 127 |
| Dead references (occurrences) | 164 |
| …categorised 755-only | 99 |
| …categorised unresolved | 26 |
| …categorised dynamic | 2 |
| Type mismatches | 0 |
| Unknown constants | 0 |
| Computed references | 18 |
| Orphan controls (never referenced) | 308 |
| Unnamed scene IDs | 16 |
| Duplicate constant values (used) | 10 |
| Dead values with candidate replacements | 51 |
| Texture problems | 1 |

Scene-level:

- `FieldScene2.bin`: 709 controls, 708 unique IDs, duplicate `6192`.
- Bounds: `[-120, -60, 804, 600]`.
- 308 of 709 controls are never referenced by `TMFieldScene.cpp` (43%).
- 16 IDs have no name in `ResourceControl.h`: `318, 319, 802–811, 1857, 1862,
  1863, 1864`.
- Texture problem: ID `1377` uses set `65`, which has no drawable frame
  (`mesh/mt010101.wys`).
- Asset warning: set `198` (`chat_sel2`) declares `ItemCount: 5` but has 4
  coordinate rows.

Dead-reference categories are dominated by 755-only controls:

| Category | Occurrences |
|---|---:|
| 755-only | 99 |
| unresolved | 26 |
| dynamic (created at runtime) | 2 |

## 3. Header statistics

`Projects/TMProject/ResourceControl.h`:

| Metric | Count |
|---|---:|
| `constexpr auto` constants | 2,197 |
| `TM*`-prefixed (748 namespace) | 1,177 |
| Distinct values with >1 name | 15 |
| Extra names on duplicated values | 15 |

Examples of duplicate values: `306` (`TMB_PKBTN`/`TMP_TITLE_PANEL2`), `307`
(`TMB_NAMEBTN`/`TMP_TITLE_PANEL3`), `308` (`TMB_CAMERABTN`/`TMP_TITLE_PANEL4`),
`312` (`TMB_AUTOTARGETBTN`/`TMP_LOGO_PANEL2`).

### Reproduce

```powershell
cd tmproject
python -c "import re,collections; t=open('Projects/TMProject/ResourceControl.h',encoding='latin-1').read(); p=re.findall(r'constexpr auto (\w+) = (-?\d+);',t); c=collections.Counter(int(v) for _,v in p); print(len(p), sum(1 for n,_ in p if n.startswith('TM')), {k:v for k,v in c.items() if v>1})"
```

## 4. Source size and call counts

| File | Lines |
|---|---:|
| `TMFieldScene.cpp` | 24,638 |
| `TMFieldScene.h` | 855 |
| `TMSelectCharScene.cpp` | 2,237 |
| `TMSelectServerScene.cpp` | 1,554 |
| `TMScene.cpp` | 2,698 |
| `SControl.cpp` | 3,051 |
| `SControl.h` | 670 |
| `SGrid.cpp` | 5,332 |
| `SGrid.h` | 152 |
| `SControlContainer.cpp` | 385 |
| `SControlContainer.h` | 40 |
| `UIBinary.h` | 135 |
| `GeomObject.cpp` / `.h` | 73 / 49 |
| `TMFont2.cpp` / `.h` | 393 / 31 |
| `TMFont3.cpp` / `.h` | 399 / 43 |
| `TreeNode.cpp` / `.h` | 224 / 54 |
| `TextureManager.cpp` / `.h` | 1,570 / 133 |

Approximate totals:

- **UI engine**: ~11.0k lines (SControl, SGrid, container, tree, geometry,
  fonts, `UIBinary`).
- **Scene UI code**: ~12k lines (strictly UI functions; ~8.2k in
  `TMFieldScene.cpp`).
- **Total UI surface**: ~23k lines.

`TMFieldScene.cpp` function sizes (top):

| Function | Lines |
|---|---:|
| `OnControlEvent` | 3,521 |
| `OnPacketAttack` | 2,272 (gameplay) |
| `InitializeScene` | 1,632 |
| `SkillUse` | 1,571 (gameplay) |
| `FrameMove` | 1,517 |
| `AutoSkillUse` | 1,099 (gameplay) |
| `UpdateScoreUI` | 802 |
| `OnMsgBoxEvent` | 465 |

`TMFieldScene::InitializeScene` internals:

| Metric | Count |
|---|---:|
| `FindControl` | 375 |
| `if (m_p…)` null guards | 104 |
| `new` expressions | 61 |
| `SetVisible` | 64 |
| `SetPos`/`SetSize` | 39 |

Whole-file counts:

| Metric | `TMFieldScene` | `TMSelectCharScene` | `TMSelectServerScene` |
|---|---:|---:|---:|
| `FindControl` | 511 | 99 | 17 |
| …numeric literal | 273 | — | — |
| `SetVisible` | 621 | 84 | 44 |
| `SetText` | 248 | 54 | 0 |
| Pointer members in header | ~330 | ~90 | ~40 |

`OnControlEvent` structure (field):

| Metric | Count |
|---|---:|
| Top-level `if (idwControlID …)` branches | 139 |
| Branches with no packet/game/sound/grid logic | 91 |
| `switch` statements | 11 |
| `case` labels | 56 |
| Median branch length | 12 lines |
| Mean branch length | 25.2 lines |
| Longest branch | 448 lines |

## 5. JSON conversion size

A straight conversion of the three scenes (pretty-printed, `indent=1`) produces:

| Scene | Binary | JSON | Ratio |
|---|---:|---:|---:|
| Server select | 1,352 B | 5,110 B | 3.8× |
| Character select | 4,820 B | 21,319 B | 4.4× |
| Field | 35,404 B | 164,754 B | 4.7× |
| **Total** | **41,576 B** | **191,183 B** | **4.6×** |

Minified it is roughly half that. Parse cost is negligible for a one-time load.

## 6. Fixed limits

| Limit | Value | Location |
|---|---:|---|
| Draw layers | 30 | `SControlContainer.h:7` |
| Modal slots | 8 | `SControlContainer.h:39` |
| Listbox items | 1,000 | `SControl.h:560` |
| Grid items | 128 | `SGrid.h:100` |
| UI strings | 500 × 64 | `TMGlobal.cpp:30` |
| Message table | 2,000 × 128 | `Basedef.h:6` |
| Text length | 120 | `TMFont2.h:3` |
| UI texture sets | 600 | `TextureManager.h:3` |
| UI textures | 512 | `TextureManager.h:4` |

## 7. Verification commands already in the repo

```powershell
# inventory + EOF + missing texture sets + source refs
python docs\tools\ui_inventory.py --assets ..\client_748 --source-refs

# control-ID rewrite (dry run by default)
python docs\tools\rewrite_control_ids.py `
  --source Projects\TMProject\TMFieldScene.cpp `
  --scene-bin ..\client_748\UI\FieldScene2.bin

# restore non-UTF-8 bytes in TMFieldScene.cpp
python docs\tools\restore_non_utf8.py --file Projects\TMProject\TMFieldScene.cpp

# launch, click, capture
powershell -File docs\tools\client_drive.ps1 -Action launch -Restart -Out artifacts\01-serverselect.png
```

Build and run notes are in `docs/port-748/build-and-test.md`.

## 8. Confidence

| Claim | Confidence | Basis |
|---|---|---|
| Scene counts and sizes | High | parsed directly, exact EOF |
| Duplicate IDs | High | parsed directly |
| Reference/orphan counts | Medium-high | from the untracked audit snapshot; sources missing |
| Line and call counts | High | direct file measurement |
| Constant counts | High | regex over the header |
| JSON size | High | generated from the real bins |
| Effort estimates | Medium | work breakdown + comparable rewrites; not measured |
| Framework claims | High | RmlUi backend list and ImGui D3D9 backend verified from upstream docs (2026) |

## 9. Caveats

- `tools/fieldscene` is untracked and its `.py` sources are gone; the numbers in
  §2 are a snapshot. Restore or rewrite the tool before relying on it in CI.
- `TMFieldScene.cpp` is **not valid UTF-8** (13 raw bytes on 9 lines). Any
  measurement or edit must be byte-preserving.
- The 748 Brazilian client (`client_748_br`) has different scene sizes; do not
  mix numbers.
- `SelServerScene.bin` (non-UI2) is a stub with 5 controls plus a copyright
  string; it is not a usable baseline.
