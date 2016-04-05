.section .text.start
.align 4
.arm

.global _start
_start:
	b	_kernel_start
	b	i2cRebootSystem
	b	i2cShutdownSystem
		
	// Kernel code
	_kernel_start:
		push	{r0-r12, lr}
		bl		main
		pop		{r0-r12, lr}
		// Get back the hook
		sub		r0, r0, #1
		mov     r1, r0, lsr#12
		ldr		pc, =0xDEADC0DE
.pool
