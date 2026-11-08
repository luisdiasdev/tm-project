// rea decompile_all
// name: FUN_0045a2c0
// address: 0x45a2c0
// body: available
// range: 0x45a2c0-0x45a488 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_0045a2c0(wchar_t *param_1,wchar_t *param_2)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  char *pcVar4;
  int *in_ECX;
  char *pcVar5;
  uint local_84 [4];
  undefined4 local_74;
  uint local_70;
  uint local_4c [4];
  undefined4 local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084b2fc;
  local_10 = ExceptionList;
  uVar2 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_14 = uVar2;
  cVar1 = FUN_00455040(param_1,param_2);
  if (cVar1 == '\0') {
    iVar3 = __wcsicmp(param_1,L"itemSize");
    if (iVar3 == 0) {
      _swscanf_s(param_2,L"%d %d",in_ECX + 0xf4,in_ECX + 0xf5);
      (**(code **)(*in_ECX + 0x70))();
    }
    else {
      iVar3 = __wcsicmp(param_1,L"itemPanel");
      if (iVar3 == 0) {
        local_70 = 0xf;
        local_74 = 0;
        local_84[0] = local_84[0] & 0xffffff00;
        local_8._0_1_ = 1;
        local_8._1_3_ = 0;
        pcVar4 = (char *)FUN_00413dc0(local_84,param_2,uVar2);
        local_38 = 0xf;
        local_3c = 0;
        local_4c[0] = local_4c[0] & 0xffffff00;
        pcVar5 = pcVar4;
        do {
          cVar1 = *pcVar5;
          pcVar5 = pcVar5 + 1;
        } while (cVar1 != '\0');
        FUN_0040c9a0(pcVar4,(int)pcVar5 - (int)(pcVar4 + 1));
        local_8._0_1_ = 2;
        FUN_00481a70(in_ECX[9],in_ECX + 0xf6,local_4c,0);
        local_8._0_1_ = 1;
        if (0xf < local_38) {
          FUN_00407f60(local_4c[0],local_38 + 1);
        }
        local_38 = 0xf;
        local_3c = 0;
        local_4c[0] = local_4c[0] & 0xffffff00;
        local_8._0_1_ = 0;
        if (0xf < local_70) {
          FUN_00407f60(local_84[0],local_70 + 1);
        }
        FUN_0040d040(param_2);
        local_8._0_1_ = 3;
        FUN_0040c8a0(local_4c,0,0xffffffff);
        local_8 = (uint)local_8._1_3_ << 8;
        if (7 < local_38) {
          FUN_0040be40(local_4c[0],local_38 + 1);
        }
        local_4c[0] = local_4c[0] & 0xffff0000;
        local_38 = 7;
        local_3c = 0;
        local_8 = 0xffffffff;
        FUN_004817e0();
      }
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

