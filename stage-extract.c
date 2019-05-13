#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "kojimahash/kojimahash.h"
#include "stage-dictionary.h"

#ifdef _WIN32
#include <direct.h>
#define createDirectory(dirname) mkdir(dirname)
#define cleanExecPath(path) *strrchr(path, '\\') = 0
#else
#include <sys/stat.h>
#include <sys/types.h>
#define cleanExecPath(path) *strrchr(path, '/') = 0
#define createDirectory(dirname) mkdir(dirname, 0777)
#endif

#define RETPRINT(x) { sprintf (retstring, "%s", x); break; }

#define BREAK(x) { printf(x); return 1; }

#define BLOCKSIZE 0x800

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

/* things for handling the stages */

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

char *getNameFromDetails(uint16_t hash, uint16_t extension, unsigned int flags, unsigned int texnum) {
	char *retstring = malloc(32);
	unsigned int result;
	if(!hash) {
		switch(flags) {
			case 0x63: RETPRINT("stg_mdl1.dar")
			case 0x6E: {
				sprintf( retstring, "%s%d.dar", "stg_tex", texnum );
				break;
			}
			case 0x72: RETPRINT("res_mdl1.dar")
		}
	}
	else {
		result = matchhash(hash, extension, stagedic, numstagedic);
		if(result != 0xFFFFFFFF) sprintf(retstring, "%s", stagedic[result].name);
		else {
			result = matchhash(hash, extension, commondic, numcommondic);
			if(result != 0xFFFFFFFF) sprintf(retstring, "%s", commondic[result].name);
			else {
				sprintf( retstring, "%04x.%c", hash, extension );
			}
		}
	}
	return retstring;
}

int unpackstage(FILE *f, uint32_t stageoffset, char *stagename) {
	fseek(f, stageoffset, SEEK_SET);
	
	unsigned int i, j, numfiles, temp;
	unsigned int fakedar = 0, numfakedar = 0;
	uint16_t flags, type, hash, texnum = 1;
	char *flagstring, *namestring;
	char outputname[128];
	uint32_t header, namehash = 1;
	fileentry *files = NULL;
	unsigned int *fileoffsets;
	
	FILE *o;
	
	fread(&header, 4, 1, f);
	
	for(i = 0; namehash; i++) {
		fread(&namehash, 4, 1, f);
		fread(&header, 4, 1, f);
	}
	i--;
	numfiles = i;
	
	//~ printf("Number of file entries in archive: %d\n", numfiles);
	files = malloc(sizeof(fileentry)*numfiles);
	fileoffsets = malloc(sizeof(unsigned int)*numfiles);
	fseek(f, stageoffset+4, SEEK_SET);
	temp = stageoffset+0x800;
	for(i = 0;i < numfiles; i++) {
		fseek(f, stageoffset+(4+(8*i)), SEEK_SET);
		fread(&files[i], sizeof(fileentry), 1, f);
		if(!files[i].size) {
			//~ printf("Begin of fake stg_mdl1.dar!\n");
			fakedar = 1;
		}
		if(files[i].hash == 0xFF630000) {
			//~ printf("End of fake stg_mdl1.dar!\n");
			files[i].size = 0;
			fakedar = 0;
			numfakedar = 0;
			if(temp % 0x800) temp += 0x800-(temp % 0x800);
			continue;
		}
		if(fakedar) {
			numfakedar++;
			//~ printf("Getting size of file %d in fake dar\n", numfakedar);
			fseek(f, stageoffset+(4+(8*i)+12), SEEK_SET);
			fread(&files[i].size, 4, 1, f);
			for( j = 1; j < numfakedar; j++ ) {
				//~ printf("Subtracting size of file %d from fake dar\n", numfakedar-j);
				files[i].size -= files[i-j].size;
			}
		}
		//~ printf("Filehash %08x - Filesize %08x - Fileoffset = %08x\n", files[i].hash, files[i].size, temp);
		fileoffsets[i] = temp;
		if((files[i].size % 0x800) && !fakedar) temp += (files[i].size + (0x800-(files[i].size % 0x800)));
		else temp += files[i].size;
	}
	
	//~ printf("Begin extracting\n");
	
	for(i = 0; i < numfiles; i++) {
		if(files[i].hash == 0xFF630000) {
			//~ printf("Skipping fake dar entry %d!\n", i);
			continue;
		}
		type = files[i].hash >> 24;
		flags = (files[i].hash >> 16) & 0xFF;
		flagstring = getInfoFromFlags(flags);
		hash = files[i].hash & 0xFFFF;
		namestring = getNameFromDetails(hash, type, flags, texnum);
		//~ printf("Info about file %d:\nType %02x(%s) - Flags %02x(%s) - Name %04x(%s)\n", i, type, typestring, flags, flagstring, hash, namestring);
		sprintf(outputname, "%s/%s", stagename, namestring);
		//~ printf("Extracting file as %s\n", outputname);
		fseek(f, fileoffsets[i], SEEK_SET);
		if( !(o = fopen( outputname, "wb" ))) {
			printf("Couldnt open file %s\n", outputname);
			return 1;
		}
		writeFile(f, files[i].size, o);
		fclose(o);
		if(!hash) texnum++;
		free(flagstring);
		free(namestring);
	}
	
	free(files);
	free(fileoffsets);
	//~ printf("Done with stage.\n");
	return numfiles;
}

/* end things for handling the stages */

int main( int argc, char **argv ) {
	
	char name[9], dicname[16];
	char execpath[4096];
	unsigned int i;
	uint32_t offset, numstages;
	
	if ( argc < 2 ) {
		printf("Not enough args!\nUse: %s STAGE.DIR-file\n", argv[0]);
		return 1;
	}
	
	if(strlen(argv[0]) > 4096) BREAK("Path to executable exceeds 4096 chars!\n")
	strcpy(execpath, argv[0]);
	if(strrchr(execpath, '/') || strrchr(execpath, '\\')) cleanExecPath(execpath);
	else sprintf(execpath, "./");
	
	FILE *f;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	fseek( f, 0, SEEK_SET );
	
	fread(&numstages, 4, 1, f);
	numstages /= 12;
	
	numcommondic = loaddic(&commondic, execpath, "mgs1-common", DIC_HASH_SINGLE_EXT, hashstring16);
	if(numcommondic > 0) numdicentries = numcommondic;
	for(i = 0;i < numstages;i++) {
		fseek(f, 4+(12*i), SEEK_SET);
		fread(name, 8, 1, f);
		name[8] = 0;
		sprintf(dicname, "%s%s", "mgs1-", name);
		fread(&offset, 4, 1, f);
		offset *= BLOCKSIZE;
		printf("Current stage: %s - offset: %08x\n", name, offset);
		createDirectory(name);
		numstagedic = loaddic(&stagedic, execpath, dicname, DIC_HASH_SINGLE_EXT, hashstring16);
		if(numstagedic > 0) numdicentries += numstagedic;
		unpackstage(f, offset, name);
		if(stagedic) free(stagedic);
		stagedic = NULL;
		if(numstagedic > 0) numdicentries -= numstagedic;
		numstagedic = 0;
	}
	
	free(commondic);
	
	printf("Done.\n");
	
	fclose(f);
	return 0;
}