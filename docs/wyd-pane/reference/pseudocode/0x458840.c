// rea decompile_all
// name: FUN_00458840
// address: 0x458840
// body: available
// range: 0x458840-0x459343 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00458840(undefined4 param_1)

{
  int *piVar1;
  code *pcVar2;
  POINT pt;
  HWND hWnd;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  BOOL BVar8;
  tagRECT *ptVar9;
  int *in_ECX;
  char cVar10;
  float fVar11;
  bool bVar12;
  undefined2 in_FPUControlWord;
  float10 fVar13;
  float10 fVar14;
  float10 extraout_ST1;
  float10 fVar15;
  tagPOINT *lpPoint;
  int local_a8;
  int local_a4;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  int local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  tagPOINT local_64;
  float local_5c;
  float local_58;
  uint local_54;
  char local_4f;
  undefined2 local_4e;
  float local_4c;
  tagRECT local_48;
  tagRECT local_38;
  RECT local_28;
  tagRECT local_18;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if ((char)in_ECX[2] != '\0') {
    fVar13 = (float10)FUN_00442db0();
    local_4c = (float)fVar13;
    GetCursorPos(&local_64);
    lpPoint = &local_64;
    hWnd = (HWND)thunk_FUN_004900e0();
    ScreenToClient(hWnd,lpPoint);
    iVar3 = FUN_0080db50();
    local_64.x = iVar3;
    local_64.y = FUN_0080db50();
    local_64.x = iVar3 - *(int *)(in_ECX[9] + 0x14);
    local_64.y = local_64.y - *(int *)(in_ECX[9] + 0x18);
    if ((in_ECX[0x101] != in_ECX[0xdf]) || (in_ECX[0x102] != in_ECX[0xe0])) {
      FUN_00458760();
    }
    local_78 = *(int *)in_ECX[0xc];
    *(uint *)(local_78 + 0x18) = *(uint *)(local_78 + 0x18) & 0xfffffffc;
    FUN_00465740(0);
    FUN_00465740(0);
    iVar3 = *(int *)(in_ECX[0xc] + 4);
    *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xfffffffc;
    FUN_00465740(0);
    FUN_00465740(0);
    FUN_00477820(local_78,*(undefined4 *)(local_78 + 100),in_ECX + 0x2f,0);
    if (DAT_009b1444 == 1) {
      (**(code **)(*in_ECX + 0x70))();
      local_18.top = in_ECX[0x67];
      local_54 = (in_ECX[0x68] - in_ECX[0x66]) / in_ECX[0xf4];
      local_18.left = in_ECX[0x66];
      local_18.bottom = in_ECX[0xf5] + local_18.top;
      local_18.right = in_ECX[0x68];
      if ((int)local_54 < 1) {
        local_54 = 1;
      }
      iVar3 = in_ECX[0xdf] * local_54;
      if (iVar3 < in_ECX[0xf1] - in_ECX[0xf0] >> 2) {
        local_58 = (float)(iVar3 + 1);
        iVar4 = in_ECX[0xf5] + local_18.top;
        while( true ) {
          local_18.left = (iVar3 % (int)local_54) * in_ECX[0xf4] + in_ECX[0x66];
          local_18.right = in_ECX[0xf4] + local_18.left;
          if (in_ECX[0x69] < iVar4) break;
          FUN_0046f890(&local_18,0x88ffffff,1);
          if ((int)local_58 % (int)local_54 == 0) {
            OffsetRect(&local_18,0,in_ECX[0xeb]);
          }
          local_58 = (float)((int)local_58 + 1);
          iVar3 = iVar3 + 1;
          if (in_ECX[0xf1] - in_ECX[0xf0] >> 2 <= iVar3) {
            __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
            return;
          }
          iVar4 = local_18.bottom;
        }
      }
    }
    else {
      if (in_ECX[100] != 0) {
        (**(code **)(*DAT_009b27ac + 0xe4))(DAT_009b27ac,0xae,1);
        fVar13 = (float10)FUN_0046edc0();
        local_4c = (float)fVar13;
        local_18.left = in_ECX[0x2f];
        local_18.right = in_ECX[0x31];
        local_18.top = in_ECX[0x30];
        local_18.bottom = in_ECX[0x32];
        iVar4 = in_ECX[9];
        local_88 = *(float *)(iVar4 + 0x14);
        local_84 = *(float *)(iVar4 + 0x18);
        local_80 = *(float *)(iVar4 + 0x1c);
        local_7c = *(float *)(iVar4 + 0x20);
        iVar4 = FUN_0080db50();
        local_4c = (float)iVar4;
        local_4c = (float)FUN_0080db50();
        local_74 = (float)(int)local_4c;
        iVar4 = FUN_0080db50();
        local_4c = (float)iVar4;
        local_4c = (float)FUN_0080db50();
        local_70 = (float)(int)local_4c;
        local_4c = (float)FUN_0080db50();
        local_6c = (float)(int)local_4c;
        local_4c = (float)FUN_0080db50();
        local_68 = (float)(int)local_4c;
        local_18.left = FUN_0080db50();
        local_18.right = FUN_0080db50();
        local_18.top = FUN_0080db50();
        uVar5 = FUN_0080db50();
        local_18.bottom = uVar5;
        (**(code **)(*DAT_009b27ac + 300))(DAT_009b27ac,&local_18);
      }
      if (in_ECX[0xf1] - in_ECX[0xf0] >> 2 != 0) {
        iVar4 = (in_ECX[0x68] - in_ECX[0x66]) / in_ECX[0xf4];
        if (iVar4 < 1) {
          iVar4 = 1;
        }
        local_58 = (float)(in_ECX[0xdf] * iVar4);
        if ((in_ECX[100] != 0) || ((int)local_58 < 0)) {
          local_58 = 0.0;
        }
        fVar11 = local_58;
        if ((in_ECX[10] != 0) && (*(char *)(in_ECX[10] + 0x48) != '\0')) {
          FUN_00442b70();
          local_4c = fVar11;
          if ((int)fVar11 < in_ECX[0xf1] - in_ECX[0xf0] >> 2) {
            fVar13 = (float10)1;
            fVar15 = (float10)0;
            do {
              local_4c = fVar11;
              fVar14 = (float10)FUN_00458780(fVar11,&local_28,&local_4f,0);
              if ((in_ECX[100] != 0) || (local_4f != '\0')) {
                iVar4 = *(int *)(in_ECX[0xf0] + (int)fVar11 * 4);
                if (iVar4 != 0) {
                  if (*(char *)((int)in_ECX + 0x18d) == '\0') {
                    if (fVar13 < (float10)*(float *)(iVar3 + 0x54) ==
                        (fVar13 == (float10)*(float *)(iVar3 + 0x54))) {
                      if (fVar15 < (float10)*(float *)(iVar3 + 0x54)) {
                        local_18._8_8_ =
                             (BADTYPE)ROUND((float10)*(float *)(iVar3 + 0x54) * extraout_ST1 +
                                            fVar14);
                        local_54 = local_18.right;
                        local_4e = in_FPUControlWord;
                      }
                      else {
                        local_54 = 0;
                      }
                    }
                    else {
                      local_54 = 0xff;
                    }
                    if (fVar13 < (float10)*(float *)(iVar3 + 0x58) ==
                        (fVar13 == (float10)*(float *)(iVar3 + 0x58))) {
                      local_5c = 0.0;
                      if (fVar15 < (float10)*(float *)(iVar3 + 0x58)) {
                        local_18._8_8_ =
                             (BADTYPE)ROUND((float10)*(float *)(iVar3 + 0x58) * extraout_ST1 +
                                            fVar14);
                        local_5c = (float)local_18.right;
                        local_4e = in_FPUControlWord;
                      }
                    }
                    else {
                      local_5c = 3.57331e-43;
                    }
                    if (fVar13 < (float10)*(float *)(iVar3 + 0x5c) ==
                        (fVar13 == (float10)*(float *)(iVar3 + 0x5c))) {
                      if (fVar15 < (float10)*(float *)(iVar3 + 0x5c)) {
                        local_18._8_8_ =
                             (BADTYPE)ROUND((float10)*(float *)(iVar3 + 0x5c) * extraout_ST1 +
                                            fVar14);
                        uVar7 = local_18.right;
                        local_4e = in_FPUControlWord;
                      }
                      else {
                        uVar7 = 0;
                      }
                    }
                    else {
                      uVar7 = 0xff;
                    }
                    if (fVar13 < (float10)*(float *)(iVar3 + 0x60) ==
                        (fVar13 == (float10)*(float *)(iVar3 + 0x60))) {
                      if (fVar15 < (float10)*(float *)(iVar3 + 0x60)) {
                        local_18._8_8_ =
                             (BADTYPE)ROUND(extraout_ST1 * (float10)*(float *)(iVar3 + 0x60) +
                                            fVar14);
                        iVar6 = local_18.right;
                        local_4e = in_FPUControlWord;
                      }
                      else {
                        iVar6 = 0;
                      }
                    }
                    else {
                      iVar6 = 0xff;
                    }
                    FUN_00476b80(*(undefined4 *)(in_ECX[10] + 0x44),iVar4 + 10,
                                 *(undefined4 *)(iVar3 + 0x14),&local_28,0xffffffff,
                                 ((iVar6 << 8 | local_54) << 8 | (uint)local_5c) << 8 | uVar7,
                                 *(undefined4 *)(local_78 + 0x18),*(undefined4 *)(iVar4 + 0x288),
                                 *(undefined4 *)(iVar4 + 0x28c),0);
                  }
                  else {
                    FUN_00455b60(in_ECX[10],iVar4,&local_28,iVar3,0);
                    local_4c = fVar11;
                  }
                  fVar15 = (float10)0;
                  fVar13 = (float10)1;
                  fVar11 = local_4c;
                }
              }
              fVar11 = (float)((int)fVar11 + 1);
              local_4c = fVar11;
            } while ((int)fVar11 < in_ECX[0xf1] - in_ECX[0xf0] >> 2);
          }
          *(undefined1 *)(in_ECX[10] + 0x48) = 0;
        }
        if (*(char *)((int)in_ECX + 0x3bd) == '\0') {
          if (in_ECX[0xeb] == 0) {
            in_ECX[0xe0] = in_ECX[0x69] - in_ECX[0x67];
          }
          else {
            in_ECX[0xe0] = (in_ECX[0x69] - in_ECX[0x67]) / in_ECX[0xeb];
          }
          FUN_00488cb0();
          FUN_00488da0();
          *(undefined1 *)((int)in_ECX + 0x3bd) = 1;
        }
        local_54 = 0;
        do {
          if ((local_54 == 0) || (local_54 == 2)) {
            if (*(char *)((int)in_ECX + 0x195) != '\0') {
              if (local_54 == 0) {
                bVar12 = *(char *)((int)in_ECX + 0x196) == '\x01';
LAB_00459024:
                if (bVar12) goto LAB_004592a9;
              }
              else if (local_54 == 2) {
                bVar12 = *(char *)((int)in_ECX + 0x196) == '\0';
                goto LAB_00459024;
              }
              fVar11 = local_58;
              if ((int)local_58 < in_ECX[0xf1] - in_ECX[0xf0] >> 2) {
                do {
                  fVar13 = (float10)FUN_00458780(fVar11,&local_28,(int)&local_4e + 1,1);
                  if (((in_ECX[100] != 0) || (local_4e._1_1_ != '\0')) &&
                     (local_4c = *(float *)(in_ECX[0xf0] + (int)fVar11 * 4), local_4c != 0.0)) {
                    cVar10 = '\0';
                    if (*(char *)((int)in_ECX + 0x3be) == '\0') {
                      if (((in_ECX[0xec] & 1U) == 0) && (fVar11 == (float)in_ECX[0xed])) {
LAB_004590e3:
                        cVar10 = '\x01';
                      }
                      else {
                        if ((in_ECX[0xec] & 1U) == 0) goto LAB_004590e5;
                        if ((char)in_ECX[0xef] == '\0') {
LAB_004590dd:
                          if (*(char *)((int)local_4c + 8) != '\0') goto LAB_004590e3;
                        }
                        else {
                          if ((int)fVar11 < in_ECX[0xed]) {
LAB_004590bf:
                            iVar4 = in_ECX[0xee];
                            if ((int)fVar11 <= iVar4) goto LAB_004590dd;
                          }
                          else {
                            iVar4 = in_ECX[0xee];
                            if (iVar4 <= (int)fVar11) {
                              if (in_ECX[0xed] < (int)fVar11) goto LAB_004590dd;
                              goto LAB_004590bf;
                            }
                          }
                          cVar10 = *(char *)(*(int *)(in_ECX[0xf0] + iVar4 * 4) + 8);
                        }
                      }
                    }
LAB_004590e5:
                    uVar7 = *(uint *)((int)local_4c + 0x280);
                    local_74 = (float)((float10)(uVar7 >> 0x10 & 0xff) * fVar13);
                    local_4c = (float)(uVar7 >> 0x18);
                    local_70 = (float)((float10)(uVar7 >> 8 & 0xff) * fVar13);
                    *(float *)(iVar3 + 0x54) = local_74;
                    *(float *)(iVar3 + 0x58) = local_70;
                    local_6c = (float)((float10)(uVar7 & 0xff) * fVar13);
                    *(float *)(iVar3 + 0x5c) = local_6c;
                    local_68 = (float)((float10)(int)local_4c * fVar13);
                    *(float *)(iVar3 + 0x60) = local_68;
                    if (cVar10 == '\0') {
                      pt.y = local_64.y;
                      pt.x = local_64.x;
                      BVar8 = PtInRect(&local_28,pt);
                      uVar7 = *(uint *)(iVar3 + 0x24 + (-(uint)(BVar8 != 0) & 2) * 4);
                      local_88 = (float)(uVar7 >> 0x10 & 0xff) * 0.003921569;
                      local_4c = (float)(uVar7 >> 0x18);
                      local_84 = (float)(uVar7 >> 8 & 0xff) * 0.003921569;
                      *(float *)(iVar3 + 0x34) = local_88;
                      local_80 = (float)(uVar7 & 0xff) * 0.003921569;
                      *(float *)(iVar3 + 0x38) = local_84;
                      *(float *)(iVar3 + 0x3c) = local_80;
                      local_7c = (float)(int)local_4c * 0.003921569;
                      *(float *)(iVar3 + 0x40) = local_7c;
                      if (*(char *)((int)in_ECX + 0x195) == '\0') goto LAB_00459291;
                      CopyRect(&local_38,&local_28);
                      ptVar9 = &local_38;
                    }
                    else {
                      uVar7 = *(uint *)(iVar3 + 0x28);
                      local_98 = (float)((float10)(uVar7 >> 0x10 & 0xff) * fVar13);
                      local_4c = (float)(uVar7 >> 0x18);
                      local_94 = (float)((float10)(uVar7 >> 8 & 0xff) * fVar13);
                      *(float *)(iVar3 + 0x34) = local_98;
                      *(float *)(iVar3 + 0x38) = local_94;
                      local_90 = (float)((float10)(uVar7 & 0xff) * fVar13);
                      *(float *)(iVar3 + 0x3c) = local_90;
                      local_8c = (float)((float10)(int)local_4c * fVar13);
                      *(float *)(iVar3 + 0x40) = local_8c;
                      CopyRect(&local_48,&local_28);
                      ptVar9 = &local_48;
                    }
                    FUN_00477820(iVar3,*(undefined4 *)(iVar3 + 100),ptVar9,0);
                  }
LAB_00459291:
                  fVar11 = (float)((int)fVar11 + 1);
                } while ((int)fVar11 < in_ECX[0xf1] - in_ECX[0xf0] >> 2);
              }
            }
          }
          else if (local_54 == 1) {
            if (in_ECX[10] != 0) {
              FUN_004762c0(in_ECX[10],local_78,in_ECX[0x66],in_ECX[0x67],in_ECX[0x10],in_ECX[0x11],0
                          );
            }
            local_5c = local_58;
            if ((int)local_58 < in_ECX[0xf1] - in_ECX[0xf0] >> 2) {
              do {
                FUN_00458780(local_5c,&local_a8,&local_4f,1);
                if ((in_ECX[100] != 0) || (local_4f != '\0')) {
                  iVar4 = *(int *)(in_ECX[0xf0] + (int)local_5c * 4);
                  local_18.bottom = iVar4;
                  if (iVar4 != 0) {
                    uVar7 = *(int *)(iVar4 + 0x294) - *(int *)(iVar4 + 0x290) >> 2;
                    if (100 < uVar7) goto LAB_00459333;
                    if ((uVar7 - 1 < 99) && (local_4c = 0.0, uVar7 != 0)) {
                      do {
                        piVar1 = *(int **)(*(int *)(iVar4 + 0x290) + (int)local_4c * 4);
                        if ((char)piVar1[2] != '\0') {
                          piVar1[5] = *(short *)((int)piVar1 + 0x5a) + local_a8;
                          pcVar2 = *(code **)(*piVar1 + 0x70);
                          piVar1[6] = (short)piVar1[0x17] + local_a4;
                          (*pcVar2)();
                          (**(code **)(*piVar1 + 0xc))(param_1);
                          iVar4 = local_18.bottom;
                        }
                        local_4c = (float)((int)local_4c + 1);
                      } while ((uint)local_4c <
                               (uint)(*(int *)(iVar4 + 0x294) - *(int *)(iVar4 + 0x290) >> 2));
                    }
                  }
                }
                local_5c = (float)((int)local_5c + 1);
              } while ((int)local_5c < in_ECX[0xf1] - in_ECX[0xf0] >> 2);
            }
          }
LAB_004592a9:
          local_54 = local_54 + 1;
        } while ((int)local_54 < 3);
      }
      if (((char)in_ECX[0x65] != '\0') &&
         ((*(char *)((int)in_ECX + 0x18e) == '\0' ||
          ((uint)in_ECX[0xe0] < (uint)(in_ECX[0xf1] - in_ECX[0xf0] >> 2))))) {
        (**(code **)(in_ECX[0x6e] + 0xc))(param_1);
      }
      if (in_ECX[100] != 0) {
        (**(code **)(*DAT_009b27ac + 0xe4))(DAT_009b27ac,0xae,0);
        __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
        return;
      }
    }
  }
LAB_00459333:
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

