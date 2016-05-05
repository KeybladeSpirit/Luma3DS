#include "power.h"
#include "patches.h"
#include "fatfs/ff.h"
#include "configure.h"

static u32 result = 0;

void drawGiantLetter(int character, int x, int y, int xScale, int yScale)
{
	extern const unsigned char font[];
    for (int yy = 0; yy < 8*yScale; yy+=yScale)
	{
        int xDisplacement = (x * BYTES_PER_PIXEL * SCREEN_HEIGHT);
        int yDisplacement = ((SCREEN_HEIGHT - (y + yy) - 1) * BYTES_PER_PIXEL);

		for(int _scaley = 0; _scaley < yScale; _scaley++)
		{
			u8* screenPos = TOP_SCREEN0 + xDisplacement + yDisplacement;
			u8 charPos = font[character * 8 + yy/yScale];
			for (int xx = 7; xx >= 0; xx--)
			{
				for(int _scalex = 0; _scalex < xScale; _scalex++)
				{
					if ((charPos >> xx) & 1)
					{
						*(screenPos + 0) = COLOR_WHITE >> 16;  // B
						*(screenPos + 1) = COLOR_WHITE >> 8;   // G
						*(screenPos + 2) = COLOR_WHITE & 0xFF; // R
					} else {
						*(screenPos + 0) = COLOR_BLACK >> 16;  // B
						*(screenPos + 1) = COLOR_BLACK >> 8;   // G
						*(screenPos + 2) = COLOR_BLACK & 0xFF; // R
					}
					screenPos += BYTES_PER_PIXEL * SCREEN_HEIGHT;
				}
			}
			yDisplacement += BYTES_PER_PIXEL;
		}
    }
}

void drawSplashString(const char* str, int y, bool anim)
{
	int xScale = 4;
	int yScale = 3;
	int charDist = 8*xScale;
	int xPos = (400 - strlen(str)*(charDist))/2;
	int waitTime = 10000000/8;
	int drawTime = 1000000/2;

	if(anim)
	{
		drawGiantLetter('_', xPos + charDist*0, y, xScale, yScale);
		ioDelay(waitTime);
		drawGiantLetter(' ', xPos + charDist*0, y, xScale, yScale);
		ioDelay(waitTime);

		for(int i = 0; i < strlen(str) + 1; i++)
		{
			for(int j = 0; j < i; j++)
			{
				drawGiantLetter(str[j], xPos + charDist*j, y, xScale, yScale);
			}
			drawGiantLetter('_', xPos + charDist*(i), y, xScale, yScale);
			ioDelay(drawTime);
		}
		drawGiantLetter(' ', xPos + charDist*strlen(str), y, xScale, yScale);
		ioDelay(waitTime);
		drawGiantLetter('_', xPos + charDist*strlen(str), y, xScale, yScale);
		ioDelay(waitTime);
		drawGiantLetter(' ', xPos + charDist*strlen(str), y, xScale, yScale);
		ioDelay(waitTime);
	}
	else
	{
		for(int i = 0; i < strlen(str); i++)
		{
			drawGiantLetter(str[i], xPos + charDist*i, y, xScale, yScale);
		}
	}
}

void splashScreen()
{
	if(!isColdBoot) return;
	if(!curConfig->powAnim) return;
	ClearScreenFull(1, 1);
	DrawString(TOP_SCREEN0, "[L] : Options", 10, 10, COLOR_WHITE, COLOR_BLACK);
	DrawString(TOP_SCREEN0, VERSION, 10, 220, COLOR_WHITE, COLOR_BLACK);
	DrawString(TOP_SCREEN0, "@2016, Jason Dellaluce", 208, 220, COLOR_WHITE, COLOR_BLACK);
	drawSplashString("PowerFirm", 88, 1);
	drawSplashString("3DS", 128, 0);
}

u8 *_memSearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
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

u8* memSearch(u8* memstart, u8* memend, u8* memblock, u32 memsize)
{
	return _memSearch(memstart, (void*)memblock, (u32)(memend - memstart), (u32)memsize);
}

char* dirToAvoid[] =
{
	"Nintendo 3DS",
	"System Volume Information",
	".",
	"..",
	"CIAngel",
	"TIKdevil",
	"3ds"
};

char pathBackup[256] = {0};

int getPayloadPath(char* path, char* baseDir)
{
	if(strlen(pathBackup) > 1)
	{
		strncpy(pathBackup, path, sizeof(pathBackup));
		return 1;
	}
	// Searches for all the files in the SD, we hope to find the
	// path of PowerFirm payload.
	DIR dir;
	static FILINFO info;
	char tmp[1024];
	info.lfname = (char*)0x21000000;
	info.lfsize = 128 + 1;

	if(f_opendir(&dir, &baseDir[0]) == FR_OK)
	{
		while(f_readdir(&dir, &info) == FR_OK)
		{
			if(info.fname[0] == 0) break;
			bool avoid = 0;
			for(int i = 0; i < sizeof(dirToAvoid)/sizeof(char*); i++)
			{
				if(strcmp(info.lfname, dirToAvoid[i]) == 0)
				{
					avoid = 1;
					break;
				}
			}
			if(avoid) continue;

			sprintf(tmp, "%s/%s", baseDir, info.lfname);

			if(info.fattrib & AM_DIR)
			{
				if(getPayloadPath(path, tmp)) return 1;
			}
			else
			{
				fsFile* file = fsOpen(tmp, 1);
				if(file)
				{
					extern u32 payloadCheckStr, _start;
					fsSeek(file, (u32)(&payloadCheckStr - &_start)*4);
					fsRead((void*)0x22000000, 1, 0x20, file);
					fsClose(file);
					if(memcmp((void*)0x22000000, (void*)&payloadCheckStr, 0x20) == 0)
					{
						char* _path = &tmp[0];
						while(*_path == '/') _path++;
						strcpy(path, "sdmc:/");
						strcat(path, _path);
						strncpy(pathBackup, path, sizeof(pathBackup));
						return 1;
					}
				}
			}
			memset(info.lfname, 0, info.lfsize);
		}
		f_closedir(&dir);
	}
	return 0;
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

	// Firm keys
	u8 keyX[0x10];
	u8 keyY[0x10];
	u8 CTR[0x10];
	u32 slot = 0x16;

	// Setup keys needed for arm9bin decryption
	memcpy((u8*)keyY, (void *)((uintptr_t)buf+0x10), 0x10);
	memcpy((u8*)CTR, (void *)((uintptr_t)buf+0x20), 0x10);
	u32 size = atoi((void *)((uintptr_t)buf+0x30));

	// Set 0x11 to key2 for the arm9bin and misc keys
	aes_setkey(0x11, (u8*)key2, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
	aes_use_keyslot(0x11);

	// Set 0x16 keyX, keyY and CTR
	if(*((u32*)(buf + 0x100)) == 0)
	{
		aes((u8*)keyX, (void *)((uintptr_t)buf+0x60), 1, NULL, AES_ECB_DECRYPT_MODE, 0);
	}
	else
	{
		aes((u8*)keyX, (void *)((uintptr_t)buf+0x00), 1, NULL, AES_ECB_DECRYPT_MODE, 0);
	}
	aes_setkey(slot, (u8*)keyX, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
	aes_setkey(slot, (u8*)keyY, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);
	aes_setiv((u8*)CTR, AES_INPUT_BE | AES_INPUT_NORMAL);
	aes_use_keyslot(slot);

	//Decrypt arm9bin
	aes((void *)(buf+0x800), (void *)(buf+0x800), size/AES_BLOCK_SIZE, CTR, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);
}

u32 getProcess9Shift(u8* data, u32 size)
{
	u8 stockCode[] = {0x30, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x30, 0x01, 0x04, 0x00};
	u8* buffer = memSearch(data, data + size, stockCode, 16);
	u32 ret = 0;
	if(buffer)
	{
		u32 process9CodeAddr = *((u32*)(buffer + 0x100));
		u32 process9CodeOffset = (buffer - data - 0x110) + 0x08006800 + (*((u32*)(buffer + 0x90)) + 1)*0x200;
		ret = process9CodeAddr - process9CodeOffset;
	}
	return ret;
}

int patchFirmLaunch(u8* data, u32 size)
{
	char path[128];
	u8 stockCode[] = { 0xDE, 0x1F, 0x8D, 0xE2, 0x20, 0x20, 0x90, 0xE5, 0x07, 0x00, 0xA0, 0xE1, 0x32, 0xFF, 0x2F, 0xE1};
	u8* buffer = memSearch(data, data + size, stockCode, 16);
	if(buffer)
	{
		buffer -= 0x10;
		u32 fopenAddr = (buffer - data) + 0x08006800 + getProcess9Shift(data, size) + 8 - ((((*((u32*)buffer) & 0x00FFFFFF) << 2)*(-1)) & 0xFFFFF);
		memcpy(buffer, patches_firmlaunch_bin, patches_firmlaunch_bin_size);
		*((u32*)memSearch(buffer, buffer + patches_firmlaunch_bin_size, (u8[]){0xED, 0x0D, 0xDC, 0xBA}, 4)) = fopenAddr + 1;
		getPayloadPath(path, "");

		for(int i = 0; i < strlen(path); i++)
		{
			*(buffer + patches_firmlaunch_bin_size - 0x80 + i*2) = (u8)path[i];
		}
		#ifdef VERBOSE
			Debug("[GOOD] FirmLaunch Patch");
		#endif
		return 0;
	}
	Debug("[FAIL] FirmLaunch Patch");
	result++;
	return 1;
}

int patchSignatureChecks(u8* data, u32 size, int isNative)
{
	if(!curConfig->powMode) return 0;
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
			#ifdef VERBOSE
				Debug("[GOOD] Signature Checks Patch");
			#endif
			return 0;
		}
		if(!isNative) return 0;
	}
	Debug("[FAIL] Signature Checks Patch");
	result ++;
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
			#ifdef VERBOSE
				Debug("[GOOD] Arm9LoaderHax Protection");
			#endif
			return 0;
		}
	}
	Debug("[FAIL] Arm9LoaderHax Protection");
	result++;
	return 1;
}

int patchArm9KernelCode(u8* data, u32 size)
{
	if(!curConfig->powMode) return 0;
	if(!curConfig->powDebug) return 0;
	u8 stockCodeK[] = {0x03, 0x00, 0x2D, 0xE9, 0xD3, 0xF0, 0x21, 0xE3, 0x0D, 0x00, 0xA0, 0xE1, 0xD2, 0xF0, 0x21, 0xE3};
	u8* buffer = memSearch(data, data + size, stockCodeK, 16);
	if(buffer)
	{
		buffer += 0x10;
		u32 kernelReturn = (buffer - data) + 8 + 0x08006800;
		*((u32*)(buffer + 0)) = (u32)0xE51FF004;	// LDR PC, [PC,#-4]
		*((u32*)(buffer + 4)) = (u32)0x0801A500;	// Our Code
		u32 magicWord = 0xDEADC0DE;
		buffer = data + 0x13D00;
		memcpy((void*)buffer, patches_arm9kernelcode_bin, patches_arm9kernelcode_bin_size);
		u8* returnAddr = memSearch(buffer, buffer + patches_arm9kernelcode_bin_size, (u8*)&magicWord, 4);
		*((u32*)returnAddr) = kernelReturn;
		#ifdef VERBOSE
			Debug("[GOOD] Kernel9 Custom Code");
		#endif
		return 0;
	}
	Debug("[FAIL] Kernel9 Custom Code");
	result++;
	return 1;
}

int patchArm9Mpu(u8* data, u32 size)
{
	if(!curConfig->powMode) return 0;
	if(!curConfig->powDebug) return 0;
	u8 stockCode[] = {0x00, 0x00, 0x10, 0x10, 0x01, 0x00, 0x00, 0x01};
	u8* buffer = memSearch(data, data + size, stockCode, 8);
	if(buffer)
	{
		buffer -= 4;
		*((u32*)(buffer + 0x00)) = 0x00360003;
		*((u32*)(buffer + 0x10)) = 0x20000035;
		*((u32*)(buffer + 0x18)) = 0x00200603;
		*((u32*)(buffer + 0x24)) = 0x001C0603;
		#ifdef VERBOSE
			Debug("[GOOD] ARM9 MPU Patch");
		#endif
		return 0;

	}
	Debug("[FAIL] ARM9 MPU Patch\n");
	result++;
	return 1;
}

int patchArm9Loader(u8* data, u32 size)
{
	cryptArm9Bin(data);
	memset((void*)data, 0x00, 0x800);
	memcpy((void*)data, patches_arm9loader_bin, patches_arm9loader_bin_size);
	#ifdef VERBOSE
		Debug("[GOOD] ARM9 Loader Fix");
	#endif
	return 0;
}

int patchAgbBootSplash(u8* data, u32 size)
{
	if(!curConfig->powMode) return 0;
	u8 stockCode[] = {0x00, 0x00, 0x01, 0xEF};
	u8* buffer = memSearch(data, data + size, stockCode, 4);
	if(buffer)
	{
		*((u32*)(buffer)) = 0xEF260000;
		#ifdef VERBOSE
			Debug("[GOOD] ARM9 AGB Boot Splash");
		#endif
		return 0;
	}
	Debug("[FAIL] ARM9 AGB Boot Splash\n");
	result++;
	return 1;
}

int patchTwlChecks(u8* data, u32 size)
{
	// This all belongs to TuxSH and Steveice10 work
	int res = 0;
	u8* buffer;

	// Patch RSA function to not report invalid signatures
	buffer = memSearch(data, data + size, (u8[8]){0x00, 0x20, 0xF6, 0xE7, 0x7F, 0xB5, 0x04, 0x00}, 8);
	if(buffer)
	{
		*((u16*)(buffer)) = 0x2001;
	}else res++;

	// Disable whitelist check
	buffer = memSearch(data, data + size, (u8[4]){0xFF, 0xF7, 0xB6, 0xFB}, 4);
	if(buffer)
	{
		*((u32*)(buffer)) = 0x00002000;
	}else res++;

	// Disable cartridge blacklist, save type, DSi cartridge save exploit checks
	buffer = memSearch(data, data + size, isNew3DS ? (u8[6]){0x20, 0x00, 0x0E, 0xF0, 0x15, 0xFE} : (u8[6]){0x20, 0x00, 0x0E, 0xF0, 0xF9, 0xFD}, 6);
	if(buffer)
	{
		u8 patch[] = {0x01, 0x20, 0x00, 0x00};
		memcpy((void*)(buffer + 02), (void*)patch, 4);
		memcpy((void*)(buffer + 14), (void*)patch, 4);
		memcpy((void*)(buffer + 26), (void*)patch, 4);
	}else res++;

	// Disable SHA check
	buffer = memSearch(data, data + size, (u8[4]){0x10, 0xB5, 0x14, 0x22}, 4);
	if(buffer)
	{
		*((u32*)(buffer)) = 0x47702001;
	}else res++;

	if(res)
	{
		Debug("[FAIL] ARM9 TWL Checks Bypass\n");
	}
	else if(res)
	{
		#ifdef VERBOSE
			Debug("[GOOD] ARM9 TWL Checks Bypass\n");
		#endif
	}
	result += res;
	return res;
}

int patchLoaderModule(u8* data, u32 size)
{
	if(!curConfig->powMode) return 0;
	u8* buffer = memSearch(data, data + size, (u8*)"loader", 7);
	if(buffer)
	{
		buffer -= 0x200;
		u8* backup = (u8*)0x21000000;
		u32 backupOffset = *((u32*)(buffer + 0x104)) * 0x200;
		u32 backupSize = (u32)(size - ((u32)(buffer - data) + backupOffset));
		memcpy(backup, buffer + backupOffset, backupSize);
		memcpy(buffer, patches_loader_bin, patches_loader_bin_size);
		memcpy(buffer + patches_loader_bin_size, backup, backupSize);
		#ifdef VERBOSE
			Debug("[GOOD] Loader Module Hack");
		#endif
		return 0;
	}
	Debug("[FAIL] Loader Module Hack");
	result++;
	return 1;
}

int patchArm11Reboot(u8* data, u32 size)
{
	u32 rebootAddr = (u32)0x1FFFFFFC;
	u8* buffer = memSearch(data, data + size, (u8*)&rebootAddr, 4);
	if(buffer)
	{
		*((u32*)buffer) = 0x1FFFFFF8;
		#ifdef VERBOSE
			Debug("[GOOD] ARM11 Reboot Address");
		#endif
		return 0;
	}
	Debug("[FAIL] ARM11 Reboot Address");
	result++;
	return 1;
}

firmType getRequestedFirm()
{
	extern u32 reqFirmwareStr;
	switch(*((u8*)&reqFirmwareStr + 0x24))
	{
		default:
		case 0x30: return NATIVE_FIRM;
		case 0x31: return TWL_FIRM;
		case 0x32: return AGB_FIRM;
	}
}

void powerFirm(u8* firm)
{
	u32 isNew = 0;

	splashScreen();
	if(getHid() & BUTTON_L1 && isColdBoot) configMenu();

	if(!firm) firm = firmGetFromTitle(getRequestedFirm());
	if(firm)
	{
		if(*((u32*)firm) == 0x4D524946)
		{
			if(curConfig->powDebug)
				memcpy((void*)0x1FF8000, (void*)patches_debugger_bin, (u32)0x3700);

			firmEntry* entry = (firmEntry*)(firm + 0x40);
			if(isNew3DS)
			{
				if(getRequestedFirm() == NATIVE_FIRM)
				{
					patchArm9Loader(firm + (u32)entry[2].data, 0);
					*((u32*)(firm + (u32)entry[2].data + 4)) = (u32)0x0801B01C;
					*((u32*)(firm + 12)) = (u32)entry[2].addr;
				}
				else
				{
					patchArm9Loader(firm + (u32)entry[3].data, 0);
					*((u32*)(firm + (u32)entry[3].data + 4)) = (u32)0x0801301C;
					*((u32*)(firm + 12)) = (u32)entry[3].addr;
				}
				isNew = 0x800;
			}

			switch(getRequestedFirm())
			{
				case NATIVE_FIRM:
				{
					u8 *k9Data = firm + (u32)entry[2].data + isNew, *p9Data = k9Data + 0x15000;
					u32 k9Size = entry[2].size - isNew, p9Size = k9Size - 0x15000;

					arm11Execute(patchLoaderModule (firm + (u32)entry[0].data, entry[0].size));
					arm11Execute(patchFirmPartitionUpdate (p9Data, p9Size));
					arm11Execute(patchFirmLaunch (p9Data, p9Size));
					arm11Execute(patchSignatureChecks (p9Data, p9Size, 1));
					arm11Execute(patchArm9Mpu (k9Data, k9Size));
					arm11Execute(patchArm9KernelCode (k9Data, k9Size));
					arm11Execute(patchArm11Reboot (firm + (u32)entry[1].data, entry[1].size));
					break;
				}
				case TWL_FIRM:
				{
					arm11Execute(patchSignatureChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew, 0));
					// arm11Execute(patchTwlChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew));
					result--;
					break;
				}
				case AGB_FIRM:
				{
					arm11Execute(patchSignatureChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew, 0));
					arm11Execute(patchAgbBootSplash (firm + (u32)entry[3].data + isNew, entry[3].size - isNew));
					break;
				}
			}
		}
		else
		{
			Debug("[ERROR] Could not read FIRM title");
			result++;
		}
	}
	else
	{
		Debug("[ERROR] Could not open FIRM title");
		result++;
	}

	if(result)
	{
		Debug(" ");
		Debug("Press A to exit");
		while(1)
		{
			u32 key = waitHid();
			if(key & BUTTON_A) return;
		}
	}

	#ifdef VERBOSE
		Debug("Launch!");
	#endif

	firmLaunchBin(firm);
}
