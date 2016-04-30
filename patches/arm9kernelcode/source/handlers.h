#pragma once

void arm11Access();
void hookSwi();
void handleFiq();
void handleInstr();
void handleData();
void handlePrefetch();
void handleException(u32* regs, char* type, char* cpu);

void armBranch(void *cur, void *dst);
void memcpy(void* dst, void* src, u32 size);
void memset(void* dst, u8 val, u32 size);
