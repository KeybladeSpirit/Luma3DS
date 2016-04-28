#include "common.h"
#include "font.h"
#include "handlers.h"
#include "i2c.h"

#define frameBuf 0x18000000

static int line = 0;

void drawChar(int character, int x, int y)
{
    for (int yy = 0; yy < 8; yy++)
	{
        int xDisplacement = (x * 3 * 240);
        int yDisplacement = ((240 - (y + yy) - 1) * 3);
        u8* screenPos = (u8*)frameBuf + xDisplacement + yDisplacement;
        u8 charPos = font[(character - 32) * 8 + yy];
        for (int xx = 7; xx >= 0; xx--)
		{
            if ((charPos >> xx) & 1)
			{
                *(screenPos + 0) = 0xFF;
                *(screenPos + 1) = 0xFF;
                *(screenPos + 2) = 0xFF;
            }
            screenPos += 3 * 240;
        }
    }
}

void drawStr(char* str)
{
	line++;
	int xpos = 10;
	while(*str)
	{
		drawChar(*(str++), xpos, line*10);
		xpos += 8;
	}
}

void strItoa(char* buf, unsigned int val, int base, int w)
{
	int i = w-1;
	int rem;
	do 
	{
		rem = val % base;
		buf[i--] = (rem < 10 ? 0x30 : 0x37) + rem;
		val /= base;
		w--;
	} while (val || w > 0);
}

void armBranch(void *cur, void *dst)
{
	*(u32*)cur = (0xEA << 24) | (((u32)dst - ((u32)cur + 8)) >> 2);
}

void memcpy(void* dst, void* src, u32 size)
{
	for(int _i = 0; _i < (int)size; _i++)
	{
		*((u8*)dst + _i) = *((u8*)src + _i);
	}
}

void memset(void* dst, u8 val, u32 size)
{
	for(int _i = 0; _i < (int)size; _i++)
	{
		*((u8*)dst + _i) = (u8)val;
	}
}

void handleException(u32* regs, char* type)
{
	// Hook ARM11 in order to control the screen
	arm11Access();
	memset((void*)frameBuf, 0x00, 0x46500);
	memcpy((void*)0x1FFF4B20, (void*)&hookSwi, 0x100);
	armBranch((void*)(0x1FFF4008), (void*)0x1FFF4B20);
	
	// Print debug info on the screen
	char* tmpStr = "R00 = 00000000    R00 = 00000000";
	while(1)
	{
		line = 0;
		drawStr("POWERFIRM - DEBUGGER");
		line++;
		drawStr("ARM9");
		drawStr(type);
		line++;
		
		// Ghetto string formatting
		for(int i = 0; i < 8; i++)
		{
			strItoa(tmpStr + 1, i, 10, 2);
			strItoa(tmpStr + 6, regs[i], 16, 8);
			strItoa(tmpStr + 19, i+8, 10, 2);
			strItoa(tmpStr + 24, regs[i+8], 16, 8);
			drawStr(tmpStr);
		}
		line++;
		drawStr("PRESS START TO REBOOT");
		if(~(*(volatile u32*)0x10146000) & (1 << 3)) i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
	}
}