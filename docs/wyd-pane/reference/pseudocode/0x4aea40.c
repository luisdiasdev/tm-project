// rea decompile_all
// name: FUN_004aea40
// address: 0x4aea40
// body: available
// range: 0x4aea40-0x4aedbf inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_004aea40(undefined4 param_1)

{
  undefined2 *puVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  void *_Dst;
  char *in_ECX;
  uint in_stack_00000018;
  undefined1 local_1ac [16];
  undefined4 local_19c;
  undefined4 local_198;
  undefined4 uStack_188;
  size_t local_15c;
  undefined4 local_158;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined2 local_28;
  undefined1 local_24;
  undefined4 local_23;
  undefined4 local_1c;
  undefined1 uStack_18;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  puStack_c = &LAB_0084e407;
  local_10 = ExceptionList;
  local_14 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_8 = 0;
  if (*in_ECX != '\0') {
    FUN_004ad670();
  }
  local_158 = local_1ac;
  local_198 = 0xf;
  local_19c = 0;
  local_1ac[0] = 0;
  FUN_0040c7b0(&param_1,0,0xffffffff);
  iVar3 = FUN_004ae7d0();
  if (iVar3 == 0) {
    local_8 = 0xffffffff;
    if (0xf < in_stack_00000018) {
      FUN_00407f60();
    }
    goto LAB_004aeda2;
  }
  if (iVar3 == 1) {
    FUN_00407230();
    local_8._0_1_ = 1;
    cVar2 = FUN_00407380();
    if (cVar2 == '\0') {
      local_8 = (uint)local_8._1_3_ << 8;
      FUN_00407310();
      local_8 = 0xffffffff;
      if (0xf < in_stack_00000018) {
        FUN_00407f60();
      }
      goto LAB_004aeda2;
    }
    _strcpy_s((char *)&local_38,0x13,"TRUEVISION-XFILE");
    local_28 = 0x2e;
    iVar4 = FUN_00409900();
    *(int *)(in_ECX + 8) = iVar4 + 0xe;
    local_1c = 0;
    uStack_18 = 0;
    uVar5 = FUN_00675c7c();
    *(undefined4 *)(in_ECX + 4) = uVar5;
    FUN_004073a0();
    uStack_188 = 0x4aeb9d;
    FUN_004073a0();
    iVar3 = *(int *)(in_ECX + 4);
    *(undefined4 *)(iVar3 + iVar4 + -4) = local_38;
    iVar3 = iVar3 + iVar4 + -4;
    *(undefined4 *)(iVar3 + 4) = local_34;
    *(undefined4 *)(iVar3 + 8) = local_30;
    *(undefined4 *)(iVar3 + 0xc) = local_2c;
    *(undefined2 *)(iVar3 + 0x10) = local_28;
LAB_004aecc4:
    thunk_FUN_00409480();
    local_8 = (uint)local_8._1_3_ << 8;
    FUN_00407310();
  }
  else {
    if (iVar3 == 2) {
      FUN_00407230();
      local_8._0_1_ = 2;
      cVar2 = FUN_00407380();
      if (cVar2 == '\0') {
        local_8 = (uint)local_8._1_3_ << 8;
        FUN_00407310();
LAB_004aec0e:
        local_8 = 0xffffffff;
        FUN_00408240();
        goto LAB_004aeda2;
      }
      _strcpy_s((char *)&local_158,4,"DDS");
      iVar3 = FUN_00409900();
      *(int *)(in_ECX + 8) = iVar3 + -1;
      local_24 = 0;
      local_23 = 0;
      uVar5 = FUN_00675c7c();
      *(undefined4 *)(in_ECX + 4) = uVar5;
      FUN_004073a0();
      uStack_188 = 0x4aec7e;
      FUN_004073a0();
      puVar1 = *(undefined2 **)(in_ECX + 4);
      *puVar1 = (undefined2)local_158;
      *(undefined1 *)(puVar1 + 1) = local_158._2_1_;
      local_1c = 0;
      uStack_18 = 0;
      FUN_004af1a0();
      *(undefined4 *)(*(int *)(in_ECX + 4) + 0x54) = local_1c;
      goto LAB_004aecc4;
    }
    if (iVar3 == 3) {
      local_158 = local_1ac;
      local_8._0_1_ = 3;
      local_198 = 0xf;
      local_19c = 0;
      local_1ac[0] = 0;
      FUN_0040c7b0(&param_1,0,0xffffffff);
      cVar2 = FUN_00407580();
      if (cVar2 == '\0') {
        local_8 = (uint)local_8._1_3_ << 8;
        FUN_004074e0();
        goto LAB_004aec0e;
      }
      _Dst = (void *)FUN_00675c7c();
      *(void **)(in_ECX + 4) = _Dst;
      uStack_188 = 0x4aed5e;
      FID_conflict__memcpy(_Dst,(void *)0x0,local_15c);
      *(size_t *)(in_ECX + 8) = local_15c;
      FUN_004074e0();
      local_8 = (uint)local_8._1_3_ << 8;
      FUN_004074e0();
    }
  }
  local_8 = 0xffffffff;
  if (0xf < in_stack_00000018) {
    FUN_00407f60();
  }
LAB_004aeda2:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

