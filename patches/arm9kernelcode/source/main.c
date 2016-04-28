#include "common.h"
#include "handlers.h"

static void initExceptionHandler()
{
	*(void **)0x08000008 = handleFiq;
	*(void **)0x08000018 = handleInstr;
	*(void **)0x0800002C = handleData;
	*(void **)0x08000020 = handlePrefetch;
	arm11Access();
}

int main()
{
	initExceptionHandler();
	/*if(~(*(volatile u32*)0x10146000) & (1 << 2))
	{
		*(u32*)0xDEADC0DE = 1024;
	}*/
	return 0;
}
