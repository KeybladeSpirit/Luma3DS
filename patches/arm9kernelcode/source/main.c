#include "common.h"
#include "i2c.h"
#include "hid.h"

u8* memSearch(u8* memstart, u8* memend, u8* memblock, u32 memsize)
{
	u8* block = (u8*)memblock;
	for(u8* mem = memstart; mem < memend; mem += 2)
	{
		int found = 1;
		for(int i = 0; i < memsize; i++)
		{
			if(*(mem + i) != *(block + i))
				found = 0;
		}
		if(found) return mem;
	}
	return 0;
}

void memCopy(u8* dst, u8* src, u32 size)
{
	if(!dst || !src) return;
	for(int i = 0; i < size; i++)
	{
		*(dst + i) = *(src + i);
	}
}	

int main()
{
	u32 pad = getHid();
	if(pad & BUTTON_A && pad & BUTTON_B && pad & BUTTON_X && pad & BUTTON_Y && pad & BUTTON_L1 && pad & BUTTON_R1)
	{
		// Fast Shutdown : A + B + X + Y + L + R
		i2cWriteRegister(I2C_DEV_MCU, 0x20, (u8)(1<<0));
	}
	return 0;
}
