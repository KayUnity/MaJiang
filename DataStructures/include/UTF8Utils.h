#ifndef __UTF8_UTILS_H
#define __UTF8_UTILS_H

#include <string>
void Gb2312ToUnicode(wchar_t *pOut, const char *gbBuffer);
std::string GB2312ToUTF_8(const char *pText, int pLen);
std::string UTF_8ToGB2312(const char *pText, int pLen);
std::wstring GB2312StrToUnicode(const char *pText, int pLen);
#endif
