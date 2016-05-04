#include "nand.h"
#include "crypto.h"
#include "sdmmc.h"
#include "draw.h"
#include "fatfs/ff.h"

u32 nandGetCtr(u8* ctr, u32 offset)
{
	u8 cid[16];
	sdmmc_get_cid( 1, (uint32_t*)cid);
    if (offset >= 0x0B100000)
	{
		// CTRNAND/AGBSAVE region
		sha(ctr, cid, 0x10, SHA_256_MODE);
    }
	else
	{
		// TWL region
		u8 sha1sum[20];
		sha(sha1sum, cid, 0x10, SHA_1_MODE);
		for(u32 i = 0; i < 16; i++)
			ctr[i] = sha1sum[15-i];
    }
    aes_advctr(ctr, offset / 0x10, AES_INPUT_BE | AES_INPUT_NORMAL);
    return 0;
}

void nandCrypt(u32 sector_no, u32 numsectors, u8 *out, nandRegion section)
{
	u8 keyslot = 0, ctr[0x10];
	switch(section)
	{	
		case TWLN: 		keyslot = 3; break;
		case TWLP:		keyslot = 3; break;
		case AGBSAVE :	keyslot = 7; break;
		case FIRM0:		keyslot = 6; break;
		case FIRM1:		keyslot = 6; break;
		case CTRNAND:	keyslot = 4; break;
		case NEWCTR:	keyslot = 5; break;
	}
	nandGetCtr(ctr, section + sector_no*0x200);
	aes_setiv(ctr, AES_INPUT_BE | AES_INPUT_NORMAL);
    aes_use_keyslot(keyslot);
    aes(out, out, numsectors*0x200/AES_BLOCK_SIZE, ctr, AES_CNT_CTRNAND_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);
}

int nandReadSectors(u32 sector_no, u32 numsectors, u8 *out, nandRegion section)
{
	if(section == CTRNAND && isNew3DS) section = NEWCTR;
	int ret = sdmmc_nand_readsectors(sector_no + section/0x200, numsectors, out);
	nandCrypt(sector_no, numsectors, out, section);
	return ret;
}

int nandWriteSectors(u32 sector_no, u32 numsectors, u8 *out, nandRegion section)
{
	if(section == CTRNAND && isNew3DS) section = NEWCTR;
	nandCrypt(sector_no, numsectors, out, section);
	int ret = sdmmc_nand_writesectors(sector_no + section/0x200, numsectors, out);
	return ret;
}
