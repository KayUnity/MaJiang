#include "UTF8Utils.h"
#include <wchar.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include <string.h>
#include <stdlib.h>
#endif

void Gb2312ToUnicode(wchar_t *pOut, const char *gbBuffer)
{
#if defined _WIN32 || defined _WIN64
    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED, gbBuffer, 2, pOut, 1);
#else
    mbtowc(pOut, gbBuffer, 2);
#endif
}

void UnicodeToUTF_8(char *pOut, wchar_t *pText)
{
    char *pchar = (char *)pText;
    pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
    pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
    pOut[2] = (0x80 | (pchar[0] & 0x3F));
}

std::string GB2312ToUTF_8(const char *pText, int pLen)
{
    std::string out = "";
    char buf[4];
    memset(buf,0,4);
    int i = 0;
    while(i < pLen)
    {
        //copy directly if it is a english char
        if (pText[i] >= 0)
        {
            char asciistr[2] = {0};
            asciistr[0] = (pText[i++]);
            out.append(asciistr);
        }
        else
        {
            wchar_t pbuffer;
            Gb2312ToUnicode(&pbuffer, pText + i);
            UnicodeToUTF_8(buf, &pbuffer);
            out.append(buf);
            i += 2;  
        }  
    }  
    return out;
}

std::wstring GB2312StrToUnicode(const char *pText, int pLen)
{
	std::wstring out = L"";
	int i = 0;
	while (i < pLen)
	{
		//copy directly if it is a english char
		if (pText[i] >= 0)
		{
			char asciistr[2] = { 0 };
			asciistr[0] = (pText[i++]);
			out.append((wchar_t*)asciistr);
		}
		else
		{
			wchar_t pbuffer;
			Gb2312ToUnicode(&pbuffer, pText + i);
			out.append(&pbuffer);
			i += 2;
		}
	}
	return out;
}

void UTF_8ToUnicode(wchar_t *pOut, const char *pText)
{
    char *uchar = (char *)pOut;
    uchar[1] = ((pText[0] & 0x0F) << 4) + ((pText[1] >> 2) & 0x0F);
    uchar[0] = ((pText[1] & 0x03) << 6) + (pText[2] & 0x3F);
}

void UnicodeToGB2312(char *pOut, wchar_t uData)
{
#if defined _WIN32 || defined _WIN64
    WideCharToMultiByte(CP_ACP, NULL, &uData, 1, pOut, sizeof(wchar_t), NULL, NULL);
#else
    wctomb(pOut, uData);
#endif
}

std::string UTF_8ToGB2312(const char *pText, int pLen)
{
    std::string out = "";
    char buf[4];
    char *rst = new char[pLen + (pLen >> 2) + 2];
    memset(buf, 0, 4);
    memset(rst, 0, pLen + (pLen >> 2) + 2);
    int i = 0;
    int j = 0;
    while (i < pLen)
    {
        if (*(pText + i) >= 0)
        {
            rst[j++] = pText[i++];
        }
        else
        {
            wchar_t Wtemp;
            UTF_8ToUnicode(&Wtemp, pText + i);
            UnicodeToGB2312(buf, Wtemp);
            rst[j] = buf[0];
            rst[j+1] = buf[1];
            rst[j+2] = buf[2];
            i += 3;  
            j += 2;  
        }  
    }  
    rst[j] = '\0';
    out = rst;
    delete [] rst;
    return out;
}  
