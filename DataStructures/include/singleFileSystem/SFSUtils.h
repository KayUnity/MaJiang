#ifndef __SFS_UTILS_H
#define __SFS_UTILS_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "HexString.h"

namespace DataStructures
{
    // calculate CRC (mode=0 - fast, 1 - full)
    __u32 CountCRC(char *buffer, int size, unsigned char mode);

    // check crc - true if calculted CRC = specified
    bool CheckCRC(char *buffer, int size, unsigned char mode, __u32 crc);
    
    // create password header; if question not specified recovery of lost password
    // will not be available
    void CreatePasswordHeader(PasswordHeader *passwordHeader, const char *password);
    
    // returns true if password is valid; Key - decoded key value
    bool CheckPassword(PasswordHeader *passwordHeader, const char *password, char *key);

	// checks if string matches pattern
	bool IsStrMatchPattern(const char *str, const char *pattern, bool ignoreCase = true);

	// extracts path and pattern
	bool ExtractPathAndPattern(const char *inPath, char *outPath, char *pattern);

	// writes current time to filetime field in TDirectoryElement
	void SetCurrentTime(struct tm &time);

	// writes filename and alternateFileName to TDirectoryElement
	void SetFileName(const char *fileName, DirectoryElement *el);

	//initialization of directory element
	void InitDirectoryElement(DirectoryElement *el);
    
    const char *FileModeToString(__u16 mode);
    
    char *ExcludeTrailingBackslash(char *path);
    char *IncludeTrailingBackslash(char *path);

    HexString GetTemporaryDirectory();
    bool RenameSFS(const char *oldName, const char *newName);
    bool CopySFS(const char *oldName, const char *newName);
    bool DeleteSFS(const char *fileName);
    bool IsSFSFile(const char *fileName);
    void MakeSFX(const char *sfsFileName, const char *sfxStubFileName, const char *sfxFileName);
    bool IsPasswordValid(const char *fileName, const char *password);
    bool IsSingleFileEncrypted(const char *fileName);
    
    bool ExtractPathAndPattern(const char *inPath, char *outPath, char *pattern);
    bool IsStrMatchPattern(char *strPtr, char *patternPtr, bool ignoreCase = true);
    // bit = 1 if bSet = true, otherwise bit = 0
    void SetBit(char *bitMap, int bitNo, bool bSet);
    // get bit from bitmap, returns true if bit = 1, otherwise returns false
    bool GetBit(char *bitMap, int bitNo);
    
    
}
#endif