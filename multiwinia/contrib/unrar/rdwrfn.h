#ifndef _RAR_DATAIO_
#define _RAR_DATAIO_

#include "crypt.h"
#include "file.h"
#include "int64.h"

class CmdAdd;
class Unpack;


class ComprDataIO
{
  private:
    void ShowUnpRead(Int64 ArcPos,Int64 ArcSize);
    void ShowUnpWrite();


    bool UnpackFromMemory;
    unsigned int UnpackFromMemorySize;
    unsigned char *UnpackFromMemoryAddr;

    bool UnpackToMemory;
    unsigned int UnpackToMemorySize;
    unsigned char *UnpackToMemoryAddr;

    unsigned int UnpWrSize;
    unsigned char *UnpWrAddr;

    Int64 UnpPackedSize;

    bool ShowProgress;
    bool TestMode;
    bool SkipUnpCRC;

    File *SrcFile;
    File *DestFile;

    CmdAdd *Command;

    FileHeader *SubHead;
    Int64 *SubHeadPos;

#ifndef NOCRYPT
    CryptData Crypt;
    CryptData Decrypt;
#endif


    int LastPercent;

    char CurrentCommand;

  public:
    ComprDataIO();
    void Init();
    int UnpRead(unsigned char *Addr,unsigned int Count);
    void UnpWrite(unsigned char *Addr,unsigned int Count);
    void EnableShowProgress(bool Show) {ShowProgress=Show;}
    void GetUnpackedData(unsigned char **Data,unsigned int *Size);
    void SetPackedSizeToRead(Int64 Size) {UnpPackedSize=Size;}
    void SetTestMode(bool Mode) {TestMode=Mode;}
    void SetSkipUnpCRC(bool Skip) {SkipUnpCRC=Skip;}
    void SetFiles(File *SrcFile,File *DestFile);
    void SetCommand(CmdAdd *Cmd) {Command=Cmd;}
    void SetSubHeader(FileHeader *hd,Int64 *Pos) {SubHead=hd;SubHeadPos=Pos;}
    void SetEncryption(int Method,char *Password,unsigned char *Salt,bool Encrypt);
    void SetAV15Encryption();
    void SetCmt13Encryption();
    void SetUnpackToMemory(unsigned char *Addr,unsigned int Size);
    void SetCurrentCommand(char Cmd) {CurrentCommand=Cmd;}

    bool PackVolume;
    bool UnpVolume;
    bool NextVolumeMissing;
    Int64 TotalPackRead;
    Int64 UnpArcSize;
    Int64 CurPackRead,CurPackWrite,CurUnpRead,CurUnpWrite;
    Int64 ProcessedArcSize,TotalArcSize;

    unsigned int PackFileCRC,UnpFileCRC,PackedCRC;

    int Encryption;
    int Decryption;
};

#endif
