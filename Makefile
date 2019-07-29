STRCODE = strcode/strcode.o
STAGEDIC = stage-dictionary.o
LODEPNG = lodepng/lodepng.o

all: archive dictionary graphics scripts
archive: stages dar face-extract.elf zar-extract.elf qar-extract_psp.elf dat-merge.elf
stages: stage-extract.elf  dat-extract_enc.elf
dar: dar-extract_pc.elf dar-extract_psx.elf dar-extract_psp.elf
dictionary: simple-hash.elf simple-hash-list.elf
graphics: txp-convert.elf
scripts: gcx-decompile.elf

stage-extract.elf: $(STRCODE) $(STAGEDIC) stage-extract.c
	$(CC) -Wall -g -o $@ $^
	
dar-extract_psx.elf: $(STRCODE) $(STAGEDIC) dar-extract_psx.c
	$(CC) -Wall -g -o $@ $^
	
dat-extract_enc.elf: $(STRCODE) $(STAGEDIC) dat-extract_enc.c
	$(CC) -Wall -g -o $@ $^ -lz
	
face-extract.elf: $(STRCODE) $(LODEPNG) $(STAGEDIC) face-extract.c
	$(CC) -Wall -g -o $@ $^
	
txp-convert.elf: $(LODEPNG) txp-convert.c
	$(CC) -Wall -g -o $@ $^ -lz
	
zar-extract.elf: zar-extract.c
	$(CC) -Wall -g -o $@ $^ -lz

simple-hash.elf: $(STRCODE) simple-hash.c
	$(CC) -Wall -g -o $@ $^
	
simple-hash-list.elf: $(STRCODE) simple-hash-list.c
	$(CC) -Wall -g -o $@ $^

%.elf: %.c
	$(CC) -Wall -g -o $@ $<
	
%.o: %.c
	$(CC) -c -Wall -g -o $@ $<
	
clean:
	-rm *.elf *.o strcode/*.o lodepng/*.o
