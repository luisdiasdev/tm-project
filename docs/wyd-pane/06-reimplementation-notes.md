# 06 — Reimplementation notes

This is a practical checklist for a clean reimplementation of the `.pane`
runtime, independent of the original C++.

## 1. Suggested pipeline

```text
read .pane
  -> decrypt
  -> verify UTF-16LE BOM
  -> decode to string
  -> split into Panel + element blocks
  -> resolve inheritance/fallbacks
  -> build widget tree
  -> resolve guimat Resource
  -> load texture aliases and font list
  -> initial layout
  -> per-frame tick + draw
```

Keep the **on-disk representation** (encrypted UTF-16 text) and the **runtime
model** separate. A parser should not care whether the text came from a file or
an editor.

## 2. Data model

Use an ordered attribute list per block, not a dictionary. This preserves:

- key order when re-saving;
- unknown keys (important for forward compatibility);
- duplicate keys (rare but possible).

Suggested JSON shape (this is what `tools/pane_json.py` emits):

```json
{
  "panel": {
    "attributes": [
      { "key": "Resource", "value": "ui/default.guimat" },
      { "key": "AlignPanelX", "value": "Center" },
      { "key": "Width", "value": "259" }
    ]
  },
  "elements": [
    {
      "type": "Image",
      "name": "bg",
      "attributes": [
        { "key": "Area", "value": "0 0 500 25" },
        { "key": "TextureAlias", "value": "UI.NoticeBG" }
      ]
    }
  ]
}
```

A runtime can then convert attributes into typed fields, while a serializer can
round-trip anything it does not understand.

## 3. Geometry

- Store the **local** rect (`left, top, width, height`) on every element.
- Compute the **absolute** rect from the parent panel and the local rect on
  every layout pass.
- The original recomputes the absolute rect in vtable `+0x70`; hit testing uses
  the absolute rect.
- `Area` is `l t w h`; `Rect` is `l t r b`.
- Panel anchors:
  - `AlignPanelX/Y`: `NONE` = use coordinates as-is; `Center` = center in the
    screen/design area; `SIDE` = snap to an edge.
  - `OffsetX/Y` are applied after anchoring.
  - `ScreenAlign` flags supplement edge alignment.
- Scale from a fixed design resolution (the original appears to use 800×600
  coordinates) to the window; do not run the original's per-control ratio math.

## 4. Draw order

- Children are appended in file order.
- `ZOrder` changes the panel's order relative to other panels.
- `BelowPanel` draws an element under the panel background.
- The original’s `split` raster mode is a 9-slice. Implement it first; most
  widgets use it.

## 5. Text

- Treat `uu_*` values as localization keys.
- `Caption` and `Text` both set text but on different widget members; keep two
  fields if you want byte-compatible behaviour.
- `FontAlign` is a token list; parse each token and OR the flags:
  - horizontal: left=0, center=1, right=2
  - vertical: top=0, vcenter=4, bottom=8
  - `wordbreak`=0x10
- `FontOffset` is a pixel nudge applied before drawing.
- `ShadowType` supports `None`, `Outline`, `BottomRight`, `Top`.
- `RichText` enables the text renderer's markup path; if you do not implement
  markup, strip the tags or render them literally behind a flag.

## 6. Input

- Maintain a global pressed element and a focused element.
- Hit test in reverse draw order (topmost first).
- Buttons capture the mouse on press and cancel if released outside.
- `ok`/`cancel` button names map to Enter/Esc.
- `Group` on buttons/check boxes implements radio/tab exclusivity.
- Edit boxes need a caret, selection anchor, scroll offset and IME composition
  buffer.
- `MouseMoveLock` on sliders allows dragging outside the track.
- `MsgReceive false` should make an element transparent to mouse events.

## 7. Texture slots

Implement the slot model literally:

```text
slot[n].alias
slot[n].rasterOp
slot[n].font
slot[n].fontAlign
slot[n].fontOffset
slot[n].textureColor[4 states]
slot[n].fontColor[4 states]
```

Then:

- `TextureAlias` (no index) = apply to slot 0 and all unset slots.
- `TextureColor` (no index) = apply to all slots.
- `Font` (no index) = apply to all slots.
- `FontAlign` / `FontOffset` (no index) = apply to all slots.
- State-specific keys set one of the 4 state colors.

This is simpler than special-casing each widget and matches the original.

## 8. Aliases and atlas rects

Resolve aliases through the guimat:

```text
alias -> (textureIndex, x1, y1, x2, y2)
```

Cache the resolved texture and source rect in the slot. If the alias is empty,
draw nothing. If `ExternalImageFile` is set on an `Image`, bypass the alias.

## 9. Validation

Implement a validator early; it saves hours:

- unknown element type → error
- unknown key → warning (keep it, do not drop it)
- `Area`/`Rect` with the wrong number of values → error
- color not 4 integers → error
- boolean not `true`/`false` → error
- `TextureAlias` not found in the guimat → warning/placeholder
- `itemPanel` path not found → warning
- `Animation` not valid JSON → warning
- `Panel` missing → error
- element outside the panel rect → warning (common in shipped files, so not an
  error)

## 10. Round-trip testing

- `decrypt(encrypt(x)) == x` and `encrypt(decrypt(x)) == x`.
- Parse every shipped file and re-emit it; compare normalized text (ignore
  comments/whitespace if your parser drops them).
- Verify every key in every shipped file is known to the schema.
- Render every panel in a test harness and compare bounding boxes with the
  original screenshots where available.

## 11. Migration from the 748 UI

The 748 `UI/*Scene*.bin` controls and the `.pane` system are different:

| 748 `.bin` | `.pane` |
|---|---|
| fixed binary records | encrypted UTF-16 text |
| `UITextureSetList.txt` set/frame indices | guimat `TextureAlias` names |
| `SControl` / `SGrid` classes | `CGuiElement` hierarchy |
| scene file per screen | panel file per window/dialog |
| code-built server window | text-built panels |

A port can support both, but keep the loaders and texture resolvers separate.

## 12. Minimal implementation order

1. Decrypt + decode + parse `Panel`/`element` blocks.
2. Build a widget tree with `Area`.
3. Resolve `Resource` and draw `Image` with `TextureAlias` and `RasterOp=stretch`.
4. Add `split` (9-slice).
5. Add `Static` text with `FontAlign` and colors.
6. Add `Button` state textures and click dispatch.
7. Add `EditBox` text/caret/selection.
8. Add `ListBox`/`ListView` with `itemPanel`.
9. Add `GaugeBar`, `Slider`, `ComboBox`, `CheckBox`.
10. Add animation (`Image` sheet animation and panel `Animation` JSON).

This order matches the dependency graph of the shipped panels and gives
visible results early.
