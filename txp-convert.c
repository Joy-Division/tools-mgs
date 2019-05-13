/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include "lodepng/lodepng.h"

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} paletteEntry;
typedef struct {
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t numTextures;
	uint32_t texlistOffset;
	
	uint32_t numFrames;
	uint32_t framelistOffset;
	uint32_t unknown3;
	uint32_t unknown4;
	// total textureHeader size 32 bytes
}__attribute__((packed)) txpHeaderPO;

typedef struct {
	uint32_t unknown1;
	uint32_t namehash;
	uint32_t numTextures;
	uint32_t numFrames;
	
	uint32_t unknown2;
	uint32_t texlistOffset;
	uint32_t framelistOffset;
	uint32_t unknown3;
	// total textureHeader size 32 bytes
}__attribute__((packed)) txpHeaderPW;

typedef struct {
	uint32_t namehash;
	
	unsigned int numtextures;
	unsigned int texturelist;
	
	unsigned int numframes;
	unsigned int framelist;
} txpHeader;

typedef struct {
	uint16_t bpp:4;
	uint16_t compressed:12;
	
	uint16_t width:12;
	uint16_t unknown1:4;
	
	uint16_t height:12;
	uint16_t unknown2:4;
	
	unsigned char unknown3[6];
	// size of bitfields and padding 12 bytes
	uint32_t offset;
	uint32_t zoffset;
	// total header size 20 bytes
}__attribute__((packed)) textureHeaderPW;

typedef struct {
	uint16_t width:12;
	uint16_t unknown1:4;
	
	uint16_t height:12;
	uint16_t unknown2:4;
	
	uint16_t bpp:4;
	uint16_t unknown3:12;
	
	uint16_t compressed:4;
	uint16_t unknown4:12;
	// size of bitfields 8 bytes
	uint32_t offset;
	uint32_t palette;
	// total header size 16 bytes
}__attribute__((packed)) textureHeaderPO;

typedef struct {
	unsigned int bpp;
	unsigned int compressed;
	unsigned int width;
	unsigned int height;
	unsigned int offset; // points do data as selected by compressed
	unsigned int palette;
	unsigned char *texpixels;
	paletteEntry *palpixels;
} textureHeader;

typedef struct {
	uint32_t unknown1;
	uint32_t hash;
	uint32_t texture;
	uint32_t palette;
	
	float fx;
	float fy;
	float fwidth;
	float fheight;
	
	uint16_t width;
	uint16_t height;
	uint16_t x;
	uint16_t y;
	// total frameHeader size 40 bytes
}__attribute__((packed)) frameHeaderPW;

typedef struct {
	uint32_t unknown1;
	uint32_t hash;
	uint32_t unknown2;
	uint16_t width;
	uint16_t height;
	
	uint32_t unknown3;
	uint32_t texture;
	uint32_t unknown4;
	float fx;
	
	float fy;
	float fwidth;
	float fheight;
	uint32_t unknown5;
	// total frameHeader size 48 bytes
}__attribute__((packed)) frameHeaderPO;

typedef struct {
	uint32_t hash;
	unsigned int width;
	unsigned int height;
	unsigned int sourcex;
	unsigned int sourcey;
	unsigned int sourcetexture;
	unsigned int palette;
	unsigned char *texpixels;
	paletteEntry *palpixels;
} frameHeader;

enum texMode {
	texMode4bpp,
	texMode8bpp,
	texMode6
};

enum games {
	mgspo,
	mgspw
};

int swizzle(int index) {
	int b = (index >> 3) & 3;
	if( b == 1 ) b = 2;
	else if( b == 2 ) b = 1;
	int swiz = b * 8;
	int b32 = (index >> 5 ) * 32;
	int final = b32 + swiz + (index & 7 );
	return final;
}

txpHeader *getTxpHeader(unsigned char *data, unsigned int game) {
	txpHeaderPO *pohead = NULL;
	txpHeaderPW *pwhead = NULL;
	txpHeader *rethead = malloc(sizeof(txpHeader));
	
	printf("Raw header values:\n");
	
	switch(game) {
		case mgspo: {
			pohead = (txpHeaderPO *)data;
			
			printf("unknown1 - unknown2 - numTexts - texloffs - numFrame - framloff - unknown3 - unknown4\n");
			printf("%08x - %08x - %08x - %08x - %08x - %08x - %08x - %08x\n",
				pohead->unknown1, pohead->unknown2, pohead->numTextures, pohead->texlistOffset,
				pohead->numFrames, pohead->framelistOffset, pohead->unknown3, pohead->unknown4);
			
			rethead->namehash = 0;
			
			rethead->numtextures = pohead->numTextures;
			rethead->texturelist = pohead->texlistOffset;
			
			rethead->numframes = pohead->numFrames;
			rethead->framelist = pohead->framelistOffset;
			break;
		}
		case mgspw: {
			pwhead = (txpHeaderPW *)data;
			
			printf("unknown1 - namehash - numTexts - numFrame - unknown2 - texloffs - framloff - unknown3\n");
			printf("%08x - %08x - %08x - %08x - %08x - %08x - %08x - %08x\n",
				pwhead->unknown1, pwhead->namehash, pwhead->numTextures, pwhead->numFrames,
				pwhead->unknown2, pwhead->texlistOffset, pwhead->framelistOffset, pwhead->unknown3);
			
			rethead->namehash = pwhead->namehash;
			
			rethead->numtextures = pwhead->numTextures;
			rethead->texturelist = pwhead->texlistOffset;
			
			rethead->numframes = pwhead->numFrames;
			rethead->framelist = pwhead->framelistOffset;
			break;
		}
		default: {
			printf("i am asplod\n");
			break;
		}
	}
	
	printf("Processed header values:\nnamehash %08x - numtex %d - texoff %08x - numframe %d - frameoff %08x\n",
		rethead->namehash, rethead->numtextures, rethead->texturelist, rethead->numframes, rethead->framelist);
	
	return rethead;
}

textureHeader *getTextureInfos(FILE *f, txpHeader header, unsigned int game) {
	unsigned int i, j;
	textureHeaderPW *pwtex = NULL;
	textureHeaderPO *potex = NULL;
	textureHeader *rethead = malloc(header.numtextures * sizeof(textureHeader));
	fseek(f, header.texturelist, SEEK_SET);
	
	printf("Raw texture values:\n");
	printf(" index ~ wdth - unk1 - hght - unk2 - bpp  - unk3 - comp - unk4 -  offset  - palette\n");
	
	switch(game) {
		case mgspo: {
			potex = malloc(header.numtextures * sizeof(textureHeaderPO));
			fread(potex, header.numtextures * sizeof(textureHeaderPO), 1, f);
			for(i = 0; i < header.numtextures; i++) {
				printf("idx %02d ~ %04x - %04x - %04x - %04x - %04x - %04x - %04x - %04x - %08x - %08x\n", i,
					potex[i].width, potex[i].unknown1, potex[i].height, potex[i].unknown2,
					potex[i].bpp, potex[i].unknown3, potex[i].compressed, potex[i].unknown4,
					potex[i].offset, potex[i].palette);
				
				rethead[i].texpixels = NULL;
				rethead[i].palpixels = NULL;
				rethead[i].bpp = potex[i].bpp;
				rethead[i].compressed = potex[i].compressed;
				rethead[i].width = potex[i].width;
				rethead[i].height = potex[i].height;
				
				rethead[i].offset = potex[i].offset;
				/* portableops has one per-texture palette */
				rethead[i].palette = potex[i].palette;
			}
			free(potex);
			break;
		}
		case mgspw: {
			pwtex = malloc(header.numtextures * sizeof(textureHeaderPW));
			fread(pwtex, header.numtextures * sizeof(textureHeaderPW), 1, f);
			for(i = 0; i < header.numtextures; i++) {
				printf("idx %02d ~ %04x - %04x - %04x - %04x - %04x - %04x\n", i,
					pwtex[i].bpp, pwtex[i].compressed, pwtex[i].width, pwtex[i].unknown1,
					pwtex[i].height, pwtex[i].unknown2);
				for(j = 0; i < 6; j++) printf("%02x ", pwtex[i].unknown3[j]);
				printf("\n%08x - %08x\n", pwtex[i].offset, pwtex[i].zoffset);
				
				rethead[i].texpixels = NULL;
				rethead[i].palpixels = NULL;
				rethead[i].bpp = pwtex[i].bpp;
				rethead[i].compressed = pwtex[i].compressed;
				rethead[i].width = pwtex[i].width;
				rethead[i].height = pwtex[i].height;
				/* peacewalker has two different offsets for compressed and uncompressed */
				rethead[i].offset = pwtex[i].compressed ? pwtex[i].zoffset : pwtex[i].offset;
				/* peacewalker has per-frame palettes, not per-texture palettes */
				rethead[i].palette = 0;
			}
			free(pwtex);
			break;
		}
		default: {
			printf("i am asplod again\n");
			break;
		}
	}
	
	printf("Processed texture data:\n");
	printf(" index ~ bpp  - comp - wdth - hght -  offset  - palette\n");
	for(i = 0; i < header.numtextures; i++) printf("idx %02d ~ %04x - %04x - %04d - %04d - %08x - %08x\n", i,
				rethead[i].bpp, rethead[i].compressed, rethead[i].width, rethead[i].height,
				rethead[i].offset, rethead[i].palette);
	
	return rethead;
}

frameHeader *getFrameInfos(FILE *f, txpHeader header, textureHeader *textures, unsigned int game) {
	unsigned int i;
	frameHeaderPW *pwframe = NULL;
	frameHeaderPO *poframe = NULL;
	frameHeader *rethead = malloc(header.numframes * sizeof(frameHeader));
	fseek(f, header.framelist, SEEK_SET);
	
	printf("Raw frame values:\n");
	
	switch(game) {
		case mgspo: {
			printf(" index ~ unknown1 -   hash   - unknown2 - wdth - hght - unknown3 - texture  - unknown4 -    fx    -    fy    -  fwidth  - fheight  - unknown5\n");
			poframe = malloc(header.numframes * sizeof(frameHeaderPO));
			fread(poframe, header.numframes * sizeof(frameHeaderPO), 1, f);
			for(i = 0; i < header.numframes; i++) {
				printf("idx %02d ~ %08x - %08x - %08x - %04x - %04x - %08x - %08x - %08x - %f - %f - %f - %f - %08x\n", i,
					poframe[i].unknown1, poframe[i].hash, poframe[i].unknown2, poframe[i].width, poframe[i].height,
					poframe[i].unknown3, poframe[i].texture, poframe[i].unknown4,
					poframe[i].fx, poframe[i].fy, poframe[i].fwidth, poframe[i].fheight,
					poframe[i].unknown5);
				
				rethead[i].texpixels = NULL;
				rethead[i].palpixels = NULL;
				rethead[i].hash = poframe[i].hash;
				rethead[i].width = poframe[i].width;
				rethead[i].height = poframe[i].height;
				/* this is just the offset to the matching texture header */
				rethead[i].sourcetexture = (poframe[i].texture - header.texturelist) / 16;
				/* portableops doesnt store the source x and y directly, but as relative floats
				   so we need the sourcetexture first */
				rethead[i].sourcex = (unsigned int)(textures[rethead[i].sourcetexture].width * poframe[i].fx);
				rethead[i].sourcey = (unsigned int)(textures[rethead[i].sourcetexture].height * poframe[i].fy);
				
				/* portableops has one palette per texture */
				rethead[i].palette = textures[rethead[i].sourcetexture].palette;
			}
			free(poframe);
			break;
		}
		case mgspw: {
			pwframe = malloc(header.numframes * sizeof(frameHeaderPW));
			fread(pwframe, header.numframes * sizeof(frameHeaderPW), 1, f);
			for(i = 0; i < header.numframes; i++) {
				printf("idx %02d ~ %08x - %08x - %08x - %08x - %f - %f - %f - %f - %04x - %04x - %04x - %04x\n", i,
					pwframe[i].unknown1, pwframe[i].hash, pwframe[i].texture, pwframe[i].palette,
					pwframe[i].fx, pwframe[i].fy, pwframe[i].fwidth, pwframe[i].fheight,
					pwframe[i].width, pwframe[i].height, pwframe[i].x, pwframe[i].y);
			
				rethead[i].texpixels = NULL;
				rethead[i].palpixels = NULL;
				rethead[i].hash = pwframe[i].hash;
				rethead[i].width = pwframe[i].width;
				rethead[i].height = pwframe[i].height;
				/* this is just the offset to the matching texture header */
				rethead[i].sourcetexture = (pwframe[i].texture - header.texturelist) / 20;
				/* peacewalker has an absolute source x and y stored as well as relative ones */
				rethead[i].sourcex = pwframe[i].x;
				rethead[i].sourcey = pwframe[i].y;
				
				/* peacewalker has one palette per frame */
				rethead[i].palette = pwframe[i].palette;
			}
			free(pwframe);
			break;
		}
		default: {
			printf("TRIPLE FAULT YAY\n");
			break;
		}
	}
	
	printf("Processed frame values:\n");
	printf(" index ~   hash   - wdth - hgth - tx - srcx - srcy - palette\n");
	for(i = 0; i < header.numframes; i++) printf("idx %02d ~ %08x - %04d - %04d - %02d - %04d - %04d - %08x\n", i,
				rethead[i].hash, rethead[i].width, rethead[i].height, rethead[i].sourcetexture,
				rethead[i].sourcex, rethead[i].sourcey, rethead[i].palette);
	
	return rethead;
}

unsigned char *unfiddle(unsigned char *source, unsigned int width, unsigned int height, unsigned int bpp) {
	unsigned char *retpixel = malloc(width*height);
	unsigned int x, y, yc0, yc1, xc0, xc1, memptr = 0;
	for(y = 0; y < height; y++) {
		yc0 = y*16;
		if(bpp == 4) {
			yc1 = y/8*(width*4-128);
			for(x = 0; x < width/2; x++) {
				xc0 = x/16*16;
				xc1 = x/16*128;
				retpixel[memptr] = source[x-xc0+xc1+yc0+yc1] & 0xF;
				memptr++;
				retpixel[memptr] = source[x-xc0+xc1+yc0+yc1] >> 4;
				memptr++;
			}
		}
		else {
			yc1 = y/8*(width*8-128);
			for(x = 0; x < width; x++) {
				xc0 = x/16*16;
				xc1 = x/16*128;
				retpixel[memptr] = source[x-xc0+xc1+yc0+yc1];
				memptr++;
			}
		}
	}
	return retpixel;
}

paletteEntry *buildPalette(unsigned char *source, unsigned int bpp) {
	paletteEntry *retpal = NULL;
	unsigned int palsize;
	palsize = (bpp==4) ? 16:256;
	retpal = malloc(sizeof(paletteEntry)*palsize);
	unsigned int i, e = 0;
	for(i = 0; i < palsize; i++) {
		retpal[i].red = source[e];
		e++;
		retpal[i].green = source[e];
		e++;
		retpal[i].blue = source[e];
		e++;
		retpal[i].alpha = source[e];
		e++;
	}
	return retpal;
}

void getTexturePixels(FILE *f, txpHeader header, textureHeader *textures) {
	unsigned int i, j, texsize, palsize;
	long unsigned int decompressed;
	uint32_t compressedsize;
	unsigned char *compressedpixels = NULL, *uncompressedpixels = NULL/*, *processedpixels = NULL*/, *palette = NULL;
	for(i = 0; i < header.numtextures; i++) {
		printf("Texture %02d ", i);
		texsize = textures[i].width * textures[i].height;
		if(textures[i].bpp == 4) texsize /= 2;
		uncompressedpixels = malloc(texsize);
		fseek(f, textures[i].offset, SEEK_SET);
		if(textures[i].compressed) {
			printf("Decompressing...");
			fread(&compressedsize, 4, 1, 0);
			compressedpixels = malloc(compressedsize);
			fread(compressedpixels, compressedsize, 1, f);
			if((j = uncompress(uncompressedpixels, &decompressed, compressedpixels, compressedsize)) != Z_OK) {
			//~ if((j = inflate(&strm, Z_FINISH)) != Z_OK) {
				printf("=====================================couldnt decompress data\n");
				switch(j) {
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
				printf("decompressed %08lx\n", decompressed);
				return;
			}
			free(compressedpixels);
		}
		else fread(uncompressedpixels, texsize, 1, f);
		
		printf("Unfiddling...");
		textures[i].texpixels = unfiddle(uncompressedpixels, textures[i].width, textures[i].height, textures[i].bpp);
		free(uncompressedpixels);
		
		if(textures[i].palette) {
			printf("Getting palette...");
			fseek(f, textures[i].palette, SEEK_SET);
			switch(textures[i].bpp) {
				case 4: {
					palsize = 16*4;
					break;
				}
				case 5: {
					palsize = 256*4;
					break;
				}
				case 6: {
					fseek(f, 20, SEEK_CUR);
					fread(&compressedsize, 4, 1, f);
					palsize = compressedsize;
					fseek(f, textures[i].palette, SEEK_SET);
					break;
				}
			}
			palette = malloc(palsize);
			fread(palette, palsize, 1, f);
			textures[i].palpixels = buildPalette(palette, textures[i].bpp);
			free(palette);
		}
		printf("\n");
	}
	return;
}

void getFramePixels(FILE *f, txpHeader header, textureHeader *textures, frameHeader *frames) {
	unsigned int i/*, j*/, palsize;
	uint32_t compressedsize;
	unsigned char *palette = NULL;
	for(i = 0; i < header.numframes; i++) {
		frames[i].texpixels = textures[frames[i].sourcetexture].texpixels;
		if(frames[i].palette == textures[frames[i].sourcetexture].palette) frames[i].palpixels = textures[frames[i].sourcetexture].palpixels;
		else {
			fseek(f, frames[i].palette, SEEK_SET);
			switch(textures[frames[i].sourcetexture].bpp) {
				case 4: {
					palsize = 16*4;
					break;
				}
				case 5: {
					palsize = 256*4;
					break;
				}
				case 6: {
					fseek(f, 20, SEEK_CUR);
					fread(&compressedsize, 4, 1, f);
					palsize = compressedsize;
					fseek(f, textures[i].palette, SEEK_SET);
					break;
				}
			}
			
			palette = malloc(palsize);
			fread(palette, palsize, 1, f);
			frames[i].palpixels = buildPalette(palette, textures[frames[i].sourcetexture].bpp);
			free(palette);
		}
	}
	return;
}

unsigned char *buildSingleFrameRgba(textureHeader *textures, frameHeader frame) {
	unsigned int x, y, mempointer = 0, framesize = frame.width * frame.height;
	unsigned char *retframe = malloc(framesize * 4), pixel;
	for(y = frame.sourcey; y < frame.sourcey+frame.height; y++) {
		for(x = frame.sourcex; x < frame.sourcex+frame.width; x++) {
			pixel = frame.texpixels[x+y*textures[frame.sourcetexture].width];
			retframe[mempointer] = frame.palpixels[pixel].red;
			mempointer++;
			retframe[mempointer] = frame.palpixels[pixel].green;
			mempointer++;
			retframe[mempointer] = frame.palpixels[pixel].blue;
			mempointer++;
			retframe[mempointer] = frame.palpixels[pixel].alpha;
			mempointer++;
		}
	}
	return retframe;
}

int main( int argc, char **argv ) {
	
	unsigned char headerdata[32], *readyframe = NULL;
	uint32_t *gamecheck = NULL;
	
	unsigned int i, game;
	
	txpHeader *header = NULL;
	textureHeader *textures = NULL;
	frameHeader *frames = NULL;
	
	char *outputname = NULL;
	
	FILE *f;
	
	if( argc < 2 ) {
		printf("Not enough args!\nUse: %s TXP-file\n", argv[0]);
		return 1;
	}
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	
	outputname = malloc( strlen( argv[1] ) + 7 + 9 + 4 + 1 ); // will contain: infilename + "-tex###" + "-frame###" + ".png" + NULL
	
	fread( headerdata, 32, 1, f );
	gamecheck = (uint32_t *)(headerdata+4);
	
	game = (*gamecheck < 256) ? mgspo : mgspw;
	printf("TXP is %s compatible\n", (*gamecheck < 256) ? "portableops" : "peacewalker");
	
	header = getTxpHeader(headerdata, game);
	textures = getTextureInfos(f, *header, game);
	frames = getFrameInfos(f, *header, textures, game);
	
	getTexturePixels(f, *header, textures);
	getFramePixels(f, *header, textures, frames);
	
	for(i = 0; i < header->numframes; i++) {
	//~ for(i = 0; i < 1; i++) {
		printf("Processing frame %02d of %02d (texture %02d)...", i, header->numframes-1, frames[i].sourcetexture);
		fflush(stdout);
		readyframe = buildSingleFrameRgba(textures, frames[i]);
		printf("built ");
		fflush(stdout);
		sprintf(outputname, "%s-tex%03d-frame%03d.png", argv[1], frames[i].sourcetexture, i);
		lodepng_encode32_file(outputname, readyframe, frames[i].width, frames[i].height);
		printf("encoded ");
		fflush(stdout);
		free(readyframe);
		printf("managed\n");
		fflush(stdout);
	}
	
	fclose(f);
	
	for(i = 0; i < header->numtextures; i++) {
		free(textures[i].texpixels);
		if(textures[i].palpixels) free(textures[i].palpixels);
	}
	if(game == mgspw) for(i = 0; i < header->numframes; i++) free(frames[i].palpixels);
	
	free(header);
	free(textures);
	free(frames);
	free(outputname);
	
	printf("Done.\n");
	return 0;
}
