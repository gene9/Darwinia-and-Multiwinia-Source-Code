#ifndef _RAR_ENCNAME_
#define _RAR_ENCNAME_

class EncodeFileName
{
  private:
    void AddFlags(int Value);

    unsigned char *EncName;
    unsigned char Flags;
    int FlagBits;
    int FlagsPos;
    int DestSize;
  public:
    EncodeFileName();
    int Encode(char *Name,wchar *NameW,unsigned char *EncName);
    void Decode(char *Name,unsigned char *EncName,int EncSize,wchar *NameW,int MaxDecSize);
};

#endif
