/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
	uint32_t hash;
	uint32_t size;
}__attribute__((packed)) entry;

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

int main( int argc, char **argv ) {
	
	char name[64];
	uint32_t len;
	unsigned int remember;
	
	entry *filelist = NULL;
	char **namelist = NULL;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s QAR-file\n", argv[0]);
		return 1;
	}
	
	uint32_t numFiles, workfiles, listoff;
	FILE *f, *o;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fseek(f, -4, SEEK_END);
	
	fread( &listoff, 4, 1, f );
	
	printf("Offset of list: %08x\n", listoff);
	
	fseek(f, listoff, SEEK_SET);
	
	fread( &numFiles, 4, 1, f );
	
	printf("Number of files: %u\n", numFiles );
	
	filelist = malloc(numFiles*sizeof(entry));
	namelist = malloc(numFiles*sizeof(char *));
	
	fread(filelist, sizeof(filelist)*numFiles, 1, f);
	
	for( workfiles = 0; workfiles < numFiles; workfiles++ ) {
		remember = ftell(f);
		len = 0;
		while(fgetc(f)) len++;
		len++;
		fseek(f, remember, SEEK_SET);
		namelist[workfiles] = malloc(len);
		
		fread(namelist[workfiles], len, 1, f );
	}
	
	fseek(f, 0, SEEK_SET);
	
	for( workfiles = 0; workfiles < numFiles; workfiles++ ) {
		printf("Current file: %s - Size %08x - Hash %08x\n", namelist[workfiles], filelist[workfiles].size, filelist[workfiles].hash);
		
		if( !(o = fopen( namelist[workfiles], "wb" ))) {
			printf("Couldnt open output %s\n", name);
			return 1;
		}
		
		writeFile( f, filelist[workfiles].size, o );
		fclose(o);
		if(ftell(f)%0x80) fseek(f, 0x80-(ftell(f)%0x80), SEEK_CUR);
		free(namelist[workfiles]);
	}
	free(namelist);
	free(filelist);
	
	printf("Done.\n");
	fclose(f);
	return 0;
}
