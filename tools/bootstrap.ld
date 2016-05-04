ENTRY(_start)
SECTIONS
{
    . = __start__;
    .text.start : { *(.text.start) }
    .text : { *(.text) }
	. = ALIGN(4);
    .data : { *(.data) }
	. = ALIGN(4);
	.rodata : { *(.rodata) }
	. = ALIGN(4);
    .bss : { *(.bss COMMON) }
    . = ALIGN(4);
	__end__ = ABSOLUTE(.);
}