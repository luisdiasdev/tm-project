// rea decompile_all
// name: FUN_004564e0
// address: 0x4564e0
// body: available
// range: 0x4564e0-0x45660e inclusive
// range: 0x456610-0x456f7f inclusive
// non_contiguous: true
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_004564e0(undefined4 param_1)

{
  float fVar1;
  char cVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  int iVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  float fVar11;
  int iVar12;
  int in_ECX;
  uint uVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  undefined4 uVar17;
  undefined4 uVar18;
  int local_54;
  int local_50;
  uint local_4c;
  uint local_48;
  uint local_44;
  uint uStack_40;
  tagRECT local_38;
  tagRECT local_28;
  tagRECT local_18;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)(in_ECX + 8) != '\0') {
    iVar16 = **(int **)(in_ECX + 0x30);
    *(uint *)(iVar16 + 0x18) = *(uint *)(iVar16 + 0x18) & 0xfffffffc;
    iVar3 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
    *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xfffffffc;
    CopyRect(&local_38,(RECT *)(in_ECX + 0xbc));
    FUN_00477820(iVar16,*(undefined4 *)(iVar16 + 100),&local_38,0);
    uVar4 = *(undefined4 *)(iVar16 + 0x38);
    uVar5 = *(undefined4 *)(iVar16 + 0x34);
    uVar6 = *(undefined4 *)(iVar16 + 0x3c);
    uVar7 = *(undefined4 *)(iVar16 + 0x40);
    if (*(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2 != 0) {
      if ((*(int *)(in_ECX + 0x28) != 0) && (*(char *)(*(int *)(in_ECX + 0x28) + 0x48) != '\0')) {
        FUN_00442b70();
        local_18.right = *(int *)(in_ECX + 0x1a0) - *(int *)(in_ECX + 0x198);
        local_18.left = 0;
        local_18.top = 0;
        local_18.bottom = *(int *)(in_ECX + 0x1a4) - *(int *)(in_ECX + 0x19c);
        iVar12 = FUN_00483700(*(undefined4 *)(iVar16 + 0x14));
        local_18.bottom = *(int *)(iVar12 + 0xc) + local_18.top;
        local_18.right = *(LONG *)(in_ECX + 0x1a0);
        iVar12 = *(int *)(in_ECX + 0x1a4);
        iVar15 = *(int *)(in_ECX + 0x19c);
        *(LONG *)(in_ECX + 0x3ac) = local_18.bottom - local_18.top;
        local_54 = *(int *)(in_ECX + 0x37c);
        if (local_54 < *(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2) {
          do {
            if (iVar12 - iVar15 < local_18.bottom) break;
            iVar8 = *(int *)(*(int *)(in_ECX + 0x3c0) + local_54 * 4);
            if (*(char *)(in_ECX + 0x3be) == '\0') {
              uVar13 = *(uint *)(in_ECX + 0x3b0) & 1;
              if ((uVar13 != 0) || (local_54 != *(int *)(in_ECX + 0x3b4))) {
                if (uVar13 == 0) goto LAB_00456941;
                if (*(char *)(in_ECX + 0x3bc) == '\0') {
LAB_00456777:
                  cVar2 = *(char *)(iVar8 + 8);
                }
                else {
                  if (local_54 < *(int *)(in_ECX + 0x3b4)) {
LAB_0045667c:
                    iVar14 = *(int *)(in_ECX + 0x3b8);
                    if (local_54 <= iVar14) goto LAB_00456777;
                  }
                  else {
                    iVar14 = *(int *)(in_ECX + 0x3b8);
                    if (iVar14 <= local_54) {
                      if (*(int *)(in_ECX + 0x3b4) < local_54) goto LAB_00456777;
                      goto LAB_0045667c;
                    }
                  }
                  cVar2 = *(char *)(*(int *)(*(int *)(in_ECX + 0x3c0) + iVar14 * 4) + 8);
                }
                if (cVar2 == '\0') goto LAB_00456941;
              }
              FUN_00465740(1);
              if (*(int *)(iVar8 + 0x280) == 0) {
                fVar1 = *(float *)(iVar3 + 0x54);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x54)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x54) * 255.0 + 0.5);
                    local_4c = local_44;
                  }
                  else {
                    local_4c = 0;
                  }
                }
                else {
                  local_4c = 0xff;
                }
                fVar1 = *(float *)(iVar3 + 0x58);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x58)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x58) * 255.0 + 0.5);
                    local_48 = local_44;
                  }
                  else {
                    local_48 = 0;
                  }
                }
                else {
                  local_48 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x5c) == (*(float *)(iVar3 + 0x5c) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x5c)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x5c) * 255.0 + 0.5);
                    uVar13 = local_44;
                  }
                  else {
                    uVar13 = 0;
                  }
                }
                else {
                  uVar13 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x60) == (*(float *)(iVar3 + 0x60) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x60)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x60) * 255.0 + 0.5);
                  }
                  else {
                    local_44 = 0;
                  }
                }
                else {
                  local_44 = 0xff;
                }
                uVar18 = 0xff000000;
                local_48 = (local_44 << 8 | local_4c) << 8 | local_48;
                uVar17 = 1;
LAB_00456c60:
                FUN_00476b80(*(undefined4 *)(*(int *)(in_ECX + 0x28) + 0x44),iVar8 + 10,
                             *(undefined4 *)(iVar3 + 0x14),&local_18,0xffffffff,
                             local_48 << 8 | uVar13,*(undefined4 *)(iVar16 + 0x18),uVar17,uVar18,0);
              }
              else {
                if (*(char *)(in_ECX + 0x195) != '\0') {
                  FUN_00477820(iVar3,*(undefined4 *)(iVar3 + 100),&local_18,0);
                }
                uVar18 = *(undefined4 *)(iVar3 + 0x54);
                uVar17 = *(undefined4 *)(iVar3 + 0x58);
                uVar9 = *(undefined4 *)(iVar3 + 0x5c);
                uVar10 = *(undefined4 *)(iVar3 + 0x60);
                uVar13 = *(uint *)(iVar8 + 0x280);
                *(float *)(iVar3 + 0x54) = (float)(uVar13 >> 0x10 & 0xff) * 0.003921569;
                *(float *)(iVar3 + 0x58) = (float)(uVar13 >> 8 & 0xff) * 0.003921569;
                *(float *)(iVar3 + 0x5c) = (float)(uVar13 & 0xff) * 0.003921569;
                *(float *)(iVar3 + 0x60) = (float)(uVar13 >> 0x18) * 0.003921569;
                if (*(char *)(in_ECX + 0x18d) == '\0') {
                  FUN_00477cd0(iVar8 + 10,iVar3,&local_18,0xffffffff,*(undefined4 *)(iVar8 + 0x288),
                               *(undefined4 *)(iVar8 + 0x28c),0);
                }
                else {
                  FUN_004561c0(iVar8,&local_18,iVar3,1);
                }
                *(undefined4 *)(iVar3 + 0x54) = uVar18;
                *(undefined4 *)(iVar3 + 0x58) = uVar17;
                *(undefined4 *)(iVar3 + 0x5c) = uVar9;
                *(undefined4 *)(iVar3 + 0x60) = uVar10;
              }
            }
            else {
LAB_00456941:
              FUN_00465740(0);
              uVar13 = *(uint *)(iVar8 + 0x284);
              if (uVar13 != 0) {
                uStack_40 = uVar13 >> 0x10 & 0xff;
                fVar11 = (float)uStack_40;
                uStack_40 = uVar13 >> 8 & 0xff;
                fVar1 = (float)uStack_40;
                uStack_40 = uVar13 & 0xff;
                *(float *)(iVar3 + 0x34) = fVar11 * 0.003921569;
                *(float *)(iVar3 + 0x38) = fVar1 * 0.003921569;
                *(float *)(iVar3 + 0x3c) = (float)uStack_40 * 0.003921569;
                *(float *)(iVar3 + 0x40) = (float)(uVar13 >> 0x18) * 0.003921569;
                if (*(char *)(in_ECX + 0x195) != '\0') {
                  FUN_00477820(iVar3,*(undefined4 *)(iVar3 + 100),&local_18,0);
                }
              }
              uVar13 = *(uint *)(iVar8 + 0x280);
              if (uVar13 == 0) {
                if (*(char *)(in_ECX + 0x18d) != '\0') {
                  FUN_004561c0(iVar8,&local_18,iVar3,0);
                }
                fVar1 = *(float *)(iVar3 + 0x54);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x54)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x54) * 255.0 + 0.5);
                    local_48 = local_44;
                  }
                  else {
                    local_48 = 0;
                  }
                }
                else {
                  local_48 = 0xff;
                }
                fVar1 = *(float *)(iVar3 + 0x58);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x58)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x58) * 255.0 + 0.5);
                    local_4c = local_44;
                  }
                  else {
                    local_4c = 0;
                  }
                }
                else {
                  local_4c = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x5c) == (*(float *)(iVar3 + 0x5c) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x5c)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x5c) * 255.0 + 0.5);
                    uVar13 = local_44;
                  }
                  else {
                    uVar13 = 0;
                  }
                }
                else {
                  uVar13 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x60) == (*(float *)(iVar3 + 0x60) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x60)) {
                    local_44 = (uint)(longlong)ROUND(*(float *)(iVar3 + 0x60) * 255.0 + 0.5);
                  }
                  else {
                    local_44 = 0;
                  }
                }
                else {
                  local_44 = 0xff;
                }
                uVar18 = *(undefined4 *)(iVar8 + 0x28c);
                uVar17 = *(undefined4 *)(iVar8 + 0x288);
                local_48 = (local_44 << 8 | local_48) << 8 | local_4c;
                goto LAB_00456c60;
              }
              local_38.left = *(LONG *)(iVar3 + 0x54);
              local_38.top = *(LONG *)(iVar3 + 0x58);
              local_38.right = *(LONG *)(iVar3 + 0x5c);
              local_38.bottom = *(LONG *)(iVar3 + 0x60);
              uStack_40 = uVar13 >> 0x10 & 0xff;
              fVar11 = (float)uStack_40;
              uStack_40 = uVar13 >> 8 & 0xff;
              fVar1 = (float)uStack_40;
              uStack_40 = uVar13 & 0xff;
              *(float *)(iVar3 + 0x54) = fVar11 * 0.003921569;
              *(float *)(iVar3 + 0x58) = fVar1 * 0.003921569;
              *(float *)(iVar3 + 0x5c) = (float)uStack_40 * 0.003921569;
              *(float *)(iVar3 + 0x60) = (float)(uVar13 >> 0x18) * 0.003921569;
              if (*(char *)(in_ECX + 0x18d) == '\0') {
                FUN_00477cd0(iVar8 + 10,iVar3,&local_18,0xffffffff,*(undefined4 *)(iVar8 + 0x288),
                             *(undefined4 *)(iVar8 + 0x28c),0);
              }
              else {
                FUN_004561c0(iVar8,&local_18,iVar3,0);
              }
              *(LONG *)(iVar3 + 0x54) = local_38.left;
              *(LONG *)(iVar3 + 0x58) = local_38.top;
              *(LONG *)(iVar3 + 0x5c) = local_38.right;
              *(LONG *)(iVar3 + 0x60) = local_38.bottom;
            }
            OffsetRect(&local_18,0,*(int *)(in_ECX + 0x3ac));
            local_54 = local_54 + 1;
          } while (local_54 < *(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2);
        }
        *(undefined1 *)(*(int *)(in_ECX + 0x28) + 0x48) = 0;
      }
      local_28.top = *(int *)(in_ECX + 0x19c);
      local_28.left = *(LONG *)(in_ECX + 0x198);
      local_28.right = *(LONG *)(in_ECX + 0x1a0);
      local_28.bottom = *(LONG *)(in_ECX + 0x1a4);
      iVar12 = FUN_00483700(*(undefined4 *)(iVar16 + 0x14));
      local_28.bottom = *(int *)(iVar12 + 0xc) + local_28.top;
      iVar12 = local_28.bottom - local_28.top;
      *(int *)(in_ECX + 0x3ac) = iVar12;
      if (*(char *)(in_ECX + 0x3bd) == '\0') {
        if (iVar12 == 0) {
          *(int *)(in_ECX + 0x380) = *(int *)(in_ECX + 0x1a4) - *(int *)(in_ECX + 0x19c);
        }
        else {
          *(int *)(in_ECX + 0x380) = (*(int *)(in_ECX + 0x1a4) - *(int *)(in_ECX + 0x19c)) / iVar12;
        }
        FUN_00488cb0();
        FUN_00488da0();
        *(undefined1 *)(in_ECX + 0x3bd) = 1;
      }
      local_28.right = *(LONG *)(in_ECX + 0x1a0);
      local_50 = *(int *)(in_ECX + 0x37c);
      if (local_50 < *(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2) {
        while (local_28.bottom <= *(int *)(in_ECX + 0x1a4)) {
          iVar12 = *(int *)(*(int *)(in_ECX + 0x3c0) + local_50 * 4);
          if (*(char *)(in_ECX + 0x3be) == '\0') {
            uVar13 = *(uint *)(in_ECX + 0x3b0) & 1;
            if ((uVar13 != 0) || (local_50 != *(int *)(in_ECX + 0x3b4))) {
              if (uVar13 == 0) goto LAB_00456e08;
              if (*(char *)(in_ECX + 0x3bc) == '\0') {
LAB_00456e3d:
                cVar2 = *(char *)(iVar12 + 8);
              }
              else {
                if (local_50 < *(int *)(in_ECX + 0x3b4)) {
LAB_00456dee:
                  iVar15 = *(int *)(in_ECX + 0x3b8);
                  if (local_50 <= iVar15) goto LAB_00456e3d;
                }
                else {
                  iVar15 = *(int *)(in_ECX + 0x3b8);
                  if (iVar15 <= local_50) {
                    if (*(int *)(in_ECX + 0x3b4) < local_50) goto LAB_00456e3d;
                    goto LAB_00456dee;
                  }
                }
                cVar2 = *(char *)(*(int *)(*(int *)(in_ECX + 0x3c0) + iVar15 * 4) + 8);
              }
              if (cVar2 == '\0') goto LAB_00456e08;
            }
            FUN_00465740(1);
            if (*(int *)(iVar12 + 0x280) == 0) {
              if (*(char *)(in_ECX + 0x195) != '\0') {
                FUN_00477820(iVar3,*(undefined4 *)(iVar3 + 100),&local_28,0);
              }
            }
            else {
              if (*(char *)(in_ECX + 0x195) != '\0') {
                FUN_00477820(iVar3,*(undefined4 *)(iVar3 + 100),&local_28,0);
              }
              *(undefined4 *)(iVar3 + 0x54) = uVar5;
              *(undefined4 *)(iVar3 + 0x58) = uVar4;
              *(undefined4 *)(iVar3 + 0x5c) = uVar6;
              *(undefined4 *)(iVar3 + 0x60) = uVar7;
            }
            break;
          }
LAB_00456e08:
          OffsetRect(&local_28,0,*(int *)(in_ECX + 0x3ac));
          local_50 = local_50 + 1;
          if (*(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2 <= local_50) break;
        }
      }
      if (*(int *)(in_ECX + 0x28) != 0) {
        FUN_004762c0(*(int *)(in_ECX + 0x28),iVar16,*(undefined4 *)(in_ECX + 0x198),
                     *(undefined4 *)(in_ECX + 0x19c),*(undefined4 *)(in_ECX + 0x40),
                     *(undefined4 *)(in_ECX + 0x44),0);
      }
    }
    *(undefined4 *)(iVar16 + 0x34) = uVar5;
    *(undefined4 *)(iVar16 + 0x38) = uVar4;
    *(undefined4 *)(iVar16 + 0x3c) = uVar6;
    *(undefined4 *)(iVar16 + 0x40) = uVar7;
    if (*(char *)(in_ECX + 0x194) != '\0') {
      if (*(char *)(in_ECX + 0x18e) == '\0') {
        (**(code **)(*(int *)(in_ECX + 0x1b8) + 0x44))
                  (*(uint *)(in_ECX + 0x380) <
                   (uint)(*(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2));
        iVar16 = *(int *)(in_ECX + 0x1b8);
      }
      else {
        if ((uint)(*(int *)(in_ECX + 0x3c4) - *(int *)(in_ECX + 0x3c0) >> 2) <=
            *(uint *)(in_ECX + 0x380)) goto LAB_00456f6f;
        iVar16 = *(int *)(in_ECX + 0x1b8);
      }
      (**(code **)(iVar16 + 0xc))(param_1);
    }
  }
LAB_00456f6f:
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

