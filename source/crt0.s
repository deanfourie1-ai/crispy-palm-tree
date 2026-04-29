@ ============================================================
@ crt0.s  –  GBA ROM header + ARM startup code
@ ============================================================
@
@ Memory layout used:
@   0x08000000  ROM  (this file is placed first by the linker)
@   0x03007F00  System/User stack (grows down)
@   0x03007FA0  IRQ stack
@   0x03007FE0  Supervisor stack
@ ============================================================

    .section .text.start
    .global  _start
    .arm

@ ------------------------------------------------------------------
@ 0x00 – Branch past the 192-byte GBA ROM header.
@ The assembler calculates the correct offset automatically.
@ ------------------------------------------------------------------
_start:
    b       rom_entry

@ ------------------------------------------------------------------
@ 0x04 – Nintendo logo (156 bytes).
@ Most emulators do not verify this block; we fill with zeroes.
@ Real hardware verifies it and will show the "game pak" screen
@ if it does not match, but the game still boots in most emulators.
@ ------------------------------------------------------------------
    .fill   156, 1, 0x00

@ ------------------------------------------------------------------
@ 0xA0 – Game title (12 bytes, uppercase ASCII, zero-padded)
@ ------------------------------------------------------------------
    .ascii  "WARCRAFTGBA\0"   @ 11 chars + 1 null = 12 bytes

@ ------------------------------------------------------------------
@ 0xAC – Game code (4 bytes)
@ ------------------------------------------------------------------
    .ascii  "AGWB"

@ ------------------------------------------------------------------
@ 0xB0 – Maker code (2 bytes)
@ ------------------------------------------------------------------
    .ascii  "00"

@ ------------------------------------------------------------------
@ 0xB2 – Fixed value required by GBA BIOS
@ ------------------------------------------------------------------
    .byte   0x96

@ ------------------------------------------------------------------
@ 0xB3 – Main unit code (0x00 = GBA)
@ 0xB4 – Device type
@ 0xB5..0xBB – Reserved
@ 0xBC – Software version
@ ------------------------------------------------------------------
    .byte   0x00, 0x00
    .fill   7, 1, 0x00
    .byte   0x00

@ ------------------------------------------------------------------
@ 0xBD – Complement check
@ Formula: -(0x19 + sum(header[0xA0..0xBC])) & 0xFF
@ With the fields above the result is 0xAC.
@ ------------------------------------------------------------------
    .byte   0xAC

@ ------------------------------------------------------------------
@ 0xBE – Reserved (2 bytes) – total header size = 0xC0 = 192 bytes
@ ------------------------------------------------------------------
    .fill   2, 1, 0x00

@ ==================================================================
@ rom_entry – actual startup code begins here at offset 0xC0
@ ==================================================================
rom_entry:
    @ Disable interrupts; put CPU in IRQ mode to set up IRQ stack
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x12       @ IRQ mode, IRQ/FIQ disabled
    orr     r0, r0, #0xC0
    msr     cpsr_c, r0
    ldr     sp, =0x03007FA0     @ IRQ stack top

    @ Supervisor mode stack
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0
    ldr     sp, =0x03007FE0     @ SVC stack top

    @ System/user mode stack (this is the C stack)
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x1F
    msr     cpsr_c, r0
    ldr     sp, =0x03007F00     @ System stack top

    @ -----------------------------------------------------------------
    @ Copy initialised .data section from ROM to EWRAM
    @ -----------------------------------------------------------------
    ldr     r0, =__data_load    @ source (in ROM)
    ldr     r1, =__data_start   @ destination (in EWRAM)
    ldr     r2, =__data_end
    cmp     r1, r2
    bge     .bss_zero
.data_copy:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    cmp     r1, r2
    blt     .data_copy

    @ -----------------------------------------------------------------
    @ Zero .bss section
    @ -----------------------------------------------------------------
.bss_zero:
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
    cmp     r0, r1
    bge     .call_main
.bss_loop:
    str     r2, [r0], #4
    cmp     r0, r1
    blt     .bss_loop

    @ -----------------------------------------------------------------
    @ Call main()
    @ -----------------------------------------------------------------
.call_main:
    bl      main

    @ If main() returns, spin forever
.hang:
    b       .hang
