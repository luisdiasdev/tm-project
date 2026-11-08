// rea decompile_all
// name: FUN_00441e20
// address: 0x441e20
// body: available
// range: 0x441e20-0x442961 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00441e20(void)

{
  short sVar1;
  int **ppiVar2;
  undefined4 uVar3;
  int iVar4;
  float fVar5;
  int in_ECX;
  short *psVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  int *piVar11;
  int iVar12;
  uint uVar13;
  undefined2 in_FPUControlWord;
  float10 fVar14;
  int local_c0 [2];
  undefined4 local_b8;
  int local_b0;
  int local_ac;
  undefined8 local_a8;
  int *local_a0;
  int local_9c;
  int *local_98;
  undefined4 local_94;
  uint local_90;
  undefined1 local_8c [56];
  tagRECT local_54;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  tagRECT local_34;
  tagRECT local_24;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084a1a7;
  local_10 = ExceptionList;
  local_14 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  if (*(char *)(in_ECX + 8) != '\0') {
    fVar14 = (float10)FUN_00442db0(local_14);
    local_94 = (float)fVar14;
    iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
    piVar11 = (int *)(in_ECX + 0x180);
    local_a8 = CONCAT44(iVar7,(uint)local_a8);
    local_a0 = piVar11;
    FUN_0048faf0(1);
    local_8 = 0;
    if (*(char *)(in_ECX + 0x3d9) != '\0') {
      local_a0 = (int *)lstrlenW((LPCWSTR)*piVar11);
      local_98 = *(int **)(in_ECX + 0x26c);
      if (local_98 == (int *)0x0) {
        psVar6 = (short *)PTR_DAT_009546ac;
        do {
          sVar1 = *psVar6;
          psVar6 = psVar6 + 1;
        } while (sVar1 != 0);
        local_98 = (int *)((int)psVar6 - (int)(PTR_DAT_009546ac + 2) >> 1);
      }
      ppiVar2 = &local_98;
      if ((int)local_a0 <= (int)local_98) {
        ppiVar2 = &local_a0;
      }
      FUN_0048fe60(PTR_DAT_009546ac,*ppiVar2);
      local_b8 = FUN_0046f1b0(*(undefined4 *)(iVar7 + 0x14));
      uVar3 = FUN_0046f1b0(*(undefined4 *)(iVar7 + 0x14));
      piVar11 = local_c0;
      *(undefined4 *)(in_ECX + 0x188) = uVar3;
      local_a0 = piVar11;
    }
    FUN_00465740((*(char *)(in_ECX + 0xcc) != '\0') - 1U & 3);
    FUN_00465740((*(char *)(in_ECX + 0xcc) != '\0') - 1U & 3);
    local_ac = 0;
    local_9c = 0;
    if (iVar7 != 0) {
      iVar4 = FUN_0046f1b0(*(undefined4 *)(iVar7 + 0x14));
      piVar11[2] = iVar4;
      FUN_004404d0(*(undefined4 *)(in_ECX + 0x248),piVar11);
    }
    if (*(char *)(in_ECX + 0x3d8) != '\0') {
      if (*(char *)(in_ECX + 0x275) != '\0') {
        FUN_00477820(**(int **)(in_ECX + 0x30),*(undefined4 *)(**(int **)(in_ECX + 0x30) + 100),
                     in_ECX + 0x1a8,0);
      }
      iVar4 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
      FUN_00477820(iVar4,*(undefined4 *)(iVar4 + 100),in_ECX + 0x198,0);
    }
    FUN_0048fbd0(*(undefined4 *)(in_ECX + 600),0,&local_b0,local_94);
    FUN_0048fbd0(*(undefined4 *)(in_ECX + 0x248),0,&local_9c,local_94);
    if (*(int *)(in_ECX + 0x248) == *(int *)(in_ECX + 0x254)) {
      local_ac = local_9c;
    }
    else {
      FUN_0048fbd0(*(int *)(in_ECX + 0x254),0,&local_ac,local_94);
    }
    if ((*(byte *)(iVar7 + 0x18) & 1) != 0) {
      local_98 = (int *)0x0;
      local_90 = 0;
      FUN_0048fbd0(*(undefined4 *)(in_ECX + 600),0,&local_90,local_94);
      FUN_0048fbd0(0xffffffff,0,&local_98,local_94);
      iVar7 = (*(int *)(in_ECX + 0x1a0) - *(int *)(in_ECX + 0x198)) / 2 -
              (int)((int)local_98 - local_90) / 2;
      if (iVar7 < 0) {
        iVar7 = 0;
      }
      local_9c = local_9c + iVar7;
      local_ac = local_ac + iVar7;
    }
    if (*(char *)(in_ECX + 0x24f) != '\0') {
      FUN_0048fbd0(*(undefined4 *)(in_ECX + 0x248),0,&local_9c,local_94);
    }
    if (*(char *)(in_ECX + 0x276) != '\0') {
      FUN_0048fbd0(0,1,&local_98,local_94);
      FUN_0048fbd0(*(undefined4 *)(in_ECX + 0x278),0,&local_90,local_94);
      SetRect(&local_54,0,*(int *)(in_ECX + 0x19c),local_90,*(int *)(in_ECX + 0x1a4));
      local_54.left = *(int *)(in_ECX + 0x194) + *(int *)(in_ECX + 0x14) + *(int *)(in_ECX + 400);
      local_54.right = local_54.left + local_90;
      *(int *)(in_ECX + 0x198) = *(int *)(in_ECX + 400) + local_54.right;
      piVar11 = local_a0;
    }
    if ((*(char *)(in_ECX + 10) != '\0') && (*(int *)(in_ECX + 0x248) != *(int *)(in_ECX + 0x254)))
    {
      iVar7 = local_9c;
      iVar4 = local_ac;
      if (local_ac < local_9c) {
        iVar7 = local_ac;
        iVar4 = local_9c;
      }
      SetRect(&local_34,iVar7,*(int *)(in_ECX + 0x19c),iVar4,*(int *)(in_ECX + 0x1a4));
      OffsetRect(&local_34,*(int *)(in_ECX + 0x198) - local_b0,0);
      IntersectRect(&local_34,(RECT *)(in_ECX + 0x198),&local_34);
      local_34.bottom = local_34.bottom + 1;
      FUN_0046f550(&local_34,*(undefined4 *)(in_ECX + 0x264));
      piVar11 = local_a0;
    }
    local_3c = *(undefined4 *)(in_ECX + 0x1a0);
    local_44 = *(undefined4 *)(in_ECX + 0x198);
    local_40 = *(undefined4 *)(in_ECX + 0x19c);
    local_38 = *(undefined4 *)(in_ECX + 0x1a4);
    local_98 = (int *)(*piVar11 + *(int *)(in_ECX + 600) * 2);
    if (*(char *)(in_ECX + 0x24f) != '\0') {
      *(undefined4 *)(*(int *)(*(int *)(in_ECX + 0x30) + 4) + 0x18) = 0x10;
      SetRect(&local_24,0,0,0x32,200);
      FUN_00475dc0(local_98,*(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_24,0xffffffff);
      FUN_0046f550(&local_24,0x6400ff00);
    }
    uVar10 = *(uint *)(in_ECX + 0x25c);
    local_24.left = (LONG)((float)(uVar10 >> 0x10 & 0xff) * 0.003921569);
    local_24.top = (LONG)((float)(uVar10 >> 8 & 0xff) * 0.003921569);
    local_90 = uVar10 >> 0x18;
    iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
    local_24.right = (LONG)((float)(uVar10 & 0xff) * 0.003921569);
    *(LONG *)(iVar7 + 0x54) = local_24.left;
    *(LONG *)(iVar7 + 0x58) = local_24.top;
    *(LONG *)(iVar7 + 0x5c) = local_24.right;
    local_24.bottom = (LONG)((float)local_90 * 0.003921569);
    *(LONG *)(iVar7 + 0x60) = local_24.bottom;
    if (*(char *)(in_ECX + 0x276) != '\0') {
      FUN_00477cd0(*(undefined4 *)(in_ECX + 700),*(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),
                   &local_54,0xffffffff,*(undefined4 *)(in_ECX + 0x40),
                   *(undefined4 *)(in_ECX + 0x44),1);
    }
    if (((local_98 == (int *)0x0) || ((short)*local_98 == 0)) && (*(short *)(in_ECX + 0x2cc) != 0))
    {
      iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
      local_24.left = 0x3ef8f8fa;
      local_24.top = 0x3ef8f8fa;
      *(undefined4 *)(iVar7 + 0x54) = 0x3ef8f8fa;
      local_24.right = 0x3ef8f8fa;
      *(undefined4 *)(iVar7 + 0x58) = 0x3ef8f8fa;
      local_24.bottom = 0x3f800000;
      *(undefined4 *)(iVar7 + 0x5c) = 0x3ef8f8fa;
      *(undefined4 *)(iVar7 + 0x60) = 0x3f800000;
      FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),in_ECX + 0x2cc,
                   *(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_44,0xffffffff,
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
      uVar10 = *(uint *)(in_ECX + 0x25c);
      local_24.left = (LONG)((float)(uVar10 >> 0x10 & 0xff) * 0.003921569);
      local_24.top = (LONG)((float)(uVar10 >> 8 & 0xff) * 0.003921569);
      local_90 = uVar10 >> 0x18;
      iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
      local_24.right = (LONG)((float)(uVar10 & 0xff) * 0.003921569);
      *(LONG *)(iVar7 + 0x54) = local_24.left;
      *(LONG *)(iVar7 + 0x58) = local_24.top;
      *(LONG *)(iVar7 + 0x5c) = local_24.right;
      local_24.bottom = (LONG)((float)local_90 * 0.003921569);
      *(LONG *)(iVar7 + 0x60) = local_24.bottom;
    }
    else {
      FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),local_98,
                   *(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_44,0xffffffff,
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
    }
    if (*(char *)(in_ECX + 10) != '\0') {
      iVar7 = *(int *)(in_ECX + 0x248);
      iVar4 = *(int *)(in_ECX + 0x254);
      if ((iVar7 != iVar4) && (*(char *)(in_ECX + 0x24f) == '\0')) {
        iVar8 = iVar4;
        if (iVar7 <= iVar4) {
          iVar8 = iVar7;
        }
        iVar12 = *(int *)(in_ECX + 600);
        if (((iVar8 < *(int *)(in_ECX + 600)) || (iVar12 = iVar7, iVar8 = iVar4, iVar7 <= iVar4)) &&
           (iVar8 = iVar12, iVar7 < iVar4)) {
          iVar7 = iVar4;
        }
        iVar4 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
        iVar12 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
        local_24.left = *(LONG *)(iVar4 + 0x54);
        local_24.top = *(LONG *)(iVar4 + 0x58);
        local_24.right = *(LONG *)(iVar4 + 0x5c);
        local_24.bottom = *(LONG *)(iVar4 + 0x60);
        *(float *)(iVar12 + 0x54) = 1.0 - *(float *)(iVar12 + 0x54);
        iVar4 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
        *(float *)(iVar4 + 0x58) = 1.0 - *(float *)(iVar4 + 0x58);
        iVar4 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
        *(float *)(iVar4 + 0x5c) = 1.0 - *(float *)(iVar4 + 0x5c);
        *(undefined1 *)(*(int *)(in_ECX + 0x28) + 0x48) = 1;
        FUN_0040d040(&DAT_0087de70);
        local_8._0_1_ = 1;
        FUN_0040c8a0(local_8c,0,0xffffffff);
        local_8._0_1_ = 0;
        FUN_0040c740();
        FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),*local_a0 + iVar8 * 2,
                     *(undefined4 *)(*(int *)(in_ECX + 0x30) + 4),&local_34,iVar7 - iVar8,0,
                     *(undefined4 *)(in_ECX + 0x44),0);
        *(undefined1 *)(*(int *)(in_ECX + 0x28) + 0x48) = 1;
        FUN_0040d040(&DAT_0087de70);
        local_8._0_1_ = 2;
        FUN_0040c8a0(local_8c,0,0xffffffff);
        local_8 = (uint)local_8._1_3_ << 8;
        FUN_0040c740();
        iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
        *(LONG *)(iVar7 + 0x54) = local_24.left;
        *(LONG *)(iVar7 + 0x58) = local_24.top;
        *(LONG *)(iVar7 + 0x5c) = local_24.right;
        *(LONG *)(iVar7 + 0x60) = local_24.bottom;
      }
    }
    FUN_004a2a20();
    fVar14 = (float10)FUN_004a0a40();
    if ((float10)*(double *)(in_ECX + 0x238) <= fVar14 - (float10)*(double *)(in_ECX + 0x240)) {
      *(bool *)(in_ECX + 0x24d) = *(char *)(in_ECX + 0x24d) == '\0';
      FUN_004a2a20();
      fVar14 = (float10)FUN_004a0a40();
      *(double *)(in_ECX + 0x240) = (double)fVar14;
    }
    if ((((*(char *)(in_ECX + 10) != '\0') && (*(char *)(in_ECX + 0x24d) != '\0')) &&
        (DAT_009546a8 == '\0')) && (*(char *)(in_ECX + 0x3da) == '\0')) {
      iVar7 = local_a8._4_4_;
      fVar5 = (float)((local_9c - local_b0) + *(int *)(in_ECX + 0x198));
      local_24.top = *(LONG *)(in_ECX + 0x19c);
      local_24.bottom = *(LONG *)(in_ECX + 0x1a4);
      local_24.left = (LONG)fVar5;
      local_24.right = (int)fVar5 + 2;
      if ((*(byte *)(local_a8._4_4_ + 0x18) & 1) != 0) {
        local_24.left = (int)fVar5 + -2;
        local_24.right = (LONG)fVar5;
      }
      if ((*(byte *)(*(int *)(*(int *)(in_ECX + 0x30) + 4) + 0x18) & 2) != 0) {
        local_24.left = *(int *)(in_ECX + 0x1a0) - local_9c;
        local_24.right = local_24.left + 2;
      }
      if (*(char *)(in_ECX + 0x24c) == '\0') {
        FUN_0048fbd0(*(undefined4 *)(in_ECX + 0x248),1,&local_90,local_94);
        if ((*(byte *)(iVar7 + 0x18) & 1) == 0) {
          local_24.right = (local_90 - local_b0) + *(int *)(in_ECX + 0x198);
        }
        else {
          local_98 = (int *)0x0;
          FUN_0048fbd0(0xffffffff,0,&local_98,local_94);
          local_98 = (int *)((*(int *)(in_ECX + 0x1a0) - *(int *)(in_ECX + 0x198)) / 2 -
                            (int)local_98 / 2);
          iVar7 = local_90 + *(int *)(in_ECX + 0x198);
          local_24.left = (int)local_98 + iVar7 + -2;
          local_24.right = (int)local_98 + iVar7 + 2;
        }
      }
      iVar7 = *(int *)(*(int *)(in_ECX + 0x30) + 4);
      fVar5 = *(float *)(iVar7 + 0x54);
      if (NAN(fVar5) || 1.0 < fVar5 == (fVar5 == 1.0)) {
        if (0.0 < *(float *)(iVar7 + 0x54)) {
          local_94 = (float)CONCAT22(in_FPUControlWord,(undefined2)local_94);
          local_a8 = (longlong)ROUND(*(float *)(iVar7 + 0x54) * 255.0 + 0.5);
          uVar10 = (uint)local_a8;
        }
        else {
          uVar10 = 0;
        }
      }
      else {
        uVar10 = 0xff;
      }
      fVar5 = *(float *)(iVar7 + 0x58);
      if (NAN(fVar5) || 1.0 < fVar5 == (fVar5 == 1.0)) {
        if (0.0 < *(float *)(iVar7 + 0x58)) {
          local_94 = (float)CONCAT22(in_FPUControlWord,(undefined2)local_94);
          local_a8 = (longlong)ROUND(*(float *)(iVar7 + 0x58) * 255.0 + 0.5);
          uVar13 = (uint)local_a8;
        }
        else {
          uVar13 = 0;
        }
      }
      else {
        uVar13 = 0xff;
      }
      if (1.0 < *(float *)(iVar7 + 0x5c) == (*(float *)(iVar7 + 0x5c) == 1.0)) {
        if (0.0 < *(float *)(iVar7 + 0x5c)) {
          local_94 = (float)CONCAT22(in_FPUControlWord,(undefined2)local_94);
          local_a8 = (longlong)ROUND(*(float *)(iVar7 + 0x5c) * 255.0 + 0.5);
          uVar9 = (uint)local_a8;
        }
        else {
          uVar9 = 0;
        }
      }
      else {
        uVar9 = 0xff;
      }
      if (1.0 < *(float *)(iVar7 + 0x60) == (*(float *)(iVar7 + 0x60) == 1.0)) {
        if (0.0 < *(float *)(iVar7 + 0x60)) {
          local_94 = (float)CONCAT22(in_FPUControlWord,(undefined2)local_94);
          local_a8 = (longlong)ROUND(*(float *)(iVar7 + 0x60) * 255.0 + 0.5);
          iVar7 = (uint)local_a8;
        }
        else {
          iVar7 = 0;
        }
      }
      else {
        iVar7 = 0xff;
      }
      FUN_0046f550(&local_24,((iVar7 << 8 | uVar10) << 8 | uVar13) << 8 | uVar9);
    }
    local_8 = 0xffffffff;
    FUN_0048fb20();
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}

