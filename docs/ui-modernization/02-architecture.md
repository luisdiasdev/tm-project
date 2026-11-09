# 02 — Target architecture

## 1. Layering

Add a `Pane` layer beside `SControl`; do not modify the field scene's control
tree or remove the `.bin` loader.

```text
                       ┌─────────────────────────────────────────────┐
                       │                    TMScene                  │
                       │  InitializeScene / FrameMove / Cleanup      │
                       │  OnMouseEvent / OnKeyDown / OnChar          │
                       └───────────────┬──────────────┬──────────────┘
                                       │              │
                     legacy input/draw │              │ pane input/draw
                                       ▼              ▼
             ┌────────────────────────────┐   ┌───────────────────────────┐
             │ SControlContainer          │   │ GuiManager                │
             │  .bin tree, grids, modals  │   │  panel registry, z-order  │
             │  GeomControl draw lists    │   │  modal stack, focus       │
             └──────────────┬─────────────┘   │  input router             │
                            │                 └────────────┬──────────────┘
                            │                              │
                            ▼                              ▼
             ┌────────────────────────────┐   ┌───────────────────────────┐
             │ ObjectManager::RenderControl│  │ GuiPanel::Draw             │
             │  layers 0..29              │   │  GuiElement tree            │
             │  RenderGeomControl          │   │  GuiRenderer (quads/text)  │
             └──────────────┬─────────────┘   └────────────┬──────────────┘
                            │                              │
                            └──────────┬───────────────────┘
                                       ▼
                              RenderDevice (D3D9)
                              TextureManager / GuiTextureCache
```

Verified in the 2026 binary: the scene factory instantiates `TMFieldScene`,
`TMSelectCharScene` and `TMSelectServerScene`; `TMFieldScene::InitializeScene`
loads `UI\FieldScene2.txt` and `ReadRCBin` loads **1,354** SControl records.
The pane layer is additive; the classic layer is live. The design must not
assume the `.bin` tree is dead data.

Three rules keep this sane:

1. **Pane panels are leaves of the frame composition, not children of the
   SControl tree.** They draw after legacy layers and receive input first when
   visible.
2. **A pane that replaces a classic window must hide/disable the matching
   SControl subtree.** Otherwise both layers draw and hit-test. This is a
   first-class integration task, not a cleanup detail (see U17).
3. **No gameplay code in `GuiElement`.** Panels (thin `GuiPanel` subclasses)
   translate element events into calls on the scene; elements only know
   visibility, geometry, slots, text and input states.

## 2. Modules

Proposed new files under `Projects/TMProject/Pane/`. Names mirror the 2026
classes so REA comparisons are easy.

| Module | Responsibility | Rough LOC |
|---|---|---:|
| `PaneFile.cpp/.h` | read file, delta decrypt, UTF-16 BOM check, line tokenizer | 350 |
| `PaneDocument.cpp/.h` | ordered attribute lists, `Panel` + element blocks, comments, round-trip | 450 |
| `PaneSchema.cpp/.h` | key typing, enums, defaults, structured validation, unknown-key report | 700 |
| `Guimat.cpp/.h` | `FontList` / `TextureList` / `TextureAlias` parse + alias lookup | 450 |
| `GuiTextureCache.cpp/.h` | path-keyed texture loading (`.wyt`/`.wys`/`.wyp`), color key, device loss, lazy load | 500 |
| `GuiFont.cpp/.h` | guimat font index → D3DXFont/GDI font, UTF-16, alignment, wrapping, shadow | 700 |
| `GuiRenderer.cpp/.h` | src-rect quad, tint, wrap/stretch/split/wSplitStretch, clip, rotation | 700 |
| `GuiElement.cpp/.h` | base: rect, absolute rect, visibility, enabled, msgreceive, name/id, slots | 900 |
| `GuiStatic.cpp/.h` | caption/text, multiline, shadow, rich-text flag | 350 |
| `GuiImage.cpp/.h` | slot 0, external file, sheet animation, rotation | 400 |
| `GuiButton.cpp/.h` | states, groups, toggle, ok/cancel, sounds, tooltip | 700 |
| `GuiCheckBox.cpp/.h` | checked state, radio group | 250 |
| `GuiIconButton.cpp/.h` | icon atlas/slot API used by item/skill/buff controllers | 350 |
| `GuiGaugeBar.cpp/.h` | range/pos/direction/outspace/text type/flash | 450 |
| `GuiEditBox.cpp/.h` | text, caret, selection, clipboard, password, IME composition | 1,100 |
| `GuiListView.cpp/.h` | item vector, scrollbar, selection, `itemPanel` rows, grid arrangement | 1,200 |
| `GuiImeNotePad.cpp/.h` | multiline IME editor (guild notice) | 350 |
| `GuiPanel.cpp/.h` | panel attributes, anchors, titlebar, close button, caption, page index, animation | 800 |
| `GuiManager.cpp/.h` | registry by path/name, open/close, modal stack, z-order, draw pass, input dispatch | 700 |
| `PaneBindings.generated.h` | element-name constants + compile-time manifest (generated) | generated |
| `PaneIntegration.cpp` | scene hooks, legacy bridge, localization, deploy checks | 500 |

Total ~12,000 lines of new C++, plus ~1,000 lines of Python tooling. That is
the basis for the 18–30 pw engine estimate.

## 3. Data model

Follow the docs: **ordered attribute list, not a dictionary**.

```cpp
struct PaneAttr { std::wstring key; std::wstring value; int line; };

struct PaneElementDef {
    std::wstring type;          // Static, Image, Button, ...
    std::wstring name;          // "Inven0", "ChatEditBox", ""
    std::vector<PaneAttr> attrs; // file order preserved
    int line;
};

struct PaneFile {
    std::vector<PaneAttr> panelAttrs;
    std::vector<PaneElementDef> elements;
    std::vector<std::wstring> comments;
    std::wstring sourcePath;
};
```

Then a `PaneSchema` pass converts strings to typed fields and **keeps** the raw
attrs for round-trip and forward compatibility. Unknown keys are warnings, not
errors; unknown types and malformed values are errors.

The Python tools already emit this exact shape
(`docs/wyd-pane/decoded-json/*.json`). Use them as the test oracle:
`pane_json.py to-json` → parse in C++ → re-emit → compare normalized JSON.

## 4. Rendering

`RenderDevice::RenderRectTex` already takes a source rect, destination size,
tint (`DWORD` ARGB), alpha flag, angle and scale. Wrap it rather than write a new
pipeline:

```cpp
void GuiRenderer::DrawQuad(const GuiTexture& tex, const RECT& src,
                           const RECT& dst, DWORD argb, int rasterOp);
```

- `stretch`: one quad.
- `split`: nine quads from the source rect. **Margin policy is an open question**
  (see risks). Start with a per-call configurable margin (default: corners =
  min(w,h)/3), compare against reference captures, then fix the constant.
- `wSplitStretch`: three columns; middle stretched vertically.
- `wrap`: tile with UV modulo; clamp at edges.
- `rotation`: one quad through `RenderRectTex`'s angle path.
- `clip`: `SetScissorRect` around list viewports and text boxes. Restore after.

Batching is not required for 1,300 elements. If draw calls become an issue,
batch consecutive quads sharing a texture later; do not block v1 on it.

Render order (important: the cursor is legacy layer 29):

1. legacy `ObjectManager::RenderControl()` layers 0..28;
2. pane panels sorted by `ZOrder`, with each panel's `BelowPanel` elements
   drawn before its background and the rest after;
3. legacy layer 29 (the cursor) and any legacy always-on-top overlays.

If panes are drawn after the whole 0..29 loop, the mouse cursor disappears
behind panel windows. Split the loop or draw the cursor explicitly after the
pane pass.

`GuiManager` owns a sorted panel list; `ZOrder` changes re-sort on the next
frame. Do not try to encode panes as layers in the 30-layer legacy list.

## 5. Text

Create one `GuiFont` per guimat font entry (index → face, size, bold, italic).
Use `D3DXCreateFontW` (already linked) and draw UTF-16.

- Alignment: horizontal left/center/right, vertical top/vcenter/bottom,
  `wordbreak` → `DT_WORDBREAK`.
- `MultiLine` → `DT_WORDBREAK`; `MultiLineGap` → line spacing.
- Shadow modes: `None`; `Outline` (8 offset draws or a 1-px outline pass);
  `BottomRight` (single offset); `Top` (single offset).
- `FontOffset` applied before draw.
- `uu_*` values resolved by `PaneStrings` (separate table from 748).
- `RichText`: v1 strips tags or renders literally; add markup only if a migrated
  screen visibly needs it (12 statics, mostly item/quest text).
- IME: reuse the existing `imm32` setup from `TMScene`/`SEditableText`; the pane
  edit box needs composition, candidate placement and focus reporting.

Keep `TMFont2` for `SControl`; do not change its behaviour.

## 6. Input and modality

A single router in `GuiManager`, called from the scene before the legacy
container:

```cpp
// TMScene::OnMouseEvent
if (g_pGuiManager && g_pGuiManager->OnMouseEvent(dwFlags, wParam, x, y))
    return 1;                        // pane consumed the event
return SControlContainer::OnMouseEvent(...);   // legacy
```

Rules:

- Panels are tested topmost-first by `ZOrder`; within a panel, children in
  reverse draw order.
- `MsgReceive false` and `visible false` elements are transparent to input.
- A pressed button captures the mouse until release; releasing outside cancels.
- `GuiManager` owns keyboard focus. When a pane edit box has focus, legacy
  `SetFocusedControl` is cleared, and vice versa. This is the main source of
  "typing goes to the wrong box" bugs — make it explicit and log it.
- Modal stack: `GuiManager` has a stack, not eight fixed slots. While a modal
  exists, non-modal panels and legacy controls receive no mouse/key input.
- `ok`/`cancel` names map to Enter/Esc inside the focused modal only.
- Mouse wheel goes to the element under the cursor (ListView/EditBox), then to
  the legacy path if unconsumed.
- Cursor: keep the existing `SCursor`; pane elements only change its image when
  they need to (buttons/list rows).
- Route **all** input entry points, not just mouse/down: `OnKeyDownEvent`,
  `OnKeyUpEvent`, `OnCharEvent`, `OnIMEEvent`, `OnChangeIME`. If the pane owns
  focus, these must not reach `SControlContainer`, or the legacy IME path will
  steal composition from a pane edit box.

## 7. Binding model

Bind by **element name**, not ID:

```cpp
class PanelInventory : public GuiPanel {
    GuiIconButton* m_pInven[80];
    GuiStatic*     m_pMoney;
    GuiButton*     m_pTab[4];
public:
    bool OnInit() override {
        m_pMoney = FindStatic(L"Money");
        for (int i = 0; i < 80; ++i)
            m_pInven[i] = FindIconButton(L"Inven" + std::to_wstring(i));
        return ReportMissing();       // logs every missing binding once
    }
    void OnElementEvent(GuiElement* e, GuiEvent ev) override;
};
```

Generate `PaneBindings.generated.h` from the decoded JSON:

```cpp
namespace PaneNames::Inventory {
    constexpr auto Money = L"Money";
    constexpr auto Inven0 = L"Inven0";
    // ...
}
```

The generator also emits a report of duplicate element names per panel, so
`Find*` can return the first match and log the ambiguity at load time. The
shipped panels currently have none, but the validator should keep checking
because a future re-export or hand edit can introduce one.

Avoid a generic `map<string, GuiElement*>` as the primary API. Typed accessors
catch wrong-type binds at startup, which is exactly the failure the 748 UI
punishes you for.

## 8. Panel lifecycle and scenes

`GuiManager` is global (`g_pGuiManager`) but panels belong to a scene. Use the
project's own `g_UIVer` (already forced to 2 in `NewApp.cpp`) as **our**
legacy-vs-pane selector, so a failing screen can fall back without a rebuild.
This is our design choice; it is not proven that the 2026 client uses the same
variable (its selector is still open — U17).

```cpp
// TMScene::InitializeScene, after legacy LoadRC and control setup
g_pGuiManager->BeginScene(m_eSceneType);

// If this screen is pane-only (e.g. login), suppress the classic window:
if (UsePanesFor(ESCENE_SELECT_SERVER))
    HideLegacyControls(/* login panel + edit boxes + server lists */);

// TMSelectServerScene
m_pLoginPanel = g_pGuiManager->Open<PanelLogin>("ui2026/UI/Panel/login/GameLogin.pane");

// TMScene::FrameMove
g_pGuiManager->Tick(dwServerTime);

// ObjectManager::RenderControl: legacy 0..28, panes, legacy 29 (cursor)
g_pGuiManager->Draw();

// TMScene::Cleanup
g_pGuiManager->EndScene();
```

Panels are created from a path, parsed once, and cached as a `PaneFile`.
Instances share the parsed document and own only runtime state (values, icon
indices, animation timers). This makes opening the same panel twice cheap and
lets the validator run once at startup.

## 9. Legacy bridge

Two bridges, both temporary:

### 9.1 Hosting a legacy `SControl` inside a pane

For item windows in S5 that still need `SGridControl`:

```cpp
class GuiLegacyHost : public GuiElement {
    SControl* m_pLegacy;    // subtree, not in the scene tree
public:
    void Draw() override;   // translate rect, render the subtree's GeomControls
    bool OnMouse(...) override; // forward with local coordinates
};
```

Use it only for trade/shop/mix if their native port slips. Do not build the
inventory on it long-term: the grid art is 748 and will not match the new frame.

### 9.2 Calling scene code from a panel

Panels should call small, named methods on the scene, e.g.
`TMFieldScene::OpenInventory()`, not reach into the 3,500-line
`OnControlEvent` with synthetic control IDs. Where a legacy handler already
exists, add a thin named wrapper; do not duplicate the logic.

Keep the existing numeric-ID path intact for the legacy screens. A pane and a
legacy control may represent the same action; the pane panel is the only one
active when the new UI is on.

## 10. Build and deployment

- Add `Pane/*.cpp` to `TMProject.vcxproj` and `.filters`; they use `pch.h`.
- The project is C++17, toolset v143, x86 output
  `../client_748/TMProject_d.exe`. `wchar_t` is native; use explicit `W` APIs.
- Do not put non-ASCII literals in `.cpp`; all localized/Korean text stays in
  UTF-16 data files. Add `/utf-8` to the Pane files if a literal is unavoidable.
- Add a post-build or script step to stage `ui2026/` into `client_748` and to
  write `ui2026/manifest.json` (panel list, alias list, texture list, hashes).
- Startup validation: parse all panels in debug builds, log unknown keys and
  missing aliases as errors, and fail fast in `WYD.log`. Do not silently render
  a white frame.
- Device loss: `GuiTextureCache` and `GuiFont` need
  `InvalidateDeviceObjects` / `RestoreDeviceObjects` hooks alongside
  `TextureManager` and `m_pDXFont`. This is easy to forget and shows up as a
  black UI after Alt-Tab.

## 11. Existing files touched

Keep the diff to the legacy tree small and reviewable.

| File | Change | Why |
|---|---|---|
| `ObjectManager.cpp` | split `RenderControl` loop around the pane pass | draw order |
| `TMScene.cpp/.h` | route mouse/key/char/IME, tick and cleanup to `GuiManager` | scene lifecycle |
| `TMSelectServerScene.cpp` | create `PanelLogin`, bind its fields, **hide the code-built login panel/edit boxes/lists** when the pane is on | wave 1 |
| `TMSelectCharScene.cpp` | create `PanelCharSelect*`, keep the 3D preview and the `SelCharScene2.bin` controls that still have a job | wave 1 |
| `TMFieldScene.cpp` | add named open/close wrappers; hide the classic controls replaced by HUD/chat panes; do **not** restructure `OnControlEvent` | waves 2–3 |
| `NewApp.cpp` | initialize/shutdown `GuiManager` after the device; read `g_UIVer` | startup |
| `TextureManager.cpp/.h` | factor a path-based `LoadTextureFile` helper; keep the index-based API | reuse `.wyt`/`.wys` |
| `RenderDevice.cpp/.h` | expose scissor/UV-quad helpers if `RenderRectTex` is insufficient | renderer |
| `TMProject.vcxproj`, `.filters` | add `Pane/*.cpp` | build |
| `docs/tools/*` | asset staging + manifest + screenshot diff | build/QA |

`TMFieldScene.cpp` is not UTF-8; prefer new files and call-site additions over
in-place edits, and run `docs/tools/restore_non_utf8.py` after any edit.

## 12. What not to do

- Do not put panes in `m_pDrawControl[30]`. The layer cap and reverse insertion
  order do not match `ZOrder`.
- Do not reuse the 512 `TextureManager` UI slots for guimat textures.
- Do not bind panels with numeric control IDs; the names are the API.
- Do not rewrite `SGridControl` to "fit" the pane model in the first pass.
- Do not add a third text pipeline; either `GuiFont` (panes) or `TMFont2`
  (legacy).
- Do not copy the 2026 `UI/` tree over the 748 one.
- Do not assume `FieldScene2.bin`/`SelCharScene2.bin` are dead. The 2026 client
  instantiates the scene classes and loads 1,354/107 classic controls from them.
- Do not draw a pane and its classic equivalent at the same time; suppress one
  explicitly.
