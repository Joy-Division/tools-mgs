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

PROC_APPEND_EXT = mv $@ $@.elf

ifeq ($(OS),Windows_NT)
PROC_APPEND_EXT =
endif

# --- archive ---
face-extract: $(STRCODE) $(LODEPNG) $(STAGEDIC) face-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^
	$(PROC_APPEND_EXT)

zar-extract: zar-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz
	$(PROC_APPEND_EXT)

qar-extract_psp: qar-extract_psp.c
	$(CC) $(GCC_ARGS) -o $@ $<
	$(PROC_APPEND_EXT)

dat-merge: dat-merge.c
	$(CC) $(GCC_ARGS) -o $@ $<
	$(PROC_APPEND_EXT)

# --- stage ---
stage-extract: $(STRCODE) $(STAGEDIC) stage-extract.c
	$(CC) $(GCC_ARGS) -o $@ $^
	$(PROC_APPEND_EXT)

dat-extract_enc: $(STRCODE) $(STAGEDIC) dat-extract_enc.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz
	$(PROC_APPEND_EXT)

# --- dar ---
dar-extract_pc: dar-extract_pc.c
	$(CC) $(GCC_ARGS) -o $@ $<
	$(PROC_APPEND_EXT)

dar-extract_psx: $(STRCODE) $(STAGEDIC) dar-extract_psx.c
	$(CC) $(GCC_ARGS) -o $@ $^
	$(PROC_APPEND_EXT)

dar-extract_psp: dar-extract_psp.c
	$(CC) $(GCC_ARGS) -o $@ $<
	$(PROC_APPEND_EXT)

# --- dictionary ---
simple-hash: $(STRCODE) simple-hash.c
	$(CC) $(GCC_ARGS) -o $@ $^
	$(PROC_APPEND_EXT)

simple-hash-list: $(STRCODE) simple-hash-list.c
	$(CC) $(GCC_ARGS) -o $@ $^
	$(PROC_APPEND_EXT)

# --- graphics ---
txp-convert: $(LODEPNG) txp-convert.c
	$(CC) $(GCC_ARGS) -o $@ $^ -lz
	$(PROC_APPEND_EXT)

# --- scripts ---
gcx-decompile: gcx-decompile.c
	$(CC) $(GCC_ARGS) -o $@ $<
	$(PROC_APPEND_EXT)

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
	-rm *.exe *.elf
