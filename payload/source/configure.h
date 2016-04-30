#pragma once

#include "common.h"
#define curConfig ((configData*)0x23F00060)

typedef struct
{
	u8 bootAnim;
	u8 cleanMode;	// This flags works in the opposite way : 1 = off, 0 = on
	u8 signPatch;
	u8 excDebug;
	u8 gbaAnim;
	u8 regionFree;
} configData;

void configMenu();