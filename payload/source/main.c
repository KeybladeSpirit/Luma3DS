#include "common.h"
#include "firmlaunch.h"
#include "power.h"

void keysInit()
{
	*(u32*)0x10000020 = 0;
    *(u32*)0x10000020 = 0x340;
	if(isNew3DS)
	{
		// CTRNAND Keyslot 0x05 initialization
		u8* buf = (u8*)0x21000000;
		u8 nKeyY[16] = {0x4D, 0x80, 0x4F, 0x4E, 0x99, 0x90, 0x19, 0x46, 0x13, 0xA2, 0x04, 0xAC, 0x58, 0x44, 0x60, 0xBE};
		aes_setkey(5, (u8*)nKeyY, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
		nandReadSectors(0, 1, buf, NEWCTR);
	}
}

int main()
{
	screenInit();
	keysInit();
	fsInit();

	powerFirm(NULL);
	
	fsExit();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, (u8)(1<<0));
    return 0;
}
