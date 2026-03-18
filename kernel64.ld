/*
 * HeatOS 64-bit bootstrap linker script
 * Long-mode bootstrap kernel is loaded at physical address 0x10000.
 */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}
