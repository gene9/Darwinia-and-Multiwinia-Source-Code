/****************************************************************************
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 ****************************************************************************/
#ifndef INCLUDED_CODER_H
#define INCLUDED_CODER_H


class Unpack;

const unsigned int RAR_TOP=1 << 24, RAR_BOT=1 << 15;

class RangeCoder
{
public:
    void InitDecoder(Unpack *UnpackRead);
    inline int GetCurrentCount();
    inline unsigned int GetCurrentShiftCount(unsigned int SHIFT);
    inline void Decode();
    inline void PutChar(unsigned int c);
    inline unsigned int GetChar();

    unsigned int low, code, range;
    struct SUBRANGE 
    {
      unsigned int LowCount, HighCount, scale;
    } SubRange;

    Unpack *UnpackRead;
};


#endif
