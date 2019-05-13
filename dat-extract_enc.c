/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <zlib.h>

#include "kojimahash/kojimahash.h"
#include "stage-dictionary.h"


#ifdef _WIN32
#include <direct.h>
#define createDirectory(dirname) mkdir(dirname)
#define cleanExecPath(path) *strrchr(path, '\\') = 0
#define bswap_16(val) \
 ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

// Swap 4 byte, 32 bit values:

#define bswap_32(val) \
 ( (((val) >> 24) & 0x000000FF) | (((val) >>  8) & 0x0000FF00) | \
   (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )

// Swap 8 byte, 64 bit values:

#define bswap_64(val) \
 ( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
   (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | \
   (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
   (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <byteswap.h>
#define cleanExecPath(path) *strrchr(path, '/') = 0
#define createDirectory(dirname) mkdir(dirname, 0777)
#endif

#define BREAK(x) { printf(x); return 1; }

#define RETPRINT(x) { sprintf (retstring, "%s", x); break; }

uint32_t key1 = 0x02E90EDD;
uint32_t key2 = 0x7A88FB59;
uint32_t key34 = 0xA78925D9;
uint32_t key5 = 0xFFFFFFFF; /* generated later? */
uint32_t key6 = 0x00009385;
uint32_t key7 = 0x00000116;
uint32_t key8 = 0x00006576;

uint32_t initialkey = 0;

uint32_t initialkeybig = 0;

enum games {
	mgs3,
	mgs4,
	mgs2,
	zoe2,
	NUMGAMES
};

enum flags {
	GAME_MGS2 =	(1<<0),
	GAME_MGS3 =	(1<<1),
	GAME_MGS4 =	(1<<2),
	GAME_ZOE2 =	(1<<3),
	GAME_TTS = 	(1<<4),
	GAME_MGS3S = 	(1<<5),
	TRIALSTAGE =	(1<<6),
	ENCRYPTED =	(1<<7),
	COMPRESSED =	(1<<8),
	NUMFLAGS
};

/* defining shortcuts regarding flags */
#define IS_BIGENDIAN (GAME_MGS4|GAME_TTS)

#define IS_BIGSTAGE (GAME_MGS4|GAME_ZOE2|GAME_MGS3S)
#define IS_SMALLENTRY (GAME_MGS2|GAME_MGS3)
#define IS_BIGENTRY GAME_ZOE2
#define IS_VERYBIGENTRY GAME_MGS4

enum sizes {
	small,
	big,
	NUMSIZES
};

dicentry *commondic = NULL, *stagedic = NULL;
int numcommondic = 0, numstagedic = 0, numdicentries = 0;

/* only the following are known good:
   gcx, txn, lt3, mtar, mtsq, geom, mdn, van, cnp, zon, nv2, phs, eqpp, sds, rvb, gsp, dld, rdv, octt, octl,
   pdl, ptl, cpef, jpg, la2, vlm, var, img, vib, raw
*/

/* mgs3 is 0, mgs4 is 1 */
/* apparently 0x60 is slot as well, but this breaks things */
char *extList[2][256] = {
	{
		"sar", "bin", "gcx", "tri", "mdh", "05", "lt2", "07", "mtar", "mtsq", "far", "mtcm", "geom", "mdl", "0e", "nav",
		"cvd", "cnp", "zon", "rpd", "abc", "nv2", "spu", "fcv", "phs", "eqpp", "phpr", "phes", "sds", "vab", "ssp", "rvb",
		"gsp", "21", "drv", "octt", "octl", "25", "26", "27", "28", "pdl", "ptl", "cpef", "2c", "2d", "2e", "2f",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "jpg", "ico", "la2",
		"60", "vpo", "fpo", "cv4", "mcl", "vlm", "lh4", "csr", "var", "img", "vib", "rat", "rcm", "ola", "raw", "mtra",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "slot", "7e", "7f",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
		"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "aio just b", "ac", "ad", "ae", "af",
		"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
		"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
		"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
		"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
		"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "psq"
	},
	{
		"sar", "bin", "gcx", "txn", "mdh", "mds", "lt3", "cv2", "mtar", "mtsq", "mtfa", "mtcm", "geom", "mdn", "0e", "nav",
		"van", "eft", "zon", "rpd", "abc", "nv2", "spu", "fcv", "phs", "eqpp", "phpr", "phes", "sds", "vab", "ssp", "rvb",
		"gsp", "dlz", "drv", "octt", "octl", "25", "26", "27", "28", "pdl", "ptl", "cpef", "2c", "2d", "2e", "2f",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "jpg", "ico", "la2",
		"60", "vpo", "fpo", "cv4", "mcl", "vlm", "lh4", "csr", "var", "img", "vib", "rat", "rcm", "ola", "raw", "mtra",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "slot", "7e", "7f",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
		"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
		"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
		"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
		"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
		"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
		"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "psq"
	}
};

char *extListZoe2[] = {
	"var", "rpd", "atr", "o2d", "mts", "eft", "scx", "pic", "ric", "hz2", "trz",
	"lit", "tex", "brk", "hzt", "mdz", "mtz", "cvz", "mdl", "mtl", "fnt", "nhz",
	"mfl", "flw", "ene", "row", "bin"
};

char *dateTimeString = "%d.%m.%Y %H:%M:%S";
char *datHeaderInfoString = "builddate %s UTC (%08x) - version %04x - numblocks %04x - numstages %03d (%04x) - game %04x - hash %08x\n";

void getDictionary(dicentry **dic, int *num, char *path, char *name, unsigned int flags) {
	char tempname[256], *token = NULL;
	int i, j;
	if(flags & GAME_MGS2) *num = loaddic(dic, path, name, DIC_HASH_SINGLE_EXT | DIC_ADJUST_EXT_MGS2, hashstring24);
	if(flags & GAME_ZOE2) *num = loaddic(dic, path, name, DIC_HASH_FULL_NAME, hashstring32);
	if(flags & (GAME_MGS3|GAME_MGS4)) {
		*num = loaddic(dic, path, name, DIC_USE_EXTERNAL, hashstring24);
		for(i = 0; i < *num; i++) {
			strcpy(tempname, (*dic)[i].name);
			token = strtok(tempname, ".");
			token = strtok(NULL, ".");
			for(j = 0; j < 256; j++) {
				if(!strcmp(token, extList[(flags & GAME_MGS4) ? 1 : 0][j])) {
					(*dic)[i].extension = j;
					printf("set extension of entry %d to %08x\n", i, j);
					break;
				}
			}
			if(j > 255) printf("unknown extension %s\n", token);
		}
	}
	if(*num > 0) numdicentries += *num;
}

unsigned int matchhash( uint32_t hash, uint32_t extension, dicentry *dictionary, int numentries ) {
	int i;
	for( i = 0; i < numentries; i++ ) {
		if((dictionary[i].hash == hash) && (dictionary[i].extension == extension)) return i;
	}
	return 0xFFFFFFFF;
}

char *getExtensionFromHash32(uint32_t hash) {
	char *retstring = malloc(16);
	memset(retstring, 0, 16);
	unsigned int i, testhash;
	for(i = 0; i < 27; i++) {
		testhash = hashstring32(extListZoe2[i]);
		if(testhash == hash) RETPRINT(extListZoe2[i])
	}
	if(strlen(retstring) < 3) sprintf(retstring, "%08x", hash);
	return retstring;
}


char *getFolderNameFromHash(uint32_t hash, unsigned int flags) {
	char *retstring = malloc(16);
	switch(hash) {
		case 0x7F000010: RETPRINT("sound")
		case 0x7F010000: RETPRINT("nocache")
		case 0x7F000002: RETPRINT("cache")
		case 0x7F000003: RETPRINT("resident")
		case 0x7F000004: RETPRINT(flags & GAME_TTS ? "tts-04" : "delayload")
		case 0x7F000005: RETPRINT(flags & GAME_TTS ? "tts-05" : "delayload_w")
		default: {
			sprintf(retstring, "%08x", hash);
			break;
		}
	}
	return retstring;
}

char *getFileNameFromHashes(uint32_t hash, uint32_t extension, unsigned int flags) {
	char *retstring = malloc(64);
	char *extensionstring = NULL;
	unsigned int result = matchhash(hash, extension, stagedic, numstagedic);
	if(result != 0xFFFFFFFF) sprintf(retstring, "%s", stagedic[result].name);
	else {
		result = matchhash(hash, extension, commondic, numcommondic);
		if(result != 0xFFFFFFFF) sprintf(retstring, "%s", commondic[result].name);
		else {
			if(flags & GAME_MGS2) sprintf(retstring, "%08x.%c", hash, extension+0x61 /* 'a' */);
			/* see extlist comment for info on how games are sorted */
			else if(flags & (GAME_MGS3|GAME_MGS4)) sprintf(retstring, "%08x.%s", hash, extList[(flags & GAME_MGS4) ? 1 : 0][extension]);
			else if(flags & GAME_ZOE2) {
				extensionstring = getExtensionFromHash32(extension);
				sprintf(retstring, "%08x.%s", hash, extensionstring);
				free(extensionstring);
			}
		}
	}
	return retstring;
}

/* length is number of uint32_t to decrypt, not number of bytes! (make sure data is multiples of 4 bytes) */
unsigned int decryptHeader(unsigned char *data, unsigned int length, unsigned int flags) {
	uint32_t *values = (uint32_t *)data;
	uint32_t seed, workkey;
	if(flags & IS_BIGENDIAN) workkey = initialkeybig;
	else workkey = initialkey;
	unsigned int i;
	for(i = 0; i < length ;i++) {
		seed = workkey * key1;
		values[i] ^= workkey;
		if(flags & IS_BIGENDIAN) workkey = seed + (initialkeybig ^ 0x0000F0F0);
		else workkey = seed + (initialkey ^ 0x0000F0F0);
	}
	return 1;
}

unsigned int decryptStage(unsigned char *data, unsigned int length, char *name, unsigned int flags) {
	uint32_t *values = (uint32_t *)data;
	uint32_t seed, workkey, namehash, workkeyt;
	unsigned int i;
	if(flags & GAME_ZOE2) namehash = hashstring32(name);
	else namehash = hashstring24(name);
	workkeyt = (namehash << 0x07) + namehash + key2;
	if(flags & IS_BIGENDIAN) workkey = (namehash << 0x07) + initialkeybig + namehash + key34;
	else workkey = (namehash << 0x07) + initialkey + namehash + key34;
	for(i = 0; i < length; i++) {
		seed = workkey * key1;
		values[i] ^= workkey;
		workkey = seed + workkeyt;
	}
	return 1;
}

unsigned int decryptContent(unsigned char *data, unsigned int length) {
	uint32_t *values = (uint32_t *)data;
	uint32_t seed, workkey, workkeyt, workkeyv;
	unsigned int i;
	workkeyv = key5 ^ key6;
	workkeyt = workkeyv * key7;
	workkey = workkeyv | ((workkeyv ^ key8) << 0x10);
	for(i = 0; i < length; i++) {
		seed = workkey * key1;
		values[i] ^= workkey;
		workkey = seed + workkeyt;
	}
	return 1;
}

int writeFile( FILE *input, int length, FILE *output ) {
	unsigned char dataBuffer[1024];
	unsigned int bytesLeft = length;
	
	while(bytesLeft) {
		unsigned int wantedRead;
		if(bytesLeft >= sizeof(dataBuffer))
			wantedRead = sizeof(dataBuffer);
		else
			wantedRead = bytesLeft;
		unsigned int haveRead = fread(dataBuffer, 1, wantedRead, input);
		if(haveRead != wantedRead) {
			printf("haveRead != wantedRead: %d != %d\n", haveRead, wantedRead);
			perror("This broke");
			return 0;
		}

		unsigned int haveWrite = fwrite(dataBuffer, 1, haveRead, output);
		if(haveWrite != haveRead) {
			printf("haveWrite != haveRead: %d != %d\n", haveWrite, haveRead);
			return 0;
		}
		
		bytesLeft -= haveRead;
	}
	return 1;
}

/*	the length of name depends on the game:
	 8 for MGS2 and 3
	16 for everything after */

typedef struct {
	char *name;
	uint32_t offset;
} tocStageEntry;

typedef struct {
	char name[8];
	uint32_t offset;
}__attribute__((packed)) tocStageEntrySmall;

typedef struct {
	char name[16];
	uint32_t offset;
}__attribute__((packed)) tocStageEntryBig;

typedef struct {
	uint32_t size;
	unsigned char *data;
} stageinfo;

typedef struct {
	uint32_t hash;
	uint32_t extension;
	uint32_t offset;
	uint32_t compressed;
} fileinfo;

typedef struct {
	uint32_t hash;
	uint32_t offset;
}__attribute__((packed)) fileinfosmall;

typedef struct {
	uint32_t extension;
	uint32_t hash;
	uint32_t offset;
}__attribute__((packed)) fileinfobig;

typedef struct {
	uint32_t offset;
	uint32_t hash;
	uint32_t compressed;
	uint32_t null;
}__attribute__((packed)) fileinfoverybig;

typedef struct {
	uint16_t version;
	uint16_t numblocks;
	uint16_t numStages;
	uint16_t game;
	uint32_t unknownhash;
}__attribute__((packed)) tocDatHeader;

unsigned char signatures[][6] = {
	{ 0x10, 0x40, 0xC3, 0x06, 0x09, 0x00 }, /* SLES-50383 MGS2 */
	{ 0x10, 0x40, 0xB8, 0x6D, 0x09, 0x00 }, /* SLES-82009 MGS2 Substance MGS2/STAGE.DAT */
	{ 0x10, 0x40, 0x64, 0xC4, 0x0E, 0x00 }, /* SLES-82009 MGS2 Substance MGS2/STAGE2.DAT */
	{ 0xBB, 0x2A, 0xEC, 0x20, 0x11, 0x00 }, /* SLES-82010 Document of MGS2 DOCUMENT/STAGE.DAT */
	{ 0xBB, 0x2A, 0x1A, 0x91, 0x00, 0x00 }, /* SLES-82010 Document of MGS2 MGS2/STAGE.DAT */
	{ 0x06, 0x08, 0x0A, 0x12, 0x06, 0x00 }, /* SLES-51113 ZOE2 SE */
};

/*	takes the tocStageEntry, gets the stage index size and data and decrypts it
	returns a pointer to an array of numstages stageinfo structs */

stageinfo *processStages( FILE *f, tocStageEntry *stages, unsigned int numstages, unsigned int flags) {
	stageinfo *retinfo = malloc(sizeof(stageinfo)*numstages);
	unsigned int i;
	unsigned char *tempdata = NULL;
	for(i = 0; i < numstages; i++) {
		fseek(f, stages[i].offset*0x800, SEEK_SET);
		fread(&retinfo[i].size, 4, 1, f);
		if(flags & ENCRYPTED) decryptStage((unsigned char *)&retinfo[i].size, 1, stages[i].name, flags);
		
		if(flags & IS_BIGENDIAN) retinfo[i].size = bswap_32(retinfo[i].size);
		
		if(flags & IS_SMALLENTRY) retinfo[i].size *= 8;
		if(flags & IS_BIGENTRY) retinfo[i].size *= 12;
		if(flags & IS_VERYBIGENTRY) retinfo[i].size *= 16;
		
		retinfo[i].size += 4;
		retinfo[i].data = malloc(retinfo[i].size-4);
		if(flags & ENCRYPTED) {
			tempdata = malloc(retinfo[i].size);
			/* if a STAGE, seek back, read _all_ data into memory and decrypt it */
			fseek(f, stages[i].offset*0x800, SEEK_SET);
			fread(tempdata, retinfo[i].size, 1, f);
			decryptStage(tempdata, (retinfo[i].size/4), stages[i].name, flags);
			memcpy(retinfo[i].data, tempdata+4, retinfo[i].size-4);
			free(tempdata);
		}
		/* if not a STAGE, just read the unencrypted data */
		else fread(retinfo[i].data, retinfo[i].size-4, 1, f);
	}
	return retinfo;
}

/*	takes a stageinfo and tocStageEntry, creates subdirectories, decrypts and decompresses the contents of the stage if necessary
	takes the quirks of unencrypted and uncompressed DATs as well as the trial into account */

void processInfoWithStage(FILE *f, stageinfo info, tocStageEntry stage, unsigned int flags) {
	FILE *o = NULL;
	char outputname[64] = {0};
	char *foldername = NULL, *filename = NULL;
	uint32_t folderhash = 0;
	uint32_t name = 0, extension = 0, offset = 0, size = 0;
	fileinfoverybig *verybigfiles = NULL;
	fileinfobig *bigfiles = NULL;
	fileinfosmall *smallfiles = NULL;
	fileinfo *files = NULL;
	
	unsigned int i, j, dataoffset = stage.offset*0x800, unpackedoffset, toread, numentries;
	long unsigned int towrite;
	
	if(info.size%0x800) dataoffset += info.size + (0x800-(info.size%0x800));
	else dataoffset += info.size;
	
	/* the trial requires a shift in offsets */
	unpackedoffset = (flags & TRIALSTAGE) ? (dataoffset-0x800) : dataoffset;
	
	if(flags & IS_VERYBIGENTRY) {
		verybigfiles = (fileinfoverybig *)info.data;
		numentries = (info.size-4)/16;
		files = malloc(sizeof(fileinfo)*numentries);
		for(i = 0; i < numentries; i++) {
			files[i].hash = bswap_32(verybigfiles[i].hash);
			files[i].extension = bswap_32(verybigfiles[i].hash);
			files[i].compressed = bswap_32(verybigfiles[i].compressed);
			files[i].offset = bswap_32(verybigfiles[i].offset);
		}
	}
	
	else if(flags & IS_BIGENTRY) {
		bigfiles = (fileinfobig *)info.data;
		numentries = (info.size-4)/12;
		files = malloc(sizeof(fileinfo)*numentries);
		for(i = 0; i < numentries; i++) {
			files[i].hash = bigfiles[i].hash;
			files[i].extension = bigfiles[i].extension;
			files[i].offset = bigfiles[i].offset;
		}
	}
			
	else {
		smallfiles = (fileinfosmall *)info.data;
		numentries = (info.size-4)/8;
		files = malloc(sizeof(fileinfo)*numentries);
		if(flags & IS_BIGENDIAN) {
			for(i = 0; i < numentries; i++) {
				files[i].hash = bswap_32(smallfiles[i].hash);
				files[i].extension = bswap_32(smallfiles[i].hash);
				files[i].offset = bswap_32(smallfiles[i].offset);
			}
		}
		else {
			for(i = 0; i < numentries; i++) {
				files[i].hash = smallfiles[i].hash;
				files[i].extension = smallfiles[i].hash;
				files[i].offset = smallfiles[i].offset;
			}
		}
	}
	
	unsigned char *comdata = NULL, *uncomdata = NULL;
	uint32_t *comvalues = NULL;
	
	snprintf(outputname, 64, "%s.hdr", stage.name);
	o = fopen(outputname, "wb");
	fwrite(info.data, info.size-4, 1, o);
	fclose(o);
	
	printf("%s: ", stage.name);
	
	i = 0;
	
	while((files[i].extension && files[i].offset) || ((flags & IS_VERYBIGENTRY) && files[i].extension)) {
		
		folderhash = files[i].extension;
		foldername = getFolderNameFromHash(folderhash, flags);
		
		if(!(flags & GAME_TTS) && ((files[i].extension == 0x7F000010) || (files[i].extension == 0x7F000005) || (files[i].extension == 0x7F000004))) {
			//~ printf("extracting plain...");
			if(flags & IS_VERYBIGENTRY) {
				uncomdata = malloc(files[i+1].offset);
				towrite = files[i+1].offset;
				snprintf(outputname, 64, "%s/%s", stage.name, foldername);
				createDirectory(outputname);
			}
			else {
				uncomdata = malloc(files[i].offset);
				towrite = files[i].offset;
				snprintf(outputname, 64, "%s/", stage.name);
				createDirectory(outputname);
			}
			
			i++;
			
			/* in unencrypted and trial files, this seems to be a thing */
			if(!(flags & ENCRYPTED) && !(flags & GAME_TTS)) unpackedoffset += 0x800;
			
			fseek(f, unpackedoffset, SEEK_SET);
			fread(uncomdata, towrite, 1, f);
			while(files[i].extension != 0x7F000000) {
				if(flags & IS_BIGENTRY) name = files[i].extension;
				else name = files[i].hash & 0x00FFFFFF;
				if(flags & IS_VERYBIGENTRY) {
					extension = files[i].hash >> 24;
					offset = files[i+1].offset;
					size = files[i+2].offset - files[i+1].offset;
					filename = getFileNameFromHashes(name, extension, flags);
				}
				else {
					offset = files[i].offset;
					size = files[i+1].offset - files[i].offset;
				}
				
				if(flags & IS_VERYBIGENTRY) snprintf(outputname, 64, "%s/%s/%s", stage.name, foldername, filename);
				else if(flags & GAME_MGS3) snprintf(outputname, 64, "%s/%05x.psq", stage.name, name);
				else snprintf(outputname, 64, "%s/pk%06x.sdx", stage.name, name);
				if( !(o = fopen( outputname, "wb" ))) {
					printf("Couldnt open file %s\n", outputname);
					return;
				}
				fwrite(uncomdata+offset, size, 1, o);
				fclose(o);
				i++;
				if(filename) {
					free(filename);
					filename = NULL;
				}
			}
			free(uncomdata);
			uncomdata = NULL;
			if(flags & IS_VERYBIGENTRY) unpackedoffset += files[i+1].offset;
			else unpackedoffset += files[i].offset;
			if(unpackedoffset%0x800) unpackedoffset += 0x800-(unpackedoffset%0x800);
		}
		else {
			snprintf(outputname, 64, "%s/%s", stage.name, foldername);
			createDirectory(outputname);
			
			if(flags & COMPRESSED) {
				if(flags & IS_VERYBIGENTRY) {
					fseek(f, unpackedoffset, SEEK_SET);
					
					i++;
					uncomdata = malloc(files[i].offset);
					towrite = files[i].offset;
					if(files[i].compressed%4) toread = files[i].compressed + (4-(files[i].compressed%4));
					else toread = files[i].compressed;
				}
				else {
					uncomdata = malloc(files[i].offset);
					towrite = files[i].offset;
					i++;
					if((files[i].extension&0x00FFFFFF)%4) toread = (files[i].extension&0x00FFFFFF) + (4-((files[i].extension&0x00FFFFFF)%4));
					else toread = (files[i].extension&0x00FFFFFF);
					fseek(f, dataoffset+files[i].offset, SEEK_SET);
				}
				
				comdata = malloc(toread);
				
				comvalues = (uint32_t *)comdata;
				fread(comdata, toread, 1, f);
				
				if(flags & ENCRYPTED) {
					//~ printf("decrypting...");
					key5 = comvalues[0] & 0x0000FFFF;
					decryptContent(comdata, toread/4);
					
					j = comvalues[0] & 0x0000FFFF;
					j += 0x8F3;
					comvalues[0] = (comvalues[0] & 0xFFFF0000) | (j & 0x0000FFFF);
				}
				//~ printf("decompressing...");
				if((j = uncompress(uncomdata, &towrite, comdata, toread)) != Z_OK) {
					printf("\n=====================================couldnt decompress data :");
					switch(j) {
						case Z_MEM_ERROR: {
							printf("memory error\n");
							break;
						}
						case Z_NEED_DICT: {
							printf("need dictionary\n");
							break;
						}
						case Z_DATA_ERROR: {
							printf("input corrupted\n");
							//~ printf("%s\n", strm.msg);
							break;
						}
						case Z_STREAM_ERROR: {
							printf("broken stream\n");
							break;
						}
						case Z_BUF_ERROR: {
							printf("buffer error\n");
							break;
						}
						default: {
							printf("unknown error\n");
							break;
						}
					}
					printf("\ntowrite %08lx\n", towrite);
					return;
				}
				i++;
			}
			else {
				uncomdata = malloc(files[i].offset);
				towrite = files[i].offset;
				fseek(f, unpackedoffset, SEEK_SET);
				fread(uncomdata, towrite, 1, f);
				i++;
			}
			
			//~ printf("extracting decompressed...");
			
			while(files[i].extension != 0x7F000000) {
				if(flags & IS_BIGENTRY) {
					name = files[i].hash;
					extension = files[i].extension;
				}
				else {
					name = files[i].hash & 0x00FFFFFF;
					extension = files[i].hash >> 24;
				}
				
				if(extension == 0x7D) { // slot file
					//~ printf("hash %08x - extension %08x - offset %08x - compressed %08x\n", files[i].hash, files[i].extension, files[i].offset, files[i].compressed);
		//~ printf("%s - i %d\n", stage.name, i);
					//~ printf("encountered subdir\n");
					//~ subdirhash = name;
					// name.slot
					
					filename = getFileNameFromHashes(name, extension, flags);
					
					snprintf(outputname, 64, "%s/%s/%s.txt", stage.name, foldername, filename);
					
					//~ offset = files[i].offset;
					//~ snprintf(outputname, 64, "%s/%s/%06x/", stage.name, foldername, subdirhash);
					//~ createDirectory(outputname);
					//~ snprintf(outputname, 64, "%s/%s/%06x.%02x", stage.name, foldername, name, extension);
					i++;
					// blank
					//~ printf("hash %08x - extension %08x - offset %08x - compressed %08x\n", files[i].hash, files[i].extension, files[i].offset, files[i].compressed);
		//~ printf("%s - i %d\n", stage.name, i);
					/*subdiritems = files[i].offset >> 24;
					subdirsize = files[i].offset & 0x00FFFFFF;*/
					//~ printf("Subdir found: size %06x - items %02x (%d)\n\t%s\n", subdirsize, subdiritems, subdiritems, outputname);
					i++;
					// has group and offset in offset
					//~ printf("hash %08x - extension %08x - offset %08x - compressed %08x\n", files[i].hash, files[i].extension, files[i].offset, files[i].compressed);
					
					
					if(flags & IS_BIGENTRY) {
						printf("!!!UNDEFINED!!!\n");
						//~ name = files[i].hash;
						//~ extension = files[i].extension;
					}
					else {
						offset = files[i].offset & 0x00FFFFFF;
						size = files[i].offset >> 24;
					}
					
					
					if( !(o = fopen( outputname, "wb" ))) {
						printf("Couldnt open file %s\n", outputname);
						return;
					}
					
					fprintf(o, "Info for SLOT file:\nGroupno: %d\nOffset (sectors): 0x%06X\nOffset (bytes): 0x%08X", size, offset, offset*0x800);
					fclose(o);
					
					free(filename);
					filename = NULL;
					i++;
					continue;
				}
				
				if(flags & IS_VERYBIGENTRY) {
					/* this looks like an off by one, gz konani */
					offset = files[i+1].offset;
					size = files[i+2].offset - files[i+1].offset;
				}
				else {
					offset = files[i].offset;
					size = files[i+1].offset - files[i].offset;
				}
				fflush(stdout);
				filename = getFileNameFromHashes(name, extension, flags);
				snprintf(outputname, 64, "%s/%s/%s", stage.name, foldername, filename);
				
				if( !(o = fopen( outputname, "wb" ))) {
					printf("Couldnt open file %s\n", outputname);
					return;
				}
				fwrite(uncomdata+offset, size, 1, o);
				fclose(o);
				free(filename);
				filename = NULL;
				i++;
			}
			if(comdata) free(comdata);
			comdata = NULL;
			comvalues = NULL;
			free(uncomdata);
			uncomdata = NULL;
			if(flags & ENCRYPTED) unpackedoffset += toread;
			else unpackedoffset += towrite;
			if(unpackedoffset%0x800) unpackedoffset += 0x800-(unpackedoffset%0x800);
		}
		free(foldername);
		i++;
	}
	printf("done\n");
	free(files);
	return;
}


int main( int argc, char **argv ) {
	
	unsigned char datheader[3*4] = {0};
	unsigned char datheaderbig[3*4] = {0};
	char execpath[4096] = {0}, stagename[32];
	unsigned char *stagelist = NULL;
	char builddatestring[32];
	char builddatestringbig[32];
	time_t buildtime;
	struct tm *buildstruct = NULL;
	
	uint16_t overridegame = 0;
	
	unsigned int i = 0;
	unsigned int flags = 0;
	
	tocDatHeader *dathead = NULL;
	tocDatHeader *datheadbig = NULL;
	stageinfo *infolist = NULL;
	tocStageEntry *stages = NULL;
	tocStageEntrySmall *smallstages = NULL;
	tocStageEntryBig *bigstages = NULL;
	
	uint32_t *headvals = NULL;
	
	if ( argc < 2 ) {
		printf("Not enough args!\nUse: %s DAT-file\n", argv[0]);
		return 1;
	}
	if( argc == 3 ) {
		overridegame = strtoul(argv[2], NULL, 16);
	}
	
	if(strlen(argv[0]) > 4096) BREAK("Path to executable exceeds 4096 chars!\n")
	strcpy(execpath, argv[0]);
	if(strrchr(execpath, '/') || strrchr(execpath, '\\')) cleanExecPath(execpath);
	else sprintf(execpath, "./");
	
	FILE *f = NULL;
	/* needed for debugging purposes */
	FILE *o = NULL;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fread( &initialkey, 4, 1, f );
	initialkeybig = bswap_32(initialkey);
	
	
	fread( datheader, 0x0C, 1, f );
	headvals = (uint32_t *)datheader;
	
	/* the trial edition does bullshit here, meh */
	if(initialkey == 0x00000001) {
		
		flags |= (GAME_MGS2|TRIALSTAGE);
		
		getDictionary(&commondic, &numcommondic, execpath, "common-trial2", flags);
		
		dathead = malloc(sizeof(tocDatHeader));
		
		buildtime = headvals[0];
		buildstruct = gmtime(&buildtime);
		strftime(builddatestring, 32, dateTimeString, buildstruct);
		
		printf("DAT Header info (endianess: little - Trial mode):\n");
		printf("builddate %s (%08x) - numstages %03d (%08x) - headersize %08x\n", builddatestring, headvals[0], headvals[1], headvals[1], headvals[2]);
		dathead->numStages = headvals[1] & 0xFFFF;
	}
	else {
		dathead = (tocDatHeader *)datheader;
		datheadbig = (tocDatHeader *)datheaderbig;
		
		/* decrypt header in BE mode */
		decryptHeader(datheader, 3, GAME_MGS4);
		
		datheadbig->version = bswap_16(dathead->version);
		datheadbig->numblocks = bswap_16(dathead->numblocks);
		datheadbig->numStages = bswap_16(dathead->numStages);
		datheadbig->game = bswap_16(dathead->game);
		datheadbig->unknownhash = bswap_32(dathead->unknownhash);
		
		/* encrypt header in BE mode */
		decryptHeader(datheader, 3, GAME_MGS4);
		/* decrypt header in LE mode */
		decryptHeader(datheader, 3, 0);
		
		buildtime = initialkey;
		buildstruct = gmtime(&buildtime);
		strftime(builddatestring, 32, dateTimeString, buildstruct);
		
		buildtime = initialkeybig;
		buildstruct = gmtime(&buildtime);
		strftime(builddatestringbig, 32, dateTimeString, buildstruct);
		
		if(((dathead->version >> 8) || (dathead->numblocks >> 8)) && ((datheadbig->version != 1))) {
			printf("unencrypted fiel\n");
			/* "encrypt" header in LE mode to turn the "decrypted" data back into usable data, as it never was encrypted */
			decryptHeader(datheader, 3, 0);
			if(!(dathead->unknownhash & 0xFF)) {
				printf("reassigning datheadbig values\n");
				datheadbig->version = bswap_16(dathead->version);
				datheadbig->numblocks = bswap_16(dathead->numblocks);
				datheadbig->numStages = bswap_16(dathead->numStages);
				datheadbig->game = bswap_16(dathead->game);
				datheadbig->unknownhash = bswap_32(dathead->unknownhash);
				dathead = datheadbig;
				flags |= GAME_TTS; /* doing this because TTS is the only BE game with unencrypted data */
			}
		}
		
		else flags |= ENCRYPTED;
		
		if((datheadbig->game == 0x0000) && (datheadbig->version == 1)) {
			dathead = datheadbig;
			flags |= GAME_MGS4;
		}
		
		printf("DAT Header info (endianness %s):\n", (flags & IS_BIGENDIAN)?"BE":"LE");
		printf(datHeaderInfoString, (flags & IS_BIGENDIAN)?builddatestringbig:builddatestring, initialkey, dathead->version, dathead->numblocks, dathead->numStages, dathead->numStages, dathead->game, dathead->unknownhash);
		
		if(!overridegame) overridegame = dathead->game;
		else printf("WARNING: overriding game from file with passed value %04x\n", overridegame);
		
		switch(overridegame) {
			case 0xCCCC: { /* TTS uses 0xCCCC as "game" magic, which conveniently works when interpreted as either LE or BE 16bit value */
				printf("TTS DAT detected based on game hash\n");
				dathead = datheadbig;
				flags |= (GAME_TTS|GAME_MGS2|COMPRESSED);
				
				getDictionary(&commondic, &numcommondic, execpath, "common-tts", flags);
				
				break;
			}
			case 0x4010:
			case 0x4016:
			case 0x2ABB: {
				printf("MGS2 DAT detected based on game hash\n");
				flags |= (GAME_MGS2 | ((flags & ENCRYPTED) ? COMPRESSED : 0));
				
				getDictionary(&commondic, &numcommondic, execpath, "common-mgs2", flags);
				break;
			}
			case 0x0000:	/* THIS MAY EXPLODE HORRIBLY POSSIBLY EVENTUALLY
					   WHO THOUGHT THIS WAS A GOOD IDEA */
					/* AS EXPECTED IT EXPLODED HORRIBLY AND ATE KITTENS */
				if(!(flags & IS_BIGENDIAN)) flags |= GAME_MGS3S;
			case 0x0025:
			case 0x0063:
			case 0x4015:
			case 0x4212: {
				if(flags & IS_BIGENDIAN) { /* BE thing for MGS4 */
					if(datheadbig->version != 1) BREAK("unexpected version, not 1\n");
					flags |= (GAME_MGS4|COMPRESSED);
					
					getDictionary(&commondic, &numcommondic, execpath, "common-mgs4", flags);
				}
				else { /* LE thing for MGS3, see comment for case 0x0000 */
					printf("MGS3 DAT detected based on game hash\n");
					flags |= (GAME_MGS3|COMPRESSED);
					
					getDictionary(&commondic, &numcommondic, execpath, "common-mgs3", flags);
				}
				break;
			}
			case 0x0806: {
				printf("ZOE2 DAT detected based on game hash\n");
				flags |= (GAME_ZOE2|COMPRESSED);
				
				getDictionary(&commondic, &numcommondic, execpath, "common-zoe2", flags);
				break;
			}
			default: {
				/* change this back, just in case something goes really bad */
				dathead = (tocDatHeader *)datheader;
				printf("DAT Header info:\n");
				printf(datHeaderInfoString, builddatestring, initialkey, dathead->version, dathead->numblocks, dathead->numStages, dathead->numStages, dathead->game, dathead->unknownhash);
				
				printf("DAT Header info BE:\n");
				printf(datHeaderInfoString, builddatestringbig, initialkeybig, datheadbig->version, datheadbig->numblocks, datheadbig->numStages, datheadbig->numStages, datheadbig->game, datheadbig->unknownhash);
				printf("overridegame was %04x\n", overridegame);
				printf("unknown game hash, aborting\n");
				fclose(f);
				return 0;
			}
		}
	}
	
	printf("Flags raw %08x\n", flags);
	printf("Current flags:\n%s%s%s%s%s%s%s%s%s\n", (flags & GAME_MGS2) ? "GAME_MGS2 " : "", (flags & GAME_MGS3) ? "GAME_MGS3 " : "", (flags & GAME_MGS4) ? "GAME_MGS4 " : "", (flags & GAME_ZOE2) ? "GAME_ZOE2 " : "", (flags & GAME_TTS) ? "GAME_TTS " : "", (flags & GAME_MGS3S) ? "GAME_MGS3S " : "", (flags & TRIALSTAGE) ? "TRIALSTAGE " : "", (flags & ENCRYPTED) ? "ENCRYPTED " : "", (flags & COMPRESSED) ? "COMPRESSED " : "");
	
	if(flags & IS_BIGSTAGE) {
		
		stagelist = malloc((3*4)+(sizeof(tocStageEntryBig)*dathead->numStages));
		stages = malloc(sizeof(tocStageEntry)*dathead->numStages);
		bigstages = (tocStageEntryBig *)(stagelist+(3*4));
		fseek(f, 4, SEEK_SET);
		fread( stagelist, (3*4)+(sizeof(tocStageEntryBig)*dathead->numStages), 1, f );
		
		if(flags & ENCRYPTED) decryptHeader(stagelist, 3+(dathead->numStages*5), flags);
		
		for(i = 0; i < dathead->numStages; i++) {
			stages[i].name = bigstages[i].name;
			stages[i].offset = (flags & IS_BIGENDIAN) ? bswap_32(bigstages[i].offset) : bigstages[i].offset;
		}
		/* dumps the stagelist for debugging purposes */
		printf("DAMPSTAGEBIG\n");
		o = fopen("stagedampbig.bin", "wb");
		fwrite(stagelist, (3*4)+(sizeof(tocStageEntryBig)*dathead->numStages), 1, o);
		fclose(o); 
	}
	else {
		stagelist = malloc((3*4)+(sizeof(tocStageEntrySmall)*dathead->numStages));
		stages = malloc(sizeof(tocStageEntry)*dathead->numStages);
		smallstages = (flags & TRIALSTAGE) ? ((tocStageEntrySmall *)stagelist) : ((tocStageEntrySmall *)(stagelist+(3*4)));
		fseek(f, (flags & TRIALSTAGE) ? 0x10 : 4, SEEK_SET); /* trial has its stage list at a different offset */
		fread( stagelist, (3*4)+(sizeof(tocStageEntrySmall)*dathead->numStages), 1, f );
		
		if(flags & ENCRYPTED) decryptHeader(stagelist, 3+(dathead->numStages*3), flags);
		
		for(i = 0; i < dathead->numStages; i++) {
			stages[i].name = smallstages[i].name;
			stages[i].offset = (flags & IS_BIGENDIAN) ? bswap_32(smallstages[i].offset) : smallstages[i].offset;
		}
		/* dumps the stagelist for debugging purposes */
		printf("DAMPSTAGESMALL\n");
		o = fopen("stagedampsmall.bin", "wb");
		fwrite(stagelist, (3*4)+(sizeof(tocStageEntrySmall)*dathead->numStages), 1, o);
		fclose(o);
	}
	
	printf("Stagelist:\n");
	for(i = 0; i < dathead->numStages; i++) printf("idx %04x Name %s - offset %08x(%08x)\n", i, stages[i].name, stages[i].offset, stages[i].offset * 0x800);
	
	infolist = processStages(f, stages, dathead->numStages, flags);
	
	for(i = 0; i < dathead->numStages; i++) {
		createDirectory(stages[i].name);
		strcpy(stagename, stages[i].name);
		if(flags & TRIALSTAGE) strcat(stagename, "-trial2");
		else if(flags & GAME_TTS) strcat(stagename, "-tts");
		else if(flags & GAME_MGS2) strcat(stagename, "-mgs2");
		else if(flags & GAME_MGS3) strcat(stagename, "-mgs3");
		else if(flags & GAME_MGS4) strcat(stagename, "-mgs4");
		else if(flags & GAME_ZOE2) strcat(stagename, "-zoe2");
		getDictionary(&stagedic, &numstagedic, execpath, stagename, flags);
		
		processInfoWithStage(f, infolist[i], stages[i], flags);
		if(stagedic) free(stagedic);
		stagedic = NULL;
		if(numstagedic > 0) numdicentries -= numstagedic;
		numstagedic = 0;
	}
	
	for(i = 0; i < dathead->numStages; i++) free(infolist[i].data);
	free(stagelist);
	free(infolist);
	free(stages);
	if(flags & TRIALSTAGE) free(dathead);
	
	if(commondic) free(commondic);
	
	printf("Done.\n");
	
	fclose(f);
	return 0;
}
