// rea decompile_all
// name: FUN_00436fb0
// address: 0x436fb0
// body: available
// range: 0x436fb0-0x43715f inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00436fb0(wchar_t *param_1,wchar_t *param_2)

{
  wchar_t wVar1;
  int iVar2;
  wchar_t *pwVar3;
  undefined4 uVar4;
  int in_ECX;
  undefined4 local_84 [5];
  uint local_70;
  uint local_4c [4];
  undefined4 local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_00849bef;
  local_10 = ExceptionList;
  local_14 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  iVar2 = __wcsicmp(param_1,L"Group");
  if (iVar2 == 0) {
    local_4c[0] = local_4c[0] & 0xffff0000;
    local_38 = 7;
    local_3c = 0;
    pwVar3 = param_2;
    do {
      wVar1 = *pwVar3;
      pwVar3 = pwVar3 + 1;
    } while (wVar1 != L'\0');
    FUN_0040caa0(param_2,(int)pwVar3 - (int)(param_2 + 1) >> 1);
    local_8 = 0;
    FUN_0040c8a0(local_4c,0,0xffffffff);
    local_8 = 0xffffffff;
    if (7 < local_38) {
      FUN_0040be40(local_4c[0],local_38 + 1);
    }
  }
  else {
    iVar2 = __wcsicmp(param_1,L"ToggleButton");
    if (iVar2 == 0) {
      iVar2 = __wcsicmp(param_2,L"true");
      *(bool *)(in_ECX + 0x216) = iVar2 == 0;
    }
    else {
      iVar2 = __wcsicmp(param_1,L"Icon");
      if (iVar2 == 0) {
        FUN_0040d040(param_2);
        local_8 = 1;
        FUN_00436490(local_84);
        local_8 = 0xffffffff;
        if (7 < local_70) {
          FUN_0040be40(local_84[0],local_70 + 1);
        }
      }
      else {
        iVar2 = __wcsicmp(param_1,L"DownTextUp");
        if (iVar2 == 0) {
          iVar2 = __wcsicmp(param_2,L"true");
          *(bool *)(in_ECX + 0x295) = iVar2 == 0;
        }
        else {
          iVar2 = __wcsicmp(param_1,L"ButtonSound");
          if (iVar2 == 0) {
            uVar4 = FUN_0080d8dc();
            *(undefined4 *)(in_ECX + 0x298) = uVar4;
          }
          else {
            FUN_0048b1e0(param_1,param_2);
          }
        }
      }
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

