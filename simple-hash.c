/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "strcode/strcode.h"

#define BREAK(x) { printf(x); return 1; }

int main( int argc, char** argv ) {
	
	if ( argc < 2 )
	{
		printf( "Not enough arguments given!\nUsage: %s text\n", argv[0] );
		return 1;
	}
	uint16_t hash1;
	uint32_t hash2;
	uint32_t hash3;
	if(strlen(argv[1]) > 512) BREAK("donut: the abuse\n");
	printf("Hashing the following string:\n%s\n", argv[1]);
	hash1 = StrCode16(argv[1]);
	hash2 = StrCode24(argv[1]);
	hash3 = StrCode32(argv[1]);
	printf("Hash16(MGS1):     %04x\n", hash1);
	printf("Hash24(MGS2+):  %06x\n", hash2);
	printf("Hash32(ZoE2): %08x\n", hash3);
	printf("Done.\n");
	return 0;
}
