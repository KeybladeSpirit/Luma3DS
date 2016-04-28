#pragma once

void arm11Access();
void hookSwi();
void handleFiq();
void handleInstr();
void handleData();
void handlePrefetch();
void handleException(u32* regs, char* type);
