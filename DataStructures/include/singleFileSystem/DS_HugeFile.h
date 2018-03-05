#ifndef __HUGE_FILE_H
#define __HUGE_FILE_H

#include "streams/DS_Stream.h"
#include "CommonTypeDefines.h"
#include "HexString.h"

#define SFS_SIGNATURE           "SFSF"
#define SFS_SIGNATURE_SIZE      4
#define HF_SIGNATURE            "HFSS"
#define HF_SIGNATURE_SIZE       4
#define FILE_BLOCK_SIZE         524288

namespace DataStructures
{

    // Function return Max File Size for drive ( extracted from FileNeme)
    __s64 GetMaxFileSize(HexString fileName);

    // Partition Header - points to the name of the next partition file
#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct HugeFilePartitionHeader
    {
        char signature[HF_SIGNATURE_SIZE];      // signature
        __s64 partitionSize;                    // max size of each partition (get used value only from first part in chain)
        char nextFileName[256];                 // name of the next file in chain
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#define HFP_HEADER_SIZE sizeof(HugeFilePartitionHeader)

//------------------------------------------------------------------------------
// Huge File (consists of several usual files)
//------------------------------------------------------------------------------
    class HugeFile
    {
    private:
        HexString mFileName;  // full file name with path
        bool mExclusive; // id file open in exclusive mode
        bool mReadOnly; // is file read-only
        bool mInMemory; // is file stored in memory
        __s64 mPosition;   // current position
        __s64 mSize;   // file size
        __s64 mPartitionSize;   // max size of each separate part (usual file)
        __s64 mPartitionDataSize; // user data size in each partition
        List<Stream *> mPartFiles; // list of partition files
        bool mIsFileOpen; // is file open or closed?
        __s64 mStubSize;
        
        // get partition No and offset there by global file position
        void GetFileNoAndOffset(__s64 pos, int &fileNo, __s64 &offset);
        // init part header
        void InitPartHeader(HugeFilePartitionHeader &hfPartHeader);
        // append part file
        bool AppendPartFile();
        // delete last part file
        void DeleteLastPartFile();
        // open files chain
        bool OpenFilesChain(const HexString fileName);
        // internal open
        bool InternalOpen(const HexString fileName, bool create, bool ignoreErrors = false);
        // detects stub and+ gets its size
        void CalculateStubSize(Stream *file);
    public:
        // constructor
        HugeFile(HexString fileName, bool readOnly, bool exculusive, bool inMemory, __s64 partionSize = -1);
        // destructor
        virtual ~HugeFile();

        // copy form source file
        __s64 CopyFrom(HugeFile *source, __s64 count);
        // save to stream
        void SaveToStream(Stream *stream);
        // load from stream
        void LoadFromStream(Stream *stream);
        // close file
        void Close();
        // flush file buffers
        void FlushBuffers();
        // open file (with params specified in constructor)
        bool Open(bool create, bool ignoreErrors = false);
        // read from file
        size_t Read(void *buffer, size_t count);
        // read known bytes from file
        void ReadBuffer(void *buffer, size_t count);
        // seek in file
        __s64 Seek(__s64 offset, int origin);
        // write into the file
        size_t Write(const void *buffer, size_t count);
        // write known bytes into the file
        void WriteBuffer(const void *buffer, size_t count);
        // copy file
        bool CopyFile(const HexString newName);
        // rename file
        bool RenameFile(const HexString newName);
        // delete file
        bool DeleteFile();

        // set in-memory mode
        void SetInMemory(bool value);
        // set exclusive mode
        void SetExclusive(bool value);
        // set file position
        void SetPosition(__s64 pos);
        // get file position
        __s64 GetPosition();
        // set read-only mode
        void SetReadOnly(bool value);
        // set file size
        void SetSize(__s64 value);
        // get file size
        __s64 GetSize();
    public:
        bool IsExclusive() { return mExclusive; }
        HexString GetFileName() { return mFileName; }
        void SetFileName(const HexString fileName) { mFileName = fileName; }
        bool IsReadOnly() { return mReadOnly; }
        bool IsInMemory() { return mInMemory; }
    };
    
}

#endif
