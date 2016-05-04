#pragma once

#include "common.h"
#define curConfig ((configData*)0x23F00060)

typedef struct
{
	u8 powAnim;
	u8 powMode;
	u8 powDebug;
} configData;

void configMenu();