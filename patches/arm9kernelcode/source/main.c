#include "common.h"
#include "i2c.h"

static void initExceptionHandler()
{
	*(void **)0x08000008 = (void*)0x1FF8000;
	*(void **)0x08000018 = (void*)0x1FF8004;
	*(void **)0x0800002C = (void*)0x1FF8008;
	*(void **)0x08000020 = (void*)0x1FF800C;
}

int main()
{
	initExceptionHandler();
	if(~(*(volatile u32*)0x10146000) & (1 << 2))
	{
		//((void (*)())0xDEADC0DE)();
	}
	return 0;
}
