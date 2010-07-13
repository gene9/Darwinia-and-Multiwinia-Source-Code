#include "rarbloat.h"

BitInput::BitInput()
{
  InBuf=new unsigned char[MAX_SIZE];
}


BitInput::~BitInput()
{
  delete[] InBuf;
}


void BitInput::faddbits(int Bits)
{
  addbits(Bits);
}


unsigned int BitInput::fgetbits()
{
  return(getbits());
}
