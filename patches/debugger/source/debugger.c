#include "common.h"
#include "i2c.h"

static char* typeException[] = 
{
	"FIQ",
	"Undefined Instruction",
	"Data Abort",
	"Prefetch Abort",
};

static char* typeCpu[] = 
{
	"ARM9", "ARM11",
};

static char* typeRegister[] = 
{
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
};

char* tmpStr = (char*)0x01FFF470;

void drawDebugInfo(u32* regs, int cpu, int type)
{
	// Control the screen
	extern void hookSwi(), arm11Access(); 	
	memSet((void*)frameBuf, backColor, 0x46500);
	arm11Access();
	memCpy((void*)0x1FFF4C00, (void*)&hookSwi, 0x200);
	armBranch((void*)(0x1FFF4008), (void*)0x1FFF4C00);
	
	// Draw Debug informations
	while(1)
	{
		drawResetLine();
		drawStr("PowerFirm - Panic Informations\n");
		drawStr("\nCPU     : ");
		drawStr(typeCpu[cpu]);
		drawStr("\nType    : ");
		drawStr(typeException[type]);
		drawStr("\nProcess : ");
		char* pStr = 0;
		switch(cpu)
		{
			default:
			case 0:
			{
				if(*((u32*)(*((u32*)0x8000040) + 0x78)) == 0)
				{
					pStr = "Kernel";
				}
				else
				{
					if(*((u32*)0x8000044))
					{
						pStr = (char*)(*((u32*)(*((u32*)0x8000044) + 0x60)) + 0x50);
						*(pStr + 0x10) = 0;
					}
					else
					{
						pStr = "NULL";
					}
				}
				break;
			}
		}
		drawStr(pStr);
		drawStr("\n\n");
		
		memSet((void*)tmpStr, 0x00, 32);
		for(int i = 0; i < 8; i++)
		{
			drawStr(typeRegister[i]);
			if(i < 10 || i > 12) drawStr(" ");
			strItoa(tmpStr, regs[i], 16, 8);
			drawStr(" : 0x");
			drawStr(tmpStr);
			drawStr("    ");
			
			drawStr(typeRegister[i+8]);
			if((i+8) < 10 || (i+8) > 12) drawStr(" ");
			strItoa(tmpStr, regs[i+8], 16, 8);
			drawStr(" : 0x");
			drawStr(tmpStr);
			drawStr("\n");
		}
		drawStr("\n\nPress START button to reboot...");
		if(~(*(volatile u32*)0x10146000) & (1 << 3)) i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
	}
}