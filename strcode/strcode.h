/*
 * String Hash Functions
 */
#ifndef INC_STRCODE_H
#define INC_STRCODE_H

// 16-bit hash used in MGS1
unsigned int StrCode16( char *string );

// 24-bit hash used in MGS2+
unsigned int StrCode24( char *string );

// 32-bit hash used in ZOE
unsigned int StrCode32( char *string );

#endif // INC_STRCODE_H