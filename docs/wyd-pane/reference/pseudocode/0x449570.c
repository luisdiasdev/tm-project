// rea decompile_all
// name: FUN_00449570
// address: 0x449570
// body: available
// range: 0x449570-0x449a36 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00449570(wchar_t *param_1,wchar_t *param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  int in_ECX;
  double dVar5;
  undefined1 local_1a4 [56];
  undefined1 local_16c [56];
  int local_134;
  int local_130;
  int local_12c;
  int local_128;
  int local_124;
  int local_120;
  int local_11c;
  int local_118;
  undefined1 local_114 [128];
  undefined1 local_94 [128];
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084a873;
  local_10 = ExceptionList;
  uVar1 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_14 = uVar1;
  iVar2 = __wcsicmp(param_1,L"BarOutSpace");
  if (iVar2 == 0) {
    _swscanf_s(param_2,L"%d %d %d %d",in_ECX + 0x220,in_ECX + 0x224,in_ECX + 0x228,in_ECX + 0x22c);
  }
  else {
    iVar2 = __wcsicmp(param_1,L"noFrame");
    if (iVar2 == 0) {
      iVar2 = __wcsicmp(param_2,L"true");
      *(bool *)(in_ECX + 0x290) = iVar2 == 0;
    }
    else {
      iVar2 = __wcsicmp(param_1,L"text_type");
      if (iVar2 == 0) {
        iVar2 = __wcsicmp(param_2,L"rate_fraction");
        if (iVar2 == 0) {
          *(undefined4 *)(in_ECX + 0x250) = 1;
        }
        else {
          iVar2 = __wcsicmp(param_2,L"rate_percent");
          if (iVar2 == 0) {
            *(undefined4 *)(in_ECX + 0x250) = 2;
          }
        }
      }
      else {
        iVar2 = __wcsicmp(param_1,L"BarRange");
        if (iVar2 == 0) {
          _swscanf_s(param_2,L"%d %d",in_ECX + 0x210,in_ECX + 0x214,uVar1);
        }
        else {
          iVar2 = __wcsicmp(param_1,L"BarPos");
          if (iVar2 == 0) {
            uVar3 = FUN_0080d8dc(param_2);
            *(undefined4 *)(in_ECX + 0x218) = uVar3;
          }
          else {
            iVar2 = __wcsicmp(param_1,L"text_postfix");
            if (iVar2 == 0) {
              FUN_0040d040(param_2);
              local_8 = 0;
              FUN_0040c8a0(local_16c,0,0xffffffff);
              local_8 = 0xffffffff;
              FUN_0040c740();
            }
            else {
              iVar2 = __wcsicmp(param_1,L"BarDirection");
              if (iVar2 == 0) {
                iVar2 = __wcsicmp(param_2,L"right");
                if (iVar2 == 0) {
                  *(undefined4 *)(in_ECX + 0x254) = 0;
                }
                else {
                  iVar2 = __wcsicmp(param_2,L"left");
                  if (iVar2 == 0) {
                    *(undefined4 *)(in_ECX + 0x254) = 1;
                  }
                  else {
                    iVar2 = __wcsicmp(param_2,L"up");
                    if (iVar2 == 0) {
                      *(undefined4 *)(in_ECX + 0x254) = 2;
                    }
                    else {
                      iVar2 = __wcsicmp(param_2,L"down");
                      if (iVar2 == 0) {
                        *(undefined4 *)(in_ECX + 0x254) = 3;
                      }
                    }
                  }
                }
              }
              else {
                iVar2 = __wcsicmp(param_1,L"BarSpark");
                if (iVar2 == 0) {
                  FUN_004484a0();
                  _swscanf_s(param_2,L"%s %d %d %d %d",local_94,0x40,&local_134,&local_130,
                             &local_12c,&local_128);
                  *(int *)(in_ECX + 0x294) = local_134;
                  local_12c = local_12c + local_134;
                  local_128 = local_128 + local_130;
                  *(int *)(in_ECX + 0x298) = local_130;
                  *(int *)(in_ECX + 0x29c) = local_12c;
                  *(int *)(in_ECX + 0x2a0) = local_128;
                  FUN_0040d040(local_94);
                  local_8 = 1;
                  puVar4 = (undefined4 *)FUN_00466bb0(local_16c);
                  *(undefined4 *)(in_ECX + 0x2b4) = *puVar4;
                  *(undefined4 *)(in_ECX + 0x2b8) = puVar4[1];
                  *(undefined4 *)(in_ECX + 700) = puVar4[2];
                  *(undefined4 *)(in_ECX + 0x2c0) = puVar4[3];
                  *(undefined4 *)(in_ECX + 0x2c4) = puVar4[4];
                  local_8 = 0xffffffff;
                  FUN_0040c740();
                }
                else {
                  iVar2 = __wcsicmp(param_1,L"BarFlash");
                  if (iVar2 == 0) {
                    FUN_004484a0();
                    _swscanf_s(param_2,L"%s %d %d %d %d",local_114,0x40,&local_124,&local_120,
                               &local_11c,&local_118);
                    *(int *)(in_ECX + 0x2a4) = local_124;
                    local_11c = local_11c + local_124;
                    local_118 = local_118 + local_120;
                    *(int *)(in_ECX + 0x2a8) = local_120;
                    *(int *)(in_ECX + 0x2ac) = local_11c;
                    *(int *)(in_ECX + 0x2b0) = local_118;
                    FUN_0040d040(local_114);
                    local_8 = 2;
                    puVar4 = (undefined4 *)FUN_00466bb0(local_1a4);
                    *(undefined4 *)(in_ECX + 0x2c8) = *puVar4;
                    *(undefined4 *)(in_ECX + 0x2cc) = puVar4[1];
                    *(undefined4 *)(in_ECX + 0x2d0) = puVar4[2];
                    *(undefined4 *)(in_ECX + 0x2d4) = puVar4[3];
                    *(undefined4 *)(in_ECX + 0x2d8) = puVar4[4];
                    local_8 = 0xffffffff;
                    FUN_0040c740();
                  }
                  else {
                    iVar2 = __wcsicmp(param_1,L"BarFlashFadeoutRate");
                    if (iVar2 == 0) {
                      dVar5 = __wtof(param_2);
                      *(float *)(in_ECX + 0x2ec) = (float)dVar5;
                    }
                    else {
                      iVar2 = __wcsicmp(param_1,L"FillDrawMethod");
                      if (iVar2 == 0) {
                        uVar3 = FUN_0080d8dc();
                        *(undefined4 *)(in_ECX + 0x2e4) = uVar3;
                      }
                      else {
                        FUN_0048b1e0(param_1,param_2);
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

