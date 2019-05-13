#ifndef KOJIMAHASH_H_
#define KOJIMAHASH_H_

/* hashing functions used in kojima games */

/* returns 16bit hash, used in MGS1 (use all 16 for names, the lower 8 for extensions) */
/* extensions are only hashing the first character of it, e.g. azm would hash a */
/* returns uint32_t to allow for easier use in dictionary handling */
uint32_t hashstring16(char *string);

/* returns 24bit hash, used in MGS2 and newer (only lower 24 used, high 8 will always be 0) */
/* extensions are, again, only hashing the first character of it */
uint32_t hashstring24(char *string);

/* return 32bit hash, used in ZoE2 exclusively */
/* the extension is hashed completely */
uint32_t hashstring32(char *string);

#endif