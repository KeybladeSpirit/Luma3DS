OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
	RAM (rx) : ORIGIN = 0xAE000000, LENGTH = 0x120
	RAMRW (rw!i) : ORIGIN = 0x08000000, LENGTH = 0x00100000
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.text.*)
		*(.rodata)
		_got_start = .;
		*(.got)
		*(.got.plt)
		*(.data.rel.ro.local)
		_got_end = .;
	}

	.bss : {
		_bss_start = .;
		*(.bss);
	}
	_bss_end = .;
}

