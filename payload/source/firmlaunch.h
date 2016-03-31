#pragma once

#include "common.h"

void firmLaunchBin(u8* firmBuffer);
void firmLaunchFile(char* filename, unsigned int offset);
fsFile* findNativeFirm();
u8* decryptNativeFirm();
void launchNativeFirm();