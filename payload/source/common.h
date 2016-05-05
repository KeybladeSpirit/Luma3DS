#pragma once

// #define VERBOSE

#define	u8		unsigned char
#define	u16		unsigned short
#define	u32		unsigned int
#define	u64		unsigned long
#define	vu8		volatile u8
#define	vu16	volatile u16
#define	vu32	volatile u32
#define	vu64	volatile u64

#define isNew3DS			(*((volatile u32*)0x10140FFC) == 7)
#define isColdBoot			(*((volatile u32*)0x10010000) == 0)
#define isFirmLaunch		(*((volatile u32*)0x23F00048) != 0)
#define arm11EntryPoint		0x1FFFFFF8
#define arm11Execute(x)		({*((vu32*)arm11EntryPoint) = (u32)x; for(volatile unsigned int _i = 0; _i < 0xF; ++_i); while(*(volatile uint32_t *)arm11EntryPoint != 0);})

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "screen.h"
#include "system/delay.h"
#include "system/hid.h"
#include "system/i2c.h"
#include "system/sdmmc.h"
#include "system/draw.h"
#include "system/crypto.h"
#include "system/nand.h"
#include "fatfs/ff.h"
#include "fatfs/fs.h"

typedef enum
{
	NATIVE_FIRM	= 0x00000002,
	TWL_FIRM	= 0x00000102,
	AGB_FIRM	= 0x00000202,
} firmType;

fsFile*	findTitleContent(u32 tid_low, u32 tid_high);			// Returns a pointer to the highest version CXI file of the requested title.
void	firmLaunchBin(u8* firmBuffer);							// Launches a FIRM binary from memory.
void	firmLaunchFile(char* filename, unsigned int offset);	// Launches a FIRM binary from a file.
u8*		firmGetFromTitle(firmType tid);							// Decrypts a FIRM from the requested title id CXI.
void	firmLaunchNative();										// Launches NATIVE_FIRM contained in the CTRNAND's CXI.
int		fileBrowser(char* folder, void(* func)(char*));			// FileBrowser Dialog : "func" will be executed when a file is selected.
void	loadPayload(char* path);								// Launches an ARM9 payload at address 0x23F00000 (standard).
