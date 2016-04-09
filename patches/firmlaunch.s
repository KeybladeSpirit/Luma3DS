.nds
.create "patches/firmlaunch.bin", 0

.definelabel fileHandle,	0x2000A000
.definelabel tmpVar,		0x2000E000
.definelabel address,		0x23F00000
.definelabel fileOpen,		0xBADC0DED	; Will be set during patching

.arm
; Code jumps here right after the sprintf call
	
doPxi:
	ldr r4, =0x44846
	ldr r0, =0x10008000
	_doPxi:
		ldrh r1, [r0,#4]
		.word 0xE1B01B81	; lsls r1, r1, #0x17
		bmi _doPxi
		ldr r0, [r0,#0xC]		
	cmp r0, r4
	bne doPxi

cleanFileHandle:
	mov r0, #0
	ldr r1, =fileHandle
	mov r2, r0
	mov r3, #0x20
	_cleanFileHandle:
		str r2, [r1]
		add r1, #4
		add r0, #1
		cmp r0, r3
		blt _cleanFileHandle
		
loadPayload:
	ldr r1, =(fileName - loadPayload - 12)
	add r1, pc
	mov r2, #1
	ldr r0, =fileHandle+8
	ldr r6, =fileOpen
	blx r6
	
	ldr r0, =fileHandle
	ldr r1, =tmpVar
	ldr r2, =address
	ldr r3, =0xFFE00
	ldr r6, [sp,#0x3A8-0x198]
	ldr r6, [r6,#0x28]	; fread(handle, bytes, buffer, size)
	blx r6
	
	ldr r1, =address+0x24
	add r0, sp, #0x3A8-0x70	
	mov r2, #0x38
	bl memcpy
	
svcKernelSetState:
	mov r2, #0
	mov r3, #0
	mov r1, #0
	mov r0, #0
	.word 0xEF00007C	; svc 0x7C
		
runKernelCode:
	ldr r0, =(kernelEntry - runKernelCode - 12)
	add r0, pc
	.word 0xEF00007B	; svc 0x7B
		
dieLoop:
	b dieLoop

memcpy:
	mov r12, lr
	stmfd sp!, {r0-r4}
	add r2, r2, r0
	_memcpy:
		ldr r3, [r0],#4
		str r3, [r1],#4
		cmp r0, r2
		blt _memcpy
	ldmfd sp!, {r0-r4}
	mov lr, r12
	bx lr

mcuRebootSystem:
	ldr r0, =0x0801A604
	.word 0xEF00007B    //SVC 0x7B
		
mcuShutdownSystem:
	ldr r0, =0x0801A608
	.word 0xEF00007B    //SVC 0x7B

kernelEntry:
	; MPU settings
	mrc p15, 0, r0, c2, c0, 0  
	mrc p15, 0, r12, c2, c0, 1
	mrc p15, 0, r1, c3, c0, 0
	mrc p15, 0, r2, c5, c0, 2
	mrc p15, 0, r3, c5, c0, 3
	ldr r4, =0x18000035 
	bic r2, r2, #0xF0000
	bic r3, r3, #0xF0000
	orr r0, r0, #0x10
	orr r2, r2, #0x30000
	orr r3, r3, #0x30000
	orr r12, r12, #0x10
	orr r1, r1, #0x10
	mcr p15, 0, r0, c2, c0, 0
	mcr p15, 0, r12, c2, c0, 1
	mcr p15, 0, r1, c3, c0, 0
	mcr p15, 0, r2, c5, c0, 2
	mcr p15, 0, r3, c5, c0, 3
	mcr p15, 0, r4, c6, c4, 0
	mrc p15, 0, r0, c2, c0, 0
	mrc p15, 0, r1, c2, c0, 1
	mrc p15, 0, r2, c3, c0, 0
	orr r0, r0, #0x20
	orr r1, r1, #0x20
	orr r2, r2, #0x20
	mcr p15, 0, r0, c2, c0, 0
	mcr p15, 0, r1, c2, c0, 1
	mcr p15, 0, r2, c3, c0, 0
	
	; Flush cache
	mov r2, #0
	mov r1, r2
	flushCache:
		mov r0, #0
		mov r3, r2, lsl #30
		_flushCache:
			orr r12, r3, r0, lsl#5
			mcr p15, 0, r1, c7, c10, 4
			mcr p15, 0, r12, c7, c14, 2
			add r0, #1
			cmp r0, #0x20
			bcc _flushCache
		add r2, #1
		cmp r2, #4
		bcc flushCache
	
	; Enable MPU
	mcr p15, 0, R1,c7,c10, 4
	ldr r0, =0x42078  
	mcr p15, 0, r0, c1, c0, 0
	mcr p15, 0, r1, c7, c5, 0
	mcr p15, 0, r1, c7, c6, 0
	mcr p15, 0, r1, c7, c10, 4
	
	; Jump to payload
	ldr r0, =address
	bx r0
	
.pool
fileName:
	.word 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.word 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.close