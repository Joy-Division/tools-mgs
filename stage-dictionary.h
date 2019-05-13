#ifndef STAGE_DICTIONARY_H_
#define STAGE_DICTIONARY_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	uint32_t hash;
	uint32_t extension;
	char name[256];
} dicentry;

enum dic_flags {
	DIC_HASH_FULL_NAME =	(1<<0),
	DIC_HASH_SINGLE_EXT =	(1<<1),
	DIC_USE_EXTERNAL =	(1<<2),
	DIC_ADJUST_EXT_MGS2 =	(1<<3),
	DIC_NUMFLAGS
};
	

int loaddic(dicentry **dic, char *path, char *name, unsigned int flags, uint32_t (*hfunc)(char *));

#endif