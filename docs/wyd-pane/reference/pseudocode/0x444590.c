// rea decompile_all
// name: FUN_00444590
// address: 0x444590
// body: available
// range: 0x444590-0x4454fc inclusive
// range: 0x445500-0x445605 inclusive
// non_contiguous: true
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */

void FUN_00444590(wchar_t *param_1,wchar_t *param_2)

{
  wchar_t wVar1;
  code *pcVar2;
  undefined4 *puVar3;
  undefined1 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int *in_ECX;
  uint uVar9;
  uint uVar10;
  undefined4 uVar11;
  uint uVar12;
  uint uVar13;
  undefined4 local_1094;
  undefined4 local_1090;
  undefined4 local_108c;
  undefined4 local_1088;
  uint local_94;
  uint local_90;
  uint local_8c;
  uint local_88;
  undefined1 local_84 [56];
  undefined1 local_4c [56];
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084a3c5;
  local_10 = ExceptionList;
  uVar5 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_14 = uVar5;
  iVar6 = __wcsicmp(param_1,L"id");
  if (iVar6 == 0) {
    iVar6 = FUN_0080d8dc(param_2,uVar5);
    in_ECX[0x14] = iVar6;
  }
  else {
    iVar6 = __wcsnicmp(param_1,L"Font[",5);
    if (iVar6 == 0) {
      iVar6 = (ushort)param_1[5] - 0x30;
      if (iVar6 < in_ECX[0xd] - in_ECX[0xc] >> 2) {
        iVar7 = FUN_0080d8dc(param_2,uVar5);
        if (iVar7 < 0) {
          *(undefined4 *)(*(int *)(in_ECX[0xc] + iVar6 * 4) + 0x14) = 0;
        }
        else {
          if (0x14 < iVar7) {
            iVar7 = 0x14;
          }
          *(int *)(*(int *)(in_ECX[0xc] + iVar6 * 4) + 0x14) = iVar7;
        }
      }
    }
    else {
      iVar6 = __wcsicmp(param_1,L"font");
      if (iVar6 == 0) {
        iVar6 = FUN_0080d8dc(param_2,uVar5);
        if (iVar6 < 0) {
          FUN_00443100(0);
        }
        else {
          if (0x14 < iVar6) {
            iVar6 = 0x14;
          }
          FUN_00443100(iVar6);
        }
      }
      else {
        iVar6 = __wcsicmp(param_1,L"PageIndex");
        if (iVar6 == 0) {
          uVar4 = FUN_0080d8dc(param_2,uVar5);
          *(undefined1 *)(in_ECX + 0x16) = uVar4;
        }
        else {
          iVar6 = __wcsicmp(param_1,L"name");
          if (iVar6 == 0) {
            FUN_00447800(param_1);
          }
          else {
            iVar6 = __wcsicmp(param_1,L"enabled");
            if (iVar6 == 0) {
              *(bool *)(in_ECX + 0x33) = param_2 == L"true";
            }
            else {
              iVar6 = __wcsicmp(param_1,L"visible");
              if (iVar6 == 0) {
                *(bool *)(in_ECX + 2) = param_2 == L"true";
              }
              else {
                iVar6 = __wcsicmp(param_1,L"MsgReceive");
                if (iVar6 == 0) {
                  *(bool *)(in_ECX + 4) = param_2 == L"true";
                }
                else {
                  iVar6 = __wcsicmp(param_1,L"area");
                  if (iVar6 == 0) {
                    FUN_004138d0(param_2,0xffffffff);
                    iVar6 = FUN_0080d8dc(local_1094);
                    in_ECX[5] = iVar6;
                    iVar6 = FUN_0080d8dc(local_1090);
                    in_ECX[6] = iVar6;
                    iVar6 = FUN_0080d8dc(local_108c);
                    in_ECX[7] = iVar6;
                    iVar6 = FUN_0080d8dc(local_1088);
                    in_ECX[8] = iVar6;
                    (**(code **)(*in_ECX + 0x70))();
                  }
                  else {
                    iVar6 = __wcsicmp(param_1,L"rect");
                    if (iVar6 == 0) {
                      FUN_004138d0(param_2,0xffffffff);
                      iVar6 = FUN_0080d8dc(local_1094);
                      in_ECX[5] = iVar6;
                      iVar6 = FUN_0080d8dc(local_1090);
                      in_ECX[6] = iVar6;
                      iVar6 = FUN_0080d8dc(local_108c);
                      iVar7 = FUN_0080d8dc(local_1088);
                      in_ECX[8] = iVar7 - in_ECX[6];
                      pcVar2 = *(code **)(*in_ECX + 0x70);
                      in_ECX[7] = iVar6 - in_ECX[5];
                      (*pcVar2)();
                    }
                    else {
                      iVar6 = __wcsicmp(param_1,L"left");
                      if (iVar6 == 0) {
                        iVar6 = FUN_0080d8dc(param_2);
                        in_ECX[5] = iVar6;
                        (**(code **)(*in_ECX + 0x70))();
                      }
                      else {
                        iVar6 = __wcsicmp(param_1,L"top");
                        if (iVar6 == 0) {
                          iVar6 = FUN_0080d8dc(param_2);
                          in_ECX[6] = iVar6;
                          (**(code **)(*in_ECX + 0x70))();
                        }
                        else {
                          iVar6 = __wcsicmp(param_1,L"width");
                          if (iVar6 == 0) {
                            iVar6 = FUN_0080d8dc(param_2);
                            in_ECX[7] = iVar6;
                            (**(code **)(*in_ECX + 0x70))();
                          }
                          else {
                            iVar6 = __wcsicmp(param_1,L"height");
                            if (iVar6 == 0) {
                              iVar6 = FUN_0080d8dc(param_2);
                              in_ECX[8] = iVar6;
                              (**(code **)(*in_ECX + 0x70))();
                            }
                            else {
                              iVar6 = __wcsicmp(param_1,L"default");
                              if (iVar6 == 0) {
                                iVar6 = __wcsicmp(param_2,L"false");
                                if (iVar6 != 0) {
                                  *(undefined1 *)((int)in_ECX + 0xb) = 1;
                                }
                              }
                              else {
                                iVar6 = __wcsicmp(param_1,L"BelowPanel");
                                if (iVar6 == 0) {
                                  iVar6 = __wcsicmp(param_2,L"true");
                                  if (iVar6 == 0) {
                                    *(undefined1 *)((int)in_ECX + 0xe) = 1;
                                  }
                                }
                                else {
                                  iVar6 = __wcsicmp(param_1,L"shadowType");
                                  if (iVar6 == 0) {
                                    iVar6 = __wcsicmp(param_2,L"outline");
                                    if (iVar6 == 0) {
                                      in_ECX[0x10] = 2;
                                    }
                                    else {
                                      iVar6 = __wcsicmp(param_2,L"bottomRight");
                                      if (iVar6 == 0) {
                                        in_ECX[0x10] = 1;
                                      }
                                      else {
                                        iVar6 = __wcsicmp(param_2,L"top");
                                        if (iVar6 == 0) {
                                          in_ECX[0x10] = 3;
                                        }
                                      }
                                    }
                                  }
                                  else {
                                    uVar5 = __wcsicmp(param_1,L"shadowColor");
                                    if (uVar5 == 0) {
                                      local_94 = uVar5;
                                      local_90 = uVar5;
                                      local_8c = uVar5;
                                      local_88 = uVar5;
                                      _swscanf_s(param_2,L"%d %d %d %d",&local_90,&local_94,
                                                 &local_8c,&local_88);
                                      in_ECX[0x11] = ((local_94 & 0xff | local_90 << 8) << 8 |
                                                     local_8c & 0xff) << 8 | local_88 & 0xff;
                                    }
                                    else {
                                      iVar6 = __wcsnicmp(param_1,L"RasterOp[",9);
                                      if (iVar6 == 0) {
                                        iVar6 = (ushort)param_1[9] - 0x30;
                                        if (iVar6 < in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                          *(undefined4 *)(*(int *)(in_ECX[0xc] + iVar6 * 4) + 100) =
                                               1;
                                          iVar7 = __wcsicmp(param_2,L"wrap");
                                          if (iVar7 == 0) {
                                            *(undefined4 *)(*(int *)(in_ECX[0xc] + iVar6 * 4) + 100)
                                                 = 0;
                                          }
                                          else {
                                            iVar7 = __wcsicmp(param_2,L"stretch");
                                            if (iVar7 == 0) {
                                              *(undefined4 *)
                                               (*(int *)(in_ECX[0xc] + iVar6 * 4) + 100) = 1;
                                            }
                                            else {
                                              iVar7 = __wcsicmp(param_2,L"split");
                                              if (iVar7 == 0) {
                                                *(undefined4 *)
                                                 (*(int *)(in_ECX[0xc] + iVar6 * 4) + 100) = 2;
                                              }
                                              else {
                                                iVar7 = __wcsicmp(param_2,L"wSplitStretch");
                                                if (iVar7 == 0) {
                                                  *(undefined4 *)
                                                   (*(int *)(in_ECX[0xc] + iVar6 * 4) + 100) = 3;
                                                }
                                                else {
                                                  iVar7 = __wcsicmp(param_2,L"hStretchSplit");
                                                  if (iVar7 == 0) {
                                                    *(undefined4 *)
                                                     (*(int *)(in_ECX[0xc] + iVar6 * 4) + 100) = 4;
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                      else {
                                        iVar6 = __wcsicmp(param_1,L"RasterOp");
                                        if (iVar6 == 0) {
                                          uVar11 = 1;
                                          iVar6 = __wcsicmp(param_2,L"wrap");
                                          if (iVar6 == 0) {
                                            uVar11 = 0;
                                          }
                                          else {
                                            iVar6 = __wcsicmp(param_2,L"stretch");
                                            if (iVar6 != 0) {
                                              iVar6 = __wcsicmp(param_2,L"split");
                                              if (iVar6 == 0) {
                                                uVar11 = 2;
                                              }
                                              else {
                                                iVar6 = __wcsicmp(param_2,L"wSplitStretch");
                                                if (iVar6 == 0) {
                                                  uVar11 = 3;
                                                }
                                                else {
                                                  iVar6 = __wcsicmp(param_2,L"hStretchSplit");
                                                  if (iVar6 == 0) {
                                                    uVar11 = 4;
                                                  }
                                                }
                                              }
                                            }
                                          }
                                          uVar5 = 0;
                                          if (in_ECX[0xd] - in_ECX[0xc] >> 2 != 0) {
                                            do {
                                              *(undefined4 *)
                                               (*(int *)(in_ECX[0xc] + uVar5 * 4) + 100) = uVar11;
                                              uVar5 = uVar5 + 1;
                                            } while (uVar5 < (uint)(in_ECX[0xd] - in_ECX[0xc] >> 2))
                                            ;
                                          }
                                        }
                                        else {
                                          iVar6 = __wcsnicmp(param_1,L"FontAlign[",10);
                                          if (iVar6 == 0) {
                                            if ((int)((ushort)param_1[10] - 0x30) <
                                                in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                              FUN_00443200(param_2,(ushort)param_1[10] - 0x30);
                                            }
                                          }
                                          else {
                                            iVar6 = __wcsicmp(param_1,L"FontAlign");
                                            if (iVar6 == 0) {
                                              FUN_00443200(param_2,0xffffffff);
                                            }
                                            else {
                                              iVar6 = __wcsicmp(param_1,L"FontOffset");
                                              if (iVar6 == 0) {
                                                local_8c = 0;
                                                local_88 = 0;
                                                _swscanf_s(param_2,L"%d %d",&local_8c,&local_88);
                                                uVar5 = 0;
                                                if (in_ECX[0xd] - in_ECX[0xc] >> 2 != 0) {
                                                  do {
                                                    iVar6 = *(int *)(in_ECX[0xc] + uVar5 * 4);
                                                    *(uint *)(iVar6 + 0x20) = local_88;
                                                    *(uint *)(iVar6 + 0x1c) = local_8c;
                                                    uVar5 = uVar5 + 1;
                                                  } while (uVar5 < (uint)(in_ECX[0xd] - in_ECX[0xc]
                                                                         >> 2));
                                                }
                                              }
                                              else {
                                                uVar5 = __wcsnicmp(param_1,L"FontOffset[",0xb);
                                                if (uVar5 == 0) {
                                                  local_8c = uVar5;
                                                  local_88 = uVar5;
                                                  _swscanf_s(param_2,L"%d %d",&local_8c,&local_88);
                                                  if ((int)((ushort)param_1[0xb] - 0x30) <
                                                      in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                    iVar6 = *(int *)(in_ECX[0xc] +
                                                                    ((ushort)param_1[0xb] - 0x30) *
                                                                    4);
                                                    *(uint *)(iVar6 + 0x1c) = local_8c;
                                                    *(uint *)(iVar6 + 0x20) = local_88;
                                                  }
                                                }
                                                else {
                                                  iVar6 = __wcsicmp(param_1,L"FontColor");
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    FUN_00443150(((local_88 & 0xff | local_8c << 8)
                                                                  << 8 | local_90 & 0xff) << 8 |
                                                                 local_94 & 0xff);
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,L"FontColor[",10);
                                                    if (iVar6 == 0) {
                                                      _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                                 &local_88,&local_90,&local_94);
                                                      if ((int)((ushort)param_1[10] - 0x30) <
                                                          in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                        FUN_004656c0(((local_88 & 0xff |
                                                                      local_8c << 8) << 8 |
                                                                     local_90 & 0xff) << 8 |
                                                                     local_94 & 0xff);
                                                      }
                                                    }
                                                    else {
                                                      iVar6 = __wcsnicmp(param_1,
                                                  L"FontColor.Normal[",0x11);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x11] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x11] -
                                                                        0x30) * 4) + 0x44) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,L"FontColor.Pressed["
                                                                       ,0x12);
                                                    if (iVar6 == 0) {
                                                      _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                                 &local_88,&local_90,&local_94);
                                                      if ((int)((ushort)param_1[0x12] - 0x30) <
                                                          in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                        *(uint *)(*(int *)(in_ECX[0xc] +
                                                                          ((ushort)param_1[0x12] -
                                                                          0x30) * 4) + 0x48) =
                                                             ((local_88 & 0xff | local_8c << 8) << 8
                                                             | local_90 & 0xff) << 8 |
                                                             local_94 & 0xff;
                                                      }
                                                    }
                                                    else {
                                                      iVar6 = __wcsnicmp(param_1,
                                                  L"FontColor.MouseOver[",0x14);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x14] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x14] -
                                                                        0x30) * 4) + 0x4c) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,
                                                  L"FontColor.Disabled[",0x13);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x13] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x13] -
                                                                        0x30) * 4) + 0x50) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsicmp(param_1,L"TextureColor");
                                                    if (iVar6 == 0) {
                                                      _swscanf_s(param_2,L"%d %d %d %d",&local_88,
                                                                 &local_8c,&local_90,&local_94);
                                                      uVar10 = local_8c & 0xff;
                                                      uVar5 = local_90 & 0xff;
                                                      uVar9 = local_94 & 0xff;
                                                      uVar13 = local_88 << 8;
                                                      uVar12 = 0;
                                                      if (in_ECX[0xd] - in_ECX[0xc] >> 2 != 0) {
                                                        do {
                                                          FUN_004656c0(((uVar13 | uVar10) << 8 |
                                                                       uVar5) << 8 | uVar9);
                                                          uVar12 = uVar12 + 1;
                                                        } while (uVar12 < (uint)(in_ECX[0xd] -
                                                                                 in_ECX[0xc] >> 2));
                                                      }
                                                    }
                                                    else {
                                                      iVar6 = __wcsnicmp(param_1,L"TextureColor[",
                                                                         0xd);
                                                      if (iVar6 == 0) {
                                                        _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                                   &local_88,&local_90,&local_94);
                                                        if ((int)((ushort)param_1[0xd] - 0x30) <
                                                            in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                          FUN_004656c0(((local_88 & 0xff |
                                                                        local_8c << 8) << 8 |
                                                                       local_90 & 0xff) << 8 |
                                                                       local_94 & 0xff);
                                                        }
                                                      }
                                                      else {
                                                        iVar6 = __wcsnicmp(param_1,
                                                  L"TextureColor.Pressed[",0x15);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x15] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x15] -
                                                                        0x30) * 4) + 0x28) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,
                                                  L"TextureColor.Normal[",0x14);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x14] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x14] -
                                                                        0x30) * 4) + 0x24) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,
                                                  L"TextureColor.MouseOver[",0x17);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x17] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x17] -
                                                                        0x30) * 4) + 0x2c) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,
                                                  L"TextureColor.Disabled[",0x16);
                                                  if (iVar6 == 0) {
                                                    _swscanf_s(param_2,L"%d %d %d %d",&local_8c,
                                                               &local_88,&local_90,&local_94);
                                                    if ((int)((ushort)param_1[0x16] - 0x30) <
                                                        in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                      *(uint *)(*(int *)(in_ECX[0xc] +
                                                                        ((ushort)param_1[0x16] -
                                                                        0x30) * 4) + 0x30) =
                                                           ((local_88 & 0xff | local_8c << 8) << 8 |
                                                           local_90 & 0xff) << 8 | local_94 & 0xff;
                                                    }
                                                  }
                                                  else {
                                                    iVar6 = __wcsnicmp(param_1,L"TextureAlias[",0xd)
                                                    ;
                                                    if (iVar6 == 0) {
                                                      wVar1 = param_1[0xd];
                                                      if ((int)((ushort)wVar1 - 0x30) <
                                                          in_ECX[0xd] - in_ECX[0xc] >> 2) {
                                                        FUN_0040d040(param_2);
                                                        local_8 = 0;
                                                        puVar8 = (undefined4 *)
                                                                 FUN_00466bb0(local_4c);
                                                        puVar3 = *(undefined4 **)
                                                                  (in_ECX[0xc] +
                                                                  ((ushort)wVar1 - 0x30) * 4);
                                                        *puVar3 = *puVar8;
                                                        puVar3[1] = puVar8[1];
                                                        puVar3[2] = puVar8[2];
                                                        puVar3[3] = puVar8[3];
                                                        puVar3[4] = puVar8[4];
                                                        local_8 = 0xffffffff;
                                                        FUN_0040c740();
                                                      }
                                                    }
                                                    else {
                                                      iVar6 = __wcsicmp(param_1,L"TextureAlias");
                                                      if (iVar6 == 0) {
                                                        FUN_0040d040(param_2);
                                                        local_8 = 1;
                                                        puVar8 = (undefined4 *)
                                                                 FUN_00466bb0(local_4c);
                                                        puVar3 = *(undefined4 **)in_ECX[0xc];
                                                        *puVar3 = *puVar8;
                                                        puVar3[1] = puVar8[1];
                                                        puVar3[2] = puVar8[2];
                                                        puVar3[3] = puVar8[3];
                                                        puVar3[4] = puVar8[4];
                                                        local_8 = 0xffffffff;
                                                        FUN_0040c740();
                                                        if ((in_ECX[0x28] == 0) &&
                                                           (uVar5 = 1,
                                                           1 < (uint)(in_ECX[0xd] - in_ECX[0xc] >> 2
                                                                     ))) {
                                                          do {
                                                            FUN_0040d040(param_2);
                                                            local_8 = 2;
                                                            puVar8 = (undefined4 *)
                                                                     FUN_00466bb0(local_4c);
                                                            puVar3 = *(undefined4 **)
                                                                      (in_ECX[0xc] + uVar5 * 4);
                                                            *puVar3 = *puVar8;
                                                            puVar3[1] = puVar8[1];
                                                            puVar3[2] = puVar8[2];
                                                            puVar3[3] = puVar8[3];
                                                            puVar3[4] = puVar8[4];
                                                            local_8 = 0xffffffff;
                                                            FUN_0040c740();
                                                            uVar5 = uVar5 + 1;
                                                          } while (uVar5 < (uint)(in_ECX[0xd] -
                                                                                  in_ECX[0xc] >> 2))
                                                          ;
                                                        }
                                                      }
                                                      else {
                                                        iVar6 = __wcsnicmp(param_1,L"TextureColor[",
                                                                           0xd);
                                                        if (iVar6 == 0) {
                                                          wVar1 = param_1[0xd];
                                                          _swscanf_s(param_2,L"%d %d %d %d");
                                                          FUN_00443070((ushort)wVar1 - 0x30,0);
                                                        }
                                                        else {
                                                          iVar6 = __wcsicmp(param_1,L"Tagname");
                                                          if (iVar6 == 0) {
                                                            FUN_0040d040(param_2);
                                                            local_8 = 3;
                                                            FUN_0040c8a0(local_84,0,0xffffffff);
                                                            local_8 = 0xffffffff;
                                                            FUN_0040c740();
                                                          }
                                                          else {
                                                            (**(code **)(*in_ECX + 0x60))
                                                                      (param_1,param_2);
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
              }
            }
          }
        }
      }
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

