#include "lib/universal_include.h"

#include <unrar/rarbloat.h>
#include <unrar/sha1.h>


void HashData( const char *_data, int _hashToken, char *_result )
{
	hash_context c;
	hash_initial(&c);

	char fullString[512];
	sprintf( fullString, "%s-%d", _data, _hashToken );

	hash_process(&c, (unsigned char *) fullString, strlen(fullString));

    uint32 hash[5];
	hash_final(&c, hash);
	
    sprintf(_result, "hsh%04x%04x%04x%04x%04x", hash[0], hash[1], hash[2], hash[3], hash[4]);
}


void HashData( char *_data, char *_result )
{
    hash_context c;
    hash_initial(&c);

    hash_process(&c, (unsigned char *) _data, strlen(_data));

    uint32 hash[5];
    hash_final(&c, hash);

    sprintf(_result, "hsh%04x%04x%04x%04x%04x", hash[0], hash[1], hash[2], hash[3], hash[4]);
}


void HashDataVoid( void *_data, int _dataLength, unsigned *_hash /* Array of 5 unsigned ints */ )
{
	hash_context c;
	hash_initial(&c);
	hash_process(&c, (unsigned char *) _data, _dataLength);	
	hash_final(&c, _hash );
}

