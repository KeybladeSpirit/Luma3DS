#include "common.h"

fsFile* findTitleContent(u32 tid_high, u32 tid_low)
{
	// Searches for the latest content installed, and returns
	// a file pointer to its NCCH.
    DIR dir;
	FILINFO info;
	char str[256];
	static char fdir[256];
	u32 tid = 0;

	sprintf(fdir, "nand:/title/%08X/%08X/content", tid_high, tid_low);

	if(f_opendir(&dir, &fdir[0]) != FR_OK) return 0;
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

	sprintf(str, "%s/%08X.APP", fdir, tid);
	fsFile *tmp = fsOpen(str, 1);
	if(tmp)
	{
		return tmp;
	}
	return 0;
}

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
	*((u32*)arm11EntryPoint) = *((u32*)(firmBuffer + 8));
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

u8* firmGetFromTitle(firmType tid)
{
	// Decrypts firm from the installed title
	if(isNew3DS) tid |= 0x20000000;
	fsFile* file = findTitleContent((u32)0x00040138, (u32)tid);

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

void firmLaunchNative()
{
	u8* firm = firmGetFromTitle(NATIVE_FIRM);
	if(firm)
	{
		firmLaunchBin(firm);
	}
}

#define BROWSER_MAX_FILES	0x100

int fileBrowser(char* folder, void(* func)(char*))
{
	// Searches for all the files in the SD, we hope to find the
	// path of PowerFirm payload.
	DIR dir;
	static FILINFO info;
	char tmp[_MAX_LFN*4];
	char entryStr[BROWSER_MAX_FILES][256];
	u8 entryAttr[BROWSER_MAX_FILES];
	int entryNum = 0;
	int sel = 0;

	info.lfname = (char*)0x21000000;
	info.lfsize = _MAX_LFN + 1;
	if(f_opendir(&dir, &folder[0]) == FR_OK)
	{
		while(f_readdir(&dir, &info) == FR_OK)
		{
			if(entryNum >= BROWSER_MAX_FILES) return 0;
			if(info.fname[0] == 0) break;
			if(info.fname[0] == '.') continue;
			if(strcmp(info.lfname, "System Volume Information") == 0) continue;

			strcpy(entryStr[entryNum], info.lfname[0] ? info.lfname : info.fname);
			entryAttr[entryNum] = info.fattrib;
			entryNum++;
		}
		f_closedir(&dir);
	}
	// BubbleSort the entries, in order to have all the folder before the files
	for(int i = 0; i < entryNum; i++)
	{
		for(int j = 0; j < entryNum - i - 1; j++)
		{
			if(!(entryAttr[j] & AM_DIR))
			{
				u8 t = entryAttr[j];
				entryAttr[j] = entryAttr[j+1];
				entryAttr[j+1] = t;
				strcpy(tmp, entryStr[j]);
				strcpy(entryStr[j], entryStr[j+1]);
				strcpy(entryStr[j+1], tmp);
			}
		}
	}
	writeThings:
	{
		DebugClear();
		Debug(folder);
		for(int i = 0; i < entryNum; i++)
		{
			Debug("  %c%s", (entryAttr[i] & AM_DIR) ? 0xF0 : ' ', entryStr[i]);
		}
		DrawString(TOP_SCREEN0, "Up/Down    : Change file", 10, 200, COLOR_WHITE, COLOR_BLACK);
		DrawString(TOP_SCREEN0, "A          : Select file", 10, 210, COLOR_WHITE, COLOR_BLACK);
		DrawString(TOP_SCREEN0, "B          : Back", 10, 220, COLOR_WHITE, COLOR_BLACK);
		while(1)
		{
			for(int i = 0; i < entryNum; i++)
			{
				DrawCharacter(TOP_SCREEN0, (sel == i) ? '-' : ' ', 10, 20 + 10*i, COLOR_WHITE, COLOR_BLACK);
			}

			u32 pad = waitHid();
			if(pad & BUTTON_UP && sel > 0) sel--;
			if(pad & BUTTON_DOWN && sel < entryNum - 1) sel++;
			if(pad & BUTTON_LEFT) sel = 0;
			if(pad & BUTTON_RIGHT) sel = entryNum - 1;
			if(pad & BUTTON_A)
			{
				sprintf(tmp, "%s/%s", folder, entryStr[sel]);
				if(entryAttr[sel] & AM_DIR)
				{
					fileBrowser(tmp, func);
					goto writeThings;
				}
				else func(tmp);
			}
			if(pad & BUTTON_B) break;
		}
	}
	return 0;
}

void loadPayload(char* path)
{
	// Defined in _start.s
	extern u32 payloadLoader;

	fsFile* file = fsOpen(path, 1);
	if(file)
	{
		fsRead((void*)0x21000000, 1, 0xFFE00, file);
		fsClose(file);
		fsExit();
		memcpy((void*)0x24000000, (void*)&payloadLoader, 0x50);
		((void (*)())0x24000000)();
	}
}
