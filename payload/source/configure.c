#include "configure.h"
#include "power.h"

void configMenu()
{
	int sel = 0;
	int numOptions = sizeof(configData)/sizeof(u8);
	
	while(1)
	{
		int cur = 0;
		DebugReset();
		Debug("PowerFirm - Configuration Menu");
		Debug("");
		Debug("");
		Debug("%c PowerFirm Boot Animation     %s", (sel == cur) ? '-' : ' ', *((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("%c Classic Mode (Stock System)  %s", (sel == cur) ? '-' : ' ', !*((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("%c Signatures Checks Bypass     %s", (sel == cur) ? '-' : ' ', *((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("%c ARM9 Exception Debugger      %s", (sel == cur) ? '-' : ' ', *((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("%c GBA Boot-Screen Animation    %s", (sel == cur) ? '-' : ' ', *((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("%c Home Menu Region-Free        %s", (sel == cur) ? '-' : ' ', *((u8*)curConfig + cur++) ? "<on>" : "<off>");
		Debug("");
		Debug("");
		Debug("Up/Down    : Select option");
		Debug("Left/Right : Change option");
		Debug("Start      : Save changes and reboot");
		
		u32 pad = waitHid();
		if(pad & BUTTON_UP && sel > 0) sel--;
		if(pad & BUTTON_DOWN && sel < numOptions - 1) sel++;
		if(pad & BUTTON_LEFT || pad & BUTTON_RIGHT) *((u8*)curConfig + sel) ^= 1;
		if(pad & BUTTON_START)
		{
			char path[256];
			getPayloadPath(path, "");
			fsFile *file = fsOpen(path, 3);
			if(file)
			{
				fsSeek(file, 0x60);
				fsWrite((void*)curConfig, 1, sizeof(configData), file);
				fsClose(file);
			}
			break;
		}
	}
	i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
}