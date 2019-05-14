/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "kojimahash/kojimahash.h"
#include "lodepng/lodepng.h"
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

#define color5to8(x) ((x*8)+(x/4))

typedef struct {
	uint16_t null;
	uint16_t hash;
	uint32_t size;
	uint32_t offset;
}__attribute__((packed)) faceinfo;

typedef struct {
	uint32_t bitmap;
	uint32_t eye0;
	uint32_t eye1;
	uint32_t eye2;
	uint32_t mouth0;
	uint32_t mouth1;
	uint32_t mouth2;
}__attribute__((packed)) simpleinfo;

typedef struct {
	uint32_t palette;
	uint32_t bitmap;
	uint16_t unknown0;
	uint16_t unknown1;
}__attribute__((packed)) animationinfo;

typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t w;
	uint8_t h;
}__attribute__((packed)) imageinfo;

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

char *getNameFromDetails(uint16_t hash) {
	char *retstring = malloc(32);
	unsigned int result;
	result = matchhash(hash, hashstring16("p"), commondic, numcommondic);
	if(result != 0xFFFFFFFF) sprintf(retstring, "%s", commondic[result].name);
	else {
		sprintf( retstring, "%04x.png", hash );
	}
	return retstring;
}

uint32_t *generatePalette(uint16_t *source) {
	unsigned int i;
	uint8_t red, green, blue;
	uint32_t *retpal = NULL;
	retpal = malloc(sizeof(uint32_t)*256);
	for(i = 0; i < 256; i++) {
		red = source[i] & 0x1F;
		green = (source[i] >> 5) & 0x1F;
		blue = (source[i] >> 10) & 0x1F;
		retpal[i] = 0xFF000000 | color5to8(red) | color5to8(green) << 8 | color5to8(blue) << 16;
	}
	return retpal;
}

unsigned char *pixelsToRGBA(unsigned char *source, uint16_t *paletteData, unsigned int pixelsWidth, unsigned int pixelsHeight) {
	unsigned int i, numPixels = pixelsWidth*pixelsHeight;
	uint8_t *idximage;
	uint32_t *palette = generatePalette(paletteData);
	idximage = source;
	uint32_t *retimage = malloc(sizeof(uint32_t)*numPixels);
	for( i = 0; i < numPixels; i++ ) retimage[i] = palette[idximage[i]];
	free(palette);
	return (unsigned char*)retimage;
}

int main( int argc, char **argv ) {
	
	char execpath[4096];
	char *namestring = NULL, dirname[128] = {0}, filename[256] = {0};
	unsigned int i, j, datsize, datoffset = 0;
	unsigned int groupnum = 0;
	unsigned char *data = NULL, *rgba = NULL;
	uint16_t *palsource = NULL;
	
	uint32_t numfaces, numframes;
	
	faceinfo *faces = NULL;
	uint32_t *simpleoffsets = NULL;
	animationinfo *fullani = NULL;
	imageinfo *image = NULL;
	
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s [FACE.DAT]\n", argv[0]);
		return 1;
	}
	
	if(strlen(argv[0]) > 4096) BREAK("Path to executable exceeds 4096 chars!\n")
	strcpy(execpath, argv[0]);
	cleanExecPath(execpath);
	
	numcommondic = loaddic(&commondic, execpath, "mgs1-face", DIC_HASH_SINGLE_EXT, hashstring16);
	
	FILE *f;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	datsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	while(datoffset<datsize) {
		//~ printf("offset %08x\n", (unsigned int)ftell(f));
		snprintf(dirname, 128, "%04d-%08x", groupnum, (unsigned int)ftell(f));
		
		fread(&numfaces, 4, 1, f);
		if(numfaces) {
			createDirectory(dirname);
			faces = malloc(sizeof(faceinfo)*numfaces);
			fread(faces, sizeof(faceinfo)*numfaces, 1, f);
			
			for(i = 0; i < numfaces; i++) {
				printf("group %d face %02d of %02d\r", groupnum, i+1, numfaces);
				namestring = getNameFromDetails(faces[i].hash);
				
				data = malloc(faces[i].size);
				fseek(f, datoffset+faces[i].offset+4, SEEK_SET);
				fread(data, faces[i].size, 1, f);
				
				numframes = *(uint32_t *)data;
				
				if(numframes == 0x20) {
					/* simple */
					//~ printf("simple\n");
					simpleoffsets = (uint32_t *)(data+4);
					palsource = (uint16_t *)(data+32);
					for(j = 0; j < 7; j++) {
						if(simpleoffsets[j]) {
							//~ printf("x");
							image = (imageinfo *)(data+simpleoffsets[j]);
							rgba = pixelsToRGBA(data+simpleoffsets[j]+4, palsource, image->w, image->h);
							snprintf(filename, 256, "%s/%03d-%02d-x%03d-y%03d-%s", dirname, i, j, image->x, image->y, namestring);
							lodepng_encode32_file(filename, rgba, image->w, image->h);
							free(rgba);
						}
						//~ else printf("-");
					}
					//~ printf("\n");
				}
				else {
					/* full */
					fullani = (animationinfo *)(data+4);
					for(j = 0; j < numframes; j++) {
						palsource = (uint16_t *)(data+fullani[j].palette);
						image = (imageinfo *)(data+fullani[j].bitmap);
						//~ printf("off %08x - w %03x - h %03d\n", fullani[j].bitmap+4, image->w, image->h);
						rgba = pixelsToRGBA(data+fullani[j].bitmap+4, palsource, image->w, image->h);
						snprintf(filename, 256, "%s/%03d-%02d-x%03d-y%03d-%s", dirname, i, j, image->x, image->y, namestring);
						lodepng_encode32_file(filename, rgba, image->w, image->h);
						free(rgba);
					}
				}
				free(data);
				free(namestring);
			}
			printf("\n");
			free(faces);
		}
		if(ftell(f)%0x800) fseek(f, (0x800-(ftell(f)%0x800)), SEEK_CUR);
		datoffset = ftell(f);
		groupnum++;
	}
	
	
	printf("Done.\n");
	fclose(f);
	return 0;
}
