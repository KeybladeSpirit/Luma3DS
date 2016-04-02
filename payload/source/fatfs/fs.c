#include "fs.h"
#include "../common.h"

#define MAX_FILES	100

fsFile fileBuf[MAX_FILES];
static FATFS sdmcfs, nandfs;

void fsInit()
{	
	sdmmc_sdcard_init();
	f_mount(&sdmcfs, "sdmc:", 0);
	f_mount(&nandfs, "nand:", 0);
	for(int i = 0; i < MAX_FILES; i++)
	{
		fileBuf[i].active = 0;
	}
}

void fsExit()
{	
	f_mount(0, "sdmc:", 0);
	f_mount(0, "nand:", 0);
	for(int i = 0; i < MAX_FILES; i++)
	{
		fileBuf[i].active = 0;
	}
}

fsFile* fsOpen(char* path, int flag)
{
	fsFile* ret = 0;
	for(int i = 0; i < MAX_FILES; i++)
	{
		if(fileBuf[i].active == 0)
		{
			fileBuf[i].active = 1;
			ret = &fileBuf[i];
			break;
		}
	}
	if(ret)
	{
		if(f_open(&ret->file, path, flag) != FR_OK)
		{
			ret = 0;
		}
	}
	return ret;
}

void fsClose(fsFile* file)
{
	if(file)
	{
		file->active = 0;
		f_close(&file->file);
	}
}

void fsSeek(fsFile* file, unsigned int off)
{
	f_lseek(&file->file, off);
}

unsigned int fsRead(void* ptr, unsigned int size, unsigned int nmemb, fsFile* file)
{
	unsigned int bytes = 0;
	f_read(&file->file, ptr, size*nmemb, &bytes);
	return bytes;
}

unsigned int fsWrite(void* ptr, unsigned int size, unsigned int nmemb, fsFile* file)
{
	unsigned int bytes = 0;
	f_write(&file->file, ptr, size*nmemb, &bytes);
	return bytes;
}

unsigned int fsGetSize(fsFile* file)
{
	return file->file.fsize;
}
