#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "kojimahash/kojimahash.h"

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

typedef struct {
	uint16_t hash;
	uint16_t extension;
	char name[32];
} dicentry;

dicentry *commondic = NULL;
unsigned int numcommondic = 0;

void loaddic(char *executable, char *name) {
	FILE *f;
	unsigned int i = 0;
	char dicname[512], scanname[32], *token = NULL;
	sprintf(dicname, "%s/%s.txt", executable, name);
	if( !(f = fopen( dicname, "r" ))) {
		printf("Couldnt open dictionary %s\n", dicname);
		numcommondic = 0;
		commondic = NULL;
		return;
	}
	fgets(scanname, 32, f);
	sscanf(scanname, "%u", &numcommondic);
	commondic = malloc(sizeof(dicentry)*numcommondic);
	i = 0;
	while(i < numcommondic) {
		fgets(scanname, 32, f);
		while(strcspn(scanname, "\r\n") < strlen(scanname)) scanname[strcspn(scanname,"\r\n")] = 0;
		strcpy(commondic[i].name, scanname);
		
		token = strtok(scanname, ".");
		commondic[i].hash = hashstring16(token);
		token = strtok(NULL, ".");
		token[1] = 0;
		commondic[i].extension = hashstring16(token);
		printf("Added entry to stagedic: %s (%04x - %02x)\n", commondic[i].name, commondic[i].hash, commondic[i].extension);
		i++;
	}
	printf("Read %d entries from dictionary %s.\n", numcommondic, dicname);
	
	fclose(f);
	return;
}

unsigned int matchhash( uint16_t hash, uint16_t extension, dicentry *dictionary, unsigned int numentries ) {
	unsigned int i;
	for( i = 0; i < numentries; i++ ) {
		if((dictionary[i].hash == hash) && (dictionary[i].extension == extension)) return i;
	}
	return 0xFFFFFFFF;
}

char *getExtensionFromType(unsigned int type) {
	char *retstring = malloc(4);
	switch(type) {
		case 0x61: RETPRINT("azm")
		case 0x62: RETPRINT("bin")
		case 0x63: RETPRINT("con")
		case 0x64: RETPRINT("dar")
		case 0x65: RETPRINT("efx")
		case 0x67: RETPRINT("gcx")
		case 0x68: RETPRINT("hzm")
		case 0x69: RETPRINT("img")
		case 0x6B: RETPRINT("kmd")
		case 0x6C: RETPRINT("lit")
		case 0x6D: RETPRINT("mdx")
		case 0x6F: RETPRINT("oar")
		case 0x70: RETPRINT("pcc")
		case 0x72: RETPRINT("rar")
		case 0x73: RETPRINT("sgt")
		case 0x77: RETPRINT("wvx")
		case 0x7A: RETPRINT("zmd")
		default: {
			sprintf( retstring, "%02x", type );
			break;
		}
	}
	return retstring;
}

char *getInfoFromFlags(unsigned int flags) {
	char *retstring = malloc(16);
	switch(flags) {
		case 0x63: RETPRINT("cache")
		case 0x6E: RETPRINT("nocache")
		case 0x72: RETPRINT("resident")
		case 0x73: RETPRINT("sound-nocache")
		default: {
			sprintf( retstring, "%02x", flags );
			break;
		}
	}
	return retstring;
}

char *getNameFromDetails(uint16_t hash, uint16_t extension) {
	char *retstring = malloc(32);
	char *extensionstring;
	unsigned int result;
	
	result = matchhash(hash, extension, commondic, numcommondic);
	if(result != 0xFFFFFFFF) sprintf(retstring, "%s", commondic[result].name);
	else {
		extensionstring = getExtensionFromType(extension);
		sprintf( retstring, "%04x.%s", hash, extensionstring );
		free(extensionstring);
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
		
		printf("loading dictionary %s/%s\n", execpath, argv[2]);
		loaddic(execpath, argv[2]);
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
	
	
	printf("Done.\n");
	fclose(f);
	return 0;
}
