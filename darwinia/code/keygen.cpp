#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keygen.h"


static char *ReadFileToBuf(char const *_filename, int *_len)
{
	FILE *in = fopen(_filename, "r");
	if (!in) return NULL;
	
	char temp[64];
	int fileLen = 0;
	while (!feof(in))
	{
		fileLen += fread(temp, sizeof(char), 64, in);
	}

	char *data = new char[fileLen+1];
	fseek(in, 0, SEEK_SET);
	fread(data, sizeof(char), fileLen, in);

	*_len = fileLen;
	return data;
}


static int GetNextNewline(char const *_text, int i, int len)
{
	while (_text[i] != '\n') 
	{
		i++;
		if (i >= len) return -1;
	}

	i++;
	if (i >= len) return -1;
	return i;
}


// Returns NULL if the specified data is valid system info. 
// Otherwise, returns a string containing a description of the error
static char const *ValidateSysInfo(char const *_sysInfoData, int _sysInfoLen)
{
	if (_sysInfoLen < 184) return "system info file too short";
	if (_sysInfoLen > 500) return "system info file too long";
	
	char const *c = _sysInfoData;
	int i = 0;

	if (strncmp(&c[0], "SysInfoGenVersion", 17) != 0)				return "sys info gen version missing"; 
	if (atoi(&c[18]) != 2)											return "wrong sys info gen version";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "CpuVendor:", 10) != 0)			return "cpu vendor info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "CpuModel:", 9) != 0)				return "cpu mdoel info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Num Processors:", 15) != 0)		return "num processors missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Country:", 8) != 0)				return "country info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Language:", 9) != 0)				return "language info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Num sound devices:", 18) != 0)	return "country info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Primary sound device:", 21) != 0)return "primary sound device info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Graphics Card:", 14) != 0)		return "graphics card info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Operating System:", 17) != 0)	return "OS info missing";

	i = GetNextNewline(c, i, _sysInfoLen);
	if (i == -1 || strncmp(&c[i], "Computer Name:", 14) != 0)		return "computer name missing";

	return NULL;
}


static char const *ValidateUserInfo(char const *_userInfoData, int _userInfoLen)
{
	if (_userInfoLen < 28) return "user info file too short";
	if (_userInfoLen > 200) return "user info file too long";
	
	char const *c = _userInfoData;
	int i = 0;
	if (strncmp(&c[0], "Username:", 9) != 0)				return "Username missing";
	
	i = GetNextNewline(c, i, _userInfoLen);
	if (i == -1 || strncmp(&c[i], "Email:", 6) != 0)		return "Email missing";

	return NULL;
}



static unsigned int GenerateKey(char const *_data, int _len)
{
	// The algorithm takes the text data and casts it to unsigned ints. Then
	// creates generates the initial key using a magic number. Then modifies
	// the key by XORing it with a randomly chosen element from the array
	// of unsigned ints. This last step is repeated a magic number of times.
	// This second magic number is chosen to be large enough to give a high
	// probablity that every byte in the initial data has been used in 
	// generating the key.
	unsigned int *data = (unsigned int *)_data;
	_len = _len / 4;

	unsigned int rv = 28477717;
	for (int i = 0; i < 1710; ++i)
	{
		int j = (rv + i) % _len;	// pseudo randomly chose an array index
#ifdef _BIG_ENDIAN
		int offset = j * 4 + 3;		
		unsigned int partner = 0;
		partner |= (unsigned char) _data[offset--];
		partner <<= 8;
		partner |= (unsigned char) _data[offset--];
		partner <<= 8;
		partner |= (unsigned char) _data[offset--];
		partner <<= 8;
		partner |= (unsigned char) _data[offset];
		rv ^= partner;
#else
		rv ^= data[j];
#endif
	}

	return rv;
}


static void MakePasswordFromKey(unsigned int _a, unsigned int _b, char const **_password)
{
	char *password = new char [8 + 8 + 1];
	sprintf(password, "%08x", _a);
	sprintf(password + 8, "%08x", _b);
	*_password = password;
}


// Returns 0 on success, -1 on failure
// If a failure occurs the password will contain a description of the error
int Keygen(char const *_sysInfoFilename, char const *_userInfoFilename, char const **_password)
{
	int fileLen;
	char const *sysInfoData = ReadFileToBuf(_sysInfoFilename, &fileLen);
	if (!sysInfoData)
	{
		*_password = "Couldn't open sys info file";
		return -1;
	}
	
	return Keygen(sysInfoData, fileLen, _userInfoFilename, _password);
}


// Returns 0 on success, -1 on failure
// If a failure occurs the password will contain a description of the error
int Keygen(char const *_sysInfoData, int _sysInfoLen, char const *_userInfoFilename, char const **_password)
{
	int userInfoLen;
	char *userInfoData = ReadFileToBuf(_userInfoFilename, &userInfoLen);
	if (!userInfoData)
	{
		*_password = "Couldn't open user info file";
		return -1;
	}

	char const *validationResult = ValidateSysInfo(_sysInfoData, _sysInfoLen);
	if (validationResult)
	{
		*_password = validationResult;
		return -1;
	}
	validationResult = ValidateUserInfo(userInfoData, userInfoLen);
	if (validationResult)
	{
		*_password = validationResult;
		return -1;
	}

	unsigned int sysKey = GenerateKey(_sysInfoData, _sysInfoLen);
	unsigned int userKey = GenerateKey(userInfoData, userInfoLen);

	MakePasswordFromKey(userKey, sysKey, _password);

	delete [] userInfoData;

	return 0;
}
