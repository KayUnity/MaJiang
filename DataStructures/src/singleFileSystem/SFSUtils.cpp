#include "singleFileSystem/SFSUtils.h"
#include "LinuxStrings.h"
#include "streams/DS_Stream.h"
#include "rand.h"
#include "HexTimeCounter.h"
#include "singleFileSystem/PageFileManager.h"
#include "singleFileSystem/DS_HugeFile.h"
#include "fileUtils/_fileExists.h"
#include "streams/DS_FileStream.h"
#include "encription/AES.h"

using namespace DataStructures;

__u32 DataStructures::CountCRC(char *buffer, int size, unsigned char mode)
{
    __u32 result = 0;
    int i, x;
    switch (mode)
    {
        case CRC_FAST:
            i = 0;
            x = size >> 2;
            while (i < x)
            {
                result = result ^ *(__u32 *)(buffer + (i << 2));
                i ++;
            }
            break;
        case CRC_FULL:
            //roc todo
            //result = CRC16(0, buffer, size);
            break;
    }
    return result;
}

bool DataStructures::CheckCRC(char *buffer, int size, unsigned char mode, __u32 crc)
{
    return CountCRC(buffer, size, mode) == crc;
}

void DataStructures::CreatePasswordHeader(PasswordHeader *passwordHeader, const char *password)
{
    memset(passwordHeader, 0, sizeof(PasswordHeader));
	if (!password || !strlen(password))
		return;
    if (strlen(password) > MAX_PASSWORD_LENGTH)
        //Password length should be shorter than MAX_PASSWORD_LENGTH
        assert(false);
    // generate random key
    unsigned char key[MAX_PASSWORD_LENGTH];
    for (int i=0; i<MAX_PASSWORD_LENGTH; i++)
        key[i] = (1 + randomMT()) % 0xfe;
    memset(passwordHeader, 0, sizeof(PasswordHeader));
    // calculate key crc
    passwordHeader->keyCRC = CountCRC((char *)key, MAX_PASSWORD_LENGTH, CRC_FULL);
    strcpy(passwordHeader->pass, password);
    passwordHeader->passCRC = CountCRC(passwordHeader->pass, MAX_PASSWORD_LENGTH, CRC_FULL);
    // encode key by password
	AESContext encContext;
	AESContext decContext;
	init_AES((unsigned char *)passwordHeader->pass, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
	blockEncrypt(&encContext, key, MAX_PASSWORD_LENGTH, (unsigned char *)passwordHeader->key);
	//using PASSWORD_MASK encode password
	init_AES((unsigned char *)PASSWORD_MASK, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
	blockEncrypt(&encContext, (unsigned char *)passwordHeader->pass, MAX_PASSWORD_LENGTH, (unsigned char *)passwordHeader->pass);
}

bool DataStructures::CheckPassword(PasswordHeader *passwordHeader, const char *password, char *key)
{
	key[0] = 0;
    if (!password || !strlen(password))
        return false;
	char pass[MAX_PASSWORD_LENGTH];
	memset(pass, 0, sizeof(pass));
	strcpy(pass, password);
    if (passwordHeader->passCRC != CountCRC(pass, MAX_PASSWORD_LENGTH, CRC_FULL))
		return false;
	AESContext encContext;
	AESContext decContext;
	init_AES((unsigned char *)PASSWORD_MASK, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
	blockDecrypt(&decContext, (unsigned char *)passwordHeader->pass, MAX_PASSWORD_LENGTH, (unsigned char *)pass);
	if (strcmp(pass, password) != 0)
		return false;
	init_AES((unsigned char *)pass, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
	blockDecrypt(&decContext, (unsigned char *)passwordHeader->key, MAX_PASSWORD_LENGTH, (unsigned char *)key);
    if (passwordHeader->keyCRC != CountCRC(key, MAX_PASSWORD_LENGTH, CRC_FULL))
	{
		key[0] = 0;
		return false;
	}
	return true;
}

bool DataStructures::IsStrMatchPattern(const char *str, const char *pattern, bool ignoreCase)
{
    char *strPtr = (char *)str;
    char *patternPtr = (char *)pattern;
    if (strcmp(patternPtr, WILD_CARD_ANYFILE) == 0)
        return true;
    while (true)
    {
        if (strcmp(patternPtr, WILD_CARD_MULTIPLE_CHAR) == 0)
            return true;
        else
            if ((*strPtr == 0) && (*patternPtr != 0))
                return false;
            else
                if (*strPtr == 0)
                    return true;
                else
                {
                    int delta;
                    switch (*patternPtr)
                    {
                        case '*':
                            for (int i=0; i<strlen(strPtr); i++)
                            {
                                if (IsStrMatchPattern(strPtr + i, patternPtr + 1, ignoreCase))
                                    return true;
                            }
                            return false;
                        case '?':
                            strPtr ++;
                            patternPtr ++;
                            break;
                        default:
                            delta = abs(*strPtr - *patternPtr);
                            if ((delta == 0) || ((delta == abs('A' - 'a')) && ignoreCase))
                            {
                                strPtr ++;
                                patternPtr ++;
                            }
                            else
                                return false;
                    }
                }
    }
    return false;
}

// extracts path and pattern
bool DataStructures::ExtractPathAndPattern(const char *inPath, char *outPath, char *pattern)
{
    if (!inPath || !strlen(inPath))
        return false;
    bool result = false;
    int len = strlen(inPath);
    int k = -1;
    for (int i=len-2; i>=0; i--)
        if ((*(inPath + i) == '/') || (*(inPath + i) == '\\'))
        {
            k = i;
            break;
        }
    if (k < 0)
        return result;
    // check pattern
    if (k == 0)
        *outPath = 0;
    else
    {
        memcpy(outPath, inPath, k);
        outPath[k] = 0;
    }
    if ((*(inPath + len - 1) == '/') || (*(inPath + len - 1) == '\\'))
        len --;
    memcpy(pattern, inPath + k + 1, len - k - 1);
	pattern[len - k - 1] = 0;
    return true;
}

void DataStructures::SetCurrentTime(struct tm &time)
{
    time = *GetCurrentTimeInTM();
}

void DataStructures::SetFileName(const char *fileName, DirectoryElement *el)
{
    int l = strlen(fileName);
    if (l > 250)
        l = 250;
    strcpy(el->fileName, fileName);
}

void DataStructures::InitDirectoryElement(DirectoryElement *el)
{
    memset(el, 0, sizeof(DirectoryElement));
}

const char *DataStructures::FileModeToString(__u16 mode)
{
    static char modeStr[32];
	switch (mode)
	{
	case FM_OPEN_READ_WRITE:
		strcpy(modeStr, "rb+");
		break;
	case FM_OPEN_READ:
		strcpy(modeStr, "rb");
		break;
	case FM_CREATE:
	case FM_OPEN_WRITE:
	case FM_CREATE | FM_OPEN_WRITE:
		strcpy(modeStr, "wb");
		break;
	case FM_CREATE | FM_OPEN_READ_WRITE:
		strcpy(modeStr, "wb+");
		break;
	default:
		strcpy(modeStr, "rb+");
		//invalid file open mode
		assert(false);
	}
    return modeStr;
}

char *DataStructures::ExcludeTrailingBackslash(char *path)
{
    if (!path || !strlen(path))
        return path;
    for (int i=strlen(path)-1; i>=0; i--)
        if ((path[i] == '/') || (path[i] == '\\'))
            path[i] = 0;
        else
            break;
    return path;
}

char *DataStructures::IncludeTrailingBackslash(char *path)
{
    if (!path)
        return path;
    ExcludeTrailingBackslash(path);
    strcat(path, "/");
    return path;
}

HexString DataStructures::GetTemporaryDirectory()
{
#if defined _WIN32 || defined _WIN64
    char tempPath[250];
    GetTempPathA(sizeof(tempPath), tempPath);
    return HexString(tempPath);
#else
    return HexString(P_tmpdir);
#endif
}

bool DataStructures::RenameSFS(const char *oldName, const char *newName)
{
    HugeFile *hf;
    hf = new HugeFile(oldName, false, true, false);
    hf->Close();
    bool result = hf->RenameFile(newName);
    delete hf;
    
    return result;
}

bool DataStructures::CopySFS(const char *oldName, const char *newName)
{
    HugeFile *hf;
    hf = new HugeFile(oldName, false, true, false);
    hf->Close();
    bool result = hf->CopyFile(newName);
    delete hf;
    
    return result;
}

bool DataStructures::DeleteSFS(const char *fileName)
{
    HugeFile *hf;
    hf = new HugeFile(fileName, false, true, false);
    hf->Close();
    bool result = hf->DeleteFile();
    delete hf;
    
    return result;
}

bool DataStructures::IsSFSFile(const char *fileName)
{
    HugeFile *hf;
    hf = new HugeFile(fileName, true, false, false);
    bool result = hf->Open(false, false);
    hf->Close();
    delete hf;
    
    return result;
}

void DataStructures::MakeSFX(const char *sfsFileName, const char *sfxStubFileName, const char *sfxFileName)
{
    if (!sfxStubFileName || !strlen(sfxStubFileName))
        //MakeSFX - SFXStub file name is blank'
        assert(false);
    if (!_fileExists(sfxStubFileName))
        //MakeSFX - Cannot open file
        assert(false);
    if (!_fileExists(sfsFileName))
        //MakeSFX - Cannot open file
        assert(false);
    
    HugeFile *hSFS = 0;
    FileStream *stubStream = 0;
    FileStream *destStream = 0;
    try
    {
        hSFS = new HugeFile(sfsFileName, true, false, false);
        if (!hSFS->Open(false, false))
            //MakeSFX - Cannot open file
            assert(false);
        stubStream = new FileStream(sfxStubFileName, "rb");
        destStream = new FileStream(sfxFileName, "wb+");
        destStream->CopyFrom(stubStream, stubStream->Length());
        hSFS->SaveToStream(destStream);
    }
    catch (...)
    {
    }
    if (hSFS != 0)
    {
        hSFS->Close();
        delete hSFS;
    }
    delete stubStream;
    delete destStream;
}

bool DataStructures::IsPasswordValid(const char *fileName, const char *password)
{
    bool result = true;
    char key[MAX_PASSWORD_LENGTH];
    PageFileManager *pfmHandle = new PageFileManager(fileName, "rb", password);
    if (pfmHandle->mHeader.encMethod != ENC_NONE)
        if (!CheckPassword(&pfmHandle->mHeader.passwordHeader, password, key))
            result = false;
    delete pfmHandle;
    
    return result;
}

bool DataStructures::IsSingleFileEncrypted(const char *fileName)
{
    bool result = false;
    PageFileManager *pfmHandle = new PageFileManager(fileName, "rb");
    if (pfmHandle->mHeader.encMethod != ENC_NONE)
        result = true;
    delete pfmHandle;
    
    return result;
}

bool DataStructures::IsStrMatchPattern(char *strPtr, char *patternPtr, bool ignoreCase)
{
    if (strcmp(patternPtr, WILD_CARD_ANYFILE) == 0)
        return true;
    unsigned char delta;
    while (true)
    {
        if (strcmp(patternPtr, WILD_CARD_MULTIPLE_CHAR) == 0)
            return true;
        else
            if ((*strPtr == 0) && (*patternPtr != 0))
                return false;
            else
                if (*strPtr == 0)
                    return true;
                else
                {
                    switch (*patternPtr)
                    {
                        case '*':
                            for (size_t i=0; i<strlen(patternPtr); i++)
                            {
                                if (IsStrMatchPattern(strPtr + i, patternPtr + 1, ignoreCase))
                                    return true;
                            }
                            return false;
                        case '?':
                            strPtr ++;
                            patternPtr ++;
                            break;
                        default:
                            delta = abs(*strPtr - *patternPtr);
                            if ((delta == 0) || (ignoreCase && (delta == abs('A' - 'a'))))
                            {
                                strPtr ++;
                                patternPtr ++;
                            }
                            else
                                return false;
                    }
                    
                }
    }
	return true;
}

void DataStructures::SetBit(char *bitMap, int bitNo, bool bSet)
{
    int segment = bitNo / 8;
    int offset = bitNo % 8;
    if (bSet)
        *(bitMap + segment) |= (1 << offset);
    else
        *(bitMap + segment) &= (~(1 << offset));
}

bool DataStructures::GetBit(char *bitMap, int bitNo)
{
    int segment = bitNo / 8;
    int offset = bitNo % 8;
    return (*(bitMap + segment) & (1 << offset)) != 0;
}

