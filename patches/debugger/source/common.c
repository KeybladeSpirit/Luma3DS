#include "common.h"
#include "font.h"

static int line = 10;
static int row = 10;

void drawResetLine()
{
	line = 10;
	row = 10;
}

void drawChar(int character, int x, int y)
{
    for (int yy = 0; yy < 8; yy++)
	{
        int xDisplacement = (x * 3 * 240);
        int yDisplacement = ((240 - (y + yy) - 1) * 3);
        u8* screenPos = (u8*)frameBuf + xDisplacement + yDisplacement;
        u8 charPos = font[(character) * 8 + yy];
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
	while(*str != 0x00)
	{
		if(row >= (320 - 10))
		{
			row = 10;
			line += 10;
		}
		switch(*str)
		{
			case '\n':
			{
				line += 10;
				row = 10;
				break;
			}
			default:
			{
				drawChar(*str, row, line);
				row += 8;
				break;
			}
		}
		str++;
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

void memCpy(void* dst, void* src, u32 size)
{
	for(int _i = 0; _i < (int)size; _i++)
	{
		*((u8*)dst + _i) = *((u8*)src + _i);
	}
}

void memSet(void* dst, u8 val, u32 size)
{
	for(int _i = 0; _i < (int)size; _i++)
	{
		*((u8*)dst + _i) = (u8)val;
	}
}

void armBranch(void *cur, void *dst)
{
	*(u32*)cur = (0xEA << 24) | (((u32)dst - ((u32)cur + 8)) >> 2);
}