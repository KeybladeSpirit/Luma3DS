#include <3ds.h>
#include <string.h>
#include "patcher.h"
#include "ifile.h"

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

// Below is stolen from http://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string_search_algorithm

#define ALPHABET_LEN 256
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)
 
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up 
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
static void make_delta1(int *delta1, u8 *pat, int patlen)
{
	int i;
	for (i=0; i < ALPHABET_LEN; i++) {
		delta1[i] = NOT_FOUND;
	}
	for (i=0; i < patlen-1; i++) {
		delta1[pat[i]] = patlen-1 - i;
	}
}
 
// true if the suffix of word starting from word[pos] is a prefix 
// of word
static int is_prefix(u8 *word, int wordlen, int pos)
{
	int i;
	int suffixlen = wordlen - pos;
	// could also use the strncmp() library function here
	for (i = 0; i < suffixlen; i++) {
		if (word[i] != word[pos+i]) {
			return 0;
		}
	}
	return 1;
}
 
// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
static int suffix_length(u8 *word, int wordlen, int pos)
{
	int i;
	// increment suffix length i to the first mismatch or beginning
	// of the word
	for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
	return i;
}
 
// delta2 table: given a mismatch at pat[pos], we want to align 
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with 
// pat[patlen-1].
// 
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
// 
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as 
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
static void make_delta2(int *delta2, u8 *pat, int patlen)
{
	int p;
	int last_prefix_index = patlen-1;
 
	// first loop
	for (p=patlen-1; p>=0; p--) {
		if (is_prefix(pat, patlen, p+1)) {
		last_prefix_index = p+1;
		}
		delta2[p] = last_prefix_index + (patlen-1 - p);
	}
 
	// second loop
	for (p=0; p < patlen-1; p++) {
		int slen = suffix_length(pat, patlen, p);
		if (pat[p - slen] != pat[patlen-1 - slen]) {
		delta2[patlen-1 - slen] = patlen-1 - p + slen;
		}
	}
}
 
static u8* boyer_moore(u8 *string, int stringlen, u8 *pat, int patlen)
{
	int i;
	int delta1[ALPHABET_LEN];
	int delta2[patlen * sizeof(int)];
	make_delta1(delta1, pat, patlen);
	make_delta2(delta2, pat, patlen);
 
	i = patlen-1;
	while (i < stringlen) {
		int j = patlen-1;
		while (j >= 0 && (string[i] == pat[j])) {
			--i;
			--j;
		}
		if (j < 0) {
			return (string + i+1);
		}
 
		i += max(delta1[string[i]], delta2[j]);
	}
	return NULL;
}

static int patch_memory(start, size, pattern, patsize, offset, replace, repsize, count)
	u8* start;
	u32 size;
	u32 patsize;
	u8 pattern[patsize];
	int offset;
	u32 repsize;
	u8 replace[repsize];
	int count;
{
	u8 *found;
	int i;
	u32 at;
	
	for (i = 0; i < count; i++)
	{
		found = boyer_moore(start, size, pattern, patsize);
		if (found == NULL)
		{
			break;
		}
		at = (u32)(found - start);
		memcpy(found + offset, replace, repsize);
		if (at + patsize > size)
		{
			size = 0;
		}
		else
		{
			size = size - (at + patsize);
		}
		start = found + patsize;
	}
	return i;
}

int patch_code(u64 progid, u8 *code, u32 size)
{
	static const char region_free_pattern[]			= {0x00, 0x00, 0x55, 0xE3, 0x01, 0x10, 0xA0, 0xE3};
	static const char region_free_patch[]			= {0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1};
	static const char block_updates_pattern[]		= {0x25, 0x79, 0x0B, 0x99};
	static const char block_updates_patch[]			= {0xE3, 0xA0};
	static const char block_eshop_updates_pattern[]	= {0x30, 0xB5, 0xF1, 0xB0};
	static const char block_eshop_updates_patch[]	= {0x00, 0x20, 0x08, 0x60, 0x70, 0x47};
	static const char stop_updates_pattern[]		= {0x0C, 0x18, 0xE1, 0xD8};
	static const char stop_updates_patch[]			= {0x0B, 0x18, 0x21, 0xC8};
	
	switch(progid)
	{
		case 0x0004003000008F02LL:
		case 0x0004003000008202LL:
		case 0x0004003000009802LL:
		case 0x000400300000A102LL:
		case 0x000400300000A902LL:
		case 0x000400300000B102LL:
		{
			// Home Menu
			if(*((u8*)(0x14000004)))
				patch_memory(code, size, region_free_pattern, sizeof(region_free_pattern), -16, region_free_patch, sizeof(region_free_patch), 1);
			break;
		}
		case 0x0004013000002C02LL:
		{
			// NIM
			patch_memory(code, size, block_updates_pattern, sizeof(block_updates_pattern), 0, block_updates_patch, sizeof(block_updates_patch), 1);
			patch_memory(code, size, block_eshop_updates_pattern, sizeof(block_eshop_updates_pattern), 0, block_eshop_updates_patch, sizeof(block_eshop_updates_patch), 1);
			break;
		}
		case 0x0004013000008002LL:
		{
			// NS
			patch_memory(code, size, stop_updates_pattern, sizeof(stop_updates_pattern), 0, stop_updates_patch, sizeof(stop_updates_patch), 2);
			break;
		}
		case 0x0004001000020000LL:
		case 0x0004001000021000LL:
		case 0x0004001000022000LL:
		case 0x0004001000026000LL:
		case 0x0004001000027000LL:
		case 0x0004001000028000LL:
		{
			// System Settings
			patch_memory(code, size, (char*)L"Ver.", 8, 0, (char*)L"%d.%d.%d-%d%ls #POW", 38, 1);
			break;
		}
	}
	return 0;
}