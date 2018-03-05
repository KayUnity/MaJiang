#ifndef __SFS_FILE_STREAMS_H
#define __SFS_FILE_STREAMS_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "singleFileSystem/SFSArray.h"
#include "HexString.h"
#include "streams/DS_Stream.h"

namespace DataStructures
{
    
    enum SFSCompressionLevel
    {
        SFS_NONE,
        SFS_QUICK_LZ
    };
    
    enum SFSOverwriteMode
    {
        OM_ALWAYS,
        OM_NEVER,
        OM_PROMPT
    };
    
    class SingleFileSystem;
    
    class SFSUserFileStream : public Stream
    {
    private:
        SingleFileSystem *mSFSHandle;
        //roc todo
        //SFSNoCancelProgressEvent mOnProgress;
        void DoOnProgress(float progress);
    protected:
        int mHandle;
    public:
        SFSUserFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password = "");
        virtual ~SFSUserFileStream();

        virtual bool CanRead();
        virtual bool CanWrite();
        virtual bool CanSeek();
        virtual void Close();
        
        virtual size_t Read(void *ptr, size_t size);
        virtual size_t Write(const void *ptr, size_t size);
        virtual bool SetLength(__s64 size);
        virtual __s64 Seek(__s64 offset, int origin);

        virtual bool SaveToStream(Stream *stream);
        virtual bool LoadFromStream(Stream *stream);
        
        int GetHandle() { return mHandle; };
        SingleFileSystem *GetSFSHandle() { return mSFSHandle; }
        bool IsEncrypted();
    };
    
    class SFSAdvancedFileStream : public Stream
    {
    private:
        int mBlockSize;
        float mProgress;
        float mProgressMax;
        SFSHeadersArray *mHeaders;
        __s64 mTrueSize;
        __s64 mPackedSize;
        int mCurrentHeader;
        __s64 mCurrentPos;
        SFSCompressionLevel mCompressionLevel;
        //SFSNoCancelProgressEvent mOnProgress;
        float mCompressionRate;
        bool mNoProgress;
    protected:
        SFSUserFileStream *mFile;
        SFSFileStreamHeader mHeader;
    private:
        void CalculateRate();
        void InternalCreate(bool create);
    private:
        void LoadHeaders();
        void SaveHeader();
        void PrepareBufferForWriting(char *inBuf, int inSize, char **outBuf, SFSHeader *hdr);
        void LoadBlock(int curHeader, char **outBuf);
    public:
        SFSAdvancedFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password = "", SFSCompressionLevel compressionLevel = SFS_NONE);
        virtual ~SFSAdvancedFileStream();
        
        virtual bool CanRead() { return mFile != 0; }
        virtual bool CanWrite() { return mFile != 0; }
        virtual bool CanSeek() { return mFile != 0; }
        virtual void Close() { mFile->Close(); }
        
        virtual size_t Read(void *ptr, size_t size);
        virtual size_t Write(const void *ptr, size_t size);
        virtual bool SetLength(__s64 size);
        virtual __s64 Seek(__s64 offset, int origin);
        
        virtual bool SaveToStream(Stream *stream);
        virtual bool LoadFromStream(Stream *stream);
        
        bool IsEncrypted();
        SingleFileSystem *GetSFSHandle();
        int GetPackedSize();
        SFSCompressionLevel GetCompressionLevel() { return mCompressionLevel; }
        float GetCompressionRate();
        void DoOnProgress(float progress);
        int GetHandle();
    };

    class SFSFileStream : public Stream
    {
    private:
        SFSAdvancedFileStream *mStream;
        //SFSNoCancelProgressEvent OnProgress;
        __u16 mMode;
        HexString mPassword;
        bool mReadOnly;
        int GetHandle();
        SingleFileSystem *GetSFSHandle();
        SFSCompressionLevel GetCompressionLevel();
        void SetCompressionLevel(SFSCompressionLevel newCompressLevel);
    public:
        void ChangeEncryption(const char *newPassword = "");
    private:
    public:
        SFSFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password = "", SFSCompressionLevel compressLevel = SFS_NONE);
        virtual ~SFSFileStream();
        
        virtual bool CanRead();
        virtual bool CanWrite();
        virtual bool CanSeek();
        virtual void Close();
        
        virtual size_t Read(void *ptr, size_t size);
        virtual size_t Write(const void *ptr, size_t size);
        virtual bool SetLength(__s64 size);
        virtual __s64 Seek(__s64 offset, int origin);
        
        virtual bool SaveToStream(Stream *stream);
        virtual bool LoadFromStream(Stream *stream);
        virtual bool SaveToFile(const char *fileName);
        virtual bool LoadFromFile(const char *fileName);

        int GetPackedSize();
        float GetCompressionRate();
        bool IsEncrypted();
    };

}

#endif