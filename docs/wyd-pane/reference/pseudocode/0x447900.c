// rea decompile_all
// name: FUN_00447900
// address: 0x447900
// body: available
// range: 0x447900-0x447e87 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


undefined4 FUN_00447900(wchar_t *param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084a657;
  local_10 = ExceptionList;
  uVar1 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  iVar2 = __wcsicmp(param_1,L"Static");
  if (iVar2 == 0) {
    iVar2 = FUN_00675c42(0x210,uVar1);
    local_8 = 0;
    if (iVar2 != 0) {
      uVar3 = FUN_0048a310(param_2);
      ExceptionList = local_10;
      return uVar3;
    }
  }
  else {
    iVar2 = __wcsicmp(param_1,L"image");
    if (iVar2 == 0) {
      iVar2 = FUN_00675c42(0x1f8,uVar1);
      local_8 = 1;
      if (iVar2 != 0) {
        uVar3 = FUN_0044a7e0(param_2);
        ExceptionList = local_10;
        return uVar3;
      }
    }
    else {
      iVar2 = __wcsicmp(param_1,L"Button");
      if (iVar2 == 0) {
        iVar2 = FUN_00675c42(0x2a0,uVar1);
        local_8 = 2;
        if (iVar2 != 0) {
          uVar3 = FUN_00436910(param_2);
          ExceptionList = local_10;
          return uVar3;
        }
      }
      else {
        iVar2 = __wcsicmp(param_1,L"CheckBox");
        if (iVar2 == 0) {
          iVar2 = FUN_00675c42(0x2c8,uVar1);
          local_8 = 3;
          if (iVar2 != 0) {
            uVar3 = FUN_00437920(param_2);
            ExceptionList = local_10;
            return uVar3;
          }
        }
        else {
          iVar2 = __wcsicmp(param_1,L"RadioButton");
          if (iVar2 == 0) {
            iVar2 = FUN_00675c42(0x2c8);
            local_8 = 4;
            if (iVar2 != 0) {
              uVar3 = FUN_00483260(param_2);
              ExceptionList = local_10;
              return uVar3;
            }
          }
          else {
            iVar2 = __wcsicmp(param_1,L"GaugeBar");
            if (iVar2 == 0) {
              iVar2 = FUN_00675c42(0x2f0);
              local_8 = 5;
              if (iVar2 != 0) {
                uVar3 = FUN_00448db0(param_2);
                ExceptionList = local_10;
                return uVar3;
              }
            }
            else {
              iVar2 = __wcsicmp(param_1,L"ComboBox");
              if (iVar2 == 0) {
                iVar2 = FUN_00675c42(0x500);
                local_8 = 6;
                if (iVar2 != 0) {
                  uVar3 = FUN_004398f0(param_2);
                  ExceptionList = local_10;
                  return uVar3;
                }
              }
              else {
                iVar2 = __wcsicmp(param_1,L"Slider");
                if (iVar2 == 0) {
                  iVar2 = FUN_00675c42(0x1b0);
                  local_8 = 7;
                  if (iVar2 != 0) {
                    uVar3 = FUN_00489660(param_2);
                    ExceptionList = local_10;
                    return uVar3;
                  }
                }
                else {
                  iVar2 = __wcsicmp(param_1,L"EditBox");
                  if (iVar2 == 0) {
                    iVar2 = FUN_00675c42(0x3e0);
                    local_8 = 8;
                    if (iVar2 != 0) {
                      uVar3 = FUN_004402c0(param_2);
                      ExceptionList = local_10;
                      return uVar3;
                    }
                  }
                  else {
                    iVar2 = __wcsicmp(param_1,L"ImeEditBox");
                    if (iVar2 == 0) {
                      iVar2 = FUN_00675c42(0x440);
                      local_8 = 9;
                      if (iVar2 != 0) {
                        uVar3 = FUN_0044c3e0(param_2);
                        ExceptionList = local_10;
                        return uVar3;
                      }
                    }
                    else {
                      iVar2 = __wcsicmp(param_1,L"ListBox");
                      if (iVar2 == 0) {
                        iVar2 = FUN_00675c42(0x3d0);
                        local_8 = 10;
                        if (iVar2 != 0) {
                          uVar3 = FUN_00455250(param_2,9);
                          ExceptionList = local_10;
                          return uVar3;
                        }
                      }
                      else {
                        iVar2 = __wcsicmp(param_1,L"ListView");
                        if (iVar2 == 0) {
                          iVar2 = FUN_00675c42(0x448);
                          local_8 = 0xb;
                          if (iVar2 != 0) {
                            uVar3 = FUN_0045bff0(param_2);
                            ExceptionList = local_10;
                            return uVar3;
                          }
                        }
                        else {
                          iVar2 = __wcsicmp(param_1,L"BalloonBox");
                          if (iVar2 == 0) {
                            iVar2 = FUN_00675c42(0x230);
                            local_8 = 0xc;
                            if (iVar2 != 0) {
                              uVar3 = FUN_00435990(param_2);
                              ExceptionList = local_10;
                              return uVar3;
                            }
                          }
                          else {
                            iVar2 = __wcsicmp(param_1,L"TooltipBox");
                            if (iVar2 == 0) {
                              iVar2 = FUN_00675c42(0x230);
                              local_8 = 0xd;
                              if (iVar2 != 0) {
                                uVar3 = FUN_00435c60(param_2);
                                ExceptionList = local_10;
                                return uVar3;
                              }
                            }
                            else {
                              iVar2 = __wcsicmp(param_1,L"ImageFont");
                              if (iVar2 == 0) {
                                iVar2 = FUN_00675c42(0x670);
                                local_8 = 0xe;
                                if (iVar2 != 0) {
                                  uVar3 = FUN_0044b970(param_2);
                                  ExceptionList = local_10;
                                  return uVar3;
                                }
                              }
                              else {
                                iVar2 = __wcsicmp(param_1,L"TreeView");
                                if (iVar2 == 0) {
                                  iVar2 = FUN_00675c42(0x460);
                                  local_8 = 0xf;
                                  if (iVar2 != 0) {
                                    uVar3 = FUN_0048bbe0(param_2);
                                    ExceptionList = local_10;
                                    return uVar3;
                                  }
                                }
                                else {
                                  iVar2 = __wcsicmp(param_1,L"ImeNotePad");
                                  if (iVar2 == 0) {
                                    iVar2 = FUN_00675c42(0x418);
                                    local_8 = 0x10;
                                    if (iVar2 != 0) {
                                      uVar3 = FUN_0044eb70(param_2,0xe);
                                      ExceptionList = local_10;
                                      return uVar3;
                                    }
                                  }
                                  else {
                                    iVar2 = __wcsicmp(param_1,L"ImeNotePadEditBox");
                                    if (iVar2 == 0) {
                                      iVar2 = FUN_00675c42(0x448);
                                      local_8 = 0x11;
                                      if (iVar2 != 0) {
                                        uVar3 = FUN_00453d10(param_2);
                                        ExceptionList = local_10;
                                        return uVar3;
                                      }
                                    }
                                    else {
                                      iVar2 = __wcsicmp(param_1,L"IconButton");
                                      if (iVar2 == 0) {
                                        iVar2 = FUN_00675c42(0x801);
                                        local_8 = 0x12;
                                        if (iVar2 != 0) {
                                          uVar3 = FUN_0042abe0(param_2);
                                          ExceptionList = local_10;
                                          return uVar3;
                                        }
                                      }
                                      else {
                                        iVar2 = __wcsicmp(param_1,L"MoMesh");
                                        if (iVar2 == 0) {
                                          iVar2 = FUN_00675c42(0x1d8);
                                          local_8 = 0x13;
                                          if (iVar2 != 0) {
                                            uVar3 = FUN_0046d550(param_2);
                                            ExceptionList = local_10;
                                            return uVar3;
                                          }
                                        }
                                        else {
                                          iVar2 = __wcsicmp(param_1,L"CustomListView");
                                          if (iVar2 != 0) {
                                            ExceptionList = local_10;
                                            return 0;
                                          }
                                          iVar2 = FUN_00675c42(0x418);
                                          local_8 = 0x14;
                                          if (iVar2 != 0) {
                                            uVar3 = FUN_0043dcc0(param_2,0x15);
                                            ExceptionList = local_10;
                                            return uVar3;
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
  return 0;
}

