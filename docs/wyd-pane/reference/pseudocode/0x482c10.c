// rea decompile_all
// name: FUN_00482c10
// address: 0x482c10
// body: available
// range: 0x482c10-0x482f1c inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00482c10(undefined4 param_1,int *param_2,undefined1 param_3,int param_4)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  bool bVar4;
  undefined1 local_270 [16];
  undefined4 local_260;
  undefined4 local_25c;
  undefined4 uStack_250;
  undefined4 uStack_24c;
  char **ppcStack_248;
  char *pcStack_244;
  char **ppcStack_240;
  int *piStack_23c;
  uint uStack_238;
  undefined1 local_228 [4];
  undefined4 local_224;
  undefined4 local_218;
  undefined1 *local_214;
  int local_210;
  undefined4 local_20c;
  char local_205;
  undefined1 local_204 [440];
  char *local_4c [4];
  undefined4 local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084ce3c;
  local_10 = ExceptionList;
  uStack_238 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_218 = CONCAT31(local_218._1_3_,param_3);
  piStack_23c = (int *)0x1;
  local_20c = param_1;
  local_210 = param_4;
  local_214 = (undefined1 *)0x0;
  ppcStack_240 = (char **)0x482c77;
  local_14 = uStack_238;
  FUN_004082a0();
  local_8 = 0;
  if ((uint)param_2[4] < 6) {
LAB_00482ce7:
    local_205 = '\0';
  }
  else {
    piStack_23c = (int *)0x5;
    ppcStack_240 = local_4c;
    pcStack_244 = (char *)0x482c8d;
    pcStack_244 = (char *)FUN_00482f60();
    uVar3 = *(uint *)((int)pcStack_244 + 0x10);
    local_214 = (undefined1 *)0x1;
    piStack_23c = (int *)uVar3;
    if (4 < uVar3) {
      piStack_23c = (int *)0x5;
    }
    if (0xf < *(uint *)((int)pcStack_244 + 0x14)) {
      pcStack_244 = *(char **)pcStack_244;
    }
    ppcStack_240 = (char **)0x883080;
    ppcStack_248 = (char **)0x482cb8;
    iVar2 = FUN_0040d1b0();
    bVar4 = false;
    if (iVar2 == 0) {
      if (uVar3 < 5) {
        uVar3 = 0xffffffff;
      }
      else {
        uVar3 = (uint)(uVar3 != 5);
      }
      bVar4 = uVar3 == 0;
    }
    local_205 = '\x01';
    param_4 = local_210;
    if (!bVar4) goto LAB_00482ce7;
  }
  local_8 = 0;
  if ((((uint)local_214 & 1) != 0) && (0xf < local_38)) {
    piStack_23c = (int *)(local_38 + 1);
    ppcStack_240 = (char **)local_4c[0];
    pcStack_244 = (char *)0x482d0f;
    FUN_00407f60();
  }
  if (local_205 == '\0') {
    piStack_23c = param_2;
    if (0xf < (uint)param_2[5]) {
      piStack_23c = (int *)*param_2;
    }
    ppcStack_240 = (char **)0x482db6;
    cVar1 = FUN_004082c0();
    if (cVar1 == '\0') {
      ppcStack_240 = (char **)param_2;
      if (0xf < (uint)param_2[5]) {
        ppcStack_240 = (char **)*param_2;
      }
      pcStack_244 = (char *)0x482dd2;
      cVar1 = FUN_004082c0();
      if (cVar1 == '\0') {
        if (local_210 == 0) {
          ppcStack_240 = (char **)local_20c;
          pcStack_244 = (char *)0x482e18;
          param_4 = FUN_0045dd00();
        }
        *(int *)(param_4 + 0x20) = *(int *)(param_4 + 0x18) + 100;
        *(undefined4 *)(param_4 + 0x28) = 0xffff0000;
        *(int *)(param_4 + 0x1c) = *(int *)(param_4 + 0x14) + 100;
        ppcStack_240 = (char **)0x482e3a;
        FUN_0046ef80();
        ppcStack_240 = (char **)0x11;
        pcStack_244 = "ui/Default.guimat";
        *(undefined1 *)(param_4 + 8) = 1;
        local_38 = 0xf;
        local_3c = 0;
        local_4c[0] = (char *)((uint)local_4c[0] & 0xffffff00);
        ppcStack_248 = (char **)0x482e5a;
        FUN_0040c9a0();
        ppcStack_240 = local_4c;
        local_8._0_1_ = 3;
        pcStack_244 = (char *)0x482e69;
        FUN_0047a110();
        local_8 = (uint)local_8._1_3_ << 8;
        if (0xf < local_38) {
          ppcStack_240 = (char **)(local_38 + 1);
          pcStack_244 = local_4c[0];
          ppcStack_248 = (char **)0x482e82;
          FUN_00407f60();
        }
        if (*(char *)(param_4 + 0x474) == '\0') {
          pcStack_244 = (char *)0x482e96;
          ppcStack_240 = (char **)param_4;
          FUN_0045f6d0();
        }
        goto LAB_00482e96;
      }
    }
    pcStack_244 = (char *)local_218;
    ppcStack_248 = (char **)local_204;
    uStack_24c = local_20c;
    uStack_250 = 0x482df7;
    ppcStack_240 = (char **)param_4;
    FUN_004825b0();
  }
  else {
    local_228[0] = 0;
    local_224 = 0;
    local_214 = local_270;
    local_25c = 0xf;
    local_260 = 0;
    local_8._0_1_ = 2;
    local_270[0] = 0;
    FUN_0040c7b0(param_2,0,0xffffffff);
    FUN_00407580();
    piStack_23c = (int *)local_228;
    ppcStack_240 = (char **)0x482d68;
    FUN_004082d0();
    pcStack_244 = (char *)local_218;
    ppcStack_248 = (char **)local_204;
    uStack_24c = local_20c;
    uStack_250 = 0x482d89;
    ppcStack_240 = (char **)param_4;
    FUN_004825b0();
    local_8 = (uint)local_8._1_3_ << 8;
    ppcStack_240 = (char **)0x482d99;
    FUN_004074e0();
  }
LAB_00482e96:
  ppcStack_240 = (char **)0xffffffff;
  pcStack_244 = (char *)0x0;
  ppcStack_248 = (char **)param_2;
  local_38 = 0xf;
  local_3c = 0;
  local_4c[0] = (char *)((uint)local_4c[0] & 0xffffff00);
  uStack_24c = 0x482eaf;
  FUN_0040c7b0();
  ppcStack_240 = (char **)0xffffffff;
  pcStack_244 = (char *)0x0;
  ppcStack_248 = local_4c;
  local_8._0_1_ = 4;
  uStack_24c = 0x482ec5;
  FUN_0040c7b0();
  local_8 = (uint)local_8._1_3_ << 8;
  if (0xf < local_38) {
    ppcStack_240 = (char **)(local_38 + 1);
    pcStack_244 = local_4c[0];
    ppcStack_248 = (char **)0x482ede;
    FUN_00407f60();
  }
  local_38 = 0xf;
  local_3c = 0;
  local_4c[0] = (char *)((uint)local_4c[0] & 0xffffff00);
  local_8 = 0xffffffff;
  ppcStack_240 = (char **)0x482efd;
  FUN_004184b0();
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

