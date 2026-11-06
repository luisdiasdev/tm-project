# Server Select (748)

## What the scene gives you

`UI/SelServerScene2.bin` contains 22 logical controls:

- age marks `4623` / `4624`
- logo panels `311` / `312`
- scene text `769`
- `TMP_SELSERVER_UP` 4621 (256x71, texture set `-2`)
- `TMP_SELSERVER` 4622 (256x128, texture set `-2`, colour `0xFFFFFFFF`)
- lists/buttons/texts as children of 4622
- login box `4608` and its children

`-2` means "no texture, solid colour". The white box in a naive port is
`TMP_SELSERVER` itself.

## What the 748 client does at runtime

`TMSelectServerScene::InitializeScene` is `FUN_004a8f14` in
`client_748/WYD.exe` (`0x4a8f14..0x4aad83`).

It selects a UI file by `DAT_005b892c`:

```c
if (DAT_005b892c == 2)
    LoadRC("UI\\SelServerScene2.txt");
else
    LoadRC("UI\\SelServerScene.txt");
```

`DAT_005b892c == 2` is the "new UI" path used by our config (`[CLASSIC] 0`,
`g_UIVer` forced to 2 in `NewApp.cpp`). `SelServerScene.bin` in this
distribution is only a stub (5 controls + copyright string), so the classic
path is not useful for testing.

For UI2, the window is assembled per-frame from these generated panels:

| Texture set | Panel ORIGIN (relative to 4622) | Size | Set name |
|---|---:|---:|---|
| -391 | (0, 0) | 143 x 256 | `NewUI_ServerList_Top` |
| -392 | (143, 0) | 40 x 45 | `NewUI_ChannelList_TopLeft` |
| -393 | (183, 0) | 75 x 45 | `NewUI_ChannelList_TopBottom` (natural 33x45, stretched) |
| -394 | (258, 0) | 40 x 45 | `NewUI_ChannelList_TopRight` |
| -396 | (142, 45) | 157 x 211 | `NewUI_ChannelList_Blank` |

The generated panels are inserted with `FUN_0054ac09`, which **prepends** to the
child list (same as `TreeNode::AddChild`). Because rendering walks the list in
order, prepending makes them draw **behind** the existing list/buttons/texts.
Port this order deliberately: `AddChild` prepends in TMProject too.

`TMP_SELSERVER` (4622) is centered:

```c
SetPos(4622,
       screenW * 0.5 - width  * 0.5,
       screenH * 0.5 - height * 0.5);
```

The old port added `+75.0f` to Y (a 755 layout offset). That was removed.

Other original offsets:

| Control | Change |
|---|---|
| 4618 group list | `Y += 22` |
| 4619 server list | `X += 11`, `Y += 27` |
| 5635 server text | `Y += 5` |
| 5636 channel text | `X += 8`, `Y += 5` |
| 5632 login box text | `Y += 5` |
| 4620 OK / 5637 exit | `Y = 225` (relative to 4622) |

Text `5636` was previously moved on X twice (`+= 8; += 5;`) instead of X/Y;
that is fixed.

Texture set `397` (`NewUI_Minibar_Panel`) is **not** the left window frame. It
is a narrow icon strip from `MainBox2.wyt`; stretching it to 143x256 produces
the column of distorted icons seen during the first attempt. The correct set is
`391`.

## Resulting screen (800x600)

Panel `4622` at `(272,236)`, 256x128. The generated frame covers
`(272,236)`–`(571,492)`. Connect/Close end up at
`(286,461)`/`(352,461)` (50x23). See `build-and-test.md` for click coordinates.

## Login panel: Enter key and list selection

- `SEditableText::OnCharEvent` forwards keys to its listener by control ID.
  `TMSelectServerScene::OnCharEvent` therefore has to call the **748** login
  button ID (`TMB_LOGIN_OK`, 4609), not the 755 ID (`65873`). The 755 ID made
  Enter do nothing. The same hardcoded ID in `TMScene::OnPacketEvent` is also
  remapped.
- The server list can contain placeholder rows and skipped `num` values, so the
  visible row index is not the `g_pServerList` index. Each
  `SListBoxServerItem` now stores the group/index it was built from; Connect
  uses those stored values instead of `GetSelectedIndex() + 1`.

## Source

- Implementation: `TMSelectServerScene::InitializeUI()` (frame creation, list
  offsets) and `InitializeScene()` (text offsets).
- Recovery: `docs/reverse-engineering/rea-workflow.md`.
