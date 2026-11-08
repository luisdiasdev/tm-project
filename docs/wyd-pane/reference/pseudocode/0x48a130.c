// rea decompile_all
// name: FUN_0048a130
// address: 0x48a130
// body: available
// range: 0x48a130-0x48a307 inclusive
// non_contiguous: false
// provider: Ghidra 12.1.3 (ghidra)
// analysis_profile: 187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c
// limitations: The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.; The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.; Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.; Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.; Windows Ghidra P0 accepts native x86, x86-64, ARMNT (Thumb-2) and ARM64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.; The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.; Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.; The Ghidra Program references PDB SD.pdb (GUID b551b31f-3da8-475e-9b2d-4a07cacc31ef, age (hex) 1, signature unknown, version RSDS); Ghidra reports load state unknown. Original names and types may be missing; an empty name search does not prove a procedure is absent. Use list_procedures and address-based analysis to inspect recovered functions. REA does not admit a caller-selected PDB companion.; This session uses Ghidra 12.1.3. The bridge is verified against 12.1.4; other 12.1.x builds are accepted and the observed version is reported.; Pseudocode is Ghidra decompiler output, not original source and not text-equivalent to Hopper output.; External functions and functions without an analyzable body return null; other decompiler failures remain explicit.
// pseudocode: provider-specific; not original source


void FUN_0048a130(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  int in_ECX;
  int iVar6;
  int iVar7;
  int iVar8;
  
  if (*(char *)(in_ECX + 8) != '\0') {
    iVar1 = **(int **)(in_ECX + 0x30);
    if (*(char *)(in_ECX + 0x20c) == '\0') {
      if ((*(int *)(in_ECX + 0x1fc) == 0) && (*(int *)(in_ECX + 0x200) == 0)) {
        piVar5 = (int *)(in_ECX + 0x1b8);
        if (7 < *(uint *)(in_ECX + 0x1cc)) {
          piVar5 = (int *)*piVar5;
        }
        FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),piVar5,iVar1,in_ECX + 0xbc,0xffffffff,
                     *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
        return;
      }
      piVar5 = (int *)(in_ECX + 0x1b8);
      if (7 < *(uint *)(in_ECX + 0x1cc)) {
        piVar5 = (int *)*piVar5;
      }
      FUN_0047b7d0(*(undefined4 *)(in_ECX + 0x28),piVar5,iVar1,in_ECX + 0xbc,0xffffffff,
                   *(undefined4 *)(in_ECX + 0x40),*(undefined4 *)(in_ECX + 0x44),0);
      return;
    }
    if ((*(int *)(in_ECX + 500) == 0) && (*(int *)(in_ECX + 0x1c8) != 0)) {
      FUN_00489de0();
    }
    iVar4 = *(int *)(in_ECX + 500);
    if (((iVar4 != 0) && (*(uint *)(iVar4 + 0x360) != 0)) &&
       (*(uint *)(iVar4 + 0x360) < DAT_009b1420)) {
      *(undefined4 *)(iVar4 + 0x360) = 0;
      *(undefined1 *)(*(int *)(in_ECX + 0x28) + 0x48) = 1;
      FUN_00489de0();
    }
    iVar4 = *(int *)(in_ECX + 500);
    if (iVar4 != 0) {
      iVar6 = *(int *)(in_ECX + 0xbc) + *(int *)(in_ECX + 0x1fc);
      iVar2 = *(int *)(in_ECX + 0xc4);
      iVar3 = *(int *)(in_ECX + 0x204);
      iVar7 = *(int *)(in_ECX + 0xc0) + *(int *)(in_ECX + 0x200);
      iVar8 = *(int *)(in_ECX + 200) - *(int *)(in_ECX + 0x208);
      *(int *)(iVar4 + 800) = iVar6;
      *(int *)(iVar4 + 0x324) = iVar7;
      *(int *)(iVar4 + 0x328) = iVar2 - iVar3;
      *(int *)(iVar4 + 0x32c) = iVar8;
      iVar4 = 0;
      if ((*(byte *)(iVar1 + 0x18) & 4) != 0) {
        iVar4 = FUN_00498630(*(undefined4 *)(in_ECX + 0x1f8));
        iVar4 = ((iVar8 - iVar7) - iVar4) / 2;
      }
      if ((*(byte *)(iVar1 + 0x18) & 8) != 0) {
        iVar4 = FUN_00498630(*(undefined4 *)(in_ECX + 0x1f8));
        iVar4 = (iVar8 - iVar7) - iVar4;
      }
      if (*(int *)(in_ECX + 0x28) != 0) {
        FUN_00498860(*(int *)(in_ECX + 0x28),iVar1,iVar6,iVar7 + iVar4,
                     *(undefined4 *)(in_ECX + 0x1f8),*(undefined4 *)(in_ECX + 0x40),
                     *(undefined4 *)(in_ECX + 0x44));
      }
    }
  }
  return;
}

