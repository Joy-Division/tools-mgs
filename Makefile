#
# Makefile for "Arms Depot"
# METAL GEAR SOLID Toolbox
#

GCC_WIN32 = i686-w64-mingw32-gcc
GXX_WIN32 = i686-w64-mingw32-g++
GCC_WIN64 = x86_64-w64-mingw32-gcc
GXX_WIN64 = x86_64-w64-mingw32-g++
GCC_ARGS  = -Wall -g

#---------------------------------------------------------------------------#

all: \
	archive \
	dictionary \
	graphics \
	scripts

archive: \
	stage dar \
	face-extract \
	zar-extract \
	qar-extract_psp \
	dat-merge

stage: \
	stage-extract \
	dat-extract_enc

dar: \
	dar-extract_pc \
	dar-extract_psx \
	dar-extract_psp

dictionary: \
	simple-hash \
	simple-hash-list

graphics: \
	txp-convert

scripts: \
	gcx-decompile

#---------------------------------------------------------------------------#
# Objects
#---------------------------------------------------------------------------#

LODEPNG  = lodepng/lodepng.o
STRCODE  = strcode/strcode.o
STAGEDIC = stage-dictionary.o

%.o: %.c
	$(CC) $(GCC_ARGS) -o $@ -c $<

#---------------------------------------------------------------------------#
# Targets
#---------------------------------------------------------------------------#

# --- archive ---
face-extract: $(STRCODE) $(LODEPNG) $(STAGEDIC) face-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^

zar-extract: zar-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz

qar-extract_psp: qar-extract_psp.c
	$(CC) $(GCC_ARGS) -o $@ $<

dat-merge: dat-merge.c
	$(CC) $(GCC_ARGS) -o $@ $<

# --- stage ---
stage-extract: $(STRCODE) $(STAGEDIC) stage-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^

dat-extract_enc: $(STRCODE) $(STAGEDIC) dat-extract_enc.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz

# --- dar ---
dar-extract_pc: dar-extract_pc.c
	$(CC) $(GCC_ARGS) -o $@ $<

dar-extract_psx: $(STRCODE) $(STAGEDIC) dar-extract_psx.c
	$(CC) $(GCC_ARGS) -o $@ $^

dar-extract_psp: dar-extract_psp.c
	$(CC) $(GCC_ARGS) -o $@ $<

# --- dictionary ---
simple-hash: $(STRCODE) simple-hash.c
	$(CC) $(GCC_ARGS) -o $@ $^

simple-hash-list: $(STRCODE) simple-hash-list.c
	$(CC) $(GCC_ARGS) -o $@ $^

# --- graphics ---
txp-convert: $(LODEPNG) txp-convert.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz

# --- scripts ---
gcx-decompile: gcx-decompile.c
	$(CC) $(GCC_ARGS) -o $@ $<

#---------------------------------------------------------------------------#

clean: \
	clean_obj \
	clean_exe

clean_obj:
	-rm *.o
	-rm $(LODEPNG)
	-rm $(STRCODE)
	-rm $(STAGEDIC)

clean_exe:
	-rm *.exe
	-rm face-extract
	-rm zar-extract
	-rm qar-extract_psp
	-rm dat-merge
	-rm stage-extract
	-rm dat-extract_enc
	-rm dar-extract_pc
	-rm dar-extract_psx
	-rm dar-extract_psp
	-rm simple-hash
	-rm simple-hash-list
	-rm txp-convert
	-rm gcx-decompile
