#pragma once

#include "common.h"

typedef enum
{
	NATIVE_FIRM		= 0x00000002,
	NEW_NATIVE_FIRM	= 0x20000002,
	TWL_FIRM		= 0x00000102,
	NEW_TWL_FIRM	= 0x20000102,
	AGB_FIRM		= 0x00000202,
	NEW_AGB_FIRM	= 0x20000202,
} firmType;

void firmLaunchBin(u8* firmBuffer);
void firmLaunchFile(char* filename, unsigned int offset);
fsFile* findTitleContent(u32 tid_low, u32 tid_high);
u8* getFirmFromTitle(firmType tid);
void launchNativeFirm();