#define REG_AESCNT				((volatile u32*)0x10009000)
#define REG_AESKEYCNT			((volatile u8 *)0x10009011)
#define REG_AESKEYFIFO			((volatile u32*)0x10009100)
#define AES_CNT_INPUT_ORDER		0x02000000
#define AES_CNT_INPUT_ENDIAN	0x00800000
#define AES_INPUT_BE			(AES_CNT_INPUT_ENDIAN)
#define AES_INPUT_NORMAL		(AES_CNT_INPUT_ORDER)
#define AES_KEYCNT_WRITE		(1 << 7)
#define AES_KEYX				1

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

void aes_setkey(u8 keyslot, const void* key, u32 keyType, u32 mode)
{
	if(keyslot <= 0x03) return; // Ignore TWL keys for now
	u32* key32 = (u32*)key;
	*REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN | AES_CNT_INPUT_ORDER)) | mode;
	*REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | keyslot | AES_KEYCNT_WRITE;

	REG_AESKEYFIFO[keyType] = key32[0];
	REG_AESKEYFIFO[keyType] = key32[1];
	REG_AESKEYFIFO[keyType] = key32[2];
	REG_AESKEYFIFO[keyType] = key32[3];
}

int main()
{
	// This is the most illegal way to do this, hardcoding the keys...
	// This is supposed to keep being private however
	u8 newKeys[8][16] = {
		{0x82, 0xE9, 0xC9, 0xBE, 0xBF, 0xB8, 0xBD, 0xB8, 0x75, 0xEC, 0xC0, 0xA0, 0x7D, 0x47, 0x43, 0x74},
		{0xF5, 0x36, 0x7F, 0xCE, 0x73, 0x14, 0x2E, 0x66, 0xED, 0x13, 0x91, 0x79, 0x14, 0xB7, 0xF2, 0xEF},
		{0xEA, 0xBA, 0x98, 0x4C, 0x9C, 0xB7, 0x66, 0xD4, 0xA3, 0xA7, 0xE9, 0x74, 0xE2, 0xE7, 0x13, 0xA3},
		{0x45, 0xAD, 0x04, 0x95, 0x39, 0x92, 0xC7, 0xC8, 0x93, 0x72, 0x4A, 0x9A, 0x7B, 0xCE, 0x61, 0x82},
		{0xC3, 0x83, 0x0F, 0x81, 0x56, 0xE3, 0x54, 0x3B, 0x72, 0x3F, 0x0B, 0xC0, 0x46, 0x74, 0x1E, 0x8F},
		{0xD6, 0xB3, 0x8B, 0xC7, 0x59, 0x41, 0x75, 0x96, 0xD6, 0x19, 0xD6, 0x02, 0x9D, 0x13, 0xE0, 0xD8},
		{0xBB, 0x62, 0x3A, 0x97, 0xDD, 0xD7, 0x93, 0xD7, 0x57, 0xC4, 0x10, 0x4B, 0x8D, 0x9F, 0xB9, 0x69},
		{0x4C, 0x28, 0xEC, 0x6E, 0xFF, 0xA3, 0xC2, 0x36, 0x46, 0x07, 0x8B, 0xBA, 0x35, 0x0C, 0x79, 0x95}
	};
	
	for(int i = 0; i < 8; i++)
	{
		aes_setkey(0x18 + i, newKeys[i], AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
	}
	
	// Start ARM9 Kernel now
	((void (*)())0x801B01C)();
	
	while(1);
	return 0;
}