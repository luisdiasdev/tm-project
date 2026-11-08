// rea decompile_all
// name: FUN_0043ba20
// address: 0x43ba20
// body: available
// range: 0x43ba20-0x43bde4 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_0043ba20(undefined4 param_1)

{
  uint *puVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  HWND hWnd;
  int *in_ECX;
  float10 fVar5;
  tagPOINT *lpPoint;
  uint local_38;
  int local_30;
  tagPOINT local_2c;
  undefined4 local_24;
  int *local_20;
  char local_19;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if ((((char)in_ECX[2] != '\0') && (in_ECX[10] != 0)) && (*(int *)(in_ECX[10] + 0x44) != 0)) {
    fVar5 = (float10)FUN_0046edc0();
    local_20 = (int *)(float)fVar5;
    (**(code **)(*DAT_009b27ac + 0xe4))(DAT_009b27ac,0xae,1);
    local_18 = in_ECX[0x2f];
    local_14 = in_ECX[0x30];
    local_10 = in_ECX[0x31];
    iVar4 = in_ECX[9];
    local_c = in_ECX[0x32];
    iVar3 = FUN_0080db50();
    local_18 = FUN_0080db50();
    local_18 = iVar3 + local_18;
    local_24 = *(undefined4 *)(iVar4 + 0x14);
    local_20 = *(int **)(iVar4 + 0x18);
    iVar3 = FUN_0080db50();
    local_10 = FUN_0080db50();
    local_10 = iVar3 + local_10;
    local_24 = *(undefined4 *)(iVar4 + 0x14);
    local_20 = *(int **)(iVar4 + 0x18);
    iVar3 = FUN_0080db50();
    local_14 = FUN_0080db50();
    local_14 = iVar3 + local_14;
    local_24 = *(undefined4 *)(iVar4 + 0x14);
    local_20 = *(int **)(iVar4 + 0x18);
    iVar4 = FUN_0080db50();
    local_c = FUN_0080db50();
    local_c = iVar4 + local_c;
    (**(code **)(*DAT_009b27ac + 300))(DAT_009b27ac,&local_18);
    GetCursorPos(&local_2c);
    lpPoint = &local_2c;
    hWnd = (HWND)thunk_FUN_004900e0();
    ScreenToClient(hWnd,lpPoint);
    iVar4 = FUN_0080db50();
    local_2c.x = iVar4;
    local_2c.y = FUN_0080db50();
    local_2c.x = iVar4 - *(int *)(in_ECX[9] + 0x14);
    local_2c.y = local_2c.y - *(int *)(in_ECX[9] + 0x18);
    iVar4 = *(int *)in_ECX[0xc];
    *(uint *)(iVar4 + 0x18) = *(uint *)(iVar4 + 0x18) & 0xfffffffc;
    FUN_00465740(0);
    FUN_00465740(0);
    puVar1 = (uint *)(*(int *)(in_ECX[0xc] + 4) + 0x18);
    *puVar1 = *puVar1 & 0xfffffffc;
    FUN_00465740(0);
    FUN_00465740(0);
    FUN_00477820(iVar4,*(undefined4 *)(iVar4 + 100),in_ECX + 0x2f,0);
    if (DAT_009b1444 == 1) {
      (**(code **)(*in_ECX + 0x70))();
      FUN_0046f890(in_ECX + 0x68,0x88ffffff,1);
      (**(code **)(*DAT_009b27ac + 0xe4))(DAT_009b27ac,0xae,0);
      __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
      return;
    }
    local_20 = (int *)0x0;
    if (in_ECX[0xec] - in_ECX[0xeb] >> 2 != 0) {
      if (*(char *)((int)in_ECX + 0x3a9) == '\0') {
        *(undefined1 *)((int)in_ECX + 0x3a9) = 1;
      }
      local_30 = 0;
      if (0 < (int)(in_ECX[0xec] - in_ECX[0xeb] & 0xfffffffcU)) {
        do {
          FUN_0043b950(local_30,&local_18,&local_19,1);
          if (((local_19 != '\0') && (iVar4 = *(int *)(in_ECX[0xeb] + local_30 * 4), iVar4 != 0)) &&
             (local_38 = 0, *(int *)(iVar4 + 0x34) - *(int *)(iVar4 + 0x30) >> 2 != 0)) {
            do {
              piVar2 = *(int **)(*(int *)(iVar4 + 0x30) + local_38 * 4);
              if ((char)piVar2[2] != '\0') {
                piVar2[5] = *(short *)((int)piVar2 + 0x5a) + local_18;
                piVar2[6] = (short)piVar2[0x17] + local_14;
                (**(code **)(*piVar2 + 0x70))();
                if ((DAT_009b2730 == piVar2) && (piVar2[0x28] == 4)) {
                  local_20 = piVar2;
                }
                (**(code **)(*piVar2 + 0xc))(param_1);
              }
              local_38 = local_38 + 1;
            } while (local_38 < (uint)(*(int *)(iVar4 + 0x34) - *(int *)(iVar4 + 0x30) >> 2));
          }
          local_30 = local_30 + 1;
        } while (local_30 < in_ECX[0xec] - in_ECX[0xeb] >> 2);
      }
    }
    piVar2 = local_20;
    (**(code **)(*DAT_009b27ac + 0xe4))(DAT_009b27ac,0xae,0);
    if (piVar2 != (int *)0x0) {
      (**(code **)(*piVar2 + 0x84))(param_1);
    }
    if ((*(char *)((int)in_ECX + 0x18e) != '\0') &&
       ((*(char *)((int)in_ECX + 0x18d) == '\0' ||
        ((uint)in_ECX[0xde] < (uint)(in_ECX[0xec] - in_ECX[0xeb] >> 2))))) {
      (**(code **)(in_ECX[0x6c] + 0xc))(param_1);
    }
  }
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

