// rea decompile_all
// name: FUN_00441000
// address: 0x441000
// body: available
// range: 0x441000-0x441304 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


undefined4 FUN_00441000(wchar_t *param_1,wchar_t *param_2)

{
  wchar_t *_Str1;
  int iVar1;
  undefined4 uVar2;
  int in_ECX;
  uint local_10;
  uint local_c;
  uint local_8;
  
  _Str1 = param_1;
  local_10 = __wcsicmp(param_1,L"SelBkColor");
  if (local_10 == 0) {
    param_1 = (wchar_t *)local_10;
    local_c = local_10;
    local_8 = local_10;
    _swscanf_s(param_2,L"%d %d %d %d",&local_8,&param_1,&local_c,&local_10);
    *(uint *)(in_ECX + 0x264) =
         (((uint)param_1 & 0xff | local_8 << 8) << 8 | local_c & 0xff) << 8 | local_10 & 0xff;
    return 1;
  }
  local_10 = __wcsicmp(_Str1,L"SelColor");
  if (local_10 == 0) {
    param_1 = (wchar_t *)local_10;
    local_c = local_10;
    local_8 = local_10;
    _swscanf_s(param_2,L"%d %d %d %d",&local_10,&param_1,&local_c,&local_8);
    *(uint *)(in_ECX + 0x260) =
         (((uint)param_1 & 0xff | local_10 << 8) << 8 | local_c & 0xff) << 8 | local_8 & 0xff;
    return 1;
  }
  local_10 = __wcsicmp(_Str1,L"CaretColor");
  if (local_10 == 0) {
    param_1 = (wchar_t *)local_10;
    local_c = local_10;
    local_8 = local_10;
    _swscanf_s(param_2,L"%d %d %d %d",&local_10,&param_1,&local_c,&local_8);
    *(uint *)(in_ECX + 0x268) =
         (((uint)param_1 & 0xff | local_10 << 8) << 8 | local_c & 0xff) << 8 | local_8 & 0xff;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"BorderWidth");
  if (iVar1 == 0) {
    uVar2 = FUN_0080d8dc(param_2);
    *(undefined4 *)(in_ECX + 400) = uVar2;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"Spacing");
  if (iVar1 == 0) {
    uVar2 = FUN_0080d8dc(param_2);
    *(undefined4 *)(in_ECX + 0x194) = uVar2;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"multiline");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    FUN_00440ab0(iVar1 == 0);
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"indicator");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    *(bool *)(in_ECX + 0x3cd) = iVar1 == 0;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"NumAttrib");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    *(bool *)(in_ECX + 0x274) = iVar1 == 0;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"Range");
  if (iVar1 == 0) {
    uVar2 = FUN_0080d8dc(param_2);
    *(undefined4 *)(in_ECX + 0x26c) = uVar2;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"DrawBox");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    *(bool *)(in_ECX + 0x275) = iVar1 == 0;
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"Password");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    *(bool *)(in_ECX + 0x3d9) = iVar1 == 0;
    return 1;
  }
  return 0;
}

