/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
	
	char name[32];
	uint32_t size, len;
	unsigned int remember;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s PAK-file\n", argv[0]);
		return 1;
	}
	
	uint32_t numFiles, workfiles;
	FILE *f, *o;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fread( &numFiles, 4, 1, f );
	
	printf("Number of files: %u\n", numFiles );
	
	for( workfiles = 0; workfiles < numFiles; workfiles++ ) {
		remember = ftell(f);
		len = 0;
		while(fgetc(f)) len++;
		len++;
		fseek(f, remember, SEEK_SET);
		if((remember+len)%4) len+= 4-((remember+len)%4);
		if(len > 32) {
			printf("namelen > 32!\n");
			return 1;
		}
		fread(name, len, 1, f );
		fread( &size, 4, 1, f );
		printf("Current file: %s - Size %08x\n", name, size);
		
		if( !(o = fopen( name, "wb" ))) {
			printf("Couldnt open output %s\n", name);
			return 1;
		}
		
		writeFile( f, size, o );
		fclose(o);
		fseek(f, 1, SEEK_CUR);
	}
	printf("Done.\n");
	fclose(f);
	return 0;
}
