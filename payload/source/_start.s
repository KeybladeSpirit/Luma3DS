.section .text.start
.align 4
.global _start
_start:
	b start

.global payloadCheckStr
payloadCheckStr:
	.ascii TIMEDATE
	.org ((payloadCheckStr - _start) + 0x20)
	
.global reqFirmwareStr
reqFirmwareStr:
	.org ((reqFirmwareStr - _start) + 0x3C)
	
.global configurationData
configurationData:
	.fill 0x10, 0x01, 1
	
start:
    @ Change the stack pointer
    mov sp, #0x27000000

    @ Disable caches / mpu
    mrc p15, 0, r4, c1, c0, 0  @ read control register
    bic r4, #(1<<12)           @ - instruction cache disable
    bic r4, #(1<<2)            @ - data cache disable
    bic r4, #(1<<0)            @ - mpu disable
    mcr p15, 0, r4, c1, c0, 0  @ write control register
    
    @ Give read/write access to all the memory regions
    ldr r5, =0x33333333
    mcr p15, 0, r5, c5, c0, 2 @ write data access
    mcr p15, 0, r5, c5, c0, 3 @ write instruction access

    @ Sets MPU permissions and cache settings
    ldr r0, =0xFFFF001D	@ ffff0000 32k  | bootrom (unprotected part)
    ldr r1, =0x3000801B	@ fff00000 16k  | dtcm
    ldr r2, =0x01FF801D	@ 01ff8000 32k  | itcm
    ldr r3, =0x08000029	@ 08000000 2M   | arm9 mem (O3DS / N3DS) 
    ldr r4, =0x10000029	@ 10000000 2M   | io mem (ARM9 / first 2MB)
    ldr r5, =0x20000037	@ 20000000 256M | fcram (O3DS / N3DS)
    ldr r6, =0x1FF00027	@ 1FF00000 1M   | dsp / axi wram
    ldr r7, =0x1800002D	@ 18000000 8M   | vram (+ 2MB)
    mov r10, #0x25
    mov r11, #0x25
    mov r12, #0x25
    mcr p15, 0, r0, c6, c0, 0
    mcr p15, 0, r1, c6, c1, 0
    mcr p15, 0, r2, c6, c2, 0
    mcr p15, 0, r3, c6, c3, 0
    mcr p15, 0, r4, c6, c4, 0
    mcr p15, 0, r5, c6, c5, 0
    mcr p15, 0, r6, c6, c6, 0
    mcr p15, 0, r7, c6, c7, 0
    mcr p15, 0, r10, c3, c0, 0	@ Write bufferable 0, 2, 5
    mcr p15, 0, r11, c2, c0, 0	@ Data cacheable 0, 2, 5
    mcr p15, 0, r12, c2, c0, 1	@ Inst cacheable 0, 2, 5

    @ Enable dctm
    ldr r1, =0x3000800A        @ set dtcm
    mcr p15, 0, r1, c9, c1, 0  @ set the dtcm Region Register
    
    @ Enable caches
    mrc p15, 0, r4, c1, c0, 0  @ read control register
    orr r4, r4, #(1<<18)       @ - itcm enable
    orr r4, r4, #(1<<16)       @ - dtcm enable
    orr r4, r4, #(1<<12)       @ - instruction cache enable
    orr r4, r4, #(1<<2)        @ - data cache enable
    orr r4, r4, #(1<<0)        @ - mpu enable
    mcr p15, 0, r4, c1, c0, 0  @ write control register

    @ Flush caches
    mov r5, #0
    mcr p15, 0, r5, c7, c5, 0  @ flush I-cache
    mcr p15, 0, r5, c7, c6, 0  @ flush D-cache
    mcr p15, 0, r5, c7, c10, 4 @ drain write buffer
	
    bl main

.die:
    b .die
	
.pool

.align 4
.global payloadLoader
payloadLoader:
	@ Will run at 0x24000000, or wherever you prefer
	mov r1, #0x24000000
	mov r0, #0x21000000
	mov r2, #0x00100000
	sub r1, r1, r2
	sub r2, r2, #0x200
	mov r4, #0
	_memcpy:
		ldr r3, [r0, r4]
		str r3, [r1, r4]
		add r4, r4, #4
		cmp r4, r2
		blt _memcpy
	mov r0, #0
    mcr p15, 0, r0, c7, c5, 0  @ flush I-cache
    mcr p15, 0, r0, c7, c6, 0  @ flush D-cache
    mcr p15, 0, r0, c7, c10, 4 @ drain write buffer
	bx r1
	
	
	