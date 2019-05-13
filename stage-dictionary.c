/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include "stage-dictionary.h"

/*	loads a dictionary name from path, allocating an array of dicentries at dic
	uses function hfunc to hash the names
	returns the number of entries loaded
	
	if the file couldnt be opened, returns -1 and sets dic to NULL
*/

int loaddic(dicentry **dic, char *path, char *name, unsigned int flags, uint32_t (*hfunc)(char *)) {
	FILE *f;
	unsigned int i = 0, countedlines = 0;
	char dicname[512], scanline[512], filterline[512], *token = NULL;
	sprintf(dicname, "%s/%s.txt", path, name);
	
	if( !(f = fopen( dicname, "r" ))) {
		printf("Couldnt open dictionary %s\n", dicname);
		*dic = NULL;
		return -1;
	}
	
	while(fgets(scanline, 512, f)) {
		if(sscanf(scanline, "%s//%*s", filterline) > 0) {
			if(!(strncmp(filterline, "//", 2))) printf("skipping comment\n");
			else countedlines++;
		}
		else printf("skipping empty\n");
	}
	
	printf("found %u useful lines\n", countedlines);
	
	rewind(f);
	
	*dic = malloc(sizeof(dicentry)*countedlines);
	for(i = 0; i < countedlines; ) {
		fgets(scanline, 512, f);
		
		if(sscanf(scanline, "%s//%*s", filterline) > 0) {
			if(!(strncmp(filterline, "//", 2))) continue;
		}
		else continue;
		
		while(strcspn(filterline, "\r\n") < strlen(filterline)) filterline[strcspn(filterline,"\r\n")] = 0;
		strncpy((*dic)[i].name, filterline, 256);
		
		token = strtok(filterline, ".");
		
		if(flags & DIC_HASH_FULL_NAME) (*dic)[i].hash = (*hfunc)((*dic)[i].name);
		else (*dic)[i].hash = (*hfunc)(token);
		
		token = strtok(NULL, ".");
		
		if(flags & DIC_HASH_SINGLE_EXT) token[1] = 0;
		if(flags & DIC_USE_EXTERNAL) (*dic)[i].extension = 0xFFFFFFFF;
		else (*dic)[i].extension = (*hfunc)(token);
		if(flags & DIC_ADJUST_EXT_MGS2) (*dic)[i].extension -= 0x61; // 'a'
		
		printf("Added entry to dictionary %s: %s (%08x - %08x)\n", name, (*dic)[i].name, (*dic)[i].hash, (*dic)[i].extension);
		i++;
	}
	printf("Read %d entries from dictionary %s.\n", countedlines, dicname);
	
	fclose(f);
	
	return countedlines;
}