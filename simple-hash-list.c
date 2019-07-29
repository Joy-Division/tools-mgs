/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "strcode/strcode.h"

#define BREAK(x) { printf(x); return 1; }

int main( int argc, char** argv ) {
	
	if ( argc < 2 )
	{
		printf( "Not enough arguments given!\nUsage: %s textlist\n", argv[0] );
		return 1;
	}
	
	uint16_t hash1;
	uint32_t hash2;
	uint32_t hash3;
	FILE *tin, *tout;
	
	if( !(tin = fopen( argv[1], "r" ))) {
		printf("Couldnt open file %s as textlist\n", argv[1]);
		return 1;
	}
	
	char scanname[512] = {0}, *outtext = malloc(strlen(argv[1])+5+4+1);
	sprintf(outtext, "%s-hash.txt", argv[1]);
	
	if( !(tout = fopen( outtext, "w" ))) {
		printf("Couldnt open file %s as outtext\n", outtext);
		return 1;
	}
	
	while(fgets(scanname, 512, tin)) {
		if(!(strncmp(scanname, "//", 2))) fprintf(tout, "%s", scanname);
		else {
			scanname[strcspn(scanname,"\r\n")] = 0;
			hash1 = StrCode16(scanname);
			hash2 = StrCode24(scanname);
			hash3 = StrCode32(scanname);
			fprintf(tout, "%-21s %04x %06x %08x\n", scanname, hash1, hash2, hash3);
		}
	}
	
	fclose(tin);
	fclose(tout);
	printf("Done.\n");
	return 0;
}
