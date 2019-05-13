/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <zlib.h>

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
	
	char *name = NULL;
	unsigned char *comdata = NULL, *uncomdata = NULL;
	uint32_t *size;
	unsigned int memoff, len;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s _zar-file\n", argv[0]);
		return 1;
	}
	
	uint32_t numfiles, workfiles, uncomsize, filesize;
	unsigned long int havebuf;
	
	FILE *f, *o;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	fread( &uncomsize, 4, 1, f );
	
	printf("Compressed size %08x (file is %08x)\n", filesize-4, filesize);
	printf("Uncompressed size: %08x\n", uncomsize );
	havebuf = uncomsize;
	
	comdata = malloc(filesize-4);
	uncomdata = malloc(uncomsize);
	
	fread(comdata, filesize-4, 1, f);
	
	if((workfiles = uncompress(uncomdata, &havebuf, comdata, filesize-4)) != Z_OK) {
		printf("=====================================couldnt decompress data\n");
		switch(workfiles) {
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
		printf("uncomsize %08x\n", uncomsize);
		return 1;
	}
	free(comdata);
	fclose(f);
	
	numfiles = *uncomdata;
	
	printf("Number of files: %u\n", numfiles);
	
	memoff = 4;
	
	for( workfiles = 0; workfiles < numfiles; workfiles++ ) {
		printf("offset %08x\n", memoff);
		name = (char *)uncomdata+memoff;
		len = strlen(name) + 1;
		if((memoff+len)%4) len+= 4-((memoff+len)%4);
		memoff += len;
		
		printf("offset %08x\n", memoff);
		size = (uint32_t *)(uncomdata+memoff);
		len = 4;
		if((memoff+len)%0x10) len += 0x10-((memoff+len)%0x10);
		memoff += len;
		
		printf("Current file: %s - Size %08x\n", name, *size);
		
		if( !(o = fopen( name, "wb" ))) {
			printf("Couldnt open output %s\n", name);
			return 1;
		}
		
		fwrite(uncomdata+memoff, *size, 1, o);
		
		fclose(o);
		
		memoff += *size + 1;
	}
	
	free(uncomdata);
	printf("Done.\n");
	return 0;
}