/* GBA Cartridge Linker Script */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    ROM   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
    EWRAM (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
    IWRAM (rwx) : ORIGIN = 0x03000000, LENGTH = 32K
}

SECTIONS {
    /* ROM: startup code and header come first, then read-only code/data */
    .text : {
        *(.text.start)      /* crt0.s must be placed first */
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        __text_end = .;
    } >ROM

    /* Initialised data: stored in ROM, copied to EWRAM at startup */
    __data_load = .;
    .data : AT(__data_load) {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } >EWRAM

    /* Zero-initialised data: placed in EWRAM, zeroed at startup */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } >EWRAM

    /* Discard C++ exception tables etc. */
    /DISCARD/ : {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    }
}
