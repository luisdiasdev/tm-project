# WYD `.pane` UI format (reverse-engineered)

This directory documents the `.pane` UI format used by the WYD client trio
`WYD.exe` (target SHA-256 `a058262d36631fd8b674603fb20d54ff18016b8f3dd38249250e953b8efc3c50`).

It is a **newer, text-based UI system** (`CGuiPanel`, `CGuiElement`,
`CGuiStatic`, `CGuiImage`, `CGuiButton`, …). It is not the same system as the
748-era `UI/*Scene*.bin` control records documented in
[`../port-748/assets-and-formats.md`](../port-748/assets-and-formats.md).

## What is here

| Path | Contents |
|---|---|
| [`01-container-and-crypto.md`](01-container-and-crypto.md) | file encryption, exact algorithm, round-trip proof |
| [`02-text-format.md`](02-text-format.md) | grammar, panel attributes, common element attributes, per-type attributes |
| [`03-widget-framework.md`](03-widget-framework.md) | class hierarchy, vtable map, lifecycle, geometry, event model |
| [`04-widget-reference.md`](04-widget-reference.md) | draw/input behaviour of each widget, recovered from `WYD.exe` |
| [`05-textures-and-guimat.md`](05-textures-and-guimat.md) | `Resource` guimat, texture aliases, `.wyt` / `.wyp` / `.wys` |
| [`06-reimplementation-notes.md`](06-reimplementation-notes.md) | practical guidance, gotchas, suggested architecture |
| [`reference/function-map.md`](reference/function-map.md) | addresses of every important function |
| [`reference/class-hierarchy.md`](reference/class-hierarchy.md) | recovered class names, vtables, inheritance, field offsets |
| [`reference/vtable-dump.md`](reference/vtable-dump.md) | exact per-class vtable override maps |
| [`reference/observed-keys.md`](reference/observed-keys.md) | key/type usage counts from all shipped panels |
| [`reference/decoded-panel-index.md`](reference/decoded-panel-index.md) | every panel, element count and element types |
| [`reference/analysis-provenance.md`](reference/analysis-provenance.md) | target hash, provider, method, limitations |
| [`reference/pseudocode/`](reference/pseudocode/) | Ghidra pseudocode for the key parsers, crypto and widget draws |
| [`reference/hexdumps/`](reference/hexdumps/) | raw encrypted/decrypted samples and round-trip report |
| [`tools/pane_codec.py`](tools/pane_codec.py) | standalone decode/encode/dump tool |
| [`tools/pane_json.py`](tools/pane_json.py) | parse `.pane` into ordered JSON and rebuild it |
| [`tools/pane_schema.json`](tools/pane_schema.json) | machine-readable key/type/enum reference |
| [`decoded-panels/`](decoded-panels/) | all 63 shipped `.pane` files plus the plaintext `.panel` |
| [`decoded-json/`](decoded-json/) | the same panels parsed into ordered JSON |

## Quick start

```bash
python tools/pane_codec.py dump   ../../.../UI/Panel/game/HUD.pane
python tools/pane_codec.py decode UI/Panel/game/HUD.pane HUD.txt
python tools/pane_codec.py encode HUD.txt HUD.pane
python tools/pane_json.py to-json UI/Panel/game/HUD.pane HUD.json
python tools/pane_json.py from-json HUD.json HUD.pane
```

`pane_json.py` accepts either an encrypted `.pane` or an already-decoded
UTF-16LE text file and detects the BOM.

## Verification status

- 63/63 shipped `.pane` files decrypt to valid UTF-16LE text with a BOM.
- `encrypt(decrypt(file)) == file` for all 63 files.
- All observed keys and element types are covered by the tables in
  [`02-text-format.md`](02-text-format.md).
- All 4 shipped `.wyp` files decrypt to PNG with the same cipher.
- All 270 shipped `.wyt` files begin with the `WT10` magic and carry an 8-byte
  TGA-footer prefix.

## Scope / caveats

- The analysis is static, from the shipped executable. No runtime tracing was
  needed to prove the container or the text grammar; widget drawing was read
  from the recovered draw functions.
- The widget reference names are ours (Ghidra has no PDB for these classes);
  class names come from MSVC RTTI strings.
- `.pane` values such as `Caption uu_210` are localization keys resolved by the
  UI text manager, not literal text. Literal text is also allowed.
- The `Animation` value is an inline JSON script passed to the animation
  manager; its schema is documented only as observed.
