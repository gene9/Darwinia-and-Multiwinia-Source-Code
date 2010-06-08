#ifndef INCLUDED_KEYGEN_H
#define INCLUDED_KEYGEN_H

int Keygen(char const *_sysInfoFilename, char const *_userInfoFilename, char const **_password);
int Keygen(char const *_sysInfoData, int _sysInfoLen, char const *_userInfoFilename, char const **_password);

#endif
