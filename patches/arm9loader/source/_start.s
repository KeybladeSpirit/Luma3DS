.section .text.start
.align 4
.arm

.global _start
_start:
	b main

.global entryPoint
entryPoint:
.word 0x801B01C