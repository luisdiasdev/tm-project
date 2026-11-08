// rea decompile_all
// name: FUN_00407580
// address: 0x407580
// body: available
// range: 0x407580-0x4076da inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00407580(undefined4 *param_1)

{
  char *pcVar1;
  char cVar2;
  uint uVar3;
  undefined4 *puVar4;
  uint uVar5;
  undefined4 uVar6;
  char *in_ECX;
  uint in_stack_00000018;
  void *local_10;
  undefined1 *puStack_c;
  int local_8;
  
  puStack_c = &LAB_00846979;
  local_10 = ExceptionList;
  uVar3 = DAT_009ae614 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_8 = 0;
  if (*in_ECX != '\0') {
    FUN_004074e0(uVar3);
  }
  FUN_00407230();
  local_8._0_1_ = 1;
  puVar4 = param_1;
  if (in_stack_00000018 < 0x10) {
    puVar4 = &param_1;
  }
  cVar2 = FUN_00407380(puVar4,0x10020);
  if (cVar2 == '\0') {
    local_8 = (uint)local_8._1_3_ << 8;
    FUN_00407310();
    local_8 = 0xffffffff;
    if (0xf < in_stack_00000018) {
      FUN_00407f60(param_1,in_stack_00000018 + 1);
    }
  }
  else {
    uVar5 = FUN_00409900();
    *(uint *)(in_ECX + 8) = uVar5;
    uVar6 = FUN_00675c7c(uVar5);
    *(undefined4 *)(in_ECX + 4) = uVar6;
    FUN_004073a0(uVar6,uVar5);
    while (uVar5 = uVar5 - 1, 0 < (int)uVar5) {
      pcVar1 = (char *)(*(int *)(in_ECX + 4) + uVar5);
      *pcVar1 = *pcVar1 + (((char)(uVar5 / 0xe) * '\x0e' -
                           *(char *)(*(int *)(in_ECX + 4) + -1 + uVar5)) - (char)uVar5);
    }
    **(char **)(in_ECX + 4) = **(char **)(in_ECX + 4) + -0xe;
    thunk_FUN_00409480();
    local_8 = (uint)local_8._1_3_ << 8;
    FUN_00407310();
    local_8 = 0xffffffff;
    if (0xf < in_stack_00000018) {
      FUN_00407f60(param_1,in_stack_00000018 + 1);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(uVar3 ^ (uint)&stack0xfffffffc);
  return;
}

