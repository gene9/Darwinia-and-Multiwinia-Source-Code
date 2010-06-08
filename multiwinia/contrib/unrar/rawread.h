#ifndef _RAR_RAWREAD_
#define _RAR_RAWREAD_

class RawRead
{
  private:
    Array<unsigned char> Data;
    File *SrcFile;
    int DataSize;
    int ReadPos;
#ifndef SHELL_EXT
    CryptData *Crypt;
#endif
  public:
    RawRead(File *SrcFile);
    void Read(int Size);
    void Read(unsigned char *SrcData,int Size);
    void Get(unsigned char &Field);
    void Get(unsigned short &Field);
    void Get(unsigned int &Field);
    void Get8(Int64 &Field);
    void Get(unsigned char *Field,int Size);
    void Get(wchar *Field,int Size);
    unsigned int GetCRC(bool ProcessedOnly);
    int Size() {return DataSize;}
    int PaddedSize() {return Data.Size()-DataSize;}
#ifndef SHELL_EXT
    void SetCrypt(CryptData *Crypt) {RawRead::Crypt=Crypt;}
#endif
};

#endif
