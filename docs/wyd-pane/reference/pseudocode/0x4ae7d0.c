// rea decompile_all
// name: FUN_004ae7d0
// address: 0x4ae7d0
// body: available
// range: 0x4ae7d0-0x4aea35 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_004ae7d0(undefined4 *param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 ****ppppuVar3;
  undefined4 ****ppppuVar4;
  uint uVar5;
  undefined4 *puVar6;
  uint uVar7;
  undefined4 *puVar8;
  bool bVar9;
  int in_stack_00000014;
  uint in_stack_00000018;
  undefined1 local_50 [4];
  undefined4 ***local_4c [4];
  uint local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  uint local_8;
  
  puStack_c = &LAB_0084e3b9;
  local_10 = ExceptionList;
  local_14 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_8 = 0;
  if (in_stack_00000014 < 5) {
    local_8 = 0xffffffff;
    if (0xf < in_stack_00000018) {
      FUN_00407f60(param_1,in_stack_00000018 + 1);
    }
  }
  else {
    puVar8 = param_1;
    if (in_stack_00000018 < 0x10) {
      puVar8 = &param_1;
    }
    puVar6 = param_1;
    if (in_stack_00000018 < 0x10) {
      puVar6 = &param_1;
    }
    if (puVar6 != (undefined4 *)((int)puVar8 + in_stack_00000014)) {
      FUN_004af570(local_50,puVar6,(undefined4 *)((int)puVar8 + in_stack_00000014),puVar8,_toupper,
                   local_14);
    }
    FUN_00482f60(local_4c,4);
    uVar1 = local_38;
    uVar5 = local_3c;
    ppppuVar4 = (undefined4 ****)local_4c[0];
    uVar7 = local_3c;
    if (3 < local_3c) {
      uVar7 = 4;
    }
    ppppuVar3 = (undefined4 ****)local_4c[0];
    if (local_38 < 0x10) {
      ppppuVar3 = local_4c;
    }
    iVar2 = FUN_0040d1b0(ppppuVar3,&DAT_008880dc,uVar7);
    bVar9 = false;
    if (iVar2 == 0) {
      if (uVar5 < 4) {
        uVar7 = 0xffffffff;
      }
      else {
        uVar7 = (uint)(uVar5 != 4);
      }
      bVar9 = uVar7 == 0;
    }
    if (bVar9) {
      local_8 = local_8 & 0xffffff00;
      if (0xf < uVar1) {
        FUN_00407f60(ppppuVar4,uVar1 + 1);
      }
      local_38 = 0xf;
      local_3c = 0;
      local_4c[0] = (undefined4 ***)((uint)local_4c[0] & 0xffffff00);
      local_8 = 0xffffffff;
      if (0xf < in_stack_00000018) {
        FUN_00407f60(param_1,in_stack_00000018 + 1);
      }
    }
    else {
      uVar7 = uVar5;
      if (3 < uVar5) {
        uVar7 = 4;
      }
      ppppuVar3 = ppppuVar4;
      if (uVar1 < 0x10) {
        ppppuVar3 = local_4c;
      }
      iVar2 = FUN_0040d1b0(ppppuVar3,&DAT_008880d4,uVar7);
      bVar9 = false;
      if (iVar2 == 0) {
        if (uVar5 < 4) {
          uVar7 = 0xffffffff;
        }
        else {
          uVar7 = (uint)(uVar5 != 4);
        }
        bVar9 = uVar7 == 0;
      }
      if (bVar9) {
        local_8 = local_8 & 0xffffff00;
        if (0xf < uVar1) {
          FUN_00407f60(ppppuVar4,uVar1 + 1);
        }
        local_38 = 0xf;
        local_3c = 0;
        local_4c[0] = (undefined4 ***)((uint)local_4c[0] & 0xffffff00);
        local_8 = 0xffffffff;
        if (0xf < in_stack_00000018) {
          FUN_00407f60(param_1,in_stack_00000018 + 1);
        }
      }
      else {
        uVar7 = uVar5;
        if (3 < uVar5) {
          uVar7 = 4;
        }
        if (uVar1 < 0x10) {
          ppppuVar4 = local_4c;
        }
        iVar2 = FUN_0040d1b0(ppppuVar4,&DAT_008880cc,uVar7);
        bVar9 = false;
        if (iVar2 == 0) {
          if (uVar5 < 4) {
            uVar5 = 0xffffffff;
          }
          else {
            uVar5 = (uint)(uVar5 != 4);
          }
          bVar9 = uVar5 == 0;
        }
        local_8 = local_8 & 0xffffff00;
        if (bVar9) {
          FUN_00408240();
          local_8 = 0xffffffff;
          FUN_00408240();
        }
        else {
          FUN_00408240();
          local_8 = 0xffffffff;
          FUN_00408240();
        }
      }
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

