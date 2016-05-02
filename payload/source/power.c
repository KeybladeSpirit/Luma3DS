#include "power.h"
#include "firmlaunch.h"
#include "patches.h"
#include "fatfs/ff.h"
#include "configure.h"

// Below is stolen from http://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string_search_algorithm

#define ALPHABET_LEN 256
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)
 
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up 
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
static void make_delta1(int *delta1, u8 *pat, int patlen)
{
	int i;
	for (i=0; i < ALPHABET_LEN; i++) {
		delta1[i] = NOT_FOUND;
	}
	for (i=0; i < patlen-1; i++) {
		delta1[pat[i]] = patlen-1 - i;
	}
}
 
// true if the suffix of word starting from word[pos] is a prefix 
// of word
static int is_prefix(u8 *word, int wordlen, int pos)
{
	int i;
	int suffixlen = wordlen - pos;
	// could also use the strncmp() library function here
	for (i = 0; i < suffixlen; i++) {
		if (word[i] != word[pos+i]) {
			return 0;
		}
	}
	return 1;
}
 
// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
static int suffix_length(u8 *word, int wordlen, int pos)
{
	int i;
	// increment suffix length i to the first mismatch or beginning
	// of the word
	for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
	return i;
}
 
// delta2 table: given a mismatch at pat[pos], we want to align 
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with 
// pat[patlen-1].
// 
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
// 
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as 
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
static void make_delta2(int *delta2, u8 *pat, int patlen)
{
	int p;
	int last_prefix_index = patlen-1;
 
	// first loop
	for (p=patlen-1; p>=0; p--) {
		if (is_prefix(pat, patlen, p+1)) {
		last_prefix_index = p+1;
		}
		delta2[p] = last_prefix_index + (patlen-1 - p);
	}
 
	// second loop
	for (p=0; p < patlen-1; p++) {
		int slen = suffix_length(pat, patlen, p);
		if (pat[p - slen] != pat[patlen-1 - slen]) {
		delta2[patlen-1 - slen] = patlen-1 - p + slen;
		}
	}
}
 
static u8* boyer_moore(u8 *string, int stringlen, u8 *pat, int patlen)
{
	int i;
	int delta1[ALPHABET_LEN];
	int delta2[patlen * sizeof(int)];
	make_delta1(delta1, pat, patlen);
	make_delta2(delta2, pat, patlen);
 
	i = patlen-1;
	while (i < stringlen) {
		int j = patlen-1;
		while (j >= 0 && (string[i] == pat[j])) {
			--i;
			--j;
		}
		if (j < 0) {
			return (string + i+1);
		}
 
		i += max(delta1[string[i]], delta2[j]);
	}
	return NULL;
}

// Currently using : Boyer Moore Alghorithm
u8* memSearch(u8* memstart, u8* memend, u8* memblock, u32 memsize)
{
	return boyer_moore(memstart, (int)(memend - memstart), memblock, (int)memsize);
}

#define ARM9ADDR 0x08006800

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
		u32 process9CodeOffset = (buffer - data - 0x110) + ARM9ADDR + (*((u32*)(buffer + 0x90)) + 1)*0x200;
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
		u32 fopenAddr = (buffer - data) + ARM9ADDR + getProcess9Shift(data, size) + 8 - ((((*((u32*)buffer) & 0x00FFFFFF) << 2)*(-1)) & 0xFFFFF);
		memcpy(buffer, patches_firmlaunch_bin, patches_firmlaunch_bin_size);
		*((u32*)memSearch(buffer, buffer + patches_firmlaunch_bin_size, (u8[]){0xED, 0x0D, 0xDC, 0xBA}, 4)) = fopenAddr + 1;
		getPayloadPath(path, "");

		for(int i = 0; i < strlen(path); i++)
		{
			*(buffer + patches_firmlaunch_bin_size - 0x80 + i*2) = (u8)path[i];
		}
		//Debug("[GOOD] FirmLaunch Patch");
		return 0;
	}
	Debug("[FAIL] FirmLaunch Patch");
	return 1;
}

int patchSignatureChecks(u8* data, u32 size)
{
	if(!curConfig->cleanMode) return 0;
	if(!curConfig->signPatch) return 0;
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
		return 1;
	}
	Debug("[FAIL] Signature Checks Patch");
	return 2;
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
	if(!curConfig->cleanMode) return 0;
	if(!curConfig->excDebug) return 0;
	u8 stockCodeK[] = {0x03, 0x00, 0x2D, 0xE9, 0xD3, 0xF0, 0x21, 0xE3, 0x0D, 0x00, 0xA0, 0xE1, 0xD2, 0xF0, 0x21, 0xE3};
	u8* buffer = memSearch(data, data + size, stockCodeK, 16);
	if(buffer)
	{
		buffer += 0x10;
		u32 kernelReturn = (buffer - data) + 8 + ARM9ADDR;
		*((u32*)(buffer + 0)) = (u32)0xE51FF004;	// LDR PC, [PC,#-4]
		*((u32*)(buffer + 4)) = (u32)0x0801A500;	// Our Code
		u32 magicWord = 0xDEADC0DE;	
		buffer = data + 0x13D00;
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
	if(!curConfig->cleanMode) return 0;
	if(!curConfig->excDebug) return 0;
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
	cryptArm9Bin(data);
	memset((void*)data, 0x00, 0x800);
	memcpy((void*)data, patches_arm9loader_bin, patches_arm9loader_bin_size);
	return 0;
}

int patchAgbBootSplash(u8* data, u32 size)
{
	if(!curConfig->gbaAnim) return 0;
	if(!curConfig->cleanMode) return 0;
	u8 stockCode[] = {0x00, 0x00, 0x01, 0xEF};
	u8* buffer = memSearch(data, data + size, stockCode, 4);
	if(buffer)
	{
		*((u32*)(buffer)) = 0xEF260000;
		//Debug("[GOOD] ARM9 AGB Boot Splash");
		return 0;

	}
	Debug("[FAIL] ARM9 AGB Boot Splash\n");
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
	
	if(res) Debug("[FAIL] ARM9 TWL Checks Bypass\n");
	//else if(res) Debug("[GOOD] ARM9 TWL Checks Bypass\n");
	
	return res;
}

int patchLoaderModule(u8* data, u32 size)
{
	if(!curConfig->cleanMode) return 0;
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
		*(buffer + 0xC02) = curConfig->regionFree;
		//Debug("[GOOD] Loader Module Hack");
		return 0;		
	}
	Debug("[FAIL] Loader Module Hack");
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

extern const unsigned char font[];

void drawGiantLetter(int character, int x, int y, int xScale, int yScale)
{
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
	if(!curConfig->bootAnim) return;
	ClearScreenFull(1, 1);
	DrawString(TOP_SCREEN0, "[L] : Options", 10, 10, COLOR_WHITE, COLOR_BLACK);
	DrawString(TOP_SCREEN0, VERSION, 10, 220, COLOR_WHITE, COLOR_BLACK);
	DrawString(TOP_SCREEN0, "@2016, Jason Dellaluce", 208, 220, COLOR_WHITE, COLOR_BLACK);
	drawSplashString("PowerFirm", 88, 1);
	drawSplashString("3DS", 128, 0);
}

void powerFirm(u8* firm)
{
	u32 isNew = 0, res = 0;
	
	splashScreen();
	if(getHid() & BUTTON_L1 && isColdBoot) configMenu();
	
	if(!firm) firm = getFirmFromTitle(getRequestedFirm());	
	if(firm)
	{
		if(*((u32*)firm) == 0x4D524946)
		{
			firmEntry* entry = (firmEntry*)(firm + 0x40);
			if(isNew3DS)
			{
				if(getRequestedFirm() == NATIVE_FIRM)
				{
					res += patchArm9Loader(firm + (u32)entry[2].data, 0);
					*((u32*)(firm + (u32)entry[2].data + 4)) = (u32)0x0801B01C;
					*((u32*)(firm + 12)) = (u32)entry[2].addr;
				}
				else
				{
					res += patchArm9Loader(firm + (u32)entry[3].data, 0);
					*((u32*)(firm + (u32)entry[3].data + 4)) = (u32)0x0801301C;
					*((u32*)(firm + 12)) = (u32)entry[3].addr;
				}
				isNew = 0x800;
			}
			
			switch(getRequestedFirm())
			{
				case NATIVE_FIRM:
				{
					res += patchLoaderModule (firm + (u32)entry[0].data, entry[0].size);
					res += patchFirmPartitionUpdate (firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
					res += patchFirmLaunch (firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
					res += patchSignatureChecks (firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
					res += patchArm9Mpu (firm + (u32)entry[2].data + isNew, entry[2].size - isNew);		
					res += patchArm9KernelCode (firm + (u32)entry[2].data + isNew, entry[2].size - isNew);
					break;
				}
				case TWL_FIRM:
				{
					patchSignatureChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew);
					// patchTwlChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew);
					break;
				}
				case AGB_FIRM:
				{
					patchSignatureChecks (firm + (u32)entry[3].data + isNew, entry[3].size - isNew);
					patchAgbBootSplash (firm + (u32)entry[3].data + isNew, entry[3].size - isNew);
					break;
				}
			}		
		}
		else
		{
			Debug("[ERROR] Could not read FIRM title");
			res++;
		}
	}
	else
	{
		Debug("[ERROR] Could not open FIRM title");
		res++;
	}
	
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

