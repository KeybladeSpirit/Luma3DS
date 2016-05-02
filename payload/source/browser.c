#include "common.h"
#include "browser.h"

int fileBrowser(char* folder, void(* func)(char*))
{
	// Searches for all the files in the SD, we hope to find the
	// path of PowerFirm payload.
	DIR dir;
	static FILINFO info;
	char tmp[1024];
	char entryStr[100][256];
	u8 entryAttr[100];
	int entryNum = 0;
	int sel = 0;
	
	info.lfname = (char*)0x21000000;
	info.lfsize = 128 + 1;	
	if(f_opendir(&dir, &folder[0]) == FR_OK)
	{
		while(f_readdir(&dir, &info) == FR_OK)
		{
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
	return 0;
}

void loadPayload(char* path)
{
	extern u32 payloadLoader;	// See _start.s
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