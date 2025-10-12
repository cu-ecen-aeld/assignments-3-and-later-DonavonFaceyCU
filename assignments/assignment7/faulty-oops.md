# Faulty Driver Trace Analysis

## Stack Trace
```
# echo "hello_world" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b6d000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: faulty(O) hello(O) scull(O)
CPU: 0 PID: 163 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc0/0x3a0
sp : ffffffc008dfbd10
x29: ffffffc008dfbd80 x28: ffffff8001aa4240 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040000000 x22: 000000000000000c x21: ffffffc008dfbdc0
x20: 000000558445f5e0 x19: ffffff8001c0bf00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc00078c000 x3 : ffffffc008dfbdc0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x120
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0x114/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
```

## Objdump Output
### Checking headers
```
objdump -x faulty.ko

faulty.ko:     file format elf64-little
faulty.ko
architecture: UNKNOWN!, flags 0x00000011:
HAS_RELOC, HAS_SYMS
start address 0x0000000000000000

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 .text         0000018c  0000000000000000  0000000000000000  00000040  2**4
                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
  1 .plt          00000001  0000000000000000  0000000000000000  000001cc  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .init.plt     00000001  0000000000000000  0000000000000000  000001cd  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .text.ftrace_trampoline 00000001  0000000000000000  0000000000000000  000001ce  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  4 .rodata.str1.8 00000007  0000000000000000  0000000000000000  000001d0  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .modinfo      00000051  0000000000000000  0000000000000000  000001d7  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  6 .note.gnu.property 00000020  0000000000000000  0000000000000000  00000228  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  7 .note.gnu.build-id 00000024  0000000000000000  0000000000000000  00000248  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  8 .note.Linux   00000030  0000000000000000  0000000000000000  0000026c  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  9 .data         00000110  0000000000000000  0000000000000000  000002a0  2**3
                  CONTENTS, ALLOC, LOAD, RELOC, DATA
 10 .exit.data    00000008  0000000000000000  0000000000000000  000003b0  2**3
                  CONTENTS, ALLOC, LOAD, RELOC, DATA
 11 .init.data    00000008  0000000000000000  0000000000000000  000003b8  2**3
                  CONTENTS, ALLOC, LOAD, RELOC, DATA
 12 .gnu.linkonce.this_module 000002c0  0000000000000000  0000000000000000  000003c0  2**6
                  CONTENTS, ALLOC, LOAD, RELOC, DATA, LINK_ONCE_DISCARD
 13 .bss          00000004  0000000000000000  0000000000000000  00000680  2**2
                  ALLOC
 14 .note.GNU-stack 00000000  0000000000000000  0000000000000000  00000680  2**0
                  CONTENTS, READONLY
 15 .comment      0000006e  0000000000000000  0000000000000000  00000680  2**0
                  CONTENTS, READONLY
SYMBOL TABLE:
0000000000000000 l    d  .text	0000000000000000 .text
0000000000000000 l    d  .plt	0000000000000000 .plt
0000000000000000 l    d  .init.plt	0000000000000000 .init.plt
0000000000000000 l    d  .text.ftrace_trampoline	0000000000000000 .text.ftrace_trampoline
0000000000000000 l    d  .rodata.str1.8	0000000000000000 .rodata.str1.8
0000000000000000 l    d  .modinfo	0000000000000000 .modinfo
0000000000000000 l    d  .note.gnu.property	0000000000000000 .note.gnu.property
0000000000000000 l    d  .note.gnu.build-id	0000000000000000 .note.gnu.build-id
0000000000000000 l    d  .note.Linux	0000000000000000 .note.Linux
0000000000000000 l    d  .data	0000000000000000 .data
0000000000000000 l    d  .exit.data	0000000000000000 .exit.data
0000000000000000 l    d  .init.data	0000000000000000 .init.data
0000000000000000 l    d  .gnu.linkonce.this_module	0000000000000000 .gnu.linkonce.this_module
0000000000000000 l    d  .bss	0000000000000000 .bss
0000000000000000 l    d  .note.GNU-stack	0000000000000000 .note.GNU-stack
0000000000000000 l    d  .comment	0000000000000000 .comment
0000000000000000 l    df *ABS*	0000000000000000 faulty.c
0000000000000000 l       .text	0000000000000000 $x
0000000000000000 l       .rodata.str1.8	0000000000000000 $d
0000000000000000 l       .data	0000000000000000 $d
0000000000000000 l       .bss	0000000000000000 $d
0000000000000000 l       .exit.data	0000000000000000 $d
0000000000000000 l     O .exit.data	0000000000000008 __UNIQUE_ID___addressable_cleanup_module223
0000000000000000 l       .init.data	0000000000000000 $d
0000000000000000 l     O .init.data	0000000000000008 __UNIQUE_ID___addressable_init_module222
0000000000000000 l     O .modinfo	0000000000000015 __UNIQUE_ID_license221
0000000000000000 l       .note.gnu.property	0000000000000000 $d
0000000000000000 l    df *ABS*	0000000000000000 faulty.mod.c
0000000000000000 l       .gnu.linkonce.this_module	0000000000000000 $d
0000000000000015 l     O .modinfo	0000000000000009 __UNIQUE_ID_depends223
000000000000001e l     O .modinfo	000000000000000c __UNIQUE_ID_name222
000000000000002a l     O .modinfo	0000000000000027 __UNIQUE_ID_vermagic221
0000000000000000 l       .note.Linux	0000000000000000 $d
0000000000000000 l     O .note.Linux	0000000000000018 _note_10
0000000000000018 l     O .note.Linux	0000000000000018 _note_9
0000000000000000 g     O .gnu.linkonce.this_module	00000000000002c0 __this_module
0000000000000000 g     O .bss	0000000000000004 faulty_major
0000000000000020 g     F .text	0000000000000058 faulty_init
0000000000000000 g     O .data	0000000000000110 faulty_fops
0000000000000080 g     F .text	0000000000000034 cleanup_module
0000000000000020 g     F .text	0000000000000058 init_module
0000000000000000         *UND*	0000000000000000 __stack_chk_fail
0000000000000000         *UND*	0000000000000000 __arch_copy_to_user
0000000000000080 g     F .text	0000000000000034 faulty_cleanup
0000000000000000         *UND*	0000000000000000 memset
00000000000000c0 g     F .text	00000000000000cc faulty_read
0000000000000000         *UND*	0000000000000000 __register_chrdev
0000000000000000 g     F .text	0000000000000018 faulty_write
0000000000000000         *UND*	0000000000000000 __unregister_chrdev


RELOCATION RECORDS FOR [.text]:
OFFSET           TYPE              VALUE 
0000000000000028 UNKNOWN           .data
0000000000000034 UNKNOWN           .bss
0000000000000038 UNKNOWN           .bss
000000000000003c UNKNOWN           .rodata.str1.8
0000000000000040 UNKNOWN           .data
0000000000000044 UNKNOWN           .rodata.str1.8
0000000000000050 UNKNOWN           __register_chrdev
0000000000000058 UNKNOWN           .bss
0000000000000060 UNKNOWN           .bss
0000000000000084 UNKNOWN           .bss
0000000000000098 UNKNOWN           .bss
000000000000009c UNKNOWN           .rodata.str1.8
00000000000000a0 UNKNOWN           .rodata.str1.8
00000000000000a4 UNKNOWN           __unregister_chrdev
00000000000000f8 UNKNOWN           memset
000000000000013c UNKNOWN           __arch_copy_to_user
0000000000000188 UNKNOWN           __stack_chk_fail


RELOCATION RECORDS FOR [.data]:
OFFSET           TYPE              VALUE 
0000000000000000 UNKNOWN           __this_module
0000000000000010 UNKNOWN           faulty_read
0000000000000018 UNKNOWN           faulty_write


RELOCATION RECORDS FOR [.exit.data]:
OFFSET           TYPE              VALUE 
0000000000000000 UNKNOWN           cleanup_module


RELOCATION RECORDS FOR [.init.data]:
OFFSET           TYPE              VALUE 
0000000000000000 UNKNOWN           init_module


RELOCATION RECORDS FOR [.gnu.linkonce.this_module]:
OFFSET           TYPE              VALUE 
0000000000000138 UNKNOWN           init_module
00000000000002b0 UNKNOWN           cleanup_module
```

### Checking full contents
```
objdump -s faulty.ko

faulty.ko:     file format elf64-little

Contents of section .text:
 0000 010080d2 000080d2 3f2303d5 bf2303d5  ........?#...#..
 0010 3f0000b9 c0035fd6 1f2003d5 1f2003d5  ?....._.. ... ..
 0020 3f2303d5 fd7bbea9 04000090 fd030091  ?#...{..........
 0030 f30b00f9 13000090 600240b9 03000090  ........`.@.....
 0040 84000091 63000091 02208052 01008052  ....c.... .R...R
 0050 00000094 a000f837 610240b9 41000035  .......7a.@.A..5
 0060 600200b9 00008052 f30b40f9 fd7bc2a8  `......R..@..{..
 0070 bf2303d5 c0035fd6 1f2003d5 1f2003d5  .#...._.. ... ..
 0080 3f2303d5 00000090 fd7bbfa9 02208052  ?#.......{... .R
 0090 01008052 fd030091 000040b9 03000090  ...R......@.....
 00a0 63000091 00000094 fd7bc1a8 bf2303d5  c........{...#..
 00b0 c0035fd6 1f2003d5 1f2003d5 1f2003d5  .._.. ... ... ..
 00c0 3f2303d5 ffc300d1 004138d5 fd7b01a9  ?#.......A8..{..
 00d0 fd430091 f35302a9 f40301aa f30302aa  .C...S..........
 00e0 01f841f9 e10700f9 010080d2 820280d2  ..A.............
 00f0 e11f8052 e0130091 00000094 014138d5  ...R.........A8.
 0100 222c40b9 7f1200f1 830080d2 7392839a  ",@.........s...
 0110 4203a836 80de4093 8002008a 0110c0d2  B..6..@.........
 0120 210013cb 3f0000eb e00313aa a3000054  !...?..........T
 0130 80fa4892 e1130091 e20313aa 00000094  ..H.............
 0140 1f000071 014138d5 007c4093 0010939a  ...q.A8..|@.....
 0150 e30740f9 22f841f9 630002eb 020080d2  ..@.".A.c.......
 0160 41010054 fd7b41a9 f35342a9 ffc30091  A..T.{A..SB.....
 0170 bf2303d5 c0035fd6 210040f9 e00314aa  .#...._.!.@.....
 0180 e1fcd736 e4ffff17 00000094           ...6........    
Contents of section .plt:
 0000 00                                   .               
Contents of section .init.plt:
 0000 00                                   .               
Contents of section .text.ftrace_trampoline:
 0000 00                                   .               
Contents of section .rodata.str1.8:
 0000 6661756c 747900                      faulty.         
Contents of section .modinfo:
 0000 6c696365 6e73653d 4475616c 20425344  license=Dual BSD
 0010 2f47504c 00646570 656e6473 3d006e61  /GPL.depends=.na
 0020 6d653d66 61756c74 79007665 726d6167  me=faulty.vermag
 0030 69633d36 2e312e34 3420534d 50206d6f  ic=6.1.44 SMP mo
 0040 645f756e 6c6f6164 20616172 63683634  d_unload aarch64
 0050 00                                   .               
Contents of section .note.gnu.property:
 0000 04000000 10000000 05000000 474e5500  ............GNU.
 0010 000000c0 04000000 02000000 00000000  ................
Contents of section .note.gnu.build-id:
 0000 04000000 14000000 03000000 474e5500  ............GNU.
 0010 cf94a7ed b26588eb a7222bae b8f3f05a  .....e..."+....Z
 0020 9b07b771                             ...q            
Contents of section .note.Linux:
 0000 06000000 04000000 01010000 4c696e75  ............Linu
 0010 78000000 00000000 06000000 01000000  x...............
 0020 00010000 4c696e75 78000000 00000000  ....Linux.......
Contents of section .data:
 0000 00000000 00000000 00000000 00000000  ................
 0010 00000000 00000000 00000000 00000000  ................
 0020 00000000 00000000 00000000 00000000  ................
 0030 00000000 00000000 00000000 00000000  ................
 0040 00000000 00000000 00000000 00000000  ................
 0050 00000000 00000000 00000000 00000000  ................
 0060 00000000 00000000 00000000 00000000  ................
 0070 00000000 00000000 00000000 00000000  ................
 0080 00000000 00000000 00000000 00000000  ................
 0090 00000000 00000000 00000000 00000000  ................
 00a0 00000000 00000000 00000000 00000000  ................
 00b0 00000000 00000000 00000000 00000000  ................
 00c0 00000000 00000000 00000000 00000000  ................
 00d0 00000000 00000000 00000000 00000000  ................
 00e0 00000000 00000000 00000000 00000000  ................
 00f0 00000000 00000000 00000000 00000000  ................
 0100 00000000 00000000 00000000 00000000  ................
Contents of section .exit.data:
 0000 00000000 00000000                    ........        
Contents of section .init.data:
 0000 00000000 00000000                    ........        
Contents of section .gnu.linkonce.this_module:
 0000 00000000 00000000 00000000 00000000  ................
 0010 00000000 00000000 6661756c 74790000  ........faulty..
 0020 00000000 00000000 00000000 00000000  ................
 0030 00000000 00000000 00000000 00000000  ................
 0040 00000000 00000000 00000000 00000000  ................
 0050 00000000 00000000 00000000 00000000  ................
 0060 00000000 00000000 00000000 00000000  ................
 0070 00000000 00000000 00000000 00000000  ................
 0080 00000000 00000000 00000000 00000000  ................
 0090 00000000 00000000 00000000 00000000  ................
 00a0 00000000 00000000 00000000 00000000  ................
 00b0 00000000 00000000 00000000 00000000  ................
 00c0 00000000 00000000 00000000 00000000  ................
 00d0 00000000 00000000 00000000 00000000  ................
 00e0 00000000 00000000 00000000 00000000  ................
 00f0 00000000 00000000 00000000 00000000  ................
 0100 00000000 00000000 00000000 00000000  ................
 0110 00000000 00000000 00000000 00000000  ................
 0120 00000000 00000000 00000000 00000000  ................
 0130 00000000 00000000 00000000 00000000  ................
 0140 00000000 00000000 00000000 00000000  ................
 0150 00000000 00000000 00000000 00000000  ................
 0160 00000000 00000000 00000000 00000000  ................
 0170 00000000 00000000 00000000 00000000  ................
 0180 00000000 00000000 00000000 00000000  ................
 0190 00000000 00000000 00000000 00000000  ................
 01a0 00000000 00000000 00000000 00000000  ................
 01b0 00000000 00000000 00000000 00000000  ................
 01c0 00000000 00000000 00000000 00000000  ................
 01d0 00000000 00000000 00000000 00000000  ................
 01e0 00000000 00000000 00000000 00000000  ................
 01f0 00000000 00000000 00000000 00000000  ................
 0200 00000000 00000000 00000000 00000000  ................
 0210 00000000 00000000 00000000 00000000  ................
 0220 00000000 00000000 00000000 00000000  ................
 0230 00000000 00000000 00000000 00000000  ................
 0240 00000000 00000000 00000000 00000000  ................
 0250 00000000 00000000 00000000 00000000  ................
 0260 00000000 00000000 00000000 00000000  ................
 0270 00000000 00000000 00000000 00000000  ................
 0280 00000000 00000000 00000000 00000000  ................
 0290 00000000 00000000 00000000 00000000  ................
 02a0 00000000 00000000 00000000 00000000  ................
 02b0 00000000 00000000 00000000 00000000  ................
Contents of section .comment:
 0000 00474343 3a202842 75696c64 726f6f74  .GCC: (Buildroot
 0010 20323032 352e3038 2d333636 2d673734   2025.08-366-g74
 0020 64636461 32353434 2d646972 74792920  dcda2544-dirty) 
 0030 31342e33 2e300000 4743433a 20284275  14.3.0..GCC: (Bu
 0040 696c6472 6f6f7420 32303235 2e30382d  ildroot 2025.08-
 0050 3336362d 67373464 63646132 3534342d  366-g74dcda2544-
 0060 64697274 79292031 342e332e 3000      dirty) 14.3.0.  
```

## Analysis
The above trace shows how the kernel crashed. Most important is the first line "Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000".
This line shows that the code tried to access a null pointer, specifically within the faulty_write function (shown in the call trace and the program counter), at an offset of 0x10.
The instruction that caused the fault (b900003f) translates to str w31, [x1] (after ChatGPT decodes it for me). This means it's trying to store w31 (which is zero typically) at address in x1
which the trace tells us is 0. Therefore it faults on the nullptr write.

The objdump output shows the offending instruction at 0x001C, where faulty_write has an offset of 0x0018, tracing back to where the faulting instruction occurred. 

The rest of the info provided gives more context to the error, but this is a pretty clear case.
