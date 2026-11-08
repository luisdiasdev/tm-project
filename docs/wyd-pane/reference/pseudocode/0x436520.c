// rea decompile_all
// name: FUN_00436520
// address: 0x436520
// body: available
// range: 0x436520-0x4368bb inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00436520(undefined4 param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  float fVar6;
  int iVar7;
  int *piVar8;
  int in_ECX;
  int iVar9;
  float10 fVar10;
  tagRECT local_28;
  tagRECT local_18;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)(in_ECX + 8) != '\0') {
    iVar7 = 0;
    iVar9 = 0;
    if (*(char *)(in_ECX + 0xcc) == '\0') {
      iVar9 = 3;
    }
    else if (*(char *)(in_ECX + 0x211) == '\0') {
      if (*(char *)(in_ECX + 9) != '\0') {
        iVar9 = 2;
      }
    }
    else {
      iVar9 = 1;
      iVar7 = (uint)(*(char *)(in_ECX + 0x295) == '\0') * 2 + -1;
    }
    local_18.left = *(int *)(in_ECX + 0xbc);
    local_18.top = *(int *)(in_ECX + 0xc0);
    local_18.right = *(LONG *)(in_ECX + 0xc4);
    local_18.bottom = *(int *)(in_ECX + 200);
    OffsetRect(&local_18,*(int *)(in_ECX + 0x1fc),*(int *)(in_ECX + 0x200) + iVar7);
    if (*(int *)(in_ECX + 0x218) != 0) {
      local_28.top = local_18.top;
      local_28.left = local_18.left;
      local_28.right = local_18.right;
      local_28.bottom = local_18.bottom;
      InflateRect(&local_28,-4,-4);
      local_28.right = (local_28.bottom - local_28.top) + local_28.left;
      local_18.left = local_28.right + 4;
    }
    if (iVar9 == 3) {
      piVar8 = *(int **)(in_ECX + 0x30);
      if ((uint)(*(int *)(in_ECX + 0x34) - (int)piVar8 >> 2) < 4) {
        iVar7 = *piVar8;
      }
      else {
        iVar7 = piVar8[3];
      }
    }
    else {
      iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + iVar9 * 4);
    }
    FUN_00465840(iVar9,param_1,0);
    FUN_00465840(iVar9,param_1,0);
    if ((((*(char *)(in_ECX + 0x21c) == '\0') ||
         ((uint)(*(int *)(in_ECX + 0x34) - *(int *)(in_ECX + 0x30) >> 2) < 3)) ||
        (*(float *)(in_ECX + 0x220) <= 0.0)) || (iVar9 != 0)) {
      FUN_00477820(iVar7,*(undefined4 *)(iVar7 + 100),in_ECX + 0xbc,0);
    }
    else {
      iVar9 = *(int *)(*(int *)(in_ECX + 0x30) + 8);
      *(undefined4 *)(iVar9 + 0x34) = *(undefined4 *)(iVar9 + 0x34);
      *(undefined4 *)(iVar9 + 0x38) = *(undefined4 *)(iVar9 + 0x38);
      *(undefined4 *)(iVar9 + 0x3c) = *(undefined4 *)(iVar9 + 0x3c);
      *(undefined4 *)(iVar9 + 0x40) = *(undefined4 *)(iVar9 + 0x40);
      uVar1 = *(undefined4 *)(iVar9 + 0x34);
      uVar2 = *(undefined4 *)(iVar9 + 0x38);
      uVar3 = *(undefined4 *)(iVar9 + 0x3c);
      uVar4 = *(undefined4 *)(iVar9 + 0x40);
      if (*(float *)(in_ECX + 0x220) != 3.4e+38) {
        fVar10 = (float10)FUN_0080e5d0();
        fVar6 = (float)fVar10 * 0.3 + 0.7;
        *(float *)(iVar9 + 0x34) = fVar6 * *(float *)(iVar9 + 0x34);
        *(float *)(iVar9 + 0x38) = *(float *)(iVar9 + 0x38) * fVar6;
        *(float *)(iVar9 + 0x3c) = fVar6 * *(float *)(iVar9 + 0x3c);
      }
      iVar5 = *(int *)(*(int *)(in_ECX + 0x30) + 8);
      FUN_00477820(iVar5,*(undefined4 *)(iVar5 + 100),in_ECX + 0xbc,0);
      *(undefined4 *)(iVar9 + 0x34) = uVar1;
      *(undefined4 *)(iVar9 + 0x38) = uVar2;
      *(undefined4 *)(iVar9 + 0x3c) = uVar3;
      *(undefined4 *)(iVar9 + 0x40) = uVar4;
    }
    OffsetRect(&local_18,*(int *)(iVar7 + 0x1c),*(int *)(iVar7 + 0x20));
    if (*(int *)(in_ECX + 500) == 0) {
      piVar8 = (int *)(in_ECX + 0x1b8);
      if (7 < *(uint *)(in_ECX + 0x1cc)) {
        piVar8 = (int *)*piVar8;
      }
      FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),piVar8,iVar7,&local_18,0xffffffff,
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
    }
    else {
      iVar9 = 0;
      if ((*(byte *)(iVar7 + 0x18) & 4) != 0) {
        iVar9 = FUN_00498630(0);
        iVar9 = (((local_18.bottom - local_18.top) - iVar9) + 1) / 2;
      }
      if ((*(byte *)(iVar7 + 0x18) & 8) != 0) {
        iVar9 = FUN_00498630(0);
        iVar9 = (local_18.bottom - iVar9) - local_18.top;
      }
      iVar5 = *(int *)(in_ECX + 0x24);
      if (*(float *)(iVar5 + 0x490) != 1.0) {
        if (*(float *)(iVar7 + 0x60) <= *(float *)(iVar5 + 0x490)) {
          uVar1 = *(undefined4 *)(iVar7 + 0x60);
        }
        else {
          uVar1 = *(undefined4 *)(iVar5 + 0x490);
        }
        *(undefined4 *)(iVar7 + 0x60) = uVar1;
      }
      if (*(int *)(in_ECX + 0x28) != 0) {
        if (*(char *)(*(int *)(in_ECX + 0x28) + 0x48) != '\0') {
          FUN_0049a8d0(in_ECX + 0x1b8,iVar7,
                       (*(int *)(in_ECX + 0x1c) - *(int *)(in_ECX + 0x204)) -
                       *(int *)(in_ECX + 0x1fc),*(undefined4 *)(in_ECX + 0x1f8),0,0);
        }
        FUN_00498860(*(undefined4 *)(in_ECX + 0x28),iVar7,local_18.left,local_18.top + iVar9,
                     *(undefined4 *)(in_ECX + 0x1f8),*(undefined4 *)(in_ECX + 0x40),
                     *(undefined4 *)(in_ECX + 0x44));
        __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
        return;
      }
    }
  }
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

