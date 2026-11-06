# Boot Fixes (commit `114d1f0`)

These were the minimum changes needed to get the 748 client to boot and reach
the login/server-select UI.

## `strdef.bin`

748 file is 56324 bytes:

- body: `440 * 128 = 56320`
- trailer: 4-byte checksum

The old reader used `size = 256000` (2000 rows) and would read past EOF / fail
the checksum. `BASE_ReadMessageBin()` now reads 440 rows and then the 4-byte
checksum at offset 56320. See `assets-and-formats.md` for the exact
`BASE_GetSum()` formula.

## Login packet packing

`MSG_CNFAccountLogin` is wrapped in `#pragma pack(push, 1)` / `pack(pop)` in
`Basedef.h`. The default MSVC alignment inserted padding, so the login packet
did not match the 748 server. With packing, login succeeds.

## Login request packet `MSG_AccountLogin`

The TMSrv/DBSrv pair used for testing expects `sizeof(MSG_AccountLogin) == 116`
with this layout:

| Field | Size |
|---|---:|
| `MSG_STANDARD` header | 12 |
| `AccountPass` | **12** |
| `AccountName` | 16 |
| `TID` / `Zero` | 52 |
| `Version` | 4 |
| `Force` | 4 |
| `Mac` / `AdapterName` | 16 |

The port had `AccountPass[16]` (120 bytes). TMSrv reads `AccountName` at offset
`0x18`; with a 16-byte password it read the password tail plus name bytes and
sent an empty account name to DBSrv, which answered "account does not exist".
Keep the 12-byte password field.

## `curse.bin`

748 is 8192 bytes = 256 records × 32 bytes. The old reader read a count then
allocated/read `count - 1` records. It now sets `dnum = 256` and reads the whole
table in one `fread`. (An 8196-byte 755 `curse.bin` also exists; that is a
different format and not used here.)

## UI texture list path

`TMPaths.h` changed:

```
UI\UITextureListN.bin  ->  UI\UITextureList.bin
```

748 ships only `UITextureList.bin`. Both are 512 × 264 bytes; the 748 file's
`dwShowTime`/`dwLastUsedTime` fields are debug-filled with `0xCD`.

## Select-server scene IDs

The initial port switched the login/server-select lookups from the 755
constants to the `TM*` ones present in `SelServerScene2.bin`:

| Old | New |
|---|---|
| `P_LOGIN_BOX` | `TMP_LOGIN_BOX` |
| `B_LOGIN_OK` | `TMB_LOGIN_OK` |
| `B_CREATE_ID` | `TMB_CREATE_ID` |
| `B_QUIT` | `TMB_QUIT` |
| `E_LOGIN_ID` | `TME_LOGIN_ID` |
| `E_LOGIN_PASSWORD` | `TME_LOGIN_PASSWORD` |
| `P_SERVER_SEL` | `TMP_SELSERVER` |
| `L_SELECT_SERVERG` | `TML_SELECT_SERVERG` |
| `L_SELECT_SERVER` | `TML_SELECT_SERVER` |
| `B_SERVER_SEL_OK` | `TMB_SELSERVER_OK` |
| `B_SERVER_SEL_EXIT` | `TMB_SELSERVER_EXIT` |

The message-value constants inside `OnControlEvent` (`B_QUIT`, `B_CREATE_ID`)
were left on the 755 values because they are compared against the same values
in `SMessageBox::m_dwMessage`. Convert both sides together if you change them.

## Local workarounds to revisit

- `TMSelectServerScene::InitializeScene()` calls `BASE_GetHttpRequest()` for
  the user counts and then **overrides the result** with
  `sprintf_s(szUserCount, 1024, "%d %d ...", 0, ...)`. This bypasses a missing
  HTTP user-count service; replace with a real endpoint or remove once the
  server setup is known.
- `nServerGroupIndex` no longer subtracts `1` when computing the group index.
  This matched the 748 server list layout; verify if the serverlist format
  changes.
- `NewApp.cpp` has `CreateConsole()` in the debug build; the console can be
  useful for logs but is not part of the game UI.

## Project output

`TMProject.vcxproj` now writes Debug/Release output to `..\client_748\` instead
of `..\client\`, so the built EXE lands next to the 748 assets. The build uses
`$(SolutionDir)Dependencies\Directx\Include`, so build the solution rather than
the project alone.
