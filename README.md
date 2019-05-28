# Arms Depot

Tools for working with files from Metal Gear Solid

---------

# Building and debugging

Building should only require a reasonably recent C compiler (e.g. GCC) and make, any required libraries are currently bundled and may be exchanged for submodules in the future (if possible).

The following libraries are currently bundled with the project and will be exchanged for git submodules in the future:
- [lodepng](https://github.com/lvandeve/lodepng)

As long as the code is still included in this repository, see the individual libraries folder for licensing of it

For building on windows, slight changes to the Makefile may be required (like changing the extensions from .elf to .exe), the code of all tools should already be compatible

Useful things for debugging are:
- gdb
- valgrind

Additionally for comparison with actual game output:
- recordings of gameplay 
- working emulator setup to play the game on your system

**Note:** Code quality and style may vary wildly between different code files. Feel free to propose a style to be used (but do not count on it).
Minimal error checking is in place, but assume that any unexpected input will result in your system eating your cat.

# Overview of included tools

## dar-extract_*, qar-extract_psp, zar-extract
Tools for unpacking archive files from the corresponding platform. dar-extract_psx supports dictionaries for giving proper names to the files within.
To use the dictionary feature, place a dictionary file ``somename.txt`` in the same folder as the executable, then use ``dar-extract_psx file somename``

## dat-extract_enc
Tool for unpacking STAGE.DAT and FACE.DAT from MGS2, MGS3, MGS4, ZoE2 and MGS Twin Snakes.
Supports dictionaries, place a file called ``common.txt``, suffixed with the game name, or one matching a stages name in the same folder as the executable, prefixes with your game name (e.g. ``mgs2-``, ``trial2-``, ``tts-``, etc.).
Dictionaries will be loaded automatically and do not need to be passed by hand.
If a file fails with the autodetection (either missing or mismatched game ID), you can pass an ID as additional argument to override the autodetection. Use this at your own risk, check the code (specifically the switch around line 870) for IDs.

## dat-merge
Tool to unpack the files from the on-disc VFS from MGS2S on Xbox. Currently, this requires the VFS files to be merged by hand. Remember to back-up your files before doing this.  
To do so, simply concatenate the files, e.g. ``cat disc1_{0..8}.dat > disc1.dat`` or ``copy /B disc1_0.dat + disc1_1.dat + [...] + disc1_8.dat disc1.dat``.
Then pass the resulting disc1.dat to dat-merge. Follow the same procedure for disc2_*.dat. The unpacked files can be unpacked just like the files from the PS2 games.

## face-extract
Tool for unpacking MGS1 FACE.DAT, supports a dictionary as well. Place one with the name ``mgs1-face.txt`` next to the executable.

## gcx-decompile
Tool for decoding a gcx script into readable text. Probably incomplete.

## simple-hash
Pass a string to compute its kojimahash for all known flavours.

## stage-extract
Tool to unpack MGS1 STAGE.DIR, supports dictionaries. See dat-extract_enc for naming convention.

## txp-convert
Converts txp textures found in the PSP games to PNG.

# Other code included

## stage-dictionary
Code for handling the dictionaries used by the unpacking tools.

## kojimahash/
Contains functions to compute the hashes used for matching dictionaries against hashes contained within the archives.

# Notes

MGS1 uses extremely weak 16bit hashing which is incredibly prone to collisions. If you build/use dictionaries with any MGS1 tool, make as much use of the separation by stage and store only surefire names in ``common``.  
The 24bit hashing used in MGS2 succeeding games is better, but still very likely to have collisions, tread with caution.
MGS1 and 2 only store the first character of the extension in the actual files. If no dictionary (entry) is found for a file, it'll be saved with that single character intact.

# License
Unless noted otherwise in a source file, see [LICENSE](LICENSE) for details. For bundled libraries, see above.
