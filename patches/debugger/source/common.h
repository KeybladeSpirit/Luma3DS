#pragma once

typedef enum{
	false = 0,
	true = 1
} bool;

#define	u8			unsigned char
#define	u16			unsigned short
#define	u32			unsigned int
#define	u64			unsigned long
#define	vu8			volatile u8
#define	vu16		volatile u16
#define	vu32		volatile u32
#define	vu64		volatile u64
#define	frameBuf	0x18000000
#define backColor	0x00
#define fontColor	0xFF

void drawResetLine();
void drawChar(int character, int x, int y);
void drawStr(char* str);
void strItoa(char* buf, unsigned int val, int base, int w);
void memCpy(void* dst, void* src, u32 size);
void memSet(void* dst, u8 val, u32 size);
void armBranch(void *cur, void *dst);
