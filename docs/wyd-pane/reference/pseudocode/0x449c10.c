// rea decompile_all
// name: FUN_00449c10
// address: 0x449c10
// body: available
// range: 0x449c10-0x44a01e inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00449c10(float param_1)

{
  byte bVar1;
  byte bVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int in_ECX;
  int iVar7;
  undefined4 uVar8;
  int *piVar9;
  byte local_1a;
  byte local_19;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)(in_ECX + 8) == '\0') goto LAB_0044a00e;
  if (*(char *)(in_ECX + 0x198) != '\0') {
    bVar1 = *(byte *)(in_ECX + 0x199);
    bVar2 = *(byte *)(in_ECX + 0x184);
    if (bVar1 < bVar2) {
      if (bVar1 == 0) {
        local_1a = 0;
        local_19 = 1;
      }
      else {
        local_1a = (byte)((ulonglong)bVar2 % (ulonglong)(longlong)(int)(uint)bVar1);
        local_19 = (char)((ulonglong)bVar2 / (ulonglong)(longlong)(int)(uint)bVar1) + 1;
      }
      if (local_1a == 0) {
        local_19 = local_19 - 1;
        local_1a = bVar1;
      }
    }
    else {
      local_19 = 1;
      local_1a = bVar2;
    }
  }
  if (*(int *)(in_ECX + 0x1b4) == 0) {
    if (*(char *)(in_ECX + 0x186) == '\0') {
      if (*(char *)(in_ECX + 0x198) == '\0') {
        iVar7 = **(int **)(in_ECX + 0x30);
        piVar9 = (int *)0x0;
        uVar8 = *(undefined4 *)(iVar7 + 100);
      }
      else {
        iVar7 = **(int **)(in_ECX + 0x30);
        local_10 = *(int *)(iVar7 + 4);
        iVar5 = (*(int *)(iVar7 + 0xc) - local_10) / (int)(uint)*(byte *)(in_ECX + 0x199);
        iVar6 = (*(int *)(iVar7 + 0x10) - *(int *)(iVar7 + 8)) /
                (int)(uint)*(byte *)(in_ECX + 0x19a);
        local_18 = (local_1a - 1) * iVar5 + local_10;
        local_14 = (local_19 - 1) * iVar6 + *(int *)(iVar7 + 8);
        local_10 = (uint)local_1a * iVar5 + -1 + local_10;
        local_c = (uint)local_19 * iVar6 + -1 + *(int *)(iVar7 + 8);
        iVar7 = **(int **)(in_ECX + 0x30);
        uVar8 = *(undefined4 *)(iVar7 + 100);
        piVar9 = &local_18;
      }
      FUN_00477820(iVar7,uVar8,in_ECX + 0xbc,piVar9);
    }
    else {
      FUN_004777b0(**(undefined4 **)(in_ECX + 0x30),
                   (float)(*(int *)(in_ECX + 0xc4) - *(int *)(in_ECX + 0xbc)) * 0.5 +
                   (float)*(int *)(in_ECX + 0xbc),
                   (float)*(int *)(in_ECX + 0xc0) +
                   (float)(*(int *)(in_ECX + 200) - *(int *)(in_ECX + 0xc0)) * 0.5,
                   (float)*(int *)(in_ECX + 0x1ac),(float)*(int *)(in_ECX + 0x1b0),0x3f800000,
                   *(undefined4 *)(in_ECX + 0x1a8));
    }
  }
  else {
    if (*(char *)(in_ECX + 0x1a6) != '\0') {
      (**(code **)(**(int **)(DAT_009b279c + 4) + 0x28))(*(int **)(DAT_009b279c + 4));
      FUN_004ac0c0(0,6,1);
      FUN_004ac0c0(0,5,1);
      FUN_004ac0c0(0,7,1);
    }
    if (*(char *)(in_ECX + 0x198) == '\0') {
      if (*(int *)(in_ECX + 0x1b8) == 0) {
        iVar5 = *(int *)(in_ECX + 0x1b4);
        local_c = *(int *)(iVar5 + 0xcc);
        local_10 = *(int *)(iVar5 + 200);
        local_18 = 0;
        local_14 = 0;
        uVar8 = *(undefined4 *)(**(int **)(in_ECX + 0x30) + 0x24);
        iVar7 = *(int *)(iVar5 + 0xe8);
        if (iVar7 == 0) {
          iVar7 = *(int *)(iVar5 + 0xe4);
        }
        goto LAB_00449df8;
      }
      FUN_0042e6d0(*(undefined4 *)(in_ECX + 0x1b4),in_ECX + 0xbc,*(int *)(in_ECX + 0x1b8),
                   *(undefined4 *)(**(int **)(in_ECX + 0x30) + 0x24));
    }
    else {
      iVar5 = *(int *)(in_ECX + 0x1b4);
      uVar3 = *(uint *)(iVar5 + 200) / (uint)*(byte *)(in_ECX + 0x199);
      uVar4 = *(uint *)(iVar5 + 0xcc) / (uint)*(byte *)(in_ECX + 0x19a);
      local_18 = (local_1a - 1) * uVar3;
      local_14 = (local_19 - 1) * uVar4;
      local_10 = local_1a * uVar3 + -1;
      local_c = local_19 * uVar4 + -1;
      iVar7 = *(int *)(iVar5 + 0xe8);
      uVar8 = *(undefined4 *)(**(int **)(in_ECX + 0x30) + 0x24);
      if (iVar7 == 0) {
        iVar7 = *(int *)(iVar5 + 0xe4);
      }
LAB_00449df8:
      FUN_00470c50(iVar7,in_ECX + 0xbc,&local_18,uVar8);
    }
    if (*(char *)(in_ECX + 0x1a6) != '\0') {
      (**(code **)(**(int **)(DAT_009b279c + 4) + 0x28))(*(int **)(DAT_009b279c + 4));
      FUN_004ac0c0(0,6,2);
      FUN_004ac0c0(0,5,2);
      FUN_004ac0c0(0,7,2);
    }
  }
  if (*(char *)(in_ECX + 0x198) != '\0') {
    param_1 = param_1 + *(float *)(in_ECX + 0x180);
    *(float *)(in_ECX + 0x180) = param_1;
    if (*(float *)(in_ECX + 0x1a0) < param_1 != (*(float *)(in_ECX + 0x1a0) == param_1)) {
      *(char *)(in_ECX + 0x184) = *(char *)(in_ECX + 0x184) + '\x01';
      *(undefined4 *)(in_ECX + 0x180) = 0;
      if (*(byte *)(in_ECX + 0x19c) < *(byte *)(in_ECX + 0x184)) {
        *(char *)(in_ECX + 0x185) = *(char *)(in_ECX + 0x185) + '\x01';
        *(undefined1 *)(in_ECX + 0x184) = *(undefined1 *)(in_ECX + 0x19b);
      }
    }
    if ((*(short *)(in_ECX + 0x1a4) != -1) &&
       (*(short *)(in_ECX + 0x1a4) <= (short)(ushort)*(byte *)(in_ECX + 0x185))) {
      *(undefined1 *)(in_ECX + 8) = 0;
    }
  }
LAB_0044a00e:
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

