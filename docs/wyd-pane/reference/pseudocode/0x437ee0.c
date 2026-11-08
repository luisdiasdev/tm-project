// rea decompile_all
// name: FUN_00437ee0
// address: 0x437ee0
// body: available
// range: 0x437ee0-0x438c26 inclusive
// range: 0x438c30-0x438df4 inclusive
// non_contiguous: true
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00437ee0(undefined4 param_1)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  int in_ECX;
  tagRECT *ptVar8;
  uint uVar9;
  undefined4 local_8c;
  int local_88;
  int local_84;
  int local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  int local_6c;
  uint local_68;
  int local_64;
  uint local_60;
  tagRECT local_58;
  tagRECT local_48;
  RECT local_38;
  RECT local_28;
  tagRECT local_18;
  uint local_8;
  
  local_8 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  if (*(char *)(in_ECX + 8) != '\0') {
    iVar2 = **(int **)(in_ECX + 0x30);
    FUN_00465840(0,param_1,0);
    FUN_00465840(0,param_1,0);
    CopyRect(&local_48,(RECT *)(in_ECX + 0xbc));
    FUN_00477820(iVar2,2,&local_48,0);
    if (*(char *)(in_ECX + 0x4a4) == '\0') {
      local_7c = *(int *)(*(int *)(in_ECX + 0x30) + 8);
      iVar3 = FUN_00483700(*(undefined4 *)(local_7c + 0x14));
      if (*(int *)(iVar3 + 0xc) == 0) {
        *(int *)(in_ECX + 0x478) = *(int *)(in_ECX + 0x4e4) - *(int *)(in_ECX + 0x4dc);
      }
      else {
        iVar3 = *(int *)(in_ECX + 0x4e4);
        iVar6 = *(int *)(in_ECX + 0x4dc);
        iVar4 = FUN_00483700(*(undefined4 *)(local_7c + 0x14));
        *(int *)(in_ECX + 0x478) =
             (iVar3 - iVar6) / (*(int *)(iVar4 + 0xc) + DAT_00954628 * 2 + _DAT_009b21ac);
      }
      FUN_00488cb0();
      FUN_00488da0();
      *(undefined1 *)(in_ECX + 0x4a4) = 1;
    }
    local_88 = FUN_0046f1b0(*(undefined4 *)(iVar2 + 0x14));
    iVar3 = *(int *)(local_88 + 0xc) + DAT_00954628 * 2 + _DAT_009b21ac;
    local_6c = iVar3;
    FUN_00444290(0,*(undefined4 *)(in_ECX + 0x1c),*(undefined4 *)(in_ECX + 0x2a8));
    FUN_00444290(1,*(undefined4 *)(in_ECX + 0x1c),iVar3);
    local_7c = *(int *)(in_ECX + 0x474);
    local_78 = *(int *)(in_ECX + 0x478) + local_7c;
    local_8c = 0;
    local_80 = 0;
    iVar3 = *(int *)(in_ECX + 0x4f0) - *(int *)(in_ECX + 0x4ec) >> 2;
    local_38.bottom = iVar3;
    FUN_00488c90(&local_8c,&local_80);
    if ((*(int *)(in_ECX + 0x28) != 0) && (*(char *)(*(int *)(in_ECX + 0x28) + 0x48) != '\0')) {
      iVar3 = *(int *)(*(int *)(in_ECX + 0x30) + 0xc);
      FUN_00442b70();
      *(undefined4 *)(iVar3 + 0x34) = *(undefined4 *)(iVar2 + 0x34);
      *(undefined4 *)(iVar3 + 0x38) = *(undefined4 *)(iVar2 + 0x38);
      *(undefined4 *)(iVar3 + 0x3c) = *(undefined4 *)(iVar2 + 0x3c);
      uVar5 = *(uint *)(iVar3 + 0x44);
      *(undefined4 *)(iVar3 + 0x40) = *(undefined4 *)(iVar2 + 0x40);
      local_28.left = (LONG)((float)(uVar5 >> 0x10 & 0xff) * 0.003921569);
      local_28.top = (LONG)((float)(uVar5 >> 8 & 0xff) * 0.003921569);
      *(LONG *)(iVar3 + 0x54) = local_28.left;
      local_28.right = (LONG)((float)(uVar5 & 0xff) * 0.003921569);
      *(LONG *)(iVar3 + 0x58) = local_28.top;
      *(LONG *)(iVar3 + 0x5c) = local_28.right;
      local_64 = 0;
      local_28.bottom = (LONG)((float)(uVar5 >> 0x18) * 0.003921569);
      *(LONG *)(iVar3 + 0x60) = local_28.bottom;
      local_48.top = *(LONG *)(iVar2 + 0x58);
      local_84 = (*(int *)(in_ECX + 0x4e4) - *(int *)(in_ECX + 0x4dc)) + local_6c;
      local_48.left = *(LONG *)(iVar2 + 0x54);
      local_48.right = *(LONG *)(iVar2 + 0x5c);
      local_48.bottom = *(LONG *)(iVar2 + 0x60);
      uVar5 = 0;
      if (*(int *)(in_ECX + 0x4f0) - *(int *)(in_ECX + 0x4ec) >> 2 != 0) {
        do {
          iVar6 = *(int *)(*(int *)(in_ECX + 0x4ec) + uVar5 * 4);
          if (iVar6 != 0) {
            *(undefined1 *)(iVar6 + 0x214) = 0;
          }
          uVar5 = uVar5 + 1;
        } while (uVar5 < (uint)(*(int *)(in_ECX + 0x4f0) - *(int *)(in_ECX + 0x4ec) >> 2));
      }
      local_74 = local_7c;
      if (local_7c < local_78) {
        do {
          iVar6 = local_64;
          local_70 = *(int *)(*(int *)(in_ECX + 0x4ec) + local_74 * 4);
          if (local_70 != 0) {
            local_84 = local_84 + ((DAT_00954628 * -2 - *(int *)(local_88 + 0xc)) - _DAT_009b21ac);
            if (local_84 < 0) {
              *(undefined1 *)(local_70 + 0x214) = 0;
            }
            else {
              local_64 = local_6c + local_64;
              SetRect((LPRECT)(local_70 + 0x204),0,iVar6,
                      *(int *)(in_ECX + 0x4e0) - *(int *)(in_ECX + 0x4d8),local_64);
              *(undefined1 *)(local_70 + 0x214) = 1;
              local_28.left = *(LONG *)(iVar3 + 0x54);
              local_28.right = *(LONG *)(iVar3 + 0x5c);
              local_28.top = *(LONG *)(iVar3 + 0x58);
              local_28.bottom = *(LONG *)(iVar3 + 0x60);
              if (local_74 == *(int *)(in_ECX + 0x2a0)) {
                FUN_00465740(0);
                *(undefined4 *)(iVar3 + 0x54) = *(undefined4 *)(local_70 + 0x218);
                *(undefined4 *)(iVar3 + 0x58) = *(undefined4 *)(local_70 + 0x21c);
                *(undefined4 *)(iVar3 + 0x5c) = *(undefined4 *)(local_70 + 0x220);
                *(undefined4 *)(iVar3 + 0x60) = *(undefined4 *)(local_70 + 0x224);
                fVar1 = *(float *)(iVar3 + 0x54);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x54)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x54) * 255.0 + 0.5);
                    local_68 = local_18.right;
                  }
                  else {
                    local_68 = 0;
                  }
                }
                else {
                  local_68 = 0xff;
                }
                fVar1 = *(float *)(iVar3 + 0x58);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x58)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x58) * 255.0 + 0.5);
                    local_60 = local_18.right;
                  }
                  else {
                    local_60 = 0;
                  }
                }
                else {
                  local_60 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x5c) == (*(float *)(iVar3 + 0x5c) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x5c)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x5c) * 255.0 + 0.5);
                    uVar5 = local_18.right;
                  }
                  else {
                    uVar5 = 0;
                  }
                }
                else {
                  uVar5 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x60) == (*(float *)(iVar3 + 0x60) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x60)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x60) * 255.0 + 0.5);
                    uVar9 = (local_18.right << 8 | local_68) << 8 | local_60;
                  }
                  else {
                    uVar9 = local_68 << 8 | local_60;
                  }
                }
                else {
                  uVar9 = (local_68 | 0xff00) << 8 | local_60;
                }
              }
              else {
                if (local_74 != *(int *)(in_ECX + 0x2a4)) {
                  *(undefined4 *)(iVar2 + 0x54) = *(undefined4 *)(local_70 + 0x218);
                  *(undefined4 *)(iVar2 + 0x58) = *(undefined4 *)(local_70 + 0x21c);
                  *(undefined4 *)(iVar2 + 0x5c) = *(undefined4 *)(local_70 + 0x220);
                  *(undefined4 *)(iVar2 + 0x60) = *(undefined4 *)(local_70 + 0x224);
                  fVar1 = *(float *)(iVar2 + 0x54);
                  if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                    if (0.0 < *(float *)(iVar2 + 0x54)) {
                      local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar2 + 0x54) * 255.0 + 0.5);
                      local_60 = local_18.right;
                    }
                    else {
                      local_60 = 0;
                    }
                  }
                  else {
                    local_60 = 0xff;
                  }
                  fVar1 = *(float *)(iVar2 + 0x58);
                  if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                    if (0.0 < *(float *)(iVar2 + 0x58)) {
                      local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar2 + 0x58) * 255.0 + 0.5);
                      local_68 = local_18.right;
                    }
                    else {
                      local_68 = 0;
                    }
                  }
                  else {
                    local_68 = 0xff;
                  }
                  if (1.0 < *(float *)(iVar2 + 0x5c) == (*(float *)(iVar2 + 0x5c) == 1.0)) {
                    if (0.0 < *(float *)(iVar2 + 0x5c)) {
                      local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar2 + 0x5c) * 255.0 + 0.5);
                      uVar5 = local_18.right;
                    }
                    else {
                      uVar5 = 0;
                    }
                  }
                  else {
                    uVar5 = 0xff;
                  }
                  if (1.0 < *(float *)(iVar2 + 0x60) == (*(float *)(iVar2 + 0x60) == 1.0)) {
                    if (0.0 < *(float *)(iVar2 + 0x60)) {
                      local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar2 + 0x60) * 255.0 + 0.5);
                      iVar6 = local_18.right;
                    }
                    else {
                      iVar6 = 0;
                    }
                  }
                  else {
                    iVar6 = 0xff;
                  }
                  FUN_00476b80(*(undefined4 *)(*(int *)(in_ECX + 0x28) + 0x44),local_70,
                               *(undefined4 *)(iVar2 + 0x14),local_70 + 0x204,0xffffffff,
                               ((iVar6 << 8 | local_60) << 8 | local_68) << 8 | uVar5,
                               *(undefined4 *)(iVar2 + 0x18),0,0xff000000,0);
                  *(LONG *)(iVar2 + 0x54) = local_48.left;
                  *(LONG *)(iVar2 + 0x58) = local_48.top;
                  *(LONG *)(iVar2 + 0x5c) = local_48.right;
                  *(LONG *)(iVar2 + 0x60) = local_48.bottom;
                  goto LAB_004387d8;
                }
                FUN_00465740(2);
                *(undefined4 *)(iVar3 + 0x54) = *(undefined4 *)(local_70 + 0x218);
                *(undefined4 *)(iVar3 + 0x58) = *(undefined4 *)(local_70 + 0x21c);
                *(undefined4 *)(iVar3 + 0x5c) = *(undefined4 *)(local_70 + 0x220);
                *(undefined4 *)(iVar3 + 0x60) = *(undefined4 *)(local_70 + 0x224);
                fVar1 = *(float *)(iVar3 + 0x54);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x54)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x54) * 255.0 + 0.5);
                    local_60 = local_18.right;
                  }
                  else {
                    local_60 = 0;
                  }
                }
                else {
                  local_60 = 0xff;
                }
                fVar1 = *(float *)(iVar3 + 0x58);
                if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x58)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x58) * 255.0 + 0.5);
                    local_68 = local_18.right;
                  }
                  else {
                    local_68 = 0;
                  }
                }
                else {
                  local_68 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x5c) == (*(float *)(iVar3 + 0x5c) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x5c)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x5c) * 255.0 + 0.5);
                    uVar5 = local_18.right;
                  }
                  else {
                    uVar5 = 0;
                  }
                }
                else {
                  uVar5 = 0xff;
                }
                if (1.0 < *(float *)(iVar3 + 0x60) == (*(float *)(iVar3 + 0x60) == 1.0)) {
                  if (0.0 < *(float *)(iVar3 + 0x60)) {
                    local_18._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x60) * 255.0 + 0.5);
                    iVar6 = local_18.right;
                  }
                  else {
                    iVar6 = 0;
                  }
                }
                else {
                  iVar6 = 0xff;
                }
                uVar9 = (iVar6 << 8 | local_60) << 8 | local_68;
              }
              FUN_00476b80(*(undefined4 *)(*(int *)(in_ECX + 0x28) + 0x44),local_70,
                           *(undefined4 *)(iVar3 + 0x14),local_70 + 0x204,0xffffffff,
                           uVar9 << 8 | uVar5,*(undefined4 *)(iVar3 + 0x18),0,0xff000000,0);
              *(LONG *)(iVar3 + 0x54) = local_28.left;
              *(LONG *)(iVar3 + 0x58) = local_28.top;
              *(LONG *)(iVar3 + 0x5c) = local_28.right;
              *(LONG *)(iVar3 + 0x60) = local_28.bottom;
            }
          }
LAB_004387d8:
          local_74 = local_74 + 1;
        } while (local_74 < local_78);
      }
      *(undefined1 *)(*(int *)(in_ECX + 0x28) + 0x48) = 0;
      iVar3 = local_38.bottom;
    }
    if ((*(int *)(in_ECX + 0x2c) != 0) && (*(char *)(*(int *)(in_ECX + 0x2c) + 0x48) != '\0')) {
      FUN_00442b70();
      iVar6 = *(int *)(in_ECX + 0x2a0);
      if ((-1 < iVar6) &&
         ((iVar6 < iVar3 &&
          (local_64 = *(int *)(*(int *)(in_ECX + 0x4ec) + iVar6 * 4), local_64 != 0)))) {
        iVar3 = *(int *)(*(int *)(in_ECX + 0x30) + 0xc);
        *(undefined4 *)(iVar3 + 0x34) = *(undefined4 *)(iVar2 + 0x34);
        *(undefined4 *)(iVar3 + 0x38) = *(undefined4 *)(iVar2 + 0x38);
        *(undefined4 *)(iVar3 + 0x3c) = *(undefined4 *)(iVar2 + 0x3c);
        uVar5 = *(uint *)(iVar3 + 0x44);
        *(undefined4 *)(iVar3 + 0x40) = *(undefined4 *)(iVar2 + 0x40);
        local_38._8_8_ = CONCAT44(uVar5 >> 0x10,local_38.right) & 0xffffffffff;
        local_28.left = (LONG)((float)local_38.bottom * 0.003921569);
        local_38._8_8_ = CONCAT44(uVar5 >> 8,local_38.right) & 0xffffffffff;
        local_28.top = (LONG)(float)local_38.bottom;
        local_38._8_8_ = CONCAT44(uVar5,local_38.right) & 0xffffffffff;
        local_28.top = (LONG)((float)local_28.top * 0.003921569);
        local_28.right = (LONG)(float)local_38.bottom;
        local_38.bottom = uVar5 >> 0x18;
        *(LONG *)(iVar3 + 0x54) = local_28.left;
        local_28.right = (LONG)((float)local_28.right * 0.003921569);
        *(LONG *)(iVar3 + 0x58) = local_28.top;
        *(LONG *)(iVar3 + 0x5c) = local_28.right;
        local_28.bottom = (LONG)((float)(uVar5 >> 0x18) * 0.003921569);
        *(LONG *)(iVar3 + 0x60) = local_28.bottom;
        SetRect(&local_48,0,0,*(int *)(in_ECX + 0x4e0) - *(int *)(in_ECX + 0x4d8),local_6c);
        FUN_00465740(0);
        *(undefined4 *)(iVar3 + 0x54) = *(undefined4 *)(local_64 + 0x218);
        *(undefined4 *)(iVar3 + 0x58) = *(undefined4 *)(local_64 + 0x21c);
        *(undefined4 *)(iVar3 + 0x5c) = *(undefined4 *)(local_64 + 0x220);
        *(undefined4 *)(iVar3 + 0x60) = *(undefined4 *)(local_64 + 0x224);
        fVar1 = *(float *)(iVar3 + 0x54);
        if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
          if (0.0 < *(float *)(iVar3 + 0x54)) {
            local_38._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x54) * 255.0 + 0.5);
            local_60 = local_38.right;
          }
          else {
            local_60 = 0;
          }
        }
        else {
          local_60 = 0xff;
        }
        fVar1 = *(float *)(iVar3 + 0x58);
        if (NAN(fVar1) || 1.0 < fVar1 == (fVar1 == 1.0)) {
          if (0.0 < *(float *)(iVar3 + 0x58)) {
            local_38._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x58) * 255.0 + 0.5);
            local_68 = local_38.right;
          }
          else {
            local_68 = 0;
          }
        }
        else {
          local_68 = 0xff;
        }
        if (1.0 < *(float *)(iVar3 + 0x5c) == (*(float *)(iVar3 + 0x5c) == 1.0)) {
          if (0.0 < *(float *)(iVar3 + 0x5c)) {
            local_38._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x5c) * 255.0 + 0.5);
            uVar5 = local_38.right;
          }
          else {
            uVar5 = 0;
          }
        }
        else {
          uVar5 = 0xff;
        }
        if (1.0 < *(float *)(iVar3 + 0x60) == (*(float *)(iVar3 + 0x60) == 1.0)) {
          if (0.0 < *(float *)(iVar3 + 0x60)) {
            local_38._8_8_ = (BADTYPE)ROUND(*(float *)(iVar3 + 0x60) * 255.0 + 0.5);
            iVar6 = local_38.right;
          }
          else {
            iVar6 = 0;
          }
        }
        else {
          iVar6 = 0xff;
        }
        FUN_00476b80(*(undefined4 *)(*(int *)(in_ECX + 0x2c) + 0x44),local_64,
                     *(undefined4 *)(iVar3 + 0x14),&local_48,0xffffffff,
                     ((iVar6 << 8 | local_60) << 8 | local_68) << 8 | uVar5,
                     *(undefined4 *)(iVar3 + 0x18),0,0xff000000,0);
        *(LONG *)(iVar3 + 0x54) = local_28.left;
        *(LONG *)(iVar3 + 0x58) = local_28.top;
        *(LONG *)(iVar3 + 0x5c) = local_28.right;
        *(LONG *)(iVar3 + 0x60) = local_28.bottom;
      }
      *(undefined1 *)(*(int *)(in_ECX + 0x2c) + 0x48) = 0;
    }
    iVar3 = *(int *)(in_ECX + 0x2a0);
    if ((((-1 < iVar3) && (iVar3 < *(int *)(in_ECX + 0x4f0) - *(int *)(in_ECX + 0x4ec) >> 2)) &&
        (*(int *)(*(int *)(in_ECX + 0x4ec) + iVar3 * 4) != 0)) && (*(int *)(in_ECX + 0x2c) != 0)) {
      uVar9 = (((*(int *)(in_ECX + 0x4b0) - *(int *)(in_ECX + 0x4e0)) - *(int *)(in_ECX + 0x4a8)) +
              *(int *)(in_ECX + 0x4d8)) / 2;
      uVar5 = ((*(int *)(in_ECX + 0x4b4) - *(int *)(in_ECX + 0x4ac)) - local_6c) / 2;
      FUN_004762c0(*(undefined4 *)(in_ECX + 0x2c),iVar2,
                   *(int *)(in_ECX + 0x4a8) + (uVar9 & ((int)uVar9 < 0) - 1),
                   *(int *)(in_ECX + 0x4ac) + (uVar5 & ((int)uVar5 < 0) - 1),
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
    }
    if ((*(char *)(in_ECX + 0x4a5) != '\0') &&
       ((*(int *)(in_ECX + 0x4f0) - *(int *)(in_ECX + 0x4ec) & 0xfffffffcU) != 0)) {
      uVar7 = *(undefined4 *)(*(int *)(in_ECX + 0x30) + 0xc);
      iVar2 = *(int *)(*(int *)(in_ECX + 0x30) + 8);
      if (local_80 != *(int *)(in_ECX + 0x478)) {
        (**(code **)(*(int *)(in_ECX + 0x2b0) + 0xc))(param_1);
      }
      FUN_00465840(0,param_1,0);
      FUN_00465840(0,param_1,0);
      CopyRect(&local_48,(RECT *)(in_ECX + 0x4c8));
      FUN_00477820(iVar2,*(undefined4 *)(iVar2 + 100),&local_48,0);
      local_6c = local_7c;
      if (local_7c < local_78) {
        do {
          iVar3 = *(int *)(*(int *)(in_ECX + 0x4ec) + local_6c * 4);
          if (local_6c == *(int *)(in_ECX + 0x2a0)) {
            local_28.right = *(int *)(iVar3 + 0x20c) + *(int *)(in_ECX + 0x4d8);
            local_28.left = *(int *)(iVar3 + 0x204) + *(int *)(in_ECX + 0x4d8);
            local_28.top = *(int *)(iVar3 + 0x208) + *(int *)(in_ECX + 0x4dc);
            local_28.bottom = *(int *)(iVar3 + 0x210) + *(int *)(in_ECX + 0x4dc);
            FUN_00465740(0);
            CopyRect(&local_48,&local_28);
            ptVar8 = &local_48;
LAB_00438cf5:
            FUN_00477820(uVar7,*(undefined4 *)(iVar2 + 100),ptVar8,0);
          }
          else if (local_6c == *(int *)(in_ECX + 0x2a4)) {
            local_38.left = *(int *)(iVar3 + 0x204) + *(int *)(in_ECX + 0x4d8);
            local_38.top = *(int *)(iVar3 + 0x208) + *(int *)(in_ECX + 0x4dc);
            local_38.bottom = *(int *)(iVar3 + 0x210) + *(int *)(in_ECX + 0x4dc);
            local_38.right = *(int *)(iVar3 + 0x20c) + *(int *)(in_ECX + 0x4d8);
            FUN_00465740(2);
            CopyRect(&local_18,&local_38);
            ptVar8 = &local_18;
            goto LAB_00438cf5;
          }
          local_6c = local_6c + 1;
        } while (local_6c < local_78);
      }
      if (*(int *)(in_ECX + 0x28) != 0) {
        FUN_004762c0(*(int *)(in_ECX + 0x28),iVar2,*(undefined4 *)(in_ECX + 0x4a8),
                     *(undefined4 *)(in_ECX + 0x4dc),*(undefined4 *)(in_ECX + 0x40),
                     *(undefined4 *)(in_ECX + 0x44),0);
      }
    }
    uVar7 = 0;
    local_60 = 0;
    if (*(char *)(in_ECX + 0xcc) == '\0') {
      uVar7 = 3;
    }
    else if (*(char *)(in_ECX + 0x211) == '\0') {
      if (*(char *)(in_ECX + 9) != '\0') {
        uVar7 = 2;
        local_60 = 0;
      }
    }
    else {
      uVar7 = 1;
      local_60 = 1;
    }
    uVar5 = local_60;
    iVar2 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
    FUN_00465840(uVar7,param_1,0);
    local_58.top = *(LONG *)(in_ECX + 0x4bc);
    local_58.right = *(LONG *)(in_ECX + 0x4c0);
    local_58.left = *(LONG *)(in_ECX + 0x4b8);
    local_58.bottom = *(LONG *)(in_ECX + 0x4c4);
    OffsetRect(&local_58,uVar5,local_60);
    CopyRect(&local_48,&local_58);
    FUN_00477820(iVar2,*(undefined4 *)(iVar2 + 100),&local_48,0);
  }
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}

