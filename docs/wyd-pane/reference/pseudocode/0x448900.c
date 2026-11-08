// rea decompile_all
// name: FUN_00448900
// address: 0x448900
// body: available
// range: 0x448900-0x448dae inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00448900(void)

{
  code *pcVar1;
  int iVar2;
  int *in_ECX;
  int iVar3;
  undefined4 uVar4;
  int *piVar5;
  int local_98;
  int local_94;
  int local_90;
  int local_8c;
  int local_88;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  int local_58;
  int local_30;
  float local_2c;
  tagRECT local_28;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)((int)in_ECX + 0x293) != '\0') {
    pcVar1 = *(code **)(*in_ECX + 0x70);
    *(undefined1 *)((int)in_ECX + 0x293) = 0;
    (*pcVar1)();
  }
  if ((char)in_ECX[2] != '\0') {
    FUN_00477820(*(int *)in_ECX[0xc],*(undefined4 *)(*(int *)in_ECX[0xc] + 100),in_ECX + 0x8c,0);
    if (in_ECX[0xb9] == 1) {
      iVar3 = in_ECX[0xc];
      iVar2 = *(int *)(iVar3 + 4);
      local_18 = *(int *)(iVar2 + 4);
      local_14 = *(int *)(iVar2 + 8);
      local_10 = *(int *)(iVar2 + 0xc);
      local_c = *(int *)(iVar2 + 0x10);
      local_2c = (float)(local_10 - local_18);
      if ((((int)local_2c < 1) || (in_ECX[0x86] <= in_ECX[0x84])) || (in_ECX[0x85] <= in_ECX[0x84]))
      {
        iVar3 = *(int *)(iVar3 + 4);
        uVar4 = *(undefined4 *)(iVar3 + 100);
        piVar5 = (int *)0x0;
      }
      else {
        local_30 = in_ECX[0x8e] - in_ECX[0x8c];
        local_2c = ((float)(in_ECX[0x92] - in_ECX[0x90]) / (float)local_30) * (float)(int)local_2c;
        local_10 = FUN_0080db50();
        if (local_10 < 1) {
          local_10 = 1;
        }
        local_10 = local_18 + local_10;
        iVar3 = *(int *)(iVar3 + 4);
        uVar4 = *(undefined4 *)(iVar3 + 100);
        piVar5 = &local_18;
      }
    }
    else if (in_ECX[0xb9] == 2) {
      iVar3 = in_ECX[0xc];
      iVar2 = *(int *)(iVar3 + 4);
      local_18 = *(int *)(iVar2 + 4);
      local_14 = *(int *)(iVar2 + 8);
      local_10 = *(int *)(iVar2 + 0xc);
      local_c = *(int *)(iVar2 + 0x10);
      local_30 = local_10 - local_18;
      if (((local_30 < 1) || (in_ECX[0x86] <= in_ECX[0x84])) || (in_ECX[0x85] <= in_ECX[0x84])) {
        iVar3 = *(int *)(iVar3 + 4);
        piVar5 = (int *)0x0;
        uVar4 = *(undefined4 *)(iVar3 + 100);
      }
      else {
        local_2c = (float)(in_ECX[0x92] - in_ECX[0x90]);
        if ((float)local_30 < local_2c == ((float)local_30 == local_2c)) {
          local_10 = FUN_0080db50();
          piVar5 = &local_18;
          iVar3 = *(int *)(iVar3 + 4);
          uVar4 = 4;
        }
        else {
          iVar3 = *(int *)(iVar3 + 4);
          piVar5 = &local_18;
          uVar4 = 2;
        }
      }
    }
    else {
      iVar3 = *(int *)(in_ECX[0xc] + 4);
      piVar5 = (int *)0x0;
      uVar4 = *(undefined4 *)(iVar3 + 100);
    }
    FUN_00477820(iVar3,uVar4,in_ECX + 0x90,piVar5);
    if ((char)in_ECX[0xa4] == '\0') {
      FUN_00477820(*(int *)(in_ECX[0xc] + 8),*(undefined4 *)(*(int *)(in_ECX[0xc] + 8) + 100),
                   in_ECX + 0x2f,0);
    }
    iVar3 = in_ECX[0xb8];
    local_30 = iVar3;
    if (0 < iVar3) {
      local_2c = (float)(in_ECX[0x31] - in_ECX[0x2f]) / (float)iVar3;
      local_30 = 1;
      if (1 < iVar3) {
        do {
          iVar3 = local_30;
          local_18 = in_ECX[0x2f];
          local_14 = in_ECX[0x30];
          local_10 = in_ECX[0x31];
          local_c = in_ECX[0x32];
          iVar2 = FUN_0080db50();
          local_18 = iVar2 + -1 + in_ECX[0x2f];
          local_10 = local_18 + 1;
          FUN_00477820(*(int *)(in_ECX[0xc] + 0xc),
                       *(undefined4 *)(*(int *)(in_ECX[0xc] + 0xc) + 100),&local_18,0);
          local_30 = iVar3 + 1;
        } while (local_30 < in_ECX[0xb8]);
      }
    }
    if (*(char *)((int)in_ECX + 0x291) != '\0') {
      FUN_00443360();
      if ((DAT_009b1444 == 1) && ((float)in_ECX[0xba] <= 0.0)) {
        FUN_00448410(0,0x3f800000);
      }
      if (0.0 < (float)in_ECX[0xba]) {
        local_2c = (float)in_ECX[0xba] - (float)in_ECX[0xbb] * _DAT_009b1414;
        if (local_2c < 0.0) {
          local_2c = 0.0;
        }
        in_ECX[0xba] = (int)local_2c;
        if ((local_2c == 0.0) && (*(char *)((int)in_ECX + 0x292) != '\0')) {
          (**(code **)(*in_ECX + 0x4c))(0);
        }
        local_28.top = in_ECX[0xaa];
        local_28.left = in_ECX[0xa9];
        local_28.right = in_ECX[0xab];
        local_28.bottom = in_ECX[0xac];
        OffsetRect(&local_28,in_ECX[0x2f],in_ECX[0x30]);
        local_98 = in_ECX[0xb2];
        local_18 = 0x3f800000;
        local_94 = in_ECX[0xb3];
        local_14 = 0x3f800000;
        local_90 = in_ECX[0xb4];
        local_10 = 0x3f800000;
        local_58 = in_ECX[0xba];
        local_8c = in_ECX[0xb5];
        local_88 = in_ECX[0xb6];
        local_60 = 0x3f800000;
        local_5c = 0x3f800000;
        local_64 = 0x3f800000;
        local_c = local_58;
        FUN_00477820(&local_98,1,&local_28,0);
      }
    }
    if (in_ECX[0x72] != 0) {
      iVar3 = *(int *)in_ECX[0xc];
      local_28.left = in_ECX[0x2f];
      local_28.top = in_ECX[0x30];
      local_28.right = in_ECX[0x31];
      local_28.bottom = in_ECX[0x32];
      OffsetRect(&local_28,*(int *)(iVar3 + 0x1c),*(int *)(iVar3 + 0x20));
      uVar4 = *(undefined4 *)(iVar3 + 0x18);
      *(undefined4 *)(iVar3 + 0x18) = 5;
      FUN_00465740(0);
      piVar5 = in_ECX + 0x6e;
      if (7 < (uint)in_ECX[0x73]) {
        piVar5 = (int *)*piVar5;
      }
      FUN_00477cd0(piVar5,iVar3,&local_28,0xffffffff,in_ECX[0x10],in_ECX[0x11],0);
      *(undefined4 *)(iVar3 + 0x18) = uVar4;
    }
    __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
    return;
  }
  in_ECX[0xba] = 0;
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

