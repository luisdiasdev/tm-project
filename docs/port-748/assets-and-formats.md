# 748 Assets and Formats

All paths are relative to `client_748/`.

## UI scene binaries (`UI/*Scene*.bin`)

`TMScene::LoadRC()` converts the requested `.txt` name to `.bin` and calls
`ReadRCBin()`. The exact rule: if the name contains `_`, it overwrites the last
7 characters with `.bin`; otherwise it overwrites the last 3 with `bin`. For
example `UI\SelServerScene2.txt` → `UI\SelServerScene2.bin`. No `.txt` scene
files ship with 748; the `.bin` is the real input. `SelServerScene2.bin` is
1352 bytes; `SelServerScene.bin` is a stub (see below).

Records are:

```
int32 controlType;   // CONTROL_TYPE
<record body>;       // size depends on controlType
```

Record sizes used by `UIBinary.h`:

| Type | ID | Struct | Size |
|---|---|---:|---:|
| PANEL | 1 | `BinPanel` | 40 |
| BUTTON | 2 | `BinButton` | 40 |
| CHECKBOX | 3 | `BinCheckBox` | 32 |
| LISTBOX | 6 | `BinListBox` | 52 |
| PROGRESS | 10 | `BinProgress` | 48 |
| TEXT | 12 | `BinText` | 52 |
| EDIT | 13 | `BinEdit` | 184 |
| 3DOBJ | 15 | `Bin3DObj` | 28 |
| GRID | 16 | `BinGrid` | 40 |

First fields in every body: `nID, nParentID, nTextureSetIndex, nStartX,
nStartY, nWidth, nHeight, ...`.

**Verification result (748):**

| File | Size | Records | EOF |
|---|---:|---:|---|
| `FieldScene2.bin` | 35404 | 709 | exact |
| `SelCharScene2.bin` | 4820 | 85 | exact |
| `SelServerScene2.bin` | 1352 | 22 | exact |

`SelServerScene.bin` (1648 bytes) is **not** a full scene in this distribution:
it holds only 5 controls (age marks, logos, scene text) followed by the string
`Copyright 2002~2005 JoyImpact, All Rights Reserved.`. The classic (non-UI2)
server window is constructed entirely in code.

Regenerate the inventory and EOF check with `docs/tools/ui_inventory.py`.

## Texture lists

### `UI/UITextureList.bin`

512 records × 264 bytes:

```c
struct stTextureListInfo {
    char szFileName[255];
    char cAlpha;              // 'A','a','N','C', ...
    unsigned int dwLastUsedTime;
    unsigned int dwShowTime;
};
```

748 ships only `UITextureList.bin`; it has no `UITextureListN.bin`. Records
0..261 match the 755 list (same filenames and alpha flags). Records 262..266
and 300..322 are **empty** in 748, while 755 points them at `UI\GameGrade.wyt`
and the `NUI\*` new-UI textures. This is expected: 748 has no `NUI\` assets, and
its `UITextureSetList.txt` only references texture indices 0..261.

The trailing 8 bytes (`dwLastUsedTime`, `dwShowTime`) differ for every record:
748 contains the MSVC debug fill `0xCDCDCDCD`, 755 contains zeroes. Do not use
`dwShowTime` as a trustworthy value for 748.

`TMPaths.h` was changed from `UITextureListN.bin` to `UITextureList.bin`.

### `UI/UITextureSetList.txt`

Text format:

```
[SetName]
SetIndex: N
ItemCount: M
texIndex,startX,startY,width,height,destX,destY
... M rows ...
```

748 has **446 sets, max index 456**. 755 has 328 sets, max index 561. The number
of sets is not comparable between versions; always resolve against the target
file. Positive set indices used by the three 748 scene binaries all exist.

Negative texture-set indices:

- `-1` and `-2` are special (text/no-fill and coloured fill respectively, see
  `RenderDevice::RenderGeomRectImage`).
- Any value `< -2` means texture set `abs(value)`.

`TextureManager::GetUITextureSet()` only checks `0 <= index <= 600`, not whether
the set was actually loaded. A missing set returns a zeroed
`ControlTextureSet` with `pTextureCoord == nullptr`, which silently renders
nothing. This is the root cause of the missing 755-era modal background.

## `UI/strdef.bin` (message strings)

- Size: **56324** bytes = `440 * 128` (56320) + 4-byte checksum at the end.
- Read 56320 bytes, then read the checksum, compare with `BASE_GetSum()`.
- Decryption: XOR every byte with `0x5A` over `2000 x 128` (the table is
  `g_pMessageStringTable[2000][128]`; the unused rows are zeroed first).
- Entry 0 is padding (`0xB8` before XOR); real strings start at index 1.
- The checksum matches for `client_748`, `client_748_br` and `client`.

`BASE_GetSum()` in `Basedef.cpp` (note the dangling `else`):

```cpp
for (int i = 0; i < size; ++i) {
    int mod = i % 7;
    if (mod == 0) sum += p[i] / 2;
    if (mod == 1) sum += p[i] ^ 0xFF;
    if (mod == 2) sum += 3 * p[i];
    if (mod == 3) sum += 2 * p[i];
    if (mod == 4) sum -= p[i] / 7;
    if (mod == 5) sum -= p[i];
    else          sum += p[i] / 3;   // applies to mod 0,1,2,3,4,6 too
}
```

`char` is signed on MSVC and integer division truncates toward zero; reproduce
that exactly if you reimplement it.

## `curse.bin`

8192 bytes = **256 records × 32 bytes** (`sizeof(stCurse)`). The 748 reader
reads the whole table in one pass and sets `dnum = 256`. The old code read
`dnum` then allocated and read `dnum - 1` records, which does not match 748.

## `UI/UIString.txt`

Text records are `<index> <token>`; 748 provides indices **1..228** with no
gaps. The scenes reference up to 228. `ReadUIString()` uses `"%d %s"`, so tokens
are single words; the only multi-word 748 entries are `C.C Mode(MC)` and
`C.C Mode(SC)`, which are truncated to `C.C`. The 755 file also contains
multi-word entries, so this is an upstream limitation, not a 748 regression.

## Login packet

`MSG_CNFAccountLogin` is wrapped in `#pragma pack(push, 1)` in `Basedef.h`.
Without packing, the MSVC default alignment inserted padding and the login
packet did not match the 748 server, so the client could connect but not log in.

## Newer `.pane` UI (755+/2026 client)

The newer client uses an encrypted UTF-16 text UI format instead of the 748
`UI/*Scene*.bin` records. It is documented separately in
[`../wyd-pane/README.md`](../wyd-pane/README.md).

## Other files observed but not yet audited for 748

`object.bin`, `ItemList.bin`, `Itemname.bin`, `InitItem.bin`, `SkillData.bin`,
`minimap.dat`, `serverlist.bin`/`sn.bin`, `Mesh/*`, `Effect/*`, `Env/*`.
Use the same rule: file size must equal `count * record size` (plus any known
header/checksum), and parse to EOF before trusting a reader.
