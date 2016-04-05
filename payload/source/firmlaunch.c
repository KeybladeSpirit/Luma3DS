#include <stdio.h>
#include "firmlaunch.h"

void firmLaunchBin(u8* firmBuffer)
{
	screenExit();
	u32* entry = (u32*)(firmBuffer + 0x40);
	for(int i = 0; i < 4; i++)
	{
		if(entry[2] > 0)
		{
			memcpy((void*)entry[1], entry[0] + (void*)firmBuffer, entry[2]);
		}
		entry += 0x30/sizeof(u32);
	}
	*((u32*)0x1FFFFFF8) = *((u32*)(firmBuffer + 8));
	*((u32*)0x1FFFFFFC) = *((u32*)(firmBuffer + 8));
	((void (*)())*((u32*)(firmBuffer + 12)))();
}

void firmLaunchFile(char* filename, unsigned int offset)
{
	u8 *firmBuffer = (u8*)0x24000000;
	fsFile* file = fsOpen(filename, 1);
	if(file)
	{
		fsSeek(file, offset);
		fsRead(firmBuffer, 1, fsGetSize(file), file);
		fsClose(file);
		firmLaunchBin(firmBuffer);
	}
}

fsFile* findTitleContent(u32 tid_low, u32 tid_high)
{
	// Searches for the latest content installed, and returns
	// a file pointer to its NCCH.
    DIR dir;
	FILINFO info;
	char str[256], fdir[256];
	u32 tid = 0;
	
	sprintf(fdir, "nand:/title/%08X/%08X/content", tid_low, tid_high);
	if(f_opendir(&dir, fdir) != FR_OK) return 0;
	while(f_readdir(&dir, &info) == FR_OK)
	{
		if(strstr(info.fname, ".app") || strstr(info.fname, ".APP"))
		{
			// We do not need it to be the proper installed version, 
			// what matters is the latest version of the content.
			// We check it by a simple tid comparision.
			u32 curId;
			sscanf(info.fname, "%08X.APP", (unsigned int *) &curId);
			if(curId > tid) tid = curId;
		}
		if(info.fname[0] == 0) break;
	}
	f_closedir(&dir);
	
	sprintf(str, "%s/%08X.app", fdir, tid);
	fsFile *tmp = fsOpen(str, 1);
	if(tmp)
	{
		return tmp;
	}
	return 0;
}

u8* getFirmFromTitle(firmType tid)
{
	// Decrypts firm from the installed title
	if(isNew3DS) tid != 0x20000000;
	fsFile* file = findTitleContent(0x00040138, tid);
	if(file)
	{
		// Read NCCH from file
		u8* ncchBuf = (u8*)(0x24000000);
		fsRead((void*)ncchBuf, 1, fsGetSize(file), file);
		fsClose(file);
		u32 exeFsSize = *((u32*)(ncchBuf + 0x1A4)) * 0x200;
		u32 exeFsAddr = *((u32*)(ncchBuf + 0x1A0)) * 0x200;
		
		// Craft the NCCH aes counter for CTR-MODE decryption
		u8 ctr[0x10];
		memset((void*)ctr, 0x00, 0x10);
		for(int i = 0; i < 8; i++) ctr[i] = *(ncchBuf + 0x108 + 7 - i);
		ctr[8] = 2;
		
		// Prepare AES engine 
		u8* keyY = ncchBuf;		// KeyY is the first 0x10 bytes of the NCCH signature
		aes_setkey(0x2C, keyY, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
		aes_setiv(ctr, AES_INPUT_BE | AES_INPUT_NORMAL);
		aes_use_keyslot(0x2C);
		
		// Decrypt 
		aes(ncchBuf + exeFsAddr, ncchBuf + exeFsAddr, exeFsSize/AES_BLOCK_SIZE, ctr, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);
		
		// Firm boot
		u8* firm = ncchBuf + exeFsAddr + 0x200;
		return firm;
	}
	Debug("[ERROR] Could not find firm title");
	return 0;
}

void launchNativeFirm()
{
	u8* firm = getFirmFromTitle(NATIVE_FIRM);
	if(firm)
	{
		firmLaunchBin(firm);
	}
}