#pragma once

#include "common.h"

typedef enum
{
	NATIVE_FIRM = 0x00000002,
	TWL_FIRM = 0x00000102,
	AGB_FIRM = 0x00000202
} firmType;

void firmLaunchBin(u8* firmBuffer);
void firmLaunchFile(char* filename, unsigned int offset);
fsFile* findTitleContent(u32 tid_low, u32 tid_high);
u8* getFirmFromTitle(firmType tid);
void launchNativeFirm();