#ifndef __NETWORK_STREAM_H
#define __NETWORK_STREAM_H

#include <iostream>
#include "lib/math/vector3.h"

// Used for ints, floats and doubles, taking into account endianness

void WriteDynamicString ( std::ostream &output, const char *string );   		// Works with NULL
char *ReadDynamicString ( std::istream &input, int maxStringLength, const char *safeString ); // Assigns space for string

void WriteUnicodeString ( std::ostream &output, const wchar_t *unicode );
wchar_t *ReadUnicodeString( std::istream &input, int maxLength, const wchar_t *safeString );

void WritePackedInt     ( std::ostream &output, int _value );
int  ReadPackedInt      ( std::istream &input );

void WriteVoidData      ( std::ostream &output, const void *data, int dataLen );       
void *ReadVoidData      ( std::istream &input, int *dataLen );				// Assigns space for data


std::istream &ReadNetworkValue( std::istream &input, bool &v );
std::ostream &WriteNetworkValue( std::ostream &output, bool v );

std::istream &ReadNetworkValue( std::istream &input, char &v );
std::ostream &WriteNetworkValue( std::ostream &output, char v );

std::istream &ReadNetworkValue( std::istream &input, Vector3 &v );
std::ostream &WriteNetworkValue( std::ostream &output, const Vector3 &v );

template <class T>
std::istream &ReadNetworkValue( std::istream &input, T &v );

template <class T>
std::ostream &WriteNetworkValue( std::ostream &output, T v );

template <class T>
void WriteNumbersData	( std::ostream &output, const T *data, int numElems );  

template <class T>
T *ReadNumbersData		( std::istream &input, int *numElems );				// Assigns space for data

#include "network_stream_inlines.cpp"

#endif

