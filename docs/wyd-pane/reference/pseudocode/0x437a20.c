// rea decompile_all
// name: FUN_00437a20
// address: 0x437a20
// body: available
// range: 0x437a20-0x437b14 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_00437a20(undefined4 param_1)

{
  undefined4 uVar1;
  int *piVar2;
  int in_ECX;
  undefined4 uVar3;
  
  if (*(char *)(in_ECX + 8) != '\0') {
    uVar3 = 0;
    if (*(char *)(in_ECX + 0xcc) == '\0') {
      uVar3 = 3;
    }
    else if (*(char *)(in_ECX + 0x211) == '\0') {
      if (*(char *)(in_ECX + 9) != '\0') {
        uVar3 = 2;
      }
    }
    else {
      uVar3 = 1;
    }
    uVar1 = **(undefined4 **)(in_ECX + 0x30);
    FUN_00465840(uVar3,param_1,0);
    FUN_00465840(uVar3,param_1,0);
    FUN_00477710(uVar1,in_ECX + 0x2a4,0);
    piVar2 = (int *)(in_ECX + 0x1b8);
    if (7 < *(uint *)(in_ECX + 0x1cc)) {
      piVar2 = (int *)*piVar2;
    }
    FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),piVar2,uVar1,in_ECX + 0x2b4,0xffffffff,
                 *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
    if (*(char *)(in_ECX + 0x2a0) != '\0') {
      uVar1 = *(undefined4 *)(*(int *)(in_ECX + 0x30) + 4);
      FUN_00465840(uVar3,param_1,0);
      FUN_00477710(uVar1,in_ECX + 0x2a4,0);
    }
  }
  return;
}

