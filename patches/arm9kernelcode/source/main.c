#include "common.h"
#include "handlers.h"
#include "i2c.h"

static void initExceptionHandler()
{
	*(void **)0x08000008 = handleFiq;
	*(void **)0x08000018 = handleInstr;
	*(void **)0x0800002C = handleData;
	*(void **)0x08000020 = handlePrefetch;
	arm11Access();
	memcpy((void*)0x1FFF4C00, (void*)&hookSwi, 0x400);
}

int main()
{
	initExceptionHandler();
	if(~(*(volatile u32*)0x10146000) & (1 << 2))
	{
		
	}
	return 0;
}
