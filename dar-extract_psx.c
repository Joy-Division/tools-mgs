#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "kojimahash/kojimahash.h"
#include "stage-dictionary.h"

#define RETPRINT(x) { sprintf (retstring, "%s", x); break; }

#define BREAK(x) { printf(x); return 1; }

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

typedef struct {
	uint32_t hash;
	uint32_t size;
}__attribute__((packed)) fileentry;

dicentry *commondic = NULL, *stagedic = NULL;
int numcommondic = 0, numstagedic = 0, numdicentries = 0;

unsigned int matchhash( uint32_t hash, uint32_t extension, dicentry *dictionary, int numentries ) {
	int i;
	for( i = 0; i < numentries; i++ ) {
		if((dictionary[i].hash == hash) && (dictionary[i].extension == extension)) return i;
	}
	return 0xFFFFFFFF;
}

char *getNameFromDetails(uint16_t hash, uint16_t extension) {
	char *retstring = malloc(32);
	unsigned int result;
	result = matchhash(hash, extension, commondic, numcommondic);
	if(result != 0xFFFFFFFF) sprintf(retstring, "%s", commondic[result].name);
	else {
		sprintf( retstring, "%04x.%c", hash, extension );
	}
	return retstring;
}

int main( int argc, char **argv ) {
	
	char execpath[4096];
	char *namestring = NULL;
	unsigned int darsize;
	fileentry thing;
	uint16_t type, hash;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s DAR-file [dictionary]\n", argv[0]);
		return 1;
	}
	
	if( argc == 3 ) {
		if(strlen(argv[0]) > 4096) BREAK("Path to executable exceeds 4096 chars!\n")
		strcpy(execpath, argv[0]);
		*strrchr(execpath, '/') = 0;
		
		numcommondic = loaddic(&commondic, execpath, argv[2], DIC_HASH_SINGLE_EXT, hashstring16);
	}
	
	FILE *f, *o;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	darsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	while(ftell(f)<darsize) {
		fread(&thing, 8, 1, f);
		type = (thing.hash >> 16) & 0xFF;
		hash = thing.hash & 0xFFFF;
		namestring = getNameFromDetails(hash, type);
		if( !(o = fopen( namestring, "wb" ))) {
			printf("Couldnt open file %s\n", namestring);
			return 1;
		}
		writeFile(f, thing.size, o);
		fclose(o);
		free(namestring);
	}
	
	free(commondic);
	
	
	printf("Done.\n");
	fclose(f);
	return 0;
}
