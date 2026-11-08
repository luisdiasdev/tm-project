// rea decompile_all
// name: FUN_0042c550
// address: 0x42c550
// body: available
// range: 0x42c550-0x42c83f inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Ghidra Listing reports a terminal call at 0x42c641 to 0x842981 FUN_00842981 (external=false, thunk=false, hasNoReturn=true); fallthrough is excluded by Ghidra's flow model. This records Ghidra's FunctionManager flag, not an independent verification that the callee cannot return.; Ghidra Listing reports a terminal call at 0x42c70c to 0x842981 FUN_00842981 (external=false, thunk=false, hasNoReturn=true); fallthrough is excluded by Ghidra's flow model. This records Ghidra's FunctionManager flag, not an independent verification that the callee cannot return.; Ghidra Listing reports a terminal call at 0x42c730 to 0x842981 FUN_00842981 (external=false, thunk=false, hasNoReturn=true); fallthrough is excluded by Ghidra's flow model. This records Ghidra's FunctionManager flag, not an independent verification that the callee cannot return.; Ghidra Listing reports a terminal call at 0x42c74f to 0x842981 FUN_00842981 (external=false, thunk=false, hasNoReturn=true); fallthrough is excluded by Ghidra's flow model. This records Ghidra's FunctionManager flag, not an independent verification that the callee cannot return.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_0042c550(void)

{
  uint *puVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  int in_ECX;
  int *piVar7;
  undefined4 uVar8;
  int local_34;
  int local_30;
  int *local_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)(in_ECX + 8) != '\0') {
    switch(*(undefined4 *)(in_ECX + 0x2a0)) {
    case 1:
    case 7:
      FUN_0042bba0();
      break;
    default:
      uVar8 = 0xffffffff;
      if (*(char *)(in_ECX + 0x363) == '\0') {
        uVar8 = 0xff7c7c7c;
      }
      iVar2 = *(int *)(in_ECX + 0x33d);
      if (iVar2 != 0) {
        iVar4 = *(int *)(iVar2 + 0xe8);
        if (iVar4 == 0) {
          iVar4 = *(int *)(iVar2 + 0xe4);
        }
        FUN_00470c50(iVar4,in_ECX + 0xbc,in_ECX + 0x34d,uVar8);
      }
      break;
    case 3:
      FUN_0042b850();
      break;
    case 4:
      FUN_0042bf70();
      break;
    case 6:
      FUN_0042a9a0();
    }
    if (((*(char *)(in_ECX + 0x2c0) != '\0') && (*(int *)(in_ECX + 0x2d1) != 0)) &&
       (*(int *)(in_ECX + 0x2c) != 0)) {
      local_18 = *(int *)(in_ECX + 0xbc);
      local_14 = *(int *)(in_ECX + 0xc0);
      local_10 = *(int *)(in_ECX + 0xc4);
      local_c = *(undefined4 *)(in_ECX + 200);
      if ((uint)(*(int *)(in_ECX + 0x34) - *(int *)(in_ECX + 0x30) >> 2) < 2) {
                    /* WARNING: Subroutine does not return */
        FUN_00842981("invalid vector<T> subscript");
      }
      piVar7 = (int *)(in_ECX + 0x2c1);
      piVar5 = piVar7;
      if (7 < *(uint *)(in_ECX + 0x2d5)) {
        piVar5 = (int *)*piVar7;
      }
      FUN_00476010(piVar5,*(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_34);
      local_c = local_30 + 4 + local_14;
      iVar2 = *(int *)(in_ECX + 0x345);
      local_18 = (local_10 - local_34) + -4;
      local_1c = *(int *)(iVar2 + 0xcc);
      local_20 = *(int *)(iVar2 + 200);
      local_28 = 0;
      local_24 = 0;
      iVar4 = *(int *)(iVar2 + 0xe8);
      if (iVar4 == 0) {
        iVar4 = *(int *)(iVar2 + 0xe4);
      }
      FUN_00470c50(iVar4,&local_18,&local_28,0x99ffffff);
      local_10 = local_10 + -1;
      local_14 = local_14 + 1;
      local_18 = (local_10 - local_34) + -2;
      local_c = local_30 + 2 + local_14;
      if ((uint)(*(int *)(in_ECX + 0x34) - *(int *)(in_ECX + 0x30) >> 2) < 2) {
                    /* WARNING: Subroutine does not return */
        FUN_00842981("invalid vector<T> subscript");
      }
      *(undefined4 *)(*(int *)(*(int *)(in_ECX + 0x30) + 4) + 0x14) = 0;
      if ((uint)(*(int *)(in_ECX + 0x34) - *(int *)(in_ECX + 0x30) >> 2) < 2) {
                    /* WARNING: Subroutine does not return */
        FUN_00842981("invalid vector<T> subscript");
      }
      puVar1 = (uint *)(*(int *)(*(int *)(in_ECX + 0x30) + 4) + 0x18);
      *puVar1 = *puVar1 & 0xfffffff0;
      if ((uint)(*(int *)(in_ECX + 0x34) - *(int *)(in_ECX + 0x30) >> 2) < 2) {
                    /* WARNING: Subroutine does not return */
        FUN_00842981("invalid vector<T> subscript");
      }
      cVar3 = FUN_0042a510(piVar7,*(undefined4 *)(in_ECX + 0x2c));
      if (cVar3 != '\0') {
        FUN_00442b70();
        local_20 = local_10 - local_18;
        local_1c = local_c - local_14;
        local_28 = 0;
        local_24 = 0;
        local_2c = piVar7;
        if (7 < *(uint *)(in_ECX + 0x2d5)) {
          local_2c = (int *)*piVar7;
        }
        piVar5 = (int *)FUN_0042e7c0(1);
        piVar6 = (int *)FUN_0042e7c0(1);
        FUN_00476b80(*(undefined4 *)(*(int *)(in_ECX + 0x2c) + 0x44),local_2c,
                     *(undefined4 *)(*piVar6 + 0x14),&local_28,0xffffffff,0xffffffff,
                     *(undefined4 *)(*piVar5 + 0x18),*(undefined4 *)(in_ECX + 0x40),
                     *(undefined4 *)(in_ECX + 0x44),0);
        FUN_0040c8a0(piVar7,0,0xffffffff);
        *(undefined1 *)(*(int *)(in_ECX + 0x2c) + 0x48) = 0;
      }
      if (7 < *(uint *)(in_ECX + 0x2d5)) {
        piVar7 = (int *)*piVar7;
      }
      FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x2c),piVar7,
                   *(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_18,0xffffffff,
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
    }
    FUN_00443360();
  }
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

