/*
	Copyright (C) 2019 Missingno_force a.k.a. Missingmew
	See LICENSE for details.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

uint32_t hashstring16(char *string) {
	unsigned char *work = (unsigned char *)string;
	uint32_t rethash = 0;
	while(*work) rethash = (((rethash << 0x05) | (rethash >> 0x0B)) + *work++) & 0x0000FFFF;
	return rethash;
}

uint32_t hashstring24(char *string) {
	unsigned char *work = (unsigned char *)string;
	uint32_t rethash = 0;
	while(*work) rethash = (((rethash << 0x05) | (rethash >> 0x13)) + *work++) & 0x00FFFFFF;
	return rethash;
}

uint32_t hashstring32(char *string) {
	unsigned int i;
	unsigned char *work = (unsigned char *)string;
	uint32_t rethash = 0;
	for(i = 0; work[i]; i++) rethash += (rethash << (work[i] & 0x0F)) | ((rethash >> 0x03) + (work[i] << (i & 0x0F)) + work[i]);
	return rethash;
}