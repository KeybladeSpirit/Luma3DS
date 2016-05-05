#include "common.h"
#include "power.h"

void arm11SlaveRoutine()
{
	__asm(".word 0xF10C01C0");
	while(1)
	{
		*((vu32*)arm11EntryPoint) = 0;
		while(!*((vu32*)arm11EntryPoint));
		((void (*)())*((vu32*)arm11EntryPoint))();
	}
}

void miscInit()
{
	// SDMC/NAND Access
	*(u32*)0x10000020 = 0;
	*(u32*)0x10000020 = 0x340;

	// N3DS ctrNand key init
	if(isNew3DS && isColdBoot)
	{
		// CTRNAND Keyslot 0x05 initialization
		u8* buf = (u8*)0x21000000;
		u8 nKeyY[16] = {0x4D, 0x80, 0x4F, 0x4E, 0x99, 0x90, 0x19, 0x46, 0x13, 0xA2, 0x04, 0xAC, 0x58, 0x44, 0x60, 0xBE};
		aes_setkey(5, (u8*)nKeyY, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
		nandReadSectors(0, 1, buf, NEWCTR);
	}

	// Turn ARM11 in hour executer slave
	arm11Execute(arm11SlaveRoutine);
}

int main()
{
	screenInit();
	miscInit();
	fsInit();

	powerFirm(NULL);

	fsExit();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, (u8)(1<<0));
    return 0;
}
