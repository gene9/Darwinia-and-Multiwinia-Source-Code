#ifndef _RAR_CRYPT_
#define _RAR_CRYPT_

#include "headers.h"
#include "rardefs.h"
#include "rijndael.h"

enum { OLD_DECODE=0,OLD_ENCODE=1,NEW_CRYPT=2 };

struct CryptKeyCacheItem
{
#ifndef _SFX_RTL_
  CryptKeyCacheItem()
  {
    *Password=0;
  }

  ~CryptKeyCacheItem()
  {
    memset(AESKey,0,sizeof(AESKey));
    memset(AESInit,0,sizeof(AESInit));
    memset(Password,0,sizeof(Password));
  }
#endif
  unsigned char AESKey[16],AESInit[16];
  char Password[MAXPASSWORD];
  bool SaltPresent;
  unsigned char Salt[SALT_SIZE];
};

class CryptData
{
  private:
    void Encode13(unsigned char *Data,unsigned int Count);
    void Decode13(unsigned char *Data,unsigned int Count);
    void Crypt15(unsigned char *Data,unsigned int Count);
    void UpdKeys(unsigned char *Buf);
    void Swap(unsigned char *Ch1,unsigned char *Ch2);
    void SetOldKeys(char *Password);

    Rijndael rin;
    
    unsigned char SubstTable[256];
    unsigned int Key[4];
    unsigned short OldKey[4];
    unsigned char PN1,PN2,PN3;

    unsigned char AESKey[16],AESInit[16];

    static CryptKeyCacheItem Cache[4];
    static int CachePos;
  public:
    CryptData();
    void SetCryptKeys(char *Password,unsigned char *Salt,bool Encrypt,bool OldOnly=false);
    void SetAV15Encryption();
    void SetCmt13Encryption();
    void EncryptBlock20(unsigned char *Buf);
    void DecryptBlock20(unsigned char *Buf);
    void EncryptBlock(unsigned char *Buf,int Size);
    void DecryptBlock(unsigned char *Buf,int Size);
    void Crypt(unsigned char *Data,unsigned int Count,int Method);
    static void SetSalt(unsigned char *Salt,int SaltSize);
};

#endif
