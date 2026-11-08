# 05 — Textures, guimat and atlas containers

`.pane` files usually refer to **aliases**, not texture files. The chain is:

```text
.pane TextureAlias
   -> guimat TextureAlias table
      -> texture index + source rectangle
         -> guimat TextureList row
            -> .wyt / .wyp / .wys file
```

## 1. `Resource` and `ui/default.guimat`

Every shipped panel contains:

```text
Resource ui/default.guimat
```

`CGuiPanel` loads that guimat through `FUN_0047a110` and uses it for:

- `FontList` — font index → face/size/weight/italic
- `TextureList` — texture index → file path
- `TextureAlias` — alias name → texture index + atlas rectangle

The guimat is **plain UTF-16LE text with a BOM** (not encrypted). Example from
the shipped `UI/Default.guimat`:

```text
FontList //...
{
    0,0,10,0,false,false
    1,0,11,0,false,false
    ...
}

TextureList
{
    0 0 ui/Image/NewUI_guimat.wyp
    1 0 UI/hppro.wyt
    2 0 ui/Image/NewUI_ui0.wyt
    3 0 NUI/ServerList.wyt
    ...
    41 1 NUI/Collection.wyt
    ...
}

TextureAlias
{
    cc.IconButtonH2 <0> 77 7 113 43
    Collection.BG <41> 0 0 520 477
    Collection.LI <41> 0 481 470 557
    ...
}
```

### TextureList row format

```text
<index> <colorkey> <path>
```

- `<index>` — texture index used by aliases.
- `<colorkey>` — transparent/color-key value. Observed values include `0`,
  `-16777216` (`0xFF000000`) and `1700468`.
- `<path>` — `.wyt`, `.wyp` or `.wys` file, relative to the client root.

### TextureAlias row format

```text
<name> <textureIndex> <x1> <y1> <x2> <y2>
```

- `<name>` — the exact string used by `TextureAlias` / `TextureAlias[n]`.
- `<textureIndex>` — index into `TextureList`.
- rectangle — source region inside the atlas.

Alias names are looked up case-sensitively or case-insensitively depending on
the manager implementation; treat them as case-sensitive to be safe and keep
the shipped spelling.

## 2. How `.pane` uses aliases

```text
TextureAlias UI.NoticeBG
TextureAlias[0] UI.ButtonN
TextureAlias[1] UI.ButtonD
TextureAlias[2] UI.ButtonD
```

- Without an index: applies to slot 0 and, when the element has no per-slot
  alias mode, to every slot.
- With `[n]`: applies to texture slot `n` only.
- A slot whose alias is empty renders nothing.

`Image` with `ExternalImageFile` bypasses the alias table and loads a standalone
file directly (`ui/CI.png` is the only one in shipped data). It may still use
the animation-sheet keys.

## 3. `.wyt` — TGA container

All 270 shipped `.wyt` files follow this layout:

```text
offset 0   : "WT10"                 4-byte magic
offset 4   : TGA 2.0 image         18-byte TGA header + pixel data
end - 8    : 8 zero bytes          first 8 bytes of the TGA 2.0 footer
```

The 18-byte TGA header at offset 4 is a normal uncompressed/RLE TGA header
(width/height/depth/descriptor). The game's texture loader (`FUN_004aea40`,
branch 1) strips the 4-byte magic and appends the standard 18-byte TGA 2.0
signature:

```text
"TRUEVISION-XFILE.\0"
```

in memory before passing the buffer to D3DX.

Practical conversion:

```text
.tga = .wyt[4:]  +  "TRUEVISION-XFILE.\0"
```

Most TGA viewers accept `.wyt[4:]` alone because the footer is optional, but
appending it produces a fully conformant TGA 2.0 file.

The tool [`tools/texture_extract.py`](tools/texture_extract.py) implements this
conversion, plus `.wyp` → `.png`.

## 4. `.wyp` — encrypted PNG

`.wyp` uses the **same delta cipher as `.pane`**. All 4 shipped `.wyp` files
decrypt to PNG:

```text
89 50 4E 47 0D 0A 1A 0A   (PNG signature)
```

So:

```python
png = decrypt(open("file.wyp","rb").read())
```

No other transformation is needed.

## 5. `.wys` — custom DDS-like container

`.wys` is not encrypted with the delta cipher. The loader (`FUN_004aea40`,
branch 2) treats it as a DDS-like container and repairs the header in memory.
It starts with `WS10` instead of `DDS `. Shipped files include
`UI/Inventory2.wys`, `UI/itemicon01.wys`…`itemicon16.wys`, `UI/thunder01.wys`.

This format is separate from `.pane`; it is not needed to render `.pane`
layout, only to decode the atlas images.

## 6. Texture sets vs aliases

Do not confuse this newer `TextureAlias` system with the 748-era
`UITextureSetList.txt` system documented in
[`../port-748/assets-and-formats.md`](../port-748/assets-and-formats.md).

- `UITextureSetList.txt` is a set/frame atlas used by `SControl` and the
  `UI/*Scene*.bin` controls.
- `.pane` uses `TextureAlias` + `<textureIndex> <rect>` from the guimat.

A port that supports both should keep the two resolvers separate.

## 7. Fonts

`FontList` rows are:

```text
<index>,<faceIndex>,<size>,<flags>,<bold>,<italic>
```

`CaptionFont` and element `Font` / `Font[n]` are indices into this list. The
game also loads `FontNanum.ttf`, `NanumBarunGothic.ttf` and other font files
from `UI/`; the guimat selects the face by index.

## 8. Localization strings

`Caption`, `Text`, `ToolTip`, `text_postfix` and similar values may be:

- literal text (`Copyright 2002~2021 Joyimpact All Rights Reserved.`), or
- a `uu_<number>` key resolved by the UI string table.

Observed keys include `uu_80`, `uu_115`, `uu_170`, `uu_210`, `uu_253`,
`uu_266`, `uu_343`. Treat anything matching `^uu_` as a localization key and
resolve it at draw time; otherwise use the literal text.

## 9. Animation sheets

For `Image` with `UseAni true`:

- `AniCutXPage` = columns
- `AniCutYPage` = rows
- `AnimationMinPage` / `AnimationMaxPage` = inclusive frame range
- `AnimationDelay` = seconds per frame
- `AnimationLoopCnt` = number of loops; `-1` = infinite

The frame index counts left-to-right, top-to-bottom:

```text
frameX = frame % AniCutXPage
frameY = frame / AniCutXPage
```

The source rect is the corresponding cell of the alias rectangle. If the alias
is a single image rather than a sheet, leave `UseAni false`.
