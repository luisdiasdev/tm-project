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

## Message panel (connection/status bar)

`SMessagePanel` had the same 755 asset problem: its base and side-cap plates
used sets `502`/`503`/`504` (`newuimessage`, `newuiMessagePanelL2`,
`newuiMessagePanelR2`), which do not exist in the 748 texture list. The message
text still drew, but the bar behind it was empty.

The 748 UI2 constructor is `FUN_00403924`. It uses:

| Element | Value |
|---|---|
| Base | set **178** `MessagePanel2` (222x24), stretched to the panel width |
| Left cap | set **259** `MessagePanelL2` (11x24) at `(-11, 0)` |
| Right cap | set **260** `MessagePanelR2` (11x24) at `(width, 0)` |
| Control ID | `5638` (`0x1606`) |
| Text 1 | `(30, 1)`, `w-52` x `h-2` |
| Text 2 | `(24, w-52)`, `50` x `14` (not added to the tree) |

The panel itself is created by `TMScene::Initialize` at `(160, 80)`, `480x24`.

## Verification

Driven by a synthetic click on the server-select Close button; the modal now
matches the original 748 client (frame, Information caption, message, button
plates). See `build-and-test.md`.

For the message panel, clicking Connect without selecting a channel produces
"Select server and server group." inside the restored dark bar.

### Long messages

`TMFont2::SetText` lays a string out as up to three chunks drawn side by side,
because a single chunk is limited by the 512 px font atlas (~42 characters). The
bar was created at a fixed `480x24`, so anything past ~82 characters was cut:
the old code hard-truncated `m_szString` at 82 characters, and even a full line
ran past the right cap of the frame.

Two changes:

- `TMFont2::SetText` now truncates at `MAX_STRLENGTH - 1` (119 characters)
  instead of 82/83. The three-chunk layout handles up to 42 + 42 + 35.
- `SMessagePanel::FitWidthToText()` measures the rendered chunks
  (`TMFont2::m_szStringSize`) after `SetText` and resizes the bar to
  `text + 60 px`, clamped to `[480, 784]`, then re-centres it and moves the
  right cap. `SetMessage()` calls it; `ObjectManager::SetCurrentScene()` calls it
  after copying the previous scene's message so a carried-over message keeps the
  right width.

Short messages keep the original `480` bar, so the 748 look is unchanged.
Verified with the 88-character server message
`"Your name is already registered by other user, or your name includes unusable characters."`.

