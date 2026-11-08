# Recovered function map

Target: `WYD.exe`, SHA-256
`a058262d36631fd8b674603fb20d54ff18016b8f3dd38249250e953b8efc3c50`.
Addresses are image-relative virtual addresses.

## Container / crypto

| Address | Function | Role |
|---|---|---|
| `0x407580` | `FUN_00407580` | delta decrypt/encrypt for `.pane` and `.wyp` |
| `0x407230` | `FUN_00407230` | file open helper used by `FUN_00407580` |
| `0x407380` | `FUN_00407380` | file open/read wrapper |
| `0x4073a0` | `FUN_004073a0` | read whole file |
| `0x409900` | `FUN_00409900` | get file size |
| `0x409480` | `FUN_00409480` | close file |
| `0x418b10` | `FUN_00418b10` | TextFileLoader set-data / BOM detection |
| `0x4189f0` | `FUN_004189f0` | TextFileLoader open |

## Panel loader and parsers

| Address | Function | Role |
|---|---|---|
| `0x482c10` | `FUN_00482c10` | panel file entry: checks `.pane`, decrypts, parses |
| `0x482a20` | `FUN_00482a20` | related panel/guimat loader |
| `0x481a70` | `FUN_00481a70` | create panel from path (used by `ListView.itemPanel`) |
| `0x4825b0` | `FUN_004825b0` | top-level line loop, `Panel`/`element` dispatch |
| `0x481fd0` | `FUN_00481fd0` | `Panel { … }` block parser |
| `0x4817f0` | `FUN_004817f0` | `element { … }` block parser |
| `0x482f60` | `FUN_00482f60` | get extension / string helper |
| `0x40d1b0` | `FUN_0040d1b0` | substring/compare helper |

## Property setters

| Address | Function | Role |
|---|---|---|
| `0x47df30` | `FUN_0047df30` | `CGuiPanel` property setter (vtable `+0x20`) |
| `0x444590` | `FUN_00444590` | `CGuiElement` common property setter |
| `0x48b1e0` | `FUN_0048b1e0` | `Static` extra properties |
| `0x44ac10` | `FUN_0044ac10` | `Image` extra properties |
| `0x436fb0` | `FUN_00436fb0` | `Button` extra properties |
| `0x483230` | `FUN_00483230` | `RadioButton` -> `Button` |
| `0x449570` | `FUN_00449570` | `GaugeBar` extra properties |
| `0x437b90` | `FUN_00437b90` | `ComboBox` extra properties |
| `0x489a80` | `FUN_00489a80` | `Slider` extra properties |
| `0x441000` | `FUN_00441000` | `EditBox` extra properties |
| `0x455040` | `FUN_00455040` | `ListBox` extra properties |
| `0x45a2c0` | `FUN_0045a2c0` | `ListView` extra properties |
| `0x48b730` | `FUN_0048b730` | `TreeView` -> `ListView` |
| `0x4506c0` | `FUN_004506c0` | `ImeNotePad` extra properties |
| `0x44c240` | `FUN_0044c240` | `ImageFont` extra properties |
| `0x42e310` | `FUN_0042e310` | `IconButton` extra properties |
| `0x46d4b0` | `FUN_0046d4b0` | `MoMesh` extra properties |
| `0x43da60` | `FUN_0043da60` | `CustomListView` extra properties |

## Element factory

| Address | Function | Role |
|---|---|---|
| `0x447900` | `FUN_00447900` | type-string -> constructor dispatch |
| `0x47c640` | `FUN_0047c640` | create element, name it, assign id, attach to panel |

## Widget constructors

| Type | Constructor | Parent |
|---|---|---|
| `CGuiElement` | `0x443740` | — |
| `CGuiStatic` | `0x48a310` | Element |
| `CGuiImage` | `0x44a7e0` | Element |
| `CGuiButton` | `0x436910` | Static |
| `CGuiCheckBox` | `0x437920` | Button |
| `CGuiRadioButton` | `0x483260` | CheckBox |
| `CGuiGaugeBar` | `0x448db0` | Static |
| `CGuiComboBox` | `0x4398f0` | Button |
| `CGuiSlider` | `0x489660` | Element |
| `CGuiEditBox` | `0x4402c0` | Element |
| `CGuiImeEditBox` | `0x44c3e0` | EditBox |
| `CGuiListBox` | `0x455250` | Element |
| `CGuiListView` | `0x45bff0` | ListBox |
| `CBalloonBox` | `0x435990` | Static |
| `CTooltipBox` | `0x435c60` | BalloonBox |
| `CGuiImageFont` | `0x44b970` | Element |
| `CGuiTreeView` | `0x48bbe0` | ListView |
| `CGuiImeNotePad` | `0x44eb70` | Element |
| `CGuiImeNotePadEditBox` | `0x453d10` | ImeEditBox |
| `CGuiIconButton` | `0x42abe0` | Button |
| `CGuiMesh` | `0x46d550` | Element |
| `CGuiCustomListView` | `0x43dcc0` | Element |

## Widget draw functions (vtable `+0x0c`)

| Type | Draw |
|---|---|
| `CGuiStatic` | `0x48a130` |
| `CGuiImage` | `0x449c10` |
| `CGuiButton` | `0x436520` |
| `CGuiCheckBox` / `CGuiRadioButton` | `0x437a20` |
| `CGuiGaugeBar` | `0x448900` |
| `CGuiComboBox` | `0x437ee0` |
| `CGuiSlider` | `0x4899b0` |
| `CGuiEditBox` / `CGuiImeEditBox` | `0x441e20` |
| `CGuiListBox` | `0x4564e0` |
| `CGuiListView` | `0x458840` |
| `CGuiImageFont` | `0x44baf0` |
| `CGuiIconButton` | `0x42c550` |
| `CGuiMesh` | `0x46d480` |
| `CGuiCustomListView` | `0x43ba20` |

## Renderer helpers

| Address | Function | Role |
|---|---|---|
| `0x477820` | `FUN_00477820` | draw textured quad with mode + source rect |
| `0x477710` | `FUN_00477710` | draw simple textured quad |
| `0x4777b0` | `FUN_004777b0` | draw rotated textured quad |
| `0x477cd0` | `FUN_00477cd0` | draw text with shadow |
| `0x47b7d0` | `FUN_0047b7d0` | draw single-line text |
| `0x498860` | `FUN_00498860` | draw multiline text |
| `0x46f550` | `FUN_0046f550` | fill rect with color |
| `0x46f890` | `FUN_0046f890` | draw grid line/rect |

## Texture / guimat

| Address | Function | Role |
|---|---|---|
| `0x478ec0` | `FUN_00478ec0` | `CGuiPanel` draw/update pass |
| `0x46f0d0` | `FUN_0046f0d0` | panel screen alignment/layout pass |
| `0x478b00` | `FUN_00478b00` | panel hit dispatch (child search) |
| `0x4aea40` | `FUN_004aea40` | texture loader (`WYT`/`WYS`/`WYP`) |
| `0x4ae7d0` | `FUN_0044ae7d0` | extension -> loader type |
| `0x47a110` | `FUN_0047a110` | load guimat resource |
| `0x466bb0` | `FUN_00466bb0` | resolve texture alias |
| `0x4656c0` | `FUN_004656c0` | set slot texture color |
| `0x465740` | `FUN_00465740` | set slot state/texture index |
| `0x465840` | `FUN_00465840` | set slot state with alpha |

## Vtables

| Class | Vtable |
|---|---|
| `CGuiElement` | `0x88041c` |
| `CGuiPanel` | `0x882880` |
| `CGuiStatic` | `0x88330c` |
| `CGuiImage` | `0x881484` |
| `CGuiButton` | `0x87fa04` |
| `CGuiCheckBox` | `0x87fb7c` |
| `CGuiRadioButton` | `0x8830a4` |
| `CGuiGaugeBar` | `0x880f8c` |
| `CGuiComboBox` | `0x87fc4c` |
| `CGuiSlider` | `0x8831ec` |
| `CGuiEditBox` | `0x87ff3c` |
| `CGuiImeEditBox` | `0x8817ec` |
| `CGuiListBox` | `0x881a9c` |
| `CGuiListView` | `0x881bc4` |
| `CBalloonBox` | `0x87f8c4` |
| `CTooltipBox` | `0x87f944` |
| `CGuiImageFont` | `0x881654` |
| `CGuiTreeView` | `0x883494` |
| `CGuiImeNotePad` | `0x881894` |
| `CGuiImeNotePadEditBox` | `0x8819d4` |
| `CGuiIconButton` | `0x87f33c` |
| `CGuiMesh` | `0x8825f4` |
| `CGuiCustomListView` | `0x87fce4` |
