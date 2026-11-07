# 05 — Target Format and Runtime Design

This is a concrete proposal, not a final spec. It is sized so that a converter
can be written first, validated against the three shipped scenes, and only then
used by the runtime.

## 1. File layout

```
ui/
  scenes/
    server-select.ui.json
    char-select.ui.json
    field.ui.json
  themes/
    wyd748.theme.json
  strings/
    en.ui.json
  actions.md            # documents the named handlers
```

- **Scene** files describe the control tree.
- **Theme** files hold reusable colors, fonts, and 9-slice definitions.
- **String** files hold text keyed by stable names.

Scenes may reference themes and string keys. A scene never references a
texture *file*; it references a texture **set index + frame index**, exactly as
today. That keeps the `.wyt` atlas pipeline and the 748 look.

## 2. Scene schema

### 2.1 Top level

```jsonc
{
  "format": "wyd.ui.scene",
  "version": 1,
  "name": "field",
  "designSize": [800, 600],
  "theme": "wyd748",
  "controls": [ /* ... */ ]
}
```

- `designSize` is the coordinate space of all `rect` values. The runtime scales
  from this to the window; no per-control ratio math.
- `version` gates migrations.

### 2.2 A control

Every node has:

| Field | Type | Required | Meaning |
|---|---|---|---|
| `type` | string | yes | `panel`, `button`, `checkbox`, `listbox`, `progress`, `text`, `edit`, `grid`, `obj3d` |
| `name` | string | yes | stable, unique within the scene; this is the binding key |
| `id` | int | no | legacy numeric ID, kept for `.bin` round-trip and for code not yet migrated |
| `rect` | `[x,y,w,h]` | yes | design-space position and size |
| `anchor` | string or object | no | `top-left` (default), `center`, `top-center`, `bottom-right`, `stretch-h`, `stretch-v`, `fill` |
| `z` | int | no | draw order among siblings; higher is on top; default = array index |
| `visible` | bool | no | initial visibility; default `true` |
| `enabled` | bool | no | whether it receives input; default `true` |
| `texSet` | int | no | texture set index; `-1` text/no fill, `-2` colored fill, `< -2` = `abs` |
| `frame` | int | no | frame index within the set; default 0 |
| `color` | int or string | no | ARGB integer or theme token |
| `style` | string | no | theme style reference |
| `tooltip` | string | no | text key |
| `events` | object | no | see §4 |
| `children` | array | no | nested controls |

Type-specific fields:

| Type | Fields |
|---|---|
| `panel` | `fill` (render mode), `pickable`, `drag` |
| `button` | `sound`, `text`, `selected` |
| `checkbox` | `checked` |
| `listbox` | `maxCount`, `visibleCount`, `select`, `scroll` |
| `progress` | `current`, `max`, `progressColor`, `style` |
| `text` | `text`, `align`, `border`, `borderColor`, `textType` |
| `edit` | `text`, `maxLength`, `password`, `align` |
| `grid` | `rows`, `cols`, `itemType` |
| `obj3d` | `objIndex` |

### 2.3 Real example (excerpt of the 748 field scene)

These are the actual first records of `FieldScene2.bin` (description panel and
system panel), converted by hand:

```json
{
  "format": "wyd.ui.scene",
  "version": 1,
  "name": "field",
  "designSize": [800, 600],
  "controls": [
    {
      "type": "panel",
      "name": "desc.panel",
      "id": 258,
      "rect": [0, 0, 240, 240],
      "texSet": -2,
      "color": "0x77000000",
      "fill": "stretch",
      "pickable": false,
      "children": [
        { "type": "text", "name": "desc.name", "id": 772,
          "rect": [0, 0, 240, 20], "texSet": -1,
          "text": "", "align": "center", "color": "0xFFFFFFAA",
          "border": 0, "borderColor": "0xFFAA8888", "textType": 0 },
        { "type": "text", "name": "desc.param1", "id": 773,
          "rect": [0, 20, 240, 16], "texSet": -1,
          "text": "", "align": "center", "color": "0xFFFFFFFF",
          "border": 0, "borderColor": "0xFFAA8888", "textType": 0 }
      ]
    },
    {
      "type": "panel",
      "name": "system.panel",
      "id": 632,
      "rect": [0, 0, 197, 91],
      "texSet": -258,
      "color": "0xFFFFFFFF",
      "fill": "stretch",
      "children": [
        { "type": "button", "name": "system.server", "id": 633,
          "rect": [19, 31, 70, 21], "texSet": -2,
          "color": "0x01010101", "sound": 1, "text": "UI:53",
          "events": { "onClick": [ { "action": "runHandler", "name": "System_Server" } ] } }
      ]
    }
  ]
}
```

Notes:

- `texSet: -258` is the existing encoding for "set 258"; the converter preserves
  it. A future schema could use `"texSet": 258, "mode": "image"` and translate
  at load, but keeping the raw value guarantees round-trip fidelity.
- `text: "UI:53"` is a string-key reference (today `nStringIndex = 53`). Inline
  text is allowed for one-offs.
- `z` is omitted because the array order is the draw order. The converter writes
  children in **back-to-front** order, which is the reverse of the current
  `prepend` convention. This removes the inversion trap (P2).

### 2.4 Ordering semantics

The runtime draws children **in array order**, first to last. The converter is
responsible for reversing the `.bin` sibling order exactly once. A round-trip
test proves it.

`z` is only needed when a control must leave its parent's array order, e.g. a
tooltip that belongs to one panel but must draw above another. Sorting is stable
by `(z, arrayIndex)`.

### 2.5 Anchors

The current system centers five hard-coded IDs (`SControl.cpp:228`). Replace
that with data:

```json
{ "name": "message.box", "anchor": "center", "rect": [0, 0, 256, 128] }
```

For a message box, `anchor: "center"` means: compute the position so the rect is
centered in the design area, then scale. No ID list.

Other useful anchors:

| Anchor | Behavior |
|---|---|
| `top-left` | `x,y` as-is (default) |
| `center` | centered in parent/design area |
| `top-center` | `x` centered, `y` as-is |
| `bottom-left` / `bottom-right` | pinned to the parent/design edge |
| `stretch-h` | `x` and `w` are margins; width fills the remaining space |
| `fill` | all four values are margins |

`stretch-h` alone removes most of the per-control width fixes in the message
panel and chat list.

## 3. Binding registry

### 3.1 Concept

```cpp
// in TMFieldScene::InitializeScene
m_pChatList   = ui.Bind<SListBox>("chat.list");
m_pChatInput  = ui.Bind<SEditableText>("chat.input");
m_pInvGrid    = ui.Bind<SGridControl>("inventory.grid");
```

- Lookup is by name, once, at init.
- `Bind<T>` checks the actual widget type and logs (or asserts in debug) on
  mismatch.
- Every unbound name is reported in a single startup summary:

  ```
  [UI] field: 412/412 bindings resolved
  [UI] field: 3 optional bindings missing: party.button, kingdom.text, pk.button
  ```

- Optional bindings use `BindOptional<T>` so 755-only features do not need a
  null guard at every use site.

### 3.2 Why this is better than IDs

| Today | Target |
|---|---|
| `(SGridControl*)FindControl(65554)` | `ui.Bind<SGridControl>("inventory.grid")` |
| wrong ID → nullptr → crash later | wrong name/type → startup error with the name |
| duplicate numeric ID → silent shadow | duplicate name → validator error |
| ID differs per client version | name is stable across 748/755 |
| 2,197-constant header | one scene file |

The numeric `id` can remain in the data during migration so old code still
works. `FindControl(id)` stays as a compatibility shim.

### 3.3 Code generation

The converter can emit a header and an init function:

```cpp
// generated: field.ui.h
struct FieldUi {
    SPanel* descPanel;
    SListBox* chatList;
    SGridControl* inventoryGrid;
    // ...
};
```

```cpp
// generated: field.ui.cpp
void BindFieldUi(UiScene& ui, FieldUi& out) {
    out.descPanel     = ui.Bind<SPanel>("desc.panel");
    out.chatList      = ui.Bind<SListBox>("chat.list");
    out.inventoryGrid = ui.Bind<SGridControl>("inventory.grid");
}
```

This is mechanical and safe: the compiler catches type errors, the runtime
catches missing names. It removes ~1,600 lines of `FindControl` + cast from
`TMFieldScene.cpp`.

Do **not** generate the gameplay logic. Generate only the handles and the tree.

## 4. Declarative actions

### 4.1 Schema

```jsonc
"events": {
  "onClick": [
    { "action": "toggle", "target": "inventory.panel" },
    { "action": "playSound", "id": 11 }
  ],
  "onMouseOver": [ { "action": "show", "target": "inventory.tooltip" } ],
  "onMouseOut":  [ { "action": "hide", "target": "inventory.tooltip" } ],
  "onOpen":  [ { "action": "runHandler", "name": "Inventory_Refresh" } ]
}
```

### 4.2 Action vocabulary (MVP)

| Action | Fields | Notes |
|---|---|---|
| `show` / `hide` / `toggle` | `target` | name of a control |
| `setText` | `target`, `value` (string or key) | |
| `setVisibleGroup` | `group`, `visible` | toggles a named group |
| `selectTab` | `group`, `index` | sets `selected` on siblings |
| `enable` / `disable` | `target` | |
| `playSound` | `id` | |
| `sendPacket` | `opcode`, `fields` | only fixed, UI-originated packets |
| `runHandler` | `name` | calls a registered C++ function |
| `closeModal` | — | pops the modal stack |
| `openScreen` | `name` | scene transition |

`runHandler` is the escape hatch. The C++ side registers:

```cpp
ui.RegisterAction("UseHealthPotion", [this] { UseHPotion(); });
```

This is how the remaining ~35% of complex handlers stay code without living in a
3,521-line function.

### 4.3 What not to put in JSON

- conditionals beyond a single `if` on a named flag;
- loops;
- item/inventory math;
- combat or skill logic;
- anything that needs server state.

If an action needs more than the vocabulary, write a handler.

## 5. Runtime loader

### 5.1 Parse

Use a vendored, header-only JSON parser. `nlohmann/json` is MIT and works with
C++17; `rapidjson` is faster but more verbose. Parse once per scene load. The
field scene is ~165 KB pretty-printed and parses in single-digit milliseconds.

### 5.2 Build

1. Resolve theme tokens and string keys.
2. Create widgets with the existing constructors (`new SPanel(...)`, …).
3. Attach by nesting, in array order.
4. Compute layout for the current resolution.
5. Register names in a hash map for `Bind<T>`.
6. Run the binding validation pass and log a single summary.

### 5.3 Keep the compatibility shim

During migration, `SControl::m_dwControlID` is still set from `id`, so
`FindControl(id)` and `OnControlEvent(idwControlID, …)` keep working. This lets
you migrate one screen at a time without touching gameplay code.

## 6. Validator

Runs in CI and at load (debug builds):

| Rule | Severity |
|---|---|
| unknown `type` | error |
| missing/duplicate `name` | error |
| duplicate `id` within a scene | error |
| `parent`/child cycle | error |
| `texSet` not present in `UITextureSetList.txt` (for `>= 0` and `< -2`) | error |
| `frame` out of range for the set | error |
| `anchor` unknown | error |
| `z` collision that changes draw order ambiguously | warning |
| `events` action unknown | error |
| `target` not found | error |
| string key not found | error |
| control never referenced by a binding and not marked `dynamic` | warning |
| nesting depth > configured limit | warning |

Output is a machine-readable report (JSON) plus a human summary. The existing
`tools/fieldscene` audit is a prototype of this.

## 7. Converter

```
tools/ui_json.py from-bin  --scene FieldScene2.bin --out field.ui.json
tools/ui_json.py to-bin    --scene field.ui.json --out FieldScene2.bin
tools/ui_json.py check     --scene field.ui.json --assets ../client_748
```

Requirements:

- `from-bin → to-bin` must be **byte-identical** to the input.
- `to-bin` must reproduce the current sibling order (reverse of array order) and
  the current parent-before-child order.
- The converter must preserve every field, including unknown ones, so no data is
  lost.
- It must run on all three shipped scenes and on the 755 `FieldScene2.bin` as a
  robustness test.

This converter is the single most important early deliverable: it proves the
schema is expressive enough before any runtime work starts.

## 8. Hot reload

- Watch the scene JSON, theme, and string files.
- On change: re-parse, validate, and rebuild the control subtree.
- Preserve state by name: visibility of user-toggled panels, scroll positions,
  focused edit, selected tab.
- If validation fails, keep the old tree and show the error in the inspector.
- Target: < 2 s from save to visible change.

A simpler first step is to hot-reload only layout/color/text on existing
controls (mutate in place) before supporting structural changes.

## 9. Editor (optional, later)

An editor is not required for the MVP. When it is built, the cheapest useful
version is a local web page:

- load the scene JSON and the atlas set list,
- render rectangles and atlas frames with `<canvas>`,
- tree view with drag/drop, z-order, rename,
- property panel driven by the schema,
- live validation,
- save back to JSON (stable formatting, one control per block).

Because the format is JSON and the schema is published, the editor is a
separate project that cannot block runtime work.

## 10. Migration mapping

| `.bin` field | JSON | Notes |
|---|---|---|
| `controlType` | `type` | enum → string |
| `nID` | `id` | kept for compatibility/round-trip |
| `nParentID` | nesting | converter builds `children` |
| `nTextureSetIndex` | `texSet` | raw value preserved |
| `nStartX/Y`, `nWidth/Height` | `rect` | design units |
| `nColor` | `color` | ARGB |
| `nFillType` | `fill` | `image`, `tile`, `stretch`, `text` |
| `nPickable` | `pickable` | |
| `nSound` | `sound` | |
| `nStringIndex` | `text` | `"UI:<n>"` key |
| `nFontColor` | `color` | |
| `nBorder`, `nBorderColor` | `border`, `borderColor` | |
| `nTextType` | `textType` | |
| `nAlignType` | `align` | `left`, `center`, `right`, `nomargin`, `battle` |
| `nMaxStringLength` | `maxLength` | |
| `nPassword` | `password` | |
| `szString` | `text` | inline |
| `nCurrent`, `nMaxValue` | `current`, `max` | |
| `nProgressColor` | `progressColor` | |
| `nStyle` | `style` | |
| `nRowCount`, `nColumnCount` | `rows`, `cols` | |
| `nType` (grid) | `itemType` | |
| `nMaxCount`, `nVisibleCount`, `nSelect`, `nScroll` | same names | |
| `n3DObjIndex` | `objIndex` | |

Anything not in this table is preserved verbatim under `"legacy": { ... }` so
the round-trip is lossless.
