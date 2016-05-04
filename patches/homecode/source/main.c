#include <3ds.h>
#include "svc.h"

extern u32 _start;
#define gspGpuHandle	(Handle)*((u32*)(*((u32*)((u8*)&_start + 4))))

Result _GSPGPU_WriteHWRegs(u32 regAddr, u32* data, u8 size)
{
	if(size>0x80 || !data)return -1;

	u32* cmdbuf=GetThreadCommandBuffer();
	cmdbuf[0]=IPC_MakeHeader(0x1,2,2); // 0x10082
	cmdbuf[1]=regAddr;
	cmdbuf[2]=size;
	cmdbuf[3]=IPC_Desc_StaticBuffer(size, 0);
	cmdbuf[4]=(u32)data;

	Result ret=0;
	if(R_FAILED(ret=svc_sendSyncRequest(gspGpuHandle)))return ret;

	return cmdbuf[1];
}

int main()
{
	//u32 data = 0x01FF00FF;
	//_GSPGPU_WriteHWRegs(0x202A04, &data, 4);
	return 0;
}