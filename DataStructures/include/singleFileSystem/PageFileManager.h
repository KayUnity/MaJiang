#ifndef __PAGE_FILE_MANAGER_H
#define __PAGE_FILE_MANAGER_H

#include "SingleFileSystemStructures.h"
#include "HexString.h"

namespace DataStructures
{
    class HugeFile;
    class Stream;
    
    class PageFileManager
    {
    public:
        HugeFile *mSFSFile;   // Single file
	private:
        // calc offset by page no
        __s64 PageNoToOffset(int pageNo);
        // encode buffer
        bool EncodeBuffer(char *buffer, long size, unsigned char encType, const char *password);
    public:
        // decode buffer
        bool DecodeBuffer(char *buffer, long size, unsigned char encType, const char *password);

        // attempt to load Single file header from specified offset
        bool InternalLoadHeader(__s64 fileOffset);

        // returns page data size without header
        long int GetPageDataSize();

        // set in-memory or disk mode
        void SetInMemory(bool value);
    public:
        HexString mFileName;  // Single file name with full path and extension
        bool mInMemory; // in memory file or disk file?
        SingleFileHeader mHeader;  // Single file header
        bool mReadOnly;
        bool mExclusive;
        char mKey[MAX_PASSWORD_LENGTH];
        int mLastError; // error code for last operation
        HexString mPassword;

        // constructor
        PageFileManager(const char *fileName, const char *mode, const char *password = "", bool isInMemory = false,
                        int pageSize = DEFAULT_PAGE_SIZE, int extentPageCount = DEFAULT_EXTENT_PAGE_COUNT, __s64 partFileSize = -1);
        // destructor
        virtual ~PageFileManager();

        // load Single file header
        void LoadSFHeader();

        // save Single file  header
        void SaveSFHeader();

        // allocate and init page
        void AllocPageBuffer(FFPage *ffPage);

        // free page
        void FreePageBuffer(FFPage *ffPage);

        // read page
        bool ReadPage(FFPage *buffer, int pageNo, int size = -1, const char *password = "", bool ignoreEncrypted = false);

        // write page
        bool WritePage(FFPage *buffer, int pageNo, int size = -1, const char *password = "");

        // append pages to the end of file
        bool AppendPages(int qty);

        // deletes pages from end of file
        void DeletePagesFromEOF(int qty);

        // rename file
        bool RenameFile(const char *newName);

        // delete file
        bool DeleteFile();

        // flush file buffers
        void FlushFileBuffers();

        // load data from stream
        void LoadFromStream(Stream *stream);

        // free space on disk
        __s64 DiskFree();
    };
    
}
#endif
