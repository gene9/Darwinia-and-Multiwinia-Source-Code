#ifndef _RAR_TYPES_
#define _RAR_TYPES_


typedef unsigned int     uint32; //32 bits exactly
#define PRESENT_INT32

#if defined(_WIN_32) || defined(__GNUC__) || defined(__sgi) || defined(_AIX) || defined(__sun)
typedef wchar_t wchar;
#else
typedef unsigned short wchar;
#endif

#define SHORT16(x) (sizeof(unsigned short)==2 ? (unsigned short)(x):((x)&0xffff))
#define UINT32(x)  (sizeof(uint32)==4 ? (uint32)(x):((x)&0xffffffff))

#endif
