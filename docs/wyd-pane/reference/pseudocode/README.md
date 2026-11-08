# Ghidra pseudocode dumps

Files are named by the function entry address. The header inside each file
records the function name, address range, provider and analysis profile.

| File | Function | Role |
|---|---|---|
| `0x407580.c` | `FUN_00407580` | `.pane` / `.wyp` decryption |
| `0x482c10.c` | `FUN_00482c10` | panel file loader |
| `0x4825b0.c` | `FUN_004825b0` | top-level parser |
| `0x481fd0.c` | `FUN_00481fd0` | `Panel` block parser |
| `0x4817f0.c` | `FUN_004817f0` | `element` block parser |
| `0x481a70.c` | `FUN_00481a70` | load panel from path (`itemPanel`) |
| `0x47df30.c` | `FUN_0047df30` | panel property setter |
| `0x444590.c` | `FUN_00444590` | common element property setter |
| `0x447900.c` | `FUN_00447900` | element type factory |
| `0x47c640.c` | `FUN_0047c640` | create/named/attach element |
| `0x442f80.c` | `FUN_00442f80` | update absolute rect |
| `0x43f610.c` | `FUN_0043f610` | hit test |
| `0x48b1e0.c` | `FUN_0048b1e0` | Static properties |
| `0x44ac10.c` | `FUN_0044ac10` | Image properties |
| `0x436fb0.c` | `FUN_00436fb0` | Button properties |
| `0x449570.c` | `FUN_00449570` | GaugeBar properties |
| `0x437b90.c` | `FUN_00437b90` | ComboBox properties |
| `0x489a80.c` | `FUN_00489a80` | Slider properties |
| `0x441000.c` | `FUN_00441000` | EditBox properties |
| `0x455040.c` | `FUN_00455040` | ListBox properties |
| `0x45a2c0.c` | `FUN_0045a2c0` | ListView properties |
| `0x4506c0.c` | `FUN_004506c0` | ImeNotePad properties |
| `0x44c240.c` | `FUN_0044c240` | ImageFont properties |
| `0x42e310.c` | `FUN_0042e310` | IconButton properties |
| `0x46d4b0.c` | `FUN_0046d4b0` | MoMesh properties |
| `0x43da60.c` | `FUN_0043da60` | CustomListView properties |
| `0x4aea40.c` | `FUN_004aea40` | texture loader |
| `0x4ae7d0.c` | `FUN_004ae7d0` | texture extension dispatch |
| `0x48a130.c` | `FUN_0048a130` | Static draw |
| `0x449c10.c` | `FUN_00449c10` | Image draw |
| `0x436520.c` | `FUN_00436520` | Button draw |
| `0x437a20.c` | `FUN_00437a20` | CheckBox/RadioButton draw |
| `0x448900.c` | `FUN_00448900` | GaugeBar draw |
| `0x437ee0.c` | `FUN_00437ee0` | ComboBox draw |
| `0x4899b0.c` | `FUN_004899b0` | Slider draw |
| `0x441e20.c` | `FUN_00441e20` | EditBox draw |
| `0x4564e0.c` | `FUN_004564e0` | ListBox draw |
| `0x458840.c` | `FUN_00458840` | ListView draw |
| `0x44baf0.c` | `FUN_0044baf0` | ImageFont draw |
| `0x42c550.c` | `FUN_0042c550` | IconButton draw |
| `0x46d480.c` | `FUN_0046d480` | Mesh draw |
| `0x43ba20.c` | `FUN_0043ba20` | CustomListView draw |

Pseudocode is Ghidra output, not original source. It is useful for field offsets
and control flow, but names are ours except where RTTI supplied a class name.
