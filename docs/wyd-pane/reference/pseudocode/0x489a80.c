// rea decompile_all
// name: FUN_00489a80
// address: 0x489a80
// body: available
// range: 0x489a80-0x489b43 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


undefined4 FUN_00489a80(wchar_t *param_1,wchar_t *param_2)

{
  wchar_t *_Str1;
  int iVar1;
  undefined4 uVar2;
  int in_ECX;
  
  _Str1 = param_1;
  iVar1 = __wcsicmp(param_1,L"range");
  if (iVar1 == 0) {
    _swscanf_s(param_2,L"%d %d",&param_1,&param_2);
    FUN_00489960(param_1,param_2);
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"value");
  if (iVar1 == 0) {
    uVar2 = FUN_0080d8dc(param_2);
    FUN_00489600(uVar2,0);
    return 1;
  }
  iVar1 = __wcsicmp(_Str1,L"MouseMoveLock");
  if (iVar1 == 0) {
    iVar1 = __wcsicmp(param_2,L"true");
    *(bool *)(in_ECX + 0x1ac) = iVar1 == 0;
    return 1;
  }
  uVar2 = FUN_00442d60(_Str1,param_2);
  return uVar2;
}

