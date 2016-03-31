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
	
	return 0;
}
