// rea decompile_all
// name: FUN_0047df30
// address: 0x47df30
// body: available
// range: 0x47df30-0x47ecd6 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */

void FUN_0047df30(wchar_t *param_1,wchar_t *param_2)

{
  char cVar1;
  wchar_t wVar2;
  undefined1 uVar3;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  wchar_t *pwVar7;
  undefined4 *puVar8;
  undefined4 uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int xLeft;
  int in_ECX;
  char *pcVar13;
  double dVar14;
  int local_10d0 [1024];
  int local_d0;
  int local_cc;
  int local_c8;
  int local_c4;
  undefined1 local_be;
  undefined1 local_bd;
  undefined1 local_bc [56];
  uint local_84 [4];
  undefined4 local_74;
  uint local_70;
  uint local_4c [4];
  undefined4 local_3c;
  uint local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084c8fb;
  local_10 = ExceptionList;
  uVar4 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_14 = uVar4;
  iVar5 = __wcsicmp(param_1,L"resource");
  if (iVar5 == 0) {
    local_70 = 0xf;
    local_74 = 0;
    local_84[0] = local_84[0] & 0xffffff00;
    local_8 = 0;
    pcVar6 = (char *)FUN_00413dc0(local_84,param_2,uVar4);
    local_38 = 0xf;
    local_3c = 0;
    local_4c[0] = local_4c[0] & 0xffffff00;
    pcVar13 = pcVar6;
    do {
      cVar1 = *pcVar13;
      pcVar13 = pcVar13 + 1;
    } while (cVar1 != '\0');
    FUN_0040c9a0(pcVar6,(int)pcVar13 - (int)(pcVar6 + 1));
    local_8._0_1_ = 1;
    FUN_0047a110(local_4c);
    local_8 = (uint)local_8._1_3_ << 8;
    if (0xf < local_38) {
      FUN_00407f60(local_4c[0],local_38 + 1);
    }
    local_38 = 0xf;
    local_3c = 0;
    local_4c[0] = local_4c[0] & 0xffffff00;
    local_8 = 0xffffffff;
    if (0xf < local_70) {
      FUN_00407f60(local_84[0],local_70 + 1);
    }
  }
  else {
    iVar5 = __wcsicmp(param_1,L"textureAlias");
    if (iVar5 == 0) {
      local_38 = 7;
      local_3c = 0;
      local_4c[0] = local_4c[0] & 0xffff0000;
      pwVar7 = param_2;
      do {
        wVar2 = *pwVar7;
        pwVar7 = pwVar7 + 1;
      } while (wVar2 != L'\0');
      FUN_0040caa0(param_2,(int)pwVar7 - (int)(param_2 + 1) >> 1);
      local_8 = 2;
      puVar8 = (undefined4 *)FUN_00466bb0(local_4c);
      *(undefined4 *)(in_ECX + 0x48) = *puVar8;
      *(undefined4 *)(in_ECX + 0x4c) = puVar8[1];
      *(undefined4 *)(in_ECX + 0x50) = puVar8[2];
      *(undefined4 *)(in_ECX + 0x54) = puVar8[3];
      *(undefined4 *)(in_ECX + 0x58) = puVar8[4];
      local_8 = 0xffffffff;
      if (7 < local_38) {
        FUN_0040be40(local_4c[0],local_38 + 1);
      }
    }
    else {
      iVar5 = __wcsicmp(param_1,L"RasterOp");
      if (iVar5 == 0) {
        *(undefined4 *)(in_ECX + 0x44) = 1;
        iVar5 = __wcsicmp(param_2,L"wrap");
        if (iVar5 == 0) {
          *(undefined4 *)(in_ECX + 0x44) = 0;
        }
        else {
          iVar5 = __wcsicmp(param_2,L"stretch");
          if (iVar5 == 0) {
            *(undefined4 *)(in_ECX + 0x44) = 1;
          }
          else {
            iVar5 = __wcsicmp(param_2,L"split");
            if (iVar5 == 0) {
              *(undefined4 *)(in_ECX + 0x44) = 2;
            }
            else {
              iVar5 = __wcsicmp(param_2,L"wSplitStretch");
              if (iVar5 == 0) {
                *(undefined4 *)(in_ECX + 0x44) = 3;
              }
              else {
                iVar5 = __wcsicmp(param_2,L"hStretchSplit");
                if (iVar5 == 0) {
                  *(undefined4 *)(in_ECX + 0x44) = 4;
                }
              }
            }
          }
        }
      }
      else {
        iVar5 = __wcsicmp(param_1,L"textureOffset");
        if (iVar5 == 0) {
          FUN_004138d0(param_2,0xffffffff);
          uVar9 = FUN_0080d8dc(local_10d0[0]);
          *(undefined4 *)(in_ECX + 0x454) = uVar9;
          uVar9 = FUN_0080d8dc(local_10d0[1]);
          *(undefined4 *)(in_ECX + 0x458) = uVar9;
          uVar9 = FUN_0080d8dc(local_10d0[2]);
          *(undefined4 *)(in_ECX + 0x45c) = uVar9;
          uVar9 = FUN_0080d8dc(local_10d0[3]);
          *(undefined4 *)(in_ECX + 0x460) = uVar9;
        }
        else {
          iVar5 = __wcsicmp(param_1,L"area");
          if (iVar5 == 0) {
            FUN_004138d0(param_2,0xffffffff);
            local_c4 = local_10d0[1];
            local_d0 = local_10d0[0];
            local_c8 = local_10d0[2];
            local_cc = local_10d0[0];
            iVar5 = FUN_0080d8dc(local_10d0[3]);
            iVar10 = FUN_0080d8dc(local_10d0[1]);
            iVar11 = FUN_0080d8dc(local_c8);
            iVar12 = FUN_0080d8dc(local_d0);
            local_c4 = FUN_0080d8dc(local_c4);
            xLeft = FUN_0080d8dc(local_cc);
            SetRect((LPRECT)(in_ECX + 0x14),xLeft,local_c4,iVar11 + iVar12,iVar5 + iVar10);
          }
          else {
            iVar5 = __wcsicmp(param_1,L"rect");
            if (iVar5 == 0) {
              FUN_004138d0(param_2,0xffffffff);
              local_cc = local_10d0[0];
              local_c4 = FUN_0080d8dc(local_10d0[3]);
              iVar5 = FUN_0080d8dc(local_10d0[2]);
              iVar10 = FUN_0080d8dc(local_10d0[1]);
              iVar11 = FUN_0080d8dc(local_cc);
              SetRect((LPRECT)(in_ECX + 0x14),iVar11,iVar10,iVar5,local_c4);
            }
            else {
              iVar5 = __wcsicmp(param_1,L"left");
              if (iVar5 == 0) {
                iVar5 = *(int *)(in_ECX + 0x1c);
                iVar10 = *(int *)(in_ECX + 0x14);
                iVar11 = FUN_0080d8dc(param_2);
                *(int *)(in_ECX + 0x14) = iVar11;
                *(int *)(in_ECX + 0x1c) = iVar11 + (iVar5 - iVar10);
              }
              else {
                iVar5 = __wcsicmp(param_1,L"top");
                if (iVar5 == 0) {
                  iVar5 = *(int *)(in_ECX + 0x20);
                  iVar10 = *(int *)(in_ECX + 0x18);
                  iVar11 = FUN_0080d8dc(param_2);
                  *(int *)(in_ECX + 0x18) = iVar11;
                  *(int *)(in_ECX + 0x20) = iVar11 + (iVar5 - iVar10);
                }
                else {
                  iVar5 = __wcsicmp(param_1,L"width");
                  if (iVar5 == 0) {
                    iVar5 = FUN_0080d8dc(param_2);
                    *(int *)(in_ECX + 0x1c) = iVar5 + *(int *)(in_ECX + 0x14);
                  }
                  else {
                    iVar5 = __wcsicmp(param_1,L"height");
                    if (iVar5 == 0) {
                      iVar5 = FUN_0080d8dc(param_2);
                      *(int *)(in_ECX + 0x20) = iVar5 + *(int *)(in_ECX + 0x18);
                    }
                    else {
                      iVar5 = __wcsicmp(param_1,L"zorder");
                      if (iVar5 == 0) {
                        uVar9 = FUN_0080d8dc(param_2);
                        FUN_00479f40(uVar9);
                      }
                      else {
                        iVar5 = __wcsicmp(param_1,L"CaptionColor");
                        if (iVar5 == 0) {
                          FUN_004138d0(param_2,0xffffffff);
                          uVar3 = FUN_0080d8dc(local_10d0[0]);
                          local_bd = FUN_0080d8dc(local_10d0[1]);
                          local_be = FUN_0080d8dc(local_10d0[2]);
                          uVar4 = FUN_0080d8dc(local_10d0[3]);
                          FUN_0046eaf0(uVar4 & 0xff |
                                       (uint)CONCAT21(CONCAT11(uVar3,local_bd),local_be) << 8);
                        }
                        else {
                          iVar5 = __wcsicmp(param_1,L"color");
                          if ((iVar5 == 0) ||
                             (iVar5 = __wcsicmp(param_1,L"texturecolor"), iVar5 == 0)) {
                            FUN_004138d0(param_2,0xffffffff);
                            uVar3 = FUN_0080d8dc(local_10d0[0]);
                            local_be = FUN_0080d8dc(local_10d0[1]);
                            local_bd = FUN_0080d8dc(local_10d0[2]);
                            uVar4 = FUN_0080d8dc(local_10d0[3]);
                            *(uint *)(in_ECX + 0x28) =
                                 uVar4 & 0xff |
                                 (uint)CONCAT21(CONCAT11(uVar3,local_be),local_bd) << 8;
                          }
                          else {
                            iVar5 = __wcsicmp(param_1,L"Caption");
                            if (iVar5 == 0) {
                              FUN_0046ef60(param_2);
                            }
                            else {
                              iVar5 = __wcsicmp(param_1,L"captionShadowtype");
                              if (iVar5 == 0) {
                                iVar5 = __wcsicmp(param_2,L"outline");
                                if (iVar5 == 0) {
                                  *(undefined4 *)(in_ECX + 0x29c) = 2;
                                }
                                else {
                                  iVar5 = __wcsicmp(param_2,L"bottomRight");
                                  if (iVar5 == 0) {
                                    *(undefined4 *)(in_ECX + 0x29c) = 1;
                                  }
                                  else {
                                    iVar5 = __wcsicmp(param_2,L"top");
                                    if (iVar5 == 0) {
                                      *(undefined4 *)(in_ECX + 0x29c) = 3;
                                    }
                                  }
                                }
                              }
                              else {
                                iVar5 = __wcsicmp(param_1,L"captionShadowcolor");
                                if (iVar5 == 0) {
                                  FUN_004138d0(param_2,0xffffffff);
                                  uVar3 = FUN_0080d8dc(local_10d0[0]);
                                  local_be = FUN_0080d8dc(local_10d0[1]);
                                  local_bd = FUN_0080d8dc(local_10d0[2]);
                                  uVar4 = FUN_0080d8dc(local_10d0[3]);
                                  *(uint *)(in_ECX + 0x298) =
                                       uVar4 & 0xff |
                                       (uint)CONCAT21(CONCAT11(uVar3,local_be),local_bd) << 8;
                                }
                                else {
                                  iVar5 = __wcsicmp(param_1,L"titlebar");
                                  if (iVar5 == 0) {
                                    iVar5 = __wcsicmp(param_2,L"true");
                                    *(bool *)(in_ECX + 0x90) = iVar5 == 0;
                                  }
                                  else {
                                    iVar5 = __wcsicmp(param_1,L"CloseButtonLocation");
                                    if (iVar5 == 0) {
                                      FUN_004138d0(param_2,0xffffffff);
                                      uVar9 = FUN_0080d8dc(local_10d0[0]);
                                      *(undefined4 *)(in_ECX + 0x30c) = uVar9;
                                      uVar9 = FUN_0080d8dc(local_10d0[1]);
                                      *(undefined4 *)(in_ECX + 0x310) = uVar9;
                                      uVar9 = FUN_0080d8dc(local_10d0[2]);
                                      *(undefined4 *)(in_ECX + 0x314) = uVar9;
                                      uVar9 = FUN_0080d8dc(local_10d0[3]);
                                      *(undefined4 *)(in_ECX + 0x318) = uVar9;
                                    }
                                    else {
                                      iVar5 = __wcsicmp(param_1,L"closebutton");
                                      if (iVar5 == 0) {
                                        iVar5 = __wcsicmp(param_2,L"true");
                                        if (iVar5 == 0) {
                                          FUN_0047c550();
                                        }
                                      }
                                      else {
                                        iVar5 = __wcsicmp(param_1,L"allowFocus");
                                        if (iVar5 == 0) {
                                          iVar5 = __wcsicmp(param_2,L"false");
                                          if (iVar5 == 0) {
                                            *(undefined1 *)(in_ECX + 0xb) = 0;
                                          }
                                        }
                                        else {
                                          iVar5 = __wcsicmp(param_1,L"PageIndex");
                                          if (iVar5 == 0) {
                                            uVar9 = FUN_0080d8dc(param_2);
                                            *(undefined4 *)(in_ECX + 0x478) = uVar9;
                                          }
                                          else {
                                            iVar5 = __wcsicmp(param_1,L"OpenSound");
                                            if (iVar5 == 0) {
                                              uVar9 = FUN_0080d8dc(param_2);
                                              *(undefined4 *)(in_ECX + 0x494) = uVar9;
                                            }
                                            else {
                                              iVar5 = __wcsicmp(param_1,L"CloseSound");
                                              if (iVar5 == 0) {
                                                uVar9 = FUN_0080d8dc(param_2);
                                                *(undefined4 *)(in_ECX + 0x498) = uVar9;
                                              }
                                              else {
                                                iVar5 = __wcsicmp(param_1,L"TitlebarHeight");
                                                if (iVar5 == 0) {
                                                  uVar9 = FUN_0080d8dc(param_2);
                                                  *(undefined4 *)(in_ECX + 0x94) = uVar9;
                                                }
                                                else {
                                                  iVar5 = __wcsicmp(param_1,L"CaptionFont");
                                                  if (iVar5 == 0) {
                                                    uVar9 = FUN_0080d8dc(param_2);
                                                    *(undefined4 *)(in_ECX + 0x2b4) = uVar9;
                                                  }
                                                  else {
                                                    iVar5 = __wcsicmp(param_1,L"resizeTip");
                                                    if (iVar5 == 0) {
                                                      iVar5 = FUN_004138d0(param_2,0xffffffff);
                                                      *(undefined4 *)(in_ECX + 0x468) = 0;
                                                      if (0 < iVar5) {
                                                        do {
                                                          local_c8 = iVar5;
                                                          iVar5 = __wcsicmp(param_2,L"bottom");
                                                          if (iVar5 == 0) {
                                                            *(uint *)(in_ECX + 0x468) =
                                                                 *(uint *)(in_ECX + 0x468) | 2;
                                                          }
                                                          else {
                                                            iVar5 = __wcsicmp(param_2,L"top");
                                                            if (iVar5 == 0) {
                                                              *(uint *)(in_ECX + 0x468) =
                                                                   *(uint *)(in_ECX + 0x468) | 1;
                                                            }
                                                            else {
                                                              iVar5 = __wcsicmp(param_2,L"left");
                                                              if (iVar5 == 0) {
                                                                *(uint *)(in_ECX + 0x468) =
                                                                     *(uint *)(in_ECX + 0x468) | 4;
                                                              }
                                                              else {
                                                                iVar5 = __wcsicmp(param_2,L"right");
                                                                if (iVar5 == 0) {
                                                                  *(uint *)(in_ECX + 0x468) =
                                                                       *(uint *)(in_ECX + 0x468) | 8
                                                                  ;
                                                                }
                                                                else {
                                                                  iVar5 = __wcsicmp(param_2,
                                                  L"topLeft");
                                                  if (iVar5 == 0) {
                                                    *(uint *)(in_ECX + 0x468) =
                                                         *(uint *)(in_ECX + 0x468) | 0x20;
                                                  }
                                                  else {
                                                    iVar5 = __wcsicmp(param_2,L"topRight");
                                                    if (iVar5 == 0) {
                                                      *(uint *)(in_ECX + 0x468) =
                                                           *(uint *)(in_ECX + 0x468) | 0x10;
                                                    }
                                                    else {
                                                      iVar5 = __wcsicmp(param_2,L"bottomLeft");
                                                      if (iVar5 == 0) {
                                                        *(uint *)(in_ECX + 0x468) =
                                                             *(uint *)(in_ECX + 0x468) | 0x80;
                                                      }
                                                      else {
                                                        iVar5 = __wcsicmp(param_2,L"bottomRight");
                                                        if (iVar5 == 0) {
                                                          *(uint *)(in_ECX + 0x468) =
                                                               *(uint *)(in_ECX + 0x468) | 0x40;
                                                        }
                                                        else {
                                                          iVar5 = __wcsicmp(param_2,L"all");
                                                          if (iVar5 == 0) {
                                                            *(undefined4 *)(in_ECX + 0x468) =
                                                                 0xffffffff;
                                                          }
                                                        }
                                                      }
                                                    }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  iVar5 = local_c8 + -1;
                                                  } while (local_c8 + -1 != 0);
                                                  local_c8 = 0;
                                                  }
                                                  }
                                                  else {
                                                    iVar5 = __wcsicmp(param_1,L"ScreenAlign");
                                                    if (iVar5 == 0) {
                                                      iVar10 = 0;
                                                      iVar5 = FUN_004138d0(param_2,0xffffffff);
                                                      if (0 < iVar5) {
                                                        do {
                                                          iVar11 = __wcsicmp((wchar_t *)
                                                                             local_10d0[iVar10],
                                                                             L"left");
                                                          if (iVar11 == 0) {
                                                            *(uint *)(in_ECX + 0x2c) =
                                                                 *(uint *)(in_ECX + 0x2c) | 1;
                                                          }
                                                          else {
                                                            iVar11 = __wcsicmp((wchar_t *)
                                                                               local_10d0[iVar10],
                                                                               L"top");
                                                            if (iVar11 == 0) {
                                                              *(uint *)(in_ECX + 0x2c) =
                                                                   *(uint *)(in_ECX + 0x2c) | 2;
                                                            }
                                                            else {
                                                              iVar11 = __wcsicmp((wchar_t *)
                                                                                 local_10d0[iVar10],
                                                                                 L"right");
                                                              if (iVar11 == 0) {
                                                                *(uint *)(in_ECX + 0x2c) =
                                                                     *(uint *)(in_ECX + 0x2c) | 4;
                                                              }
                                                              else {
                                                                iVar11 = __wcsicmp((wchar_t *)
                                                                                   local_10d0
                                                                                   [iVar10],
                                                  L"bottom");
                                                  if (iVar11 == 0) {
                                                    *(uint *)(in_ECX + 0x2c) =
                                                         *(uint *)(in_ECX + 0x2c) | 8;
                                                  }
                                                  }
                                                  }
                                                  }
                                                  iVar10 = iVar10 + 1;
                                                  } while (iVar10 < iVar5);
                                                  }
                                                  }
                                                  else {
                                                    iVar5 = __wcsicmp(param_1,L"AlignPanelX");
                                                    if (iVar5 == 0) {
                                                      iVar5 = __wcsicmp(param_2,L"NONE");
                                                      if (iVar5 == 0) {
                                                        *(undefined4 *)(in_ECX + 0x71c) = 0;
                                                      }
                                                      else {
                                                        iVar5 = __wcsicmp(param_2,L"Center");
                                                        if (iVar5 == 0) {
                                                          *(undefined4 *)(in_ECX + 0x71c) = 1;
                                                        }
                                                        else {
                                                          iVar5 = __wcsicmp(param_2,L"SIDE");
                                                          if (iVar5 == 0) {
                                                            *(undefined4 *)(in_ECX + 0x71c) = 2;
                                                          }
                                                        }
                                                      }
                                                    }
                                                    else {
                                                      iVar5 = __wcsicmp(param_1,L"AlignPanelY");
                                                      if (iVar5 == 0) {
                                                        iVar5 = __wcsicmp(param_2,L"NONE");
                                                        if (iVar5 == 0) {
                                                          *(undefined4 *)(in_ECX + 0x720) = 0;
                                                        }
                                                        else {
                                                          iVar5 = __wcsicmp(param_2,L"Center");
                                                          if (iVar5 == 0) {
                                                            *(undefined4 *)(in_ECX + 0x720) = 1;
                                                          }
                                                          else {
                                                            iVar5 = __wcsicmp(param_2,L"SIDE");
                                                            if (iVar5 == 0) {
                                                              *(undefined4 *)(in_ECX + 0x720) = 2;
                                                            }
                                                          }
                                                        }
                                                      }
                                                      else {
                                                        iVar5 = __wcsicmp(param_1,L"OffsetX");
                                                        if (iVar5 == 0) {
                                                          dVar14 = __wtof(param_2);
                                                          *(float *)(in_ECX + 0x724) = (float)dVar14
                                                          ;
                                                        }
                                                        else {
                                                          iVar5 = __wcsicmp(param_1,L"OffsetY");
                                                          if (iVar5 == 0) {
                                                            dVar14 = __wtof(param_2);
                                                            *(float *)(in_ECX + 0x728) =
                                                                 (float)dVar14;
                                                          }
                                                          else {
                                                            iVar5 = __wcsicmp(param_1,
                                                  L"PanelScaleType");
                                                  if (iVar5 == 0) {
                                                    iVar5 = FUN_0080d8dc(param_2);
                                                    if (iVar5 != 0) {
                                                      FUN_0046edb0(1);
                                                    }
                                                  }
                                                  else {
                                                    iVar5 = __wcsicmp(param_1,L"Animation");
                                                    if (iVar5 == 0) {
                                                      if (*(int *)(in_ECX + 0x764) == 0) {
                                                        local_cc = FUN_00675c42(0x48);
                                                        local_8 = 3;
                                                        if (local_cc == 0) {
                                                          iVar5 = 0;
                                                        }
                                                        else {
                                                          iVar5 = FUN_00481580();
                                                        }
                                                        local_8 = 0xffffffff;
                                                        *(int *)(in_ECX + 0x764) = iVar5;
                                                        if (iVar5 == 0) goto LAB_0047ecb9;
                                                      }
                                                      FUN_0040d040(param_2);
                                                      local_8 = 4;
                                                      FUN_0040c8a0(local_bc,0,0xffffffff);
                                                      local_8 = 0xffffffff;
                                                      FUN_0040c740();
                                                      FUN_004340d0();
                                                    }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
LAB_0047ecb9:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

