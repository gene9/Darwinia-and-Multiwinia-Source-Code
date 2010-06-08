#ifndef _RAR_CRC_
#define _RAR_CRC_

extern unsigned int CRCTab[256];

void InitCRC();
unsigned int CRC(unsigned int StartCRC,void *Addr,unsigned int Size);
unsigned short OldCRC(unsigned short StartCRC,void *Addr,unsigned int Size);

#endif
