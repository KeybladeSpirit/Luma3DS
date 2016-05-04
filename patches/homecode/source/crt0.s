.section ".init"
.arm
.align 4
.global _start
_start:
	b	_skipvars
	
_homeMenuSymbols:
	.word 0, 0, 0, 0, 0, 0, 0, 0
	
_skipvars:
	stmfd	sp!, {r0-r12,lr}
	bl		main
	ldmfd	sp!, {r0-r12,pc}
.pool

