#ifndef STRCVT_H
#define STRCVT_H

#define _wcstombsz xwcstombsz
#define _mbstowcsz xmbstowcsz

int xwcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int xmbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

#endif //STRCVT_H
