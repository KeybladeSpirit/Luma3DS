#include "power.h"
#include "firmlaunch.h"
#include "patches.h"

#define ARM9ADDR	0x08006800

typedef struct
{
	u32 data;
	u32 addr;
	u32 size;
	u32 type;
	u8 hash[0x20];
} firmEntry;

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

u32 getProcess9Shift(u8* data, u32 size)
{
	u8 stockCode[] = {0x30, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x30, 0x01, 0x04, 0x00};
	u8* buffer = memSearch(data, data + size, stockCode, 16);
	u32 ret = 0;
	if(buffer)
	{
		u32 process9CodeAddr = *((u32*)(buffer + 0x100));
		u32 process9CodeOffset = (buffer - data - 0x110) + ARM9ADDR + (*((u32*)(buffer + 0x90)) + 1)*0x200;
		ret = process9CodeAddr - process9CodeOffset;
	}
	return ret;
}

int patchFirmLaunch(u8* data, u32 size)
{
	u8 stockCode[] = { 0xDE, 0x1F, 0x8D, 0xE2, 0x20, 0x20, 0x90, 0xE5, 0x07, 0x00, 0xA0, 0xE1, 0x32, 0xFF, 0x2F, 0xE1};
	u8* buffer = memSearch(data, data + size, stockCode, 16);
	if(buffer)
	{
		buffer -= 0x10;
		u32 fopenAddr = (buffer - data) + ARM9ADDR + getProcess9Shift(data, size) + 8 - ((((*((u32*)buffer) & 0x00FFFFFF) << 2)*(-1)) & 0xFFFFF);
		memcpy(buffer, patches_firmlaunch_bin, patches_firmlaunch_bin_size);
		*((u32*)memSearch(buffer, buffer + patches_firmlaunch_bin_size, (u8[]){0xED, 0x0D, 0xDC, 0xBA}, 4)) = fopenAddr + 1;
		//Debug("[GOOD] FirmLaunch Patch");
		return 0;
	}
	Debug("[FAIL] FirmLaunch Patch");
	return 1;
}

int patchSignatureChecks(u8* data, u32 size)
{
	u8 stockCode[] = { 0x70, 0xB5, 0x22, 0x4D, 0x0C, 0x00, 0x69, 0x68, 0xCE, 0xB0, 0xCB, 0x08, 0x01, 0x68, 0x0E, 0x68};
	u8* buffer = memSearch(data, data + size, stockCode, 16);
	if(buffer)
	{
		*((u16*)(buffer + 0)) = (u16)0x2000;		// MOV R0, #0
		*((u16*)(buffer + 2)) = (u16)0x4770;		// BX LR
		u8 stockCode1[] = {0x02, 0x48, 0xC0, 0x1C, 0x76, 0xE7, 0x20, 0x00, 0x74, 0xE7, 0x22, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF};
		buffer = memSearch(data, data + size, stockCode1, 16);
		if(buffer)
		{
			*((u16*)(buffer + 2)) = (u16)0x2000;	// MOV R0, #0
			//Debug("[GOOD] Signature Checks Patch");
			return 0;
		}
	}
	Debug("[FAIL] Signature Checks Patch");
	return 1;
}

int patchFirmPartitionUpdate(u8* data, u32 size)
{
	u8 stockCode[] = { 0x00, 0x28, 0x01, 0xDA, 0x04, 0x00};
	u8* buffer = memSearch(data, data + size, (u8*)"exe:/%016llx/.firm", 18);
	if(buffer)
	{
		buffer -= 0x100;
		buffer = memSearch(buffer, buffer + 0x100, stockCode, 6);
		if(buffer)
		{
			*((u16*)(buffer + 0)) = (u16)0x2000;		// MOV R0, #0
			*((u16*)(buffer + 2)) = (u16)0x46C0;		// NOP
			//Debug("[GOOD] Arm9LoaderHax Protection");
			return 0;
		}
	}
	Debug("[FAIL] Arm9LoaderHax Protection");
	return 1;
}

int patchArm9KernelCode(u8* data, u32 size)
{
	u8 stockCodeK[] = {0x03, 0x00, 0x2D, 0xE9, 0xD3, 0xF0, 0x21, 0xE3, 0x0D, 0x00, 0xA0, 0xE1, 0xD2, 0xF0, 0x21, 0xE3};
	u8* buffer = memSearch(data, data + size, stockCodeK, 16);
	if(buffer)
	{
		buffer += 0x10;
		u32 kernelReturn = (buffer - data) + 8 + ARM9ADDR;
		*((u32*)(buffer + 0)) = (u32)0xE51FF004;	// LDR PC, [PC,#-4]
		*((u32*)(buffer + 4)) = (u32)0x0801A600;	// Our Code
		u32 magicWord = 0xDEADC0DE;	
		buffer = data + 0x13E00;
		memcpy((void*)buffer, patches_arm9kernelcode_bin, patches_arm9kernelcode_bin_size);
		u8* returnAddr = memSearch(buffer, buffer + patches_arm9kernelcode_bin_size, (u8*)&magicWord, 4);
		*((u32*)returnAddr) = kernelReturn;
		//Debug("[GOOD] Kernel9 Custom Code");
		return 0;

	}
	Debug("[FAIL] Kernel9 Custom Code");
	return 1;
}

int patchArm9Mpu(u8* data, u32 size)
{
	u8 stockCode[] = {0x00, 0x00, 0x10, 0x10, 0x01, 0x00, 0x00, 0x01};
	u8* buffer = memSearch(data, data + size, stockCode, 8);
	if(buffer)
	{
		buffer -= 4;
		*((u32*)(buffer + 0x00)) = 0x00360003;
		*((u32*)(buffer + 0x10)) = 0x20000035;
		*((u32*)(buffer + 0x18)) = 0x00200603;
		*((u32*)(buffer + 0x24)) = 0x001C0603;
		//Debug("[GOOD] ARM9 MPU Patch");
		return 0;

	}
	Debug("[FAIL] ARM9 MPU Patch\n");
	return 1;
}

int patchArm9Loader(u8* data, u32 size)
{
	//Debug("[GOOD] ARM9 Loader Fix");
	memcpy((void*)data, patches_arm9loader_bin, patches_arm9loader_bin_size);
	return 0;
}

int patchLoaderModule(u8* data, u32 size)
{
	u8* buffer = memSearch(data, data + size, "loader", 7);
	if(buffer)
	{
		buffer -= 0x200;
		u8* backup = (u8*)0x21000000;
		u32 backupOffset = *((u32*)(buffer + 0x104)) * 0x200;
		u32 backupSize = (u32)(size - ((u32)(buffer - data) + backupOffset));
		memcpy(backup, buffer + backupOffset, backupSize);
		memcpy(buffer, patches_loader_bin, patches_loader_bin_size);
		memcpy(buffer + patches_loader_bin_size, backup, backupSize);
		//Debug("[GOOD] Loader Module Hack");
		return 0;		
	}
	return 1;
}

void cryptArm9Bin(u8* buf)
{
	u8* key2;
	u8 secretKeys[][16] = {
		{0x07, 0x29, 0x44, 0x38, 0xF8, 0xC9, 0x75, 0x93, 0xAA, 0x0E, 0x4A, 0xB4, 0xAE, 0x84, 0xC1, 0xD8},
		{0x42, 0x3F, 0x81, 0x7A, 0x23, 0x52, 0x58, 0x31, 0x6E, 0x75, 0x8E, 0x3A, 0x39, 0x43, 0x2E, 0xD0}
	};
	if(*((u32*)(buf + 0x50)) == 0x324C394B) key2 = secretKeys[1];
	else key2 = secretKeys[0];
	
	 //Firm keys
    u8 keyX[0x10];
    u8 keyY[0x10];
    u8 CTR[0x10];
    u32 slot = 0x16;
    
    //Setup keys needed for arm9bin decryption
    memcpy((u8*)keyY, (void *)((uintptr_t)buf+0x10), 0x10);
    memcpy((u8*)CTR, (void *)((uintptr_t)buf+0x20), 0x10);
    u32 size = atoi((void *)((uintptr_t)buf+0x30));

    //Set 0x11 to key2 for the arm9bin and misc keys
    aes_setkey(0x11, (u8*)key2, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(0x11);
    
    //Set 0x16 keyX, keyY and CTR
	if(*((u32*)(buf + 0x100)) == 0)
		aes((u8*)keyX, (void *)((uintptr_t)buf+0x60), 1, NULL, AES_ECB_DECRYPT_MODE, 0);
	else
		aes((u8*)keyX, (void *)((uintptr_t)buf+0x00), 1, NULL, AES_ECB_DECRYPT_MODE, 0);
    aes_setkey(slot, (u8*)keyX, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_setkey(slot, (u8*)keyY, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_setiv((u8*)CTR, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(slot);
    
    //Decrypt arm9bin
    aes((void *)(buf+0x800), (void *)(buf+0x800), size/AES_BLOCK_SIZE, CTR, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);
}

void powerFirm()
{
	Debug("PowerFirm 3DS - @2016, Jason Dellaluce");
	//Debug("Buildtime : %s %s", __TIME__, __DATE__);
	
	u8* firm = decryptNativeFirm();
	
	if(!firm)
	{
		Debug("[ERROR] Could not open FIRM title");
	}
	
	if(*((u32*)firm) != 0x4D524946)
	{
		Debug("[ERROR] Could not read FIRM title");
	}
	
	firmEntry* entry = (firmEntry*)(firm + 0x40);
	u32 isNew = 0, res = 0;
		
	if(isNew3DS)
	{
		cryptArm9Bin(firm + (u32)entry[2].data);
		res += patchArm9Loader(firm + (u32)entry[2].data, 0);
		*((u32*)(firm + 12)) = (u32)0x08006000;
		isNew = 0x800;
	}
	
	res += patchLoaderModule		(firm + (u32)entry[0].data, entry[0].size);
	res += patchFirmPartitionUpdate	(firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
	res += patchFirmLaunch			(firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
	res += patchSignatureChecks		(firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
	res += patchArm9Mpu				(firm + (u32)entry[2].data + isNew, entry[2].size - isNew);		
	res += patchArm9KernelCode		(firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
	
	if(res)
	{
		Debug(" ");
		Debug("Press A to exit");
		while(1)
		{
			u32 key = waitHid();
			if(key & BUTTON_A) return;
		}
	}
	firmLaunchBin(firm);
}

