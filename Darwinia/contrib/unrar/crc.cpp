#include "rarbloat.h"

unsigned int CRCTab[256];

void InitCRC()
{
  for (int I=0;I<256;I++)
  {
    unsigned int C=I;
    for (int J=0;J<8;J++)
      C=(C & 1) ? (C>>1)^0xEDB88320L : (C>>1);
    CRCTab[I]=C;
  }
}


unsigned int CRC(unsigned int StartCRC,void *Addr,unsigned int Size)
{
  if (CRCTab[1]==0)
    InitCRC();
  unsigned char *Data=(unsigned char *)Addr;
#if defined(LITTLE_ENDIAN) && defined(PRESENT_INT32)
  while (Size>0 && ((int)Data & 7))
  {
    StartCRC=CRCTab[(unsigned char)(StartCRC^Data[0])]^(StartCRC>>8);
    Size--;
    Data++;
  }
  while (Size>=8)
  {
    StartCRC^=*(uint32 *)Data;
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC^=*(uint32 *)(Data+4);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    StartCRC=CRCTab[(unsigned char)StartCRC]^(StartCRC>>8);
    Data+=8;
    Size-=8;
  }
#endif
  for (int I=0;I<Size;I++)
    StartCRC=CRCTab[(unsigned char)(StartCRC^Data[I])]^(StartCRC>>8);
  return(StartCRC);
}

#ifndef SFX_MODULE
unsigned short OldCRC(unsigned short StartCRC,void *Addr,unsigned int Size)
{
  unsigned char *Data=(unsigned char *)Addr;
  for (int I=0;I<Size;I++)
  {
    StartCRC=(StartCRC+Data[I])&0xffff;
    StartCRC=((StartCRC<<1)|(StartCRC>>15))&0xffff;
  }
  return(StartCRC);
}
#endif
