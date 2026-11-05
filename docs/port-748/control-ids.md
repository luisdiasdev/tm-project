# Control IDs and the `TM*` Namespace

## Two namespaces in one header

`Projects/TMProject/ResourceControl.h` contains both:

- **plain names** (`P_CHAT`, `B_CHAR`, `T_LOGIN_BOX_TEXT`, ...) — 755/760 IDs,
  mostly `65537+`, `901xx`, `1054xxx`, `7602xxx`.
- **`TM*` names** (`TMP_CHAT`, `TMB_CHAR`, `TMT_LOGIN_BOX_TEXT`, ...) — the
  older namespace used by 748. Examples:

| Logical control | 755 constant = value | 748 constant = value |
|---|---|---:|
| Chat panel | `P_CHAT` = 65670 | `TMP_CHAT` = 5739 |
| Character window | `B_CHAR` = 65790 | `TMB_CHAR` = 293 |
| Equipment | `B_EQUIP` = 65791 | `TMB_EQUIP` = 294 |
| Inventory grid | `G_INV_GRID` = 65554 | `TMG_INV_GRID` = 337 |
| Cargo grid | `G_CARGO_GRID` = 65690 | `TMG_CARGO_GRID` = 1830 |
| Quest panel | `P_QUEST_PANEL` = 1054256 | `TMP_QUEST_PANEL` = 320 |
| Chat list | `L_CHATLIST` = 65667 | `TML_CHATLIST` = 5377 |
| Input gold edit | `E_INPUT_GOLD` = 65889 | `TME_INPUT_GOLD` = 627 |

The `TM` prefix is a useful heuristic, not a guarantee: ~900 `TM*` names have
no plain counterpart and ~743 plain names have no `TM*` counterpart. Always
verify the candidate value exists in the target scene binary **and** has the
expected control type before using it.

## What was changed

`TMFieldScene.cpp` had **264 of 453** numeric `FindControl()` calls using
`>= 65536` IDs that do not exist in `FieldScene2.bin`. A scripted pass replaced:

- 198 numeric `FindControl(...)`/`idwControlID == ...`/`case ...` values
- 31 symbolic plain constants (e.g. `B_CHAR`, `E_INPUT_GOLD`)

with the `TM*` names whose values are present in the 748 field scene.
**229 bindings total.** A cast/type audit against the scene record type reported
**0 mismatches**.

The rewriter only changes a reference when:

1. the old value is absent from the target scene, and
2. a `TM` + name constant exists, and
3. its value is present in the target scene.

This avoids the trap where a `TM` alias value exists in the scene but means a
different control there (e.g. 65889 → 627 is valid in `FieldScene2`, but 627 in
`SelCharScene2` is a different control).

## Remaining field bindings

`docs/port-748/reference/missing-source-bindings.txt` contains the live list
(118 references at the time of writing). Categories:

- **Dynamically created controls** — their IDs are not supposed to be in the
  scene. Examples: the message box (`4617`), IME button (`4628`), created
  grids/panels.
- **755-only features** — kingdom UI (`131072..131074`), chat-select panel
  (`90113`, `90114`, `90128`, `90129`), PK button (`65786`), auto-run (`316`),
  system message list (`5695`), auto-skill children (`65775..65784`), party
  auto (`7602193..7602196`), daily quest (`1054278`), item-mix pages, cargo/
  inventory page buttons (`67072..67080`, `67332..67334`).
- **Unmapped scene IDs** — `FieldScene2.bin` contains 16 IDs not present in
  `ResourceControl.h`: `318, 319, 802..811, 1857, 1862, 1863, 1864`. The file is
  correct; the header simply lacks names for them.

Controls that do not exist in 748 are now **null-guarded** in
`TMFieldScene::InitializeScene` and `SetAutoSkillNum`. Do not re-add a random
binding just to remove the null; find the 748 equivalent or leave the feature
disabled.

## Character select

`TMSelectCharScene.cpp` already mostly uses 748 IDs. The audit reports a few
missing references, which are all dynamic or 755-only:

| ID | Meaning | Status |
|---:|---|---|
| `4617` | message box | created at runtime by `TMScene` |
| `4628` | IME button | 755-only |
| `65886/65887` | account-lock OK/Cancel | 755-only |
| `66433..66435` | char-select extras | not in 748 scene |

These paths must be null-guarded if the screens are exercised.

## Audit command

```powershell
python docs\tools\ui_inventory.py --assets ..\client_748 --source-refs
```

It prints, per scene: record counts, exact-EOF status, IDs missing from
`ResourceControl.h`, missing texture sets, and source references whose IDs are
not present.
