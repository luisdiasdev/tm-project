// rea decompile_all
// name: FUN_00481fd0
// address: 0x481fd0
// body: available
// range: 0x481fd0-0x482245 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */

void FUN_00481fd0(undefined4 param_1,int *param_2)

{
  bool bVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  wchar_t *pwVar5;
  undefined4 local_10d8;
  undefined4 local_10d4;
  uint local_bc [4];
  undefined4 local_ac;
  uint local_a8;
  uint local_84 [4];
  undefined4 local_74;
  uint local_70;
  undefined4 local_4c;
  int local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084cc8e;
  local_10 = ExceptionList;
  local_14 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  bVar2 = false;
  FUN_004180b0(&DAT_00880534);
  local_38 = 7;
  local_3c = 0;
  local_4c = (wchar_t *)((uint)local_4c._2_2_ << 0x10);
  local_8._0_1_ = 1;
  local_8._1_3_ = 0;
  do {
    cVar3 = FUN_00419180(&local_4c,&DAT_0087f0cc,L" \t\r\n");
    if (cVar3 == '\0') {
      local_70 = 7;
      local_74 = 0;
      local_84[0] = local_84[0] & 0xffff0000;
      FUN_0040caa0(L"no found: \'{\'",0xd);
      local_8._0_1_ = 2;
      FUN_004193d0(local_84);
      local_8._0_1_ = 1;
      if (7 < local_70) {
        FUN_0040be40(local_84[0],local_70 + 1);
      }
    }
    if (local_3c == 0) {
LAB_004820e9:
      bVar1 = true;
    }
    else {
      iVar4 = FUN_0040fa20(local_84,0,2);
      bVar2 = true;
      local_8 = CONCAT31(local_8._1_3_,3);
      iVar4 = FUN_0040d570(0,*(undefined4 *)(iVar4 + 0x10),&DAT_00881d78,2);
      bVar1 = false;
      if (iVar4 == 0) goto LAB_004820e9;
    }
    local_8._0_1_ = 1;
    local_8._1_3_ = 0;
    if (bVar2) {
      bVar2 = false;
      if (7 < local_70) {
        FUN_0040be40(local_84[0],local_70 + 1);
      }
    }
    if (!bVar1) {
      pwVar5 = local_4c;
      if (local_38 < 8) {
        pwVar5 = (wchar_t *)&local_4c;
      }
      iVar4 = __wcsicmp(pwVar5,L"}");
      if (iVar4 == 0) {
        local_8 = 0xffffffff;
        if (7 < local_38) {
          FUN_0040be40(local_4c,local_38 + 1);
        }
        ExceptionList = local_10;
        __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
        return;
      }
      pwVar5 = local_4c;
      if (local_38 < 8) {
        pwVar5 = (wchar_t *)&local_4c;
      }
      iVar4 = FUN_004138d0(pwVar5,2);
      if (iVar4 < 2) {
        local_a8 = 7;
        local_ac = 0;
        local_bc[0] = local_bc[0] & 0xffff0000;
        FUN_0040caa0(L"invalid param",0xd);
        local_8._0_1_ = 4;
        FUN_004193d0(local_bc);
        local_8._0_1_ = 1;
        if (7 < local_a8) {
          FUN_0040be40(local_bc[0],local_a8 + 1);
        }
        local_a8 = 7;
        local_ac = 0;
        local_bc[0] = local_bc[0] & 0xffff0000;
      }
      (**(code **)(*param_2 + 0x20))(local_10d8,local_10d4);
    }
  } while( true );
}

