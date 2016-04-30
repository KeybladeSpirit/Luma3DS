#pragma once

#include "common.h"
#define curConfig ((configData*)0x23F00060)

typedef struct
{
	u8 bootAnim;
	u8 signPatch;
	u8 gbaAnim;
	u8 excDebug;
	u8 cleanMode;
} configData;

void configMenu();