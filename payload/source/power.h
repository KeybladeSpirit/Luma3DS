#pragma once

#include "common.h"

typedef struct
{
	u32 data;
	u32 addr;
	u32 size;
	u32 type;
	u8 hash[0x20];
} firmEntry;

void powerFirm(u8* firm);