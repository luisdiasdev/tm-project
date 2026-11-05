# Message Box / Modals (748)

## Symptom

Clicking Close (server-select, ID `5637`, reused for other confirms) showed the
title, message and Confirm/Cancel labels, but **no modal background and no
button plates**.

## Cause

`SMessageBox` was the 755 constructor:

```cpp
: SPanel(-2, x, y, 256.0f, 172.0f, ...)
m_pPanel1 = new SPanel(501, 0, 0, 256.0f, 172.0f, ...);
m_pOKButton     = new SButton(-2, 17, 132, 103, 21, 0, 1, "Confirm");
m_pCancelButton = new SButton(-2, 137, 132, 103, 21, 0, 1, "Cancel");
```

Set `501` is `newuiPopup` (256x172, `NUI/popup.wyt`) in the 755 list. 748's
`UITextureSetList.txt` only goes to set `456`; `GetUITextureSet(501)` returns a
zeroed entry and renders nothing. Even if the index existed, 748 has no `NUI\`
assets (texture records 300+ are empty), so the 748 constructor is the only
correct source. The `-2`/colour-0 buttons had no art either.

## Recovered 748 constructor

`SMessageBox::SMessageBox` is `FUN_00403eb8` in `client_748/WYD.exe`. The UI2
branch (`DAT_005b892c == 2`) is:

| Element | Value |
|---|---|
| Box base | `SPanel(-2, x, y, 256, 128, 0x01010101, RENDER_IMAGE_STRETCH)` |
| Control ID | `0x1209` (4617) |
| `SetCenterPos` | `(id, x, y, 232, 128)` |
| Background `m_pPanel1` | set **164** `MessageBox_BG2`, (0,0) 232x107 |
| Message text 1 | `(10, 36)`, height 12 |
| Message text 2 | `(10, 52)`, height 12 |
| Caption | `(96, 9)`, height 12; text `g_pMessageStringTable[237]` |
| Confirm plate | set **165** `MessageBox_Btn2`, (20, 76) 88x23 |
| Cancel plate | set **165**, (124, 76) 88x23 |
| Confirm button | `SButton(-2, 20, 77, 88, 21, 0, 1, table[238])` |
| Cancel button | `SButton(-2, 117, 77, 88, 21, 0, 1, table[239])` |

For `TMC_MESSAGEBOX_OK` (type 4), UI2 uses one plate at `(70,76)` 88x23 and one
button at `(70,77)` 88x21.

The non-UI2 branches use background set `9` (256x97) and button sets `14`/`15`
(103x21). We target UI2, so those are not ported.

## Ordering detail

`TreeNode::AddChild` and the original `FUN_0054ac09` both **prepend**. The
background must be added **last** so it becomes the first child and renders
behind texts and buttons. Buttons must be added **before** their plates for the
same reason.

The old 755 code created `m_pPanelBtn1` for the OK box but never added it; the
748 port adds it.

## Files

- `Projects/TMProject/SControl.cpp`, `SMessageBox::SMessageBox`.
- Control ID and loader: `TMScene::Initialize` creates two boxes (`type 0` and
  `type 4`) via `new SMessageBox(...)` at `TMScene.cpp` lines ~98/104.

## Verification

Driven by a synthetic click on the server-select Close button; the modal now
matches the original 748 client (frame, Information caption, message, button
plates). See `build-and-test.md`.
