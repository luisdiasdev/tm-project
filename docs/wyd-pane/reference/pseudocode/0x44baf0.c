// rea decompile_all
// name: FUN_0044baf0
// address: 0x44baf0
// body: available
// range: 0x44baf0-0x44bf99 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Ghidra Listing reports a terminal call at 0x44bd53 to 0x842981 FUN_00842981 (external=false, thunk=false, hasNoReturn=true); fallthrough is excluded by Ghidra's flow model. This records Ghidra's FunctionManager flag, not an independent verification that the callee cannot return.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_0044baf0(void)

{
  RECT *lprc2;
  int *piVar1;
  int iVar2;
  undefined4 *puVar3;
  char cVar4;
  BOOL BVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int in_ECX;
  int iVar9;
  int iVar10;
  uint uVar11;
  float10 extraout_ST1;
  float10 fVar12;
  int local_30;
  uint local_20;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if ((((*(char *)(in_ECX + 8) != '\0') &&
       (lprc2 = (RECT *)(in_ECX + 0xbc),
       *(int *)(in_ECX + 0xc4) != *(int *)(in_ECX + 0xbc) &&
       -1 < *(int *)(in_ECX + 0xc4) - *(int *)(in_ECX + 0xbc))) &&
      (*(int *)(in_ECX + 200) != *(int *)(in_ECX + 0xc0) &&
       -1 < *(int *)(in_ECX + 200) - *(int *)(in_ECX + 0xc0))) && (*(int *)(in_ECX + 0x194) != 0)) {
    if (((*(int *)(in_ECX + 0x66c) != *(int *)(in_ECX + 0x600)) ||
        (BVar5 = EqualRect((RECT *)(in_ECX + 0x604),lprc2), BVar5 == 0)) ||
       (cVar4 = FUN_0044b550(in_ECX + 0x614,in_ECX + 0x184), cVar4 != '\0')) {
      *(undefined4 *)(in_ECX + 0x66c) = *(undefined4 *)(in_ECX + 0x600);
      FUN_0040c7b0(in_ECX + 0x184,0,0xffffffff);
      *(LONG *)(in_ECX + 0x604) = lprc2->left;
      *(undefined4 *)(in_ECX + 0x608) = *(undefined4 *)(in_ECX + 0xc0);
      *(int *)(in_ECX + 0x60c) = *(int *)(in_ECX + 0xc4);
      iVar2 = *(int *)(in_ECX + 0xc4) + *(int *)(in_ECX + 0x1f8) * -2;
      *(undefined4 *)(in_ECX + 0x610) = *(undefined4 *)(in_ECX + 200);
      local_30 = lprc2->left;
      *(int *)(in_ECX + 0x65c) = local_30;
      *(undefined4 *)(in_ECX + 0x660) = *(undefined4 *)(in_ECX + 0xc0);
      *(int *)(in_ECX + 0x664) = iVar2;
      *(int *)(in_ECX + 0x668) = *(int *)(in_ECX + 200) + *(int *)(in_ECX + 0x1fc) * -2;
      local_20 = 0;
      if (*(int *)(in_ECX + 0x194) != 0) {
        fVar12 = (float10)1;
        do {
          puVar8 = (undefined4 *)(in_ECX + 0x184);
          if (*(uint *)(in_ECX + 0x194) <= local_20) goto LAB_0044bd4e;
          if (0xf < *(uint *)(in_ECX + 0x198)) {
            puVar8 = (undefined4 *)*puVar8;
          }
          iVar9 = (int)*(char *)((int)puVar8 + local_20);
          if (((iVar9 - 1U < 0x7f) && (iVar6 = *(int *)(in_ECX + 0x200 + iVar9 * 4), iVar6 != 0)) &&
             ((fVar12 <= (float10)(*(int *)(iVar6 + 0xc) - *(int *)(iVar6 + 4)) &&
              (fVar12 <= (float10)(*(int *)(iVar6 + 0x10) - *(int *)(iVar6 + 8)))))) {
            iVar6 = FUN_0080db50();
            *(int *)(in_ECX + 0x400 + iVar9 * 4) = iVar6;
            local_30 = iVar6 + local_30;
            if (iVar2 < local_30) break;
            *(int *)(in_ECX + 0x664) = local_30;
            fVar12 = extraout_ST1;
          }
          local_20 = local_20 + 1;
        } while (local_20 < *(uint *)(in_ECX + 0x194));
      }
      *(int *)(in_ECX + 0x668) = *(int *)(in_ECX + 0x660) + *(int *)(in_ECX + 0x600);
    }
    if ((*(int *)(in_ECX + 0x664) != *(int *)(in_ECX + 0x65c) &&
         -1 < *(int *)(in_ECX + 0x664) - *(int *)(in_ECX + 0x65c)) &&
       (*(int *)(in_ECX + 0x668) != *(int *)(in_ECX + 0x660) &&
        -1 < *(int *)(in_ECX + 0x668) - *(int *)(in_ECX + 0x660))) {
      iVar9 = **(int **)(in_ECX + 0x30);
      iVar2 = *(int *)(in_ECX + 0x664);
      iVar6 = *(int *)(in_ECX + 0x65c);
      iVar10 = *(int *)(in_ECX + 0x668);
      if ((*(uint *)(iVar9 + 0x18) & 2) == 0) {
        if ((*(uint *)(iVar9 + 0x18) & 1) == 0) {
          iVar7 = *(int *)(in_ECX + 0x1f8);
        }
        else {
          iVar7 = (((*(int *)(in_ECX + 0xc4) - iVar2) - lprc2->left) + iVar6) / 2 +
                  *(int *)(in_ECX + 0x1f8);
        }
        iVar7 = iVar7 + lprc2->left;
      }
      else {
        iVar7 = ((*(int *)(in_ECX + 0xc4) - *(int *)(in_ECX + 0x1f8)) - iVar2) + iVar6;
      }
      iVar2 = iVar2 + (iVar7 - iVar6);
      if ((*(uint *)(iVar9 + 0x18) & 8) == 0) {
        if ((*(uint *)(iVar9 + 0x18) & 4) == 0) {
          iVar6 = *(int *)(in_ECX + 0xc0) + *(int *)(in_ECX + 0x1fc);
          iVar10 = iVar10 + (iVar6 - *(int *)(in_ECX + 0x660));
        }
        else {
          iVar6 = ((*(int *)(in_ECX + 200) - *(int *)(in_ECX + 0x600)) - *(int *)(in_ECX + 0xc0)) /
                  2 + *(int *)(in_ECX + 0xc0) + *(int *)(in_ECX + 0x1fc);
          iVar10 = iVar10 + (iVar6 - *(int *)(in_ECX + 0x660));
        }
      }
      else {
        iVar6 = (*(int *)(in_ECX + 200) - *(int *)(in_ECX + 0x600)) - *(int *)(in_ECX + 0x1fc);
        iVar10 = iVar10 + (iVar6 - *(int *)(in_ECX + 0x660));
      }
      if (*(char *)(in_ECX + 0x180) != '\0') {
        (**(code **)(**(int **)(DAT_009b279c + 4) + 0x28))(*(int **)(DAT_009b279c + 4));
        FUN_004ac0c0(0,6,1);
        FUN_004ac0c0(0,5,1);
        FUN_004ac0c0(0,7,1);
      }
      piVar1 = (int *)(in_ECX + 0x64c);
      *piVar1 = iVar7;
      *(int *)(in_ECX + 0x650) = iVar6;
      *(int *)(in_ECX + 0x654) = iVar2;
      *(int *)(in_ECX + 0x658) = iVar10;
      *piVar1 = *piVar1 - *(int *)(in_ECX + 0x1f8);
      *(int *)(in_ECX + 0x650) = *(int *)(in_ECX + 0x650) - *(int *)(in_ECX + 0x1fc);
      *(int *)(in_ECX + 0x654) = *(int *)(in_ECX + 0x654) + *(int *)(in_ECX + 0x1f8);
      *(int *)(in_ECX + 0x658) = *(int *)(in_ECX + 0x658) + *(int *)(in_ECX + 0x1fc);
      FUN_00477820(iVar9,*(undefined4 *)(iVar9 + 100),piVar1,0);
      uVar11 = 0;
      local_18 = iVar7;
      local_14 = iVar6;
      local_10 = iVar2;
      local_c = iVar10;
      if (*(int *)(in_ECX + 0x194) != 0) {
        do {
          puVar8 = (undefined4 *)(in_ECX + 0x184);
          if (*(uint *)(in_ECX + 0x194) <= uVar11) {
LAB_0044bd4e:
                    /* WARNING: Subroutine does not return */
            FUN_00842981("invalid string position");
          }
          if (0xf < *(uint *)(in_ECX + 0x198)) {
            puVar8 = (undefined4 *)*puVar8;
          }
          iVar9 = (int)*(char *)((int)puVar8 + uVar11);
          if (((iVar9 - 1U < 0x7f) &&
              (puVar8 = *(undefined4 **)(in_ECX + 0x200 + iVar9 * 4), puVar8 != (undefined4 *)0x0))
             && (*(int *)(in_ECX + 0x400 + iVar9 * 4) != 0)) {
            puVar3 = *(undefined4 **)(*(int *)(in_ECX + 0x30) + 4);
            *puVar3 = *puVar8;
            puVar3[1] = puVar8[1];
            puVar3[2] = puVar8[2];
            puVar3[3] = puVar8[3];
            puVar3[4] = puVar8[4];
            local_10 = *(int *)(in_ECX + 0x400 + iVar9 * 4) + local_18;
            if (iVar2 < local_10) break;
            FUN_00477820(*(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),1,&local_18,0);
            local_18 = local_10;
          }
          uVar11 = uVar11 + 1;
        } while (uVar11 < *(uint *)(in_ECX + 0x194));
      }
      if (*(char *)(in_ECX + 0x180) != '\0') {
        (**(code **)(**(int **)(DAT_009b279c + 4) + 0x28))(*(int **)(DAT_009b279c + 4));
        FUN_004ac0c0(0,6,2);
        FUN_004ac0c0(0,5,2);
        FUN_004ac0c0(0,7,2);
      }
    }
  }
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

