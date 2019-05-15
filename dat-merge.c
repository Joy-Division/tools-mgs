#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
	char name[16];
	uint32_t offset;
	uint32_t size;
} __attribute__((packed)) vfsfile;

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

int main(int argc, char** argv) {
	FILE *f = NULL;
	FILE *o = NULL;
	
	uint32_t numvfsfiles;
	vfsfile* vfsfiles;
	
	unsigned int i;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s dat-file\n", argv[0]);
		return 1;
	}
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fread(&numvfsfiles, 4, 1, f);
	vfsfiles = malloc(sizeof(vfsfile)*numvfsfiles);
	fread(vfsfiles, sizeof(vfsfile)*numvfsfiles, 1, f);
	
	for(i = 0; i < numvfsfiles; i++) {
		printf("File %s offset %08x size %08x sectors\n", vfsfiles[i].name, vfsfiles[i].offset, vfsfiles[i].size);
		fseek(f, vfsfiles[i].offset*0x800, SEEK_SET);
		if( !(o = fopen(vfsfiles[i].name, "wb" ))) {
			printf("Couldnt open files %s\n", vfsfiles[i].name);
			return 1;
		}
		writeFile(f, vfsfiles[i].size*0x800, o);
		fclose(o);
	}
	
	free(vfsfiles);
	fclose(f);
	
	printf("Done.\n");
	return 0;
}