#include <3ds.h>
#include <string.h>
#include "patcher.h"
#include "ifile.h"

static u8 *_memSearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
{
    const u8 *patternc = (const u8 *)pattern;
	
    //Preprocessing
    u32 table[256];
    for(u32 i = 0; i < 256; ++i)
        table[i] = patternSize + 1;
    for(u32 i = 0; i < patternSize; ++i)
        table[patternc[i]] = patternSize - i;
		
    //Searching
    u32 j = 0;
    while(j <= size - patternSize)
    {
        if(memcmp(patternc, startPos + j, patternSize) == 0)
            return startPos + j;
        j += table[startPos[j + patternSize]];
    }
    return NULL;
}

static u8* patch_memory(start, size, pattern, patsize, offset, replace, repsize, count)
	u8* start;
	u32 size;
	u32 patsize;
	u8 pattern[patsize];
	int offset;
	u32 repsize;
	u8 replace[repsize];
	int count;
{
	u8 *found;
	int i;
	u32 at;
	
	for (i = 0; i < count; i++)
	{
		found = _memSearch(start, pattern, size, patsize);
		if (found == NULL)
		{
			break;
		}
		at = (u32)(found - start);
		memcpy(found + offset, replace, repsize);
		if (at + patsize > size)
		{
			size = 0;
		}
		else
		{
			size = size - (at + patsize);
		}
		start = found + patsize;
	}
	return (found + offset);
}

int getArmBoff(void *p)
{
	int i;

	i = *(int *)p & 0xFFFFFF;
	if (i & 0x800000)
		i -= 0x1000000;

	return i << 2;
}

void findHomeMenuSymbols(u32* outbuf, u8* code, u32 size)
{
	u8* ref;
	
	// gspGpuHandle
	ref = _memSearch(code, (u8[]){0x06, 0x30, 0xA0, 0xE1, 0x04, 0x20, 0xA0, 0xE1, 0x05, 0x10, 0xA0, 0xE1}, size, 12) - 4;
	ref += ((*((u32*)ref) & 0xFFFFFF) << 2) + 8;
	outbuf[0] = *((u32*)(ref + 8));
}

int patch_code(u64 progid, u8 *code, u32 size)
{
	static const char region_free_pattern[]			= {0x00, 0x00, 0x55, 0xE3, 0x01, 0x10, 0xA0, 0xE3};
	static const char region_free_patch[]			= {0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1};
	static const char block_updates_pattern[]		= {0x25, 0x79, 0x0B, 0x99};
	static const char block_updates_patch[]			= {0xE3, 0xA0};
	static const char block_eshop_updates_pattern[]	= {0x30, 0xB5, 0xF1, 0xB0};
	static const char block_eshop_updates_patch[]	= {0x00, 0x20, 0x08, 0x60, 0x70, 0x47};
	static const char stop_updates_pattern[]		= {0x0C, 0x18, 0xE1, 0xD8};
	static const char stop_updates_patch[]			= {0x0B, 0x18, 0x21, 0xC8};
	extern const u8 homecode_bin[];
	extern const u32 homecode_bin_size;
	
	switch(progid)
	{
		case 0x0004003000008F02LL:
		case 0x0004003000008202LL:
		case 0x0004003000009802LL:
		case 0x000400300000A102LL:
		case 0x000400300000A902LL:
		case 0x000400300000B102LL:
		{
			// Home Menu
			if(*((u8*)(0x14000004)))
				patch_memory(code, size, region_free_pattern, sizeof(region_free_pattern), -16, region_free_patch, sizeof(region_free_patch), 1);
			
			// Code hook
			u32 addr = (u32)patch_memory(code, size, (u8[]){0x06, 0x30, 0xA0, 0xE1, 0x04, 0x20, 0xA0, 0xE1, 0x05, 0x10, 0xA0, 0xE1}, 12, -24, (u8*)homecode_bin, homecode_bin_size, 1);
			// Relocate the symbols
			for(int i = 0; i < homecode_bin_size; i+=4)
			{
				if(((*((u32*)(addr + i)) & 0xFF000000) >> 24) == 0xAE)
				{
					*((u32*)(addr + i)) &= 0x00FFFFFF;
					*((u32*)(addr + i)) += (addr - (u32)code) + 0x100000;
				}
			}
			// Find Home Menu symbols
			findHomeMenuSymbols((u32*)(addr + 4), code, size);
			break;
		}
		case 0x0004013000002C02LL:
		{
			// NIM
			patch_memory(code, size, block_updates_pattern, sizeof(block_updates_pattern), 0, block_updates_patch, sizeof(block_updates_patch), 1);
			patch_memory(code, size, block_eshop_updates_pattern, sizeof(block_eshop_updates_pattern), 0, block_eshop_updates_patch, sizeof(block_eshop_updates_patch), 1);
			break;
		}
		case 0x0004013000008002LL:
		{
			// NS
			patch_memory(code, size, stop_updates_pattern, sizeof(stop_updates_pattern), 0, stop_updates_patch, sizeof(stop_updates_patch), 2);
			break;
		}
		case 0x0004001000020000LL:
		case 0x0004001000021000LL:
		case 0x0004001000022000LL:
		case 0x0004001000026000LL:
		case 0x0004001000027000LL:
		case 0x0004001000028000LL:
		{
			// System Settings
			patch_memory(code, size, (char*)L"Ver.", 8, 0, (char*)L"%d.%d.%d-%d%ls #POW", 38, 1);
			break;
		}
	}
	return 0;
}