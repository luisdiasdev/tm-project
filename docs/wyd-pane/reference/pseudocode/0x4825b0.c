// rea decompile_all
// name: FUN_004825b0
// address: 0x4825b0
// body: available
// range: 0x4825b0-0x48262a inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */

void FUN_004825b0(undefined4 param_1,undefined4 param_2,char param_3,int *param_4)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined4 ***pppuVar4;
  int iVar5;
  undefined2 *puVar6;
  undefined4 uVar7;
  bool bVar8;
  uint uStack_20d0;
  undefined4 uStack_20b8;
  uint uStack_20b4;
  wchar_t *pwStack_10b0;
  undefined4 uStack_10ac;
  int *local_8c;
  undefined4 **local_88 [4];
  int local_78;
  uint local_74;
  uint auStack_50 [4];
  undefined4 uStack_40;
  uint uStack_3c;
  uint local_18;
  undefined1 *local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  puStack_c = &LAB_0084cd72;
  local_10 = ExceptionList;
  uStack_20d0 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  local_14 = (undefined1 *)&uStack_20d0;
  ExceptionList = &local_10;
  bVar1 = false;
  local_74 = 7;
  local_78 = 0;
  local_88[0] = (undefined4 **)((uint)local_88[0] & 0xffff0000);
  local_8c = param_4;
  local_8._0_1_ = 1;
  local_8._1_3_ = 0;
  local_18 = uStack_20d0;
  do {
    cVar2 = FUN_00419180(local_88,&DAT_0087f0cc,L" \t\r\n");
    if (cVar2 == '\0') {
      local_8 = 0;
      if (param_3 == '\0') {
        *(undefined1 *)((int)param_4 + 0xb) = 0;
      }
      (**(code **)(*param_4 + 0x14))();
      FUN_00477350();
      (**(code **)(*param_4 + 0x10))();
      if ((char)param_4[0x11d] == '\0') {
        FUN_0045f6d0(param_4);
      }
      local_8 = 0xffffffff;
      if (7 < local_74) {
        FUN_0040be40(local_88[0],local_74 + 1);
      }
      ExceptionList = local_10;
      __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
      return;
    }
    if (local_78 == 0) {
LAB_0048270c:
      bVar8 = true;
    }
    else {
      iVar3 = FUN_0040fa20(auStack_50,0,2);
      bVar1 = true;
      local_8 = CONCAT31(local_8._1_3_,2);
      iVar3 = FUN_0040d570(0,*(undefined4 *)(iVar3 + 0x10),&DAT_00881d78,2);
      if (iVar3 == 0) goto LAB_0048270c;
      bVar8 = false;
    }
    local_8._0_1_ = 1;
    local_8._1_3_ = 0;
    if ((bVar1) && (bVar1 = false, 7 < uStack_3c)) {
      FUN_0040be40(auStack_50[0],uStack_3c + 1);
    }
    if (!bVar8) {
      pppuVar4 = (undefined4 ***)local_88[0];
      if (local_74 < 8) {
        pppuVar4 = local_88;
      }
      iVar3 = FUN_004138d0(pppuVar4,0xffffffff);
      if (iVar3 < 1) {
        uStack_3c = 7;
        uStack_40 = 0;
        auStack_50[0] = auStack_50[0] & 0xffff0000;
        FUN_0040caa0(L"invalid line",0xc);
        local_8._0_1_ = 3;
        FUN_004193d0(auStack_50);
        local_8._0_1_ = 1;
        if (7 < uStack_3c) {
          FUN_0040be40(auStack_50[0],uStack_3c + 1);
        }
      }
      iVar5 = __wcsicmp(pwStack_10b0,L"panel");
      if (iVar5 == 0) {
        local_8 = CONCAT31(local_8._1_3_,4);
        if (local_8c == (int *)0x0) {
          local_8c = (int *)FUN_0045dd00(param_1);
        }
        local_8._0_1_ = 1;
        local_8._1_3_ = 0;
        FUN_00481fd0(param_2,local_8c);
        param_4 = local_8c;
      }
      else {
        iVar5 = __wcsicmp(pwStack_10b0,L"element");
        param_4 = local_8c;
        if (iVar5 == 0) {
          if (iVar3 < 2) {
            auStack_50[0] = auStack_50[0] & 0xffff0000;
            uStack_3c = 7;
            uStack_40 = 0;
            FUN_0040caa0(L"invalid line",0xc);
            local_8._0_1_ = 7;
            FUN_004193d0(auStack_50);
            local_8._0_1_ = 1;
            if (7 < uStack_3c) {
              FUN_0040be40(auStack_50[0],uStack_3c + 1);
            }
          }
          iVar3 = FUN_004138d0(uStack_10ac,0xffffffff);
          if ((iVar3 != 1) && (iVar3 != 2)) {
            FUN_0040d040(L"invalid line");
            local_8._0_1_ = 8;
            FUN_004193d0(auStack_50);
            local_8._0_1_ = 1;
            FUN_0040c740();
          }
          local_8 = CONCAT31(local_8._1_3_,9);
          puVar6 = (undefined2 *)((iVar3 != 2) - 1 & uStack_20b4);
          if (puVar6 == (undefined2 *)0x0) {
            puVar6 = &DAT_0087de70;
          }
          uVar7 = FUN_0047c640(uStack_20b8,puVar6);
          local_8._0_1_ = 1;
          local_8._1_3_ = 0;
          FUN_004817f0(param_2,uVar7);
        }
      }
    }
  } while( true );
}

