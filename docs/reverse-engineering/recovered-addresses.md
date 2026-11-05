# Recovered Addresses and Constants (748 `WYD.exe`)

SHA-256: `b545ea104de50641e820f00b6bc54e4b2b14583ed75c7dcec06f50ba5042619c`

## Functions

| Address | Meaning | Notes |
|---|---|---|
| `0x4a8f14`–`0x4aad83` | `TMSelectServerScene::InitializeScene` | Loads `SelServerScene2.txt`, builds the UI2 server window, populates server groups. |
| `0x493e70`–`0x494bff` | `TMScene::Initialize` | Creates `SMessageBox` x2, `SMessagePanel`, IME text; contains the `"Message Box"` string refs. |
| `0x403eb8` | `SMessageBox::SMessageBox` | UI2 branch: background set 164, buttons set 165, 256x128 box. |
| `0x401466` | `SPanel` constructor | `(texSet, x, y, w, h, color, fillType)`. |
| `0x401d03` | `SText` constructor | `(texSet, text, color, x, y, w, h, border, borderColor, type, align)`. |
| `0x402f01` | `SButton` constructor | `(texSet, x, y, w, h, color, sound, text)`. |
| `0x407203` | `SListBoxItem` constructor | Used for server/channel rows. |
| `0x54ac09` | prepend child (AddChild) | ECX = parent; new child becomes first. |
| `0x40116e` | `SetCenterPos` | Only X is actually adjusted for IDs in a hard-coded list. |

## Strings and xrefs

| String | Address | Xref | Containing function |
|---|---:|---:|---|
| `UI\SelServerScene2.txt` | `0x5b95dc` | `0x4a8f42` | `0x4a8f14` |
| `UI\SelServerScene.txt` | `0x5b95f4` | — | `0x4a8f14` |
| `Message Box` (type 0) | `0x5b895c` | `0x4944e6` | `0x493e70` |
| `Message Box` (type 4) | `0x5b8968` | `0x4945b0` | `0x493e70` |

## Globals / flags

| Symbol | Meaning |
|---|---|
| `DAT_005b892c` | UI version. `2` = new UI (our target); other values select classic assets/layouts. |
| `DAT_005d04f4` | Layout offset: `0x8c` (140) for UI2, `0x78` (120) otherwise. Used in the server list sizing. |
| `g_UIVer` (TMProject) | Equivalent UI-version flag; forced to `2` in `NewApp.cpp`. |

## Server window constants (UI2)

| Control | Value |
|---|---|
| Generated panel sets | `-391`, `-392`, `-393`, `-394`, `-396` |
| -391 origin/size | `(0,0)` `143x256` (`NewUI_ServerList_Top`) |
| -392 origin/size | `(143,0)` `40x45` (`NewUI_ChannelList_TopLeft`) |
| -393 origin/size | `(183,0)` `75x45` (`NewUI_ChannelList_TopBottom`, texture is 33x45) |
| -394 origin/size | `(258,0)` `40x45` (`NewUI_ChannelList_TopRight`) |
| -396 origin/size | `(142,45)` `157x211` (`NewUI_ChannelList_Blank`) |
| Group-row plate set | `-395` `NewUI_ServerList_Button` (created per server group in the original) |
| Panel 4622 | centered `(screen - size) / 2`, then no +75 offset |
| 4618 group list | `Y += 22` |
| 4619 server list | `X += 11`, `Y += 27` |
| 5635 / 5636 titles | `Y += 5`; channel also `X += 8` |
| 4620 / 5637 buttons | `Y = 225` (relative to 4622) |

## Message box constants (UI2)

| Element | Value |
|---|---|
| Base panel | `256x128`, color `0x01010101`, fill 4 |
| Control ID | `0x1209` (4617) |
| `SetCenterPos` | `(id, x, y, 232, 128)` |
| Background | set `164` `MessageBox_BG2`, `(0,0)` `232x107` |
| Message text | `(10,36)` and `(10,52)`, height 12 |
| Caption | `(96,9)`, height 12 |
| Button plate | set `165` `MessageBox_Btn2`, `(20,76)` / `(124,76)`, `88x23` |
| Buttons | `-2`, `(20,77)` / `(117,77)`, `88x21` |
| OK variant | plate `165` `(70,76)`, button `(70,77)` |

Non-UI2 fallback (not ported): background set `9` `MessageBox_BG` `256x97`,
buttons sets `14`/`15` at `103x21`.

## Texture-set names used by the fixes

| Set | Name | Atlas / size |
|---:|---|---|
| 164 | `MessageBox_BG2` | `UI\MessageBox2.wyt`, 232x107 |
| 165 | `MessageBox_Btn2` | `UI\MessageBox2.wyt`, 86x21 |
| 391 | `NewUI_ServerList_Top` | 143x256 |
| 392 | `NewUI_ChannelList_TopLeft` | 40x45 |
| 393 | `NewUI_ChannelList_TopBottom` | 33x45 |
| 394 | `NewUI_ChannelList_TopRight` | 40x45 |
| 395 | `NewUI_ServerList_Button` | 109x22 |
| 396 | `NewUI_ChannelList_Blank` | 113x211 |
| 397 | `NewUI_Minibar_Panel` | 38x215 icon strip — not the window frame |
