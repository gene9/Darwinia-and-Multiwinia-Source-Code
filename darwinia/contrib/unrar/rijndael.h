#ifndef _RIJNDAEL_H_
#define _RIJNDAEL_H_

/**************************************************************************
 * This code is based on Szymon Stefanek AES implementation:              *
 * http://www.esat.kuleuven.ac.be/~rijmen/rijndael/rijndael-cpplib.tar.gz *
 *                                                                        *
 * Dynamic tables generation is based on the Brian Gladman's work:        *
 * http://fp.gladman.plus.com/cryptography_technology/rijndael            *
 **************************************************************************/

#define _MAX_KEY_COLUMNS (256/32)
#define _MAX_ROUNDS      14
#define MAX_IV_SIZE      16

class Rijndael
{	
  public:
    enum Direction { Encrypt , Decrypt };
  private:
    void keySched(unsigned char key[_MAX_KEY_COLUMNS][4]);
    void keyEncToDec();
    void encrypt(const unsigned char a[16], unsigned char b[16]);
    void decrypt(const unsigned char a[16], unsigned char b[16]);
    void GenerateTables();

    Direction m_direction;
    unsigned char     m_initVector[MAX_IV_SIZE];
    unsigned char     m_expandedKey[_MAX_ROUNDS+1][4][4];
  public:
    Rijndael();
    void init(Direction dir,const unsigned char *key,unsigned char *initVector);
    int blockEncrypt(const unsigned char *input, int inputLen, unsigned char *outBuffer);
    int blockDecrypt(const unsigned char *input, int inputLen, unsigned char *outBuffer);
};
	
#endif // _RIJNDAEL_H_
