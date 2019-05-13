#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BREAK(x) { printf(x); return 1; }

#ifdef _WIN32
#include <direct.h>
#define createDirectory(dirname) mkdir(dirname)
#define cleanExecPath(path) *strrchr(path, '\\') = 0
#define bswap_16(val) \
 ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

// Swap 4 byte, 32 bit values:

#define bswap_32(val) \
 ( (((val) >> 24) & 0x000000FF) | (((val) >>  8) & 0x0000FF00) | \
   (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )

// Swap 8 byte, 64 bit values:

#define bswap_64(val) \
 ( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
   (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | \
   (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
   (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <byteswap.h>
#define cleanExecPath(path) *strrchr(path, '/') = 0
#define createDirectory(dirname) mkdir(dirname, 0777)
#endif

#define sizeofarr(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
	uint16_t hash;
	char name[32]; // this should be enough?
} gcx_command;

gcx_command gcxcommands[] = {
	{ 0x64C0, "eval" },
	{ 0x0D86, "if" },
	{ 0xC8BB, "load" },
	{ 0x9906, "chara" },
	// foo
	{ 0x226D, "menu" },
	{ 0x22FF, "mesg" },
	{ 0x306A, "light" },
	{ 0x430D, "delay" },
	{ 0x4AD9, "system" },
	{ 0x5C9E, "varsave" },
	{ 0x698D, "sound" },
	{ 0x9A1F, "start" },
	{ 0xA242, "demo" },
};

typedef struct {
	uint16_t name;
	uint16_t off;
}__attribute__((packed)) gcxdef;

uint16_t gcx_proc(gcxdef def, unsigned char *data, FILE *out);
uint16_t gcx_cmd(unsigned char *data, FILE *out);

int gcx_iscommand(unsigned char *data) {
	unsigned int i;
	uint16_t cmd = bswap_16(*(uint16_t *)data);
	for(i = 0; i < sizeofarr(gcxcommands); i++) {
		if(gcxcommands[i].hash == cmd) return i;
	}
	return -1;
}

uint16_t gcx_param(unsigned char *data, FILE *out) {
	unsigned int i;
	uint16_t size = 0;
	gcxdef blank;
	blank.name = 0;
	blank.off = 0;
	printf("gcx_param == type %04x == size ????\n", data[0]);
	switch(data[0]) {
		case 0x30: {
			size = data[1];
			fprintf(out, "STACKOP!\n");
			break;
		}
		case 0x40: {
			gcx_proc(blank, data, out);
			break;
		}
		case 0x50: {
			// size of params plus param letter
			size = data[2] + 1;
			switch(data[1]) {
				case 'm': {
					fprintf(out, "-m [%02x]:%04x\n", data[3], bswap_16(*(uint16_t *)(data+4)));
					break;
				}
				case 's': {
					fprintf(out, "-s [%02x]:%02x\n", data[3], data[4]);
					break;
				}
				default: {
					fprintf(out, "-%c ", *(char *)(data+1));
					for(i = 0; i < (size-1); size++) fprintf(out, "%02x", data[3+i]);
					fprintf(out, "\n");
					break;
				}
			}
			break;
		}
		default: {
			printf("unknown param %02x size %04x\n", data[0], size);
			break;
		}
	}
	return size+1;
}

uint16_t gcx_cmd(unsigned char *data, FILE *out) {
	int cmd = gcx_iscommand(data);
	uint16_t size = bswap_16(*(uint16_t *)(data+2)) >> 8, cur = 0;
	printf("gcx_cmd == cmd %s (%d) == size %04x\n", gcxcommands[cmd].name, cmd, size);
	fprintf(out, "%s(", gcxcommands[cmd].name);
	cur = 3;
	switch(cmd) {
		case 0: // eval
		case 3:
		case 1: { // if
			while(data[cur]) cur += gcx_param(data+cur, out);
			break;
		}
		case 2: { // load
			fprintf(out, "%02x \"%s\"\n", data[cur], (char *)(data+cur+2));
			cur += data[cur];
			while(data[cur]) cur += gcx_param(data+cur, out);
			break;
		}
		default: {
			printf("unknown cmd %02x size %04x\n", cmd, size);
			break;
		}
	}
	fprintf(out, ")\n");
	return size+2;
}

uint16_t gcx_proc(gcxdef def, unsigned char *data, FILE *out) {
	uint16_t size = bswap_16(*(uint16_t *)(data + 1)), cur = 0;
	printf("gcx_proc == type %02x == size %04x\n", data[0], size);
	switch(data[0]) {
		case 0x40: {
			fprintf(out, "proc %04x {\n", def.name);
			cur = 3;
			while(data[cur]) cur += gcx_proc(def, data+cur, out);
			fprintf(out, "}\n");
			break;
		}
		case 0x60: {
			fprintf(out, "begin {\n");
			cur = 3;
			while(gcx_iscommand(data+cur) > -1) cur += gcx_cmd(data+cur, out);
			fprintf(out, "}\n");
			break;
		}
		case 0x70: {
			size >>= 8;
			fprintf(out, "call %04x\n", bswap_16(*(uint16_t *)(data+2)));
			break;
		}
		default: {
			printf("unknown type %02x size %04x\n", data[0], size);
			break;
		}
	}
	return (size+1);
}

int main( int argc, char** argv ) {
	
	if ( argc < 2 )
	{
		printf( "Not enough arguments given!\nUsage: %s file.gcx\n", argv[0] );
		return 1;
	}
	
	FILE *f, *o;
	unsigned int i, j, numdefs;
	unsigned long int used;
	unsigned char *gcxdata = NULL, **procs = NULL, *endofproc = NULL;
	char *outputname = NULL;
	
	gcxdef *deflist = NULL;
	
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	outputname = malloc(strlen(argv[1])+4+1);
	strcpy(outputname, argv[1]);
	strcat(outputname, ".txt");
	
	if( !(o = fopen(outputname, "w" ))) {
		printf("Couldnt open file %s\n", outputname);
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	i = ftell(f);
	fseek(f, 0, SEEK_SET);
	gcxdata = malloc(i);
	fread(gcxdata, i, 1, f);
	fclose(f);
	
	deflist = (gcxdef *)(gcxdata+4);
	
	for(numdefs = 0; deflist[numdefs].name; numdefs++) {
		deflist[numdefs].off = bswap_16(deflist[numdefs].off);
		deflist[numdefs].name = bswap_16(deflist[numdefs].name);
		printf("proc %02u - hash %04x - offset %04x\n", numdefs, deflist[numdefs].name, deflist[numdefs].off);
	}
	printf("have %u definitions\n", numdefs);
	
	procs = malloc(sizeof(unsigned char *) * numdefs);
	// skip 4 bytes at beginning, skip numdefs procdefs, skip 4 zerobytes at end of proctable, add proc off
	for(i = 0; i < numdefs; i++) procs[i] = (gcxdata + 4 + sizeof(gcxdef)*numdefs + 4 + deflist[i].off);
	
	for(i = 0; i < numdefs; i++) {
	//~ for(i = 0; i < 1; i++) {
		printf("parsing proc %u %04x\n", i, deflist[i].name);
		printf("proc is at %08lx\n", procs[i]-gcxdata);
		used = gcx_proc(deflist[i], procs[i], o);
		endofproc = procs[i];
		for(j = 0; j < used; j++) endofproc++;
		printf("end of proc is at %08lx\n", endofproc-gcxdata);
		fprintf(o, "\n");
	}
	
	fclose(o);
	free(outputname);
	free(gcxdata);
	free(procs);
	printf("Done.\n");
	return 0;
}