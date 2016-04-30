.section .text
.arm
.align 4

storeRegs:	@ Except PC
	sub	sp, sp, #16
	push { r0-r12 }
	mrs	r0, cpsr
	mov	r1, sp
	mrs	r2, spsr
	str	r2, [sp, #64]
	tst	r2, #0xF
	orreq	r2, r2, #0xF
	bic	r2, r2, #0x10
	msr	cpsr_c, r2
	str	lr, [r1, #52]
	str	sp, [r1, #56]
	msr	cpsr_c, r0
	
	mov	r0, sp
	bx lr

.global	arm11Access
.type arm11Access, %function
arm11Access:
	@ Needed in order to control ARM11
	push {r0-r1, lr}
	mrs r0, cpsr
	mov r1, r0
	and r1, r1, #0xFFFFFFF0
	orr r1, r1, #0x13
	msr cpsr_c, r1
	ldr r1, =0x10000037;
	mcr p15, 0, r1, c6, c3, 0
	msr cpsr_c, r0
	pop {r0-r1, pc}

.global	handleFiq
.type handleFiq, %function
handleFiq:
	str	lr, [sp, #-8]
	bl	storeRegs
	ldr r1, =handleFiqStr
	ldr r2, =cpuArm9Str
	b	handleException

.global	handleInstr
.type handleInstr, %function
handleInstr:
	str	lr, [sp, #-8]
	bl	storeRegs
	ldr r1, =handleInstrStr
	ldr r2, =cpuArm9Str
	b	handleException	

.global	handleData
.type handleData, %function
handleData:
	str	lr, [sp, #-8]
	bl	storeRegs
	ldr r1, =handleDataStr
	ldr r2, =cpuArm9Str
	b	handleException	

.global	handlePrefetch
.type handlePrefetch, %function
handlePrefetch:
	str	lr, [sp, #-8]
	bl	storeRegs
	ldr r1, =handlePrefetchStr
	ldr r2, =cpuArm9Str
	b	handleException	
.pool

@ The following will be copied and executed by ARM11
.align 4
.global hookSwi
.type hookSwi, %function
hookSwi:
	b setupScreen

setupScreen:
	cpsid aif
	mov	r1, #0x18000000
	add	r0, r1, #0x06F00000
	mov	r3, #0
	
	@ Set VRAM
	str	r1, [r0, #0x568]
	str	r1, [r0, #0x56C]
	
	@ Set LCD color format and turn off 3D
	ldrb r1, [r0, #0x570]
	mov r1, #1
	strb r1, [r0, #0x570]
	
	@ Commit changes
	str	r3, [r0, #0x578]	
	b setupScreen
	
.align 4
cpuArm9Str:			.ascii "ARM9\0"
cpuArm11Str:		.ascii "ARM11\0"
handleFiqStr:		.ascii "FIQ\0"
handleInstrStr:		.ascii "UNDEFINED INSTRUCTION\0"
handleDataStr:		.ascii "DATA ABORT\0"
handlePrefetchStr:	.ascii "PREFETCH ABORT\0"
.align 4
.pool

