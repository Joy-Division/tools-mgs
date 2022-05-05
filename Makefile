##
##	Makefile for "Arms Depot"
##	( METAL GEAR SOLID Toolbox )
##

###############################################################################

OLEVEL                  ?= 2
GLEVEL                  ?=

CFLAGS                  += -g$(GLEVEL)
CFLAGS                  += -O$(OLEVEL)
CFLAGS                  += -Wall -Wno-comment

CXXFLAGS                := $(CFLAGS)

LDFLAGS                 += -Wl,-Map,$(basename $@).map

###############################################################################
.PHONY: default all

default: all

TARGETS                 :=\
                        face-extract     \
                        stage-extract    \
                        dat-merge        \
                        dat-extract_enc  \
                        dar-extract_psx  \
                        dar-extract_pc   \
                        dar-extract_psp  \
                        qar-extract_psp  \
                        zar-extract      \
                        simple-hash      \
                        simple-hash-list \
                        txp-convert      \
                        gcx-decompile

all: $(TARGETS)

###############################################################################

LODEPNG.O               := lodepng/lodepng.o
STRCODE.O               := strcode/strcode.o
STAGEDIC.O              := stage-dictionary.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

#------------------------------------------------------------------------------
# ARCHIVE ( MGS1 *.dat )
#------------------------------------------------------------------------------

face-extract: $(STRCODE.O) $(LODEPNG.O) $(STAGEDIC.O) face-extract.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage-extract: $(STRCODE.O) $(STAGEDIC.O) stage-extract.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#------------------------------------------------------------------------------
# ARCHIVE ( MGS2+ *.dat )
#------------------------------------------------------------------------------

dat-merge: dat-merge.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

dat-extract_enc: $(STRCODE.O) $(STAGEDIC.O) dat-extract_enc.c
	$(CC) $(CFLAGS) -o $@ $^ -lz $(LDFLAGS)

#------------------------------------------------------------------------------
# ARCHIVE ( dar/qar )
#------------------------------------------------------------------------------

dar-extract_psx: $(STRCODE.O) $(STAGEDIC.O) dar-extract_psx.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

dar-extract_pc: dar-extract_pc.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

dar-extract_psp: dar-extract_psp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

qar-extract_psp: qar-extract_psp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

zar-extract: zar-extract.c
	$(CC) $(CFLAGS) -o $@ $^ -lz $(LDFLAGS)

#------------------------------------------------------------------------------
# StrCode Utils
#------------------------------------------------------------------------------

simple-hash: $(STRCODE.O) simple-hash.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simple-hash-list: $(STRCODE.O) simple-hash-list.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#------------------------------------------------------------------------------
# TEXTURE
#------------------------------------------------------------------------------

txp-convert: $(LODEPNG.O) txp-convert.c
	$(CC) $(CFLAGS) -o $@ $^ -lz $(LDFLAGS)

#------------------------------------------------------------------------------
# GCL SCRIPT
#------------------------------------------------------------------------------

gcx-decompile: gcx-decompile.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

###############################################################################
.PHONY: clean

clean:
	$(RM) *.o
	$(RM) *.map
	$(RM) $(LODEPNG.O)
	$(RM) $(STRCODE.O)
	$(RM) $(STAGEDIC.O)
	$(RM) $(TARGETS)
