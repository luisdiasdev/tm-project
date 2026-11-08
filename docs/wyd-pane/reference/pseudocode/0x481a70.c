// rea decompile_all
// name: FUN_00481a70
// address: 0x481a70
// body: available
// range: 0x481a70-0x481fa6 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


/* WARNING: Function: __alloca_probe replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x00481d96) */
/* WARNING: Removing unreachable block (ram,0x00481c89) */
/* WARNING: Removing unreachable block (ram,0x00481cbf) */
/* WARNING: Removing unreachable block (ram,0x00481c2f) */
/* WARNING: Removing unreachable block (ram,0x00481cd5) */
/* WARNING: Removing unreachable block (ram,0x00481ce4) */
/* WARNING: Removing unreachable block (ram,0x00481cfa) */
/* WARNING: Removing unreachable block (ram,0x00481d35) */
/* WARNING: Removing unreachable block (ram,0x00481d53) */
/* WARNING: Removing unreachable block (ram,0x00481d58) */
/* WARNING: Removing unreachable block (ram,0x00481da4) */
/* WARNING: Removing unreachable block (ram,0x00481dd0) */
/* WARNING: Removing unreachable block (ram,0x00481dd5) */
/* WARNING: Removing unreachable block (ram,0x00481e01) */
/* WARNING: Removing unreachable block (ram,0x00481e0e) */
/* WARNING: Removing unreachable block (ram,0x00481e06) */
/* WARNING: Removing unreachable block (ram,0x00481e10) */
/* WARNING: Removing unreachable block (ram,0x00481e3f) */
/* WARNING: Removing unreachable block (ram,0x00481e47) */
/* WARNING: Removing unreachable block (ram,0x00481e71) */
/* WARNING: Removing unreachable block (ram,0x00481e7e) */
/* WARNING: Removing unreachable block (ram,0x00481f0f) */
/* WARNING: Removing unreachable block (ram,0x00481f25) */
/* WARNING: Removing unreachable block (ram,0x00481f2a) */
/* WARNING: Removing unreachable block (ram,0x00481f56) */
/* WARNING: Removing unreachable block (ram,0x00481f73) */
/* WARNING: Removing unreachable block (ram,0x00481f90) */
/* WARNING: Removing unreachable block (ram,0x00481f80) */
/* WARNING: Removing unreachable block (ram,0x00481e90) */
/* WARNING: Removing unreachable block (ram,0x00481e95) */
/* WARNING: Removing unreachable block (ram,0x00481ec1) */
/* WARNING: Removing unreachable block (ram,0x00481edd) */
/* WARNING: Removing unreachable block (ram,0x00481ef9) */
/* WARNING: Removing unreachable block (ram,0x00481eea) */

void FUN_00481a70(undefined4 param_1,undefined4 param_2,int param_3)

{
  bool bVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  bool bVar6;
  uint local_3c;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0084cc21;
  local_10 = ExceptionList;
  uVar3 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  bVar1 = false;
  FUN_004082a0();
  local_8 = 0;
  if (5 < *(uint *)(param_3 + 0x10)) {
    iVar4 = FUN_00482f60();
    uVar5 = *(uint *)(iVar4 + 0x10);
    bVar1 = true;
    iVar4 = FUN_0040d1b0();
    bVar6 = false;
    if (iVar4 == 0) {
      if (uVar5 < 5) {
        uVar5 = 0xffffffff;
      }
      else {
        uVar5 = (uint)(uVar5 != 5);
      }
      bVar6 = uVar5 == 0;
    }
    if (bVar6) {
      bVar6 = true;
      goto LAB_00481b48;
    }
  }
  bVar6 = false;
LAB_00481b48:
  local_8 = 0;
  if ((bVar1) && (0xf < local_3c)) {
    FUN_00407f60();
  }
  if (bVar6) {
    local_8._0_1_ = 2;
    FUN_0040c7b0(param_3,0,0xffffffff);
    FUN_00407580();
    FUN_004082d0();
    local_8 = (uint)local_8._1_3_ << 8;
    FUN_004074e0();
  }
  else {
    FUN_004082c0();
  }
  while( true ) {
    local_8._0_1_ = 4;
    cVar2 = FUN_00419180();
    if (cVar2 == '\0') break;
    local_8._1_3_ = 0;
  }
  local_8 = 0xffffffff;
  FUN_004184b0();
  ExceptionList = local_10;
  __security_check_cookie(uVar3 ^ (uint)&stack0xfffffffc);
  return;
}

