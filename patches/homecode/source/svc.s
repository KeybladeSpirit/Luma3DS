.arm
.align 4

.global GetThreadCommandBuffer
.type GetThreadCommandBuffer, %function
GetThreadCommandBuffer:
	mrc p15, 0, r0, c13, c0, 3
	add r0, #0x80
	bx lr

.global svc_sendSyncRequest
.type svc_sendSyncRequest, %function
svc_sendSyncRequest:
	svc 0x32
	bx lr
