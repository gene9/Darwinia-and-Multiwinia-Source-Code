#include "coder.h"
#include "unpack.h"


inline unsigned int RangeCoder::GetChar()
{
  return(UnpackRead->GetChar());
}


void RangeCoder::InitDecoder(Unpack *UnpackRead)
{
  RangeCoder::UnpackRead=UnpackRead;

  low=code=0;
  range=(unsigned int)(-1);
  for (int i=0;i < 4;i++)
    code=(code << 8) | GetChar();
}


#define ARI_DEC_NORMALIZE(code,low,range,read)                           \
{                                                                        \
  while ((low^(low+range))<RAR_TOP || range<RAR_BOT && ((range=-low&(RAR_BOT-1)),1)) \
  {                                                                      \
    code=(code << 8) | read->GetChar();                                  \
    range <<= 8;                                                         \
    low <<= 8;                                                           \
  }                                                                      \
}


inline int RangeCoder::GetCurrentCount() 
{
  return (code-low)/(range /= SubRange.scale);
}


inline unsigned int RangeCoder::GetCurrentShiftCount(unsigned int SHIFT) 
{
  return (code-low)/(range >>= SHIFT);
}


inline void RangeCoder::Decode()
{
  low += range*SubRange.LowCount;
  range *= SubRange.HighCount-SubRange.LowCount;
}
