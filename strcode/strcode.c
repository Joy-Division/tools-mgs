/*
 * String Hash Functions
 */

// "METAL GEAR SOLID" 16-bit hash function
unsigned int StrCode16( char *string )
{
	unsigned char c;
	unsigned char *p = (unsigned char *)string;
	unsigned short id = 0;
	
	while ( c = *p++ )
	{
		id = (( id >> 11 ) | ( id << 5 ));
		id += c;
	}
	return id;
}

// "METAL GEAR SOLID 2" 24-bit hash function
unsigned int StrCode24( char *string )
{
	unsigned char c;
	unsigned char *p = (unsigned char *)string;
	unsigned int id, mask = 0x00FFFFFF;
	
	for ( id = 0 ; c = *p ; p++ )
	{
		id = (( id >> 19 ) | ( id << 5 ));
		id += c;
		id &= mask;
	}
	return ( !id ) ? 1 : id;
}

// "ZONE OF THE ENDERS" 32-bit hash function
unsigned int StrCode32( char *string )
{
	unsigned int c;
	signed   int n  = 0;
	unsigned int id = 0;
	
	while ( c = *string++ )
	{
		id += (( id << ( c & 0x0F )) | (( id >> 3 ) + ( c << ( n & 0x0F )) + c ));
		n++;
	}
	return id;
}
