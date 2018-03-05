#include "singleFileSystem/SFSFileStreams.h"
#include "singleFileSystem/SingleFileSystem.h"
#include "singleFileSystem/SFSArray.h"
#include "singleFileSystem/SFSUtils.h"
#include "compression/DS_QuickLZCore.h"
#include "encription/AES.h"

using namespace DataStructures;

//------------------------------------------------------- SFSUserFileStream ------------------------------------------------------
bool SFSUserFileStream::IsEncrypted()
{
    return mSFSHandle->IsFileEncrypted(mFileName);
}

void SFSUserFileStream::DoOnProgress(float progress)
{
    //roc todo
    /*
    if (mOnProgress)
        mOnProgress(this, progress);
    */
}

bool SFSUserFileStream::SetLength(__s64 size)
{
    return mSFSHandle->FileSetSize(mHandle, size) == size;
}

SFSUserFileStream::SFSUserFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password)
{
    strcpy(mFileName, fileName);
    if (singleFileSystem == 0)
        //Single file system == NULL
        assert(false);
    mSFSHandle = singleFileSystem;
    mHandle = mSFSHandle->FileOpen(fileName, mode, password);
    if (mHandle <= NONE)
        //unable to open file
        assert(false);
}

SFSUserFileStream::~SFSUserFileStream()
{
    mSFSHandle->FileClose(mHandle);
}

bool SFSUserFileStream::CanRead()
{
    return true;
}

bool SFSUserFileStream::CanWrite()
{
    return true;
}

bool SFSUserFileStream::CanSeek()
{
    return true;
}

void SFSUserFileStream::Close()
{
    mSFSHandle->FileClose(mHandle);
}

size_t SFSUserFileStream::Read(void *ptr, size_t size)
{
    return mSFSHandle->FileRead(mHandle, (char *)ptr, size);
}

size_t SFSUserFileStream::Write(const void *ptr, size_t size)
{
    return mSFSHandle->FileWrite(mHandle, (char *)ptr, size);
}

__s64 SFSUserFileStream::Seek(__s64 offset, int origin)
{
    return mSFSHandle->FileSeek(mHandle, offset, origin);
}

bool SFSUserFileStream::SaveToStream(Stream *stream)
{
    __s64 oldPos = Position();
    __s64 oldPos1 = stream->Position();
    Seek(0, SEEK_SET);
    __s64 outBytes = 0;
    DoOnProgress(0);
    __s64 inSize = Length();
    char *buf = (char *)malloc(DEFAULT_MAX_BLOCK_SIZE);
    __s64 progressMax = inSize;
    while (outBytes < inSize)
    {
        __s64 outSize;
        if (inSize - outBytes > DEFAULT_MAX_BLOCK_SIZE)
            outSize = DEFAULT_MAX_BLOCK_SIZE;
        else
            outSize = Length() - outBytes;
        Read(buf, (size_t)outSize);
        stream->Write(buf, (size_t)outSize);
        outBytes += outSize;
        DoOnProgress(outBytes / progressMax * 100.0f);
    }
    free(buf);
    Seek(oldPos, SEEK_SET);
    stream->Seek(oldPos1, SEEK_SET);
    DoOnProgress(100.0);
    
    return outBytes == inSize;
}

bool SFSUserFileStream::LoadFromStream(Stream *stream)
{
	__s64 oldPos = Position();
	__s64 oldPos1 = stream->Position();
	float progressMax = stream->Length();
	stream->Seek(0, SEEK_SET);
	SetLength(0);
	Seek(0, SEEK_SET);
	DoOnProgress(0.0f);

	char *buf = (char *)malloc(DEFAULT_MAX_BLOCK_SIZE);
	while (stream->Position() < stream->Length())
	{
		__s64 outSize;
		if (stream->Length() - stream->Position() > DEFAULT_MAX_BLOCK_SIZE)
			outSize = DEFAULT_MAX_BLOCK_SIZE;
	    else
			outSize = stream->Length() - stream->Position();
		stream->Read(buf, outSize);
	    Write(buf, outSize);
	    float progress = stream->Position();
	    DoOnProgress(progress / progressMax * 100.0f);
	}
	free(buf);

	Seek(oldPos, SEEK_SET);
	stream->Seek(oldPos1, SEEK_SET);

	DoOnProgress(100.0f);
    return true;
}

//------------------------------------------------------- SFSAdvancedFileStream ------------------------------------------------------
bool SFSAdvancedFileStream::IsEncrypted()
{
    return mFile->IsEncrypted();
}

SingleFileSystem *SFSAdvancedFileStream::GetSFSHandle()
{
    return mFile->GetSFSHandle();
}

bool SFSAdvancedFileStream::SetLength(__s64 size)
{
    if (mCompressionLevel == SFS_NONE)
        return mFile->SetLength(size);
    if (size == mTrueSize)
        return true;
    else
    {
        if (size > mTrueSize)
        {
            Seek(0, SEEK_END);
            __s64 extSize = size - mTrueSize;
            char *buf = (char *)malloc((size_t)extSize);
            mNoProgress = true;
            if (Write(buf, (size_t)extSize) != extSize)
            {
                mNoProgress = false;
                free(buf);
                //SetSize - error extending file.
                assert(false);
            }
            mNoProgress = false;
            free(buf);
        }
        else
        {
            int curHdr = (int)(size / mHeader.blockSize);
            int numBlocks = curHdr;
            int extSize = size % mHeader.blockSize;
            char *outBuf = 0;
            if (extSize > 0)
            {
                numBlocks ++;
                char *buf;
                LoadBlock(curHdr, &buf);
                PrepareBufferForWriting(buf, extSize, &outBuf, &mHeaders->mItems[curHdr]);
                free(buf);
            }
            mHeader.numBlocks = numBlocks;
            mHeaders->SetSize(numBlocks);
            SaveHeader();
            if (extSize > 0)
            {
                mFile->SetLength(mHeaders->mPositions[curHdr]);
                mFile->Seek(0, SEEK_END);
                mHeaders->mItems[curHdr].nextHeaderNo = mFile->Position() + mHeaders->mItems[curHdr].packedSize + SFS_COMPRESSED_HEADER_SIZE;
                mHeaders->mPositions[curHdr] = mFile->Position();
                mFile->Write(&mHeaders->mItems[curHdr], SFS_COMPRESSED_HEADER_SIZE);
                mFile->Write(outBuf, mHeaders->mItems[curHdr].packedSize);
                free(outBuf);
            }
            mTrueSize = size;
            if (mCurrentPos > mTrueSize)
                Seek(0, SEEK_END);
            CalculateRate();
        }
    }
    return true;
}

int SFSAdvancedFileStream::GetPackedSize()
{
    return mFile->Length();
}

float SFSAdvancedFileStream::GetCompressionRate()
{
    CalculateRate();
    return mCompressionRate;
}

void SFSAdvancedFileStream::DoOnProgress(float progress)
{
    //roc todo
    /*
    if (mOnProgress && !mNoProgress)
        mOnProgress(this, progress);
    */
}

void SFSAdvancedFileStream::CalculateRate()
{
    mPackedSize = 0;
    for (int i=0; i<mHeaders->mItemCount; i++)
         mPackedSize += mHeaders->mItems[i].packedSize;
    int f1 = mTrueSize;
    int f = mPackedSize;
    if (mCompressionLevel == SFS_NONE)
    {
        mPackedSize = mTrueSize;
        mCompressionRate = 0.0f;
        mHeader.blockSize = DEFAULT_MAX_BLOCK_SIZE;
        return;
    }
    if (mTrueSize <= 0)
    {
        mPackedSize = mTrueSize;
        mCompressionRate = 0.0f;
        return;
    }
    mCompressionRate = (1.0f - (float)f / (float)f1) * 100.0f;
}

void SFSAdvancedFileStream::InternalCreate(bool create)
{
    mBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    mNoProgress = false;
    mTrueSize = mFile->Length();
    mFile->Seek(0, SEEK_SET);

    mHeaders = new SFSHeadersArray();
    if (create && (mCompressionLevel == SFS_NONE))
        return;
    if (create)
    {
        memcpy(mHeader.signature, SFS_STREAM_SIGNATURE, sizeof(mHeader.signature));
        mHeader.blockSize = BLOCK_SIZE_FOR_FASTEST;
        mHeader.numBlocks = 0;
        mHeader.version = SFS_COMPRESS_CURRENT_VERSION;
        mHeader.compressionLevel = mCompressionLevel;
        mHeader.crcMode = 0;
        mFile->Seek(0, SEEK_SET);
        if (mFile->Write(&mHeader, sizeof(mHeader)) != sizeof(mHeader))
            //error writing to data stream. May be data stream in read only mode.
            assert(false);
        mFile->SetLength(sizeof(mHeader));
    }
    mFile->Seek(0, SEEK_SET);
    mTrueSize = 0;
    LoadHeaders();
    mCurrentHeader = 0;
    mCurrentPos = 0;
    mNoProgress = false;
    mFile->Seek(0, SEEK_SET);
}

int SFSAdvancedFileStream::GetHandle()
{
    return mFile->GetHandle();
}

void SFSAdvancedFileStream::LoadHeaders()
{
    __s64 oldPos = mFile->Position();
    mPackedSize = 0;
    __s64 size = mFile->Length();
    mFile->Seek(0, SEEK_SET);
    if (size < sizeof(SFSFileStreamHeader))
    {
        mTrueSize = size;
        mFile->Seek(oldPos, SEEK_SET);
        mCompressionLevel = SFS_NONE;
        CalculateRate();
        return;
    }
    mFile->Read(&mHeader, sizeof(SFSFileStreamHeader));
    if (memcmp(mHeader.signature, SFS_STREAM_SIGNATURE, sizeof(mHeader.signature)) != 0)
    {
        mTrueSize = size;
        mFile->Seek(oldPos, SEEK_SET);
        mCompressionLevel = SFS_NONE;
        CalculateRate();
        return;
    }
    mFile->Seek(mHeader.customHeaderSize, SEEK_CUR);

    mHeaders->SetSize(0);
    mCompressionLevel = (SFSCompressionLevel)mHeader.compressionLevel;
    mTrueSize = 0;

    for (int i=0; i<mHeader.numBlocks; i++)
    {
        __s64 pos = mFile->Position();
		size = mFile->Length();
        if (size - mFile->Position() < SFS_COMPRESSED_HEADER_SIZE)
        {
            mFile->SetLength(mFile->Position());
            mHeader.numBlocks = i;
            mHeaders->SetSize(i);
            SaveHeader();
            break;
        }
        SFSHeader cHeader;
        mFile->Read(&cHeader, SFS_COMPRESSED_HEADER_SIZE);
        mTrueSize += cHeader.trueSize;
        mPackedSize += cHeader.packedSize;
        mHeaders->AppendItem(cHeader, pos);
        mFile->Seek(cHeader.nextHeaderNo, SEEK_SET);
    }
    mFile->Seek(oldPos, SEEK_SET);
    mBlockSize = mHeader.blockSize;
    CalculateRate();
}

void SFSAdvancedFileStream::SaveHeader()
{
    mFile->Seek(0, SEEK_SET);
    mFile->Write(&mHeader, sizeof(mHeader));
}

void SFSAdvancedFileStream::PrepareBufferForWriting(char *inBuf, int inSize, char **outBuf, SFSHeader *hdr)
{
    *outBuf = 0;
    hdr->trueSize = inSize;
    hdr->crc32 = CountCRC(inBuf, inSize, CRC_FULL);
    QuickLZCoreLevel1 compressor;
    hdr->packedSize = compressor.EncodeArray((unsigned char *)inBuf, inSize, (unsigned char **)outBuf);

    if (hdr->packedSize == 0)
    {
        if (*outBuf != 0)
            free(*outBuf);
        *outBuf = 0;
        //PrepareBufferForWriting - compression error in PrepareBuffer.
        assert(false);
    }
}

void SFSAdvancedFileStream::LoadBlock(int curHeader, char **outBuf)
{
    __u32 pSize = mHeaders->mItems[curHeader].packedSize;
    char *inBuf = (char *)malloc(pSize);
    if (inBuf == 0)
        assert(false);
    mFile->Seek(mHeaders->mPositions[curHeader] + SFS_COMPRESSED_HEADER_SIZE, SEEK_SET);
    if (mFile->Read(inBuf, pSize) != pSize)
    {
        free(inBuf);
        return;
    }

    __u32 size = mHeaders->mItems[curHeader].trueSize;
    QuickLZCoreLevel1 decompressor;
    __u32 dSize = decompressor.DecodeArray((unsigned char *)inBuf, (unsigned char **)outBuf);
    if (dSize == 0)
    {
        free(inBuf);
        if (*outBuf != 0)
            free(*outBuf);
        *outBuf = 0;
        //LoadBlock - decompression error
        assert(false);
    }
    if (size != dSize)
    {
        free(inBuf);
        free(*outBuf);
        //LoadBlock - decompression error, invalid size
        assert(false);
    }
    if (mHeaders->mItems[curHeader].crc32 != CountCRC(*outBuf, size, CRC_FULL))
    {
        free(inBuf);
        free(*outBuf);
        *outBuf = 0;
        //LoadBlock - decompression crc error.
        assert(false);
    }
    free(inBuf);
}

SFSAdvancedFileStream::SFSAdvancedFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password, SFSCompressionLevel compressionLevel)
{
	memset(&mHeader, 0, sizeof(SFSFileStreamHeader));
    strcpy(mFileName, fileName);
    if (singleFileSystem == 0)
        //SingleFileSystem == 0
        assert(false);
    mFile = new SFSUserFileStream(singleFileSystem, fileName, mode, password);
    if (!mFile)
        //error creating user file stream
        assert(false);
    mCompressionLevel = compressionLevel;
    if (mCompressionLevel == SFS_NONE)
        mCompressionLevel = (SFSCompressionLevel)DEFAULT_COMPRESSON_LEVEL;
    InternalCreate(mode == FM_CREATE);
}

SFSAdvancedFileStream::~SFSAdvancedFileStream()
{
    delete mFile;
    delete mHeaders;
}

size_t SFSAdvancedFileStream::Read(void *ptr, size_t size)
{
    DoOnProgress(0.0f);
    if (mCompressionLevel == SFS_NONE)
    {
        size_t result = mFile->Read(ptr, size);
        DoOnProgress(100.0f);
        return result;
    }
    size_t result = 0;
    mProgress = 0.0f;
    if (mCurrentPos > mTrueSize)
        return result;
    if (mCurrentPos < mTrueSize)
        mCurrentHeader = mCurrentPos / mHeader.blockSize;

    mProgressMax = mHeaders->mItemCount - mCurrentHeader;
    while ((mCurrentPos < mTrueSize) && (result < size))
    {
        DoOnProgress(mProgress);
        char *outBuf = 0;
        LoadBlock(mCurrentHeader, &outBuf);
        __s64 count = mHeader.blockSize - ((mCurrentPos + mHeader.blockSize) % mHeader.blockSize);
        if (result + count > size)
            count = size - result;
        if (mCurrentPos + count >= mTrueSize)
            count = mTrueSize - mCurrentPos;
        memcpy((char *)ptr + result, outBuf + (mCurrentPos + mHeader.blockSize) % mHeader.blockSize, count);
        free(outBuf);
        result += count;
        if (result < size)
            mCurrentHeader ++;
        mCurrentPos += count;
        mProgress = mProgress + 100.0f / mProgressMax;
    }
    DoOnProgress(100.0f);
 
    return result;
}

size_t SFSAdvancedFileStream::Write(const void *ptr, size_t size)
{
    DoOnProgress(0.0f);
    if (mCompressionLevel == SFS_NONE)
    {
        size_t result = mFile->Write(ptr, size);
        DoOnProgress(100.0f);
        return result;
    }
    size_t result = 0;
    if (size <= 0)
        return result;
    if (mCurrentPos > mTrueSize)
    {
        int d = mCurrentPos;
        Seek(0, SEEK_SET);
        SetLength(d);
        Seek(d, SEEK_SET);
        if (mCurrentPos != d)
            return 0;
    }

    DataStructures::List<void *> packedBuffers;
    __s64 startPos = mCurrentPos;
    __s64 endPos = mCurrentPos + size - 1;
    __s64 startHeaderNo = startPos / mHeader.blockSize;
    __s64 endHeaderNo = endPos / mHeader.blockSize;
    __s64 oldLastHeaderNo = mHeaders->mItemCount - 1;
    bool rewriteEnd = false;
    if (endHeaderNo > oldLastHeaderNo)
    {
        rewriteEnd = true;
        mHeaders->SetSize(endHeaderNo + 1);
    }

    if (mTrueSize <= 0)
        mHeaders->mPositions[0] = SFS_FILE_STREAM_HEADER_SIZE + mHeader.customHeaderSize;
    mProgress = 0;
    mProgressMax = mHeaders->mItemCount - mCurrentHeader;
    __s64 curPos = mCurrentPos;
    if (mCurrentPos + size > mTrueSize)
        mTrueSize = mCurrentPos + size;
    mCurrentPos = mCurrentPos + size;
    int i = startHeaderNo;
    while (i <= mHeaders->mItemCount - 1)
    {
        if (!rewriteEnd && (i > endHeaderNo))
            break;
        DoOnProgress(mProgress);

        char *buf = 0;
        if ((rewriteEnd && (i <= oldLastHeaderNo)) || (!rewriteEnd && (i <= endHeaderNo)))
        {
            LoadBlock(i, &buf);
            buf = (char *)realloc(buf, mHeader.blockSize);
        }
        else
        {
            buf = (char *)malloc(mHeader.blockSize);
        }
        if (i <= endHeaderNo)
        {
            __s64 sPos = (curPos + mHeader.blockSize) % mHeader.blockSize;
            __s64 tSize;
            if (i == endHeaderNo)
                tSize = endPos % mHeader.blockSize - sPos + 1;
            else
                tSize = mHeader.blockSize - sPos;
            memcpy(buf + sPos, (char *)ptr + curPos - startPos, tSize);
        }
        __s64 pSize = mHeaders->mItems[i].packedSize;
        if (i == mHeaders->mItemCount - 1)
        {
            if (mTrueSize % mHeader.blockSize > 0)
                mHeaders->mItems[i].trueSize = mTrueSize % mHeader.blockSize;
            else
                mHeaders->mItems[i].trueSize = mHeader.blockSize;
        }
        else
        {
            mHeaders->mItems[i].trueSize = mHeader.blockSize;
        }
        char *outBuf = 0;
        PrepareBufferForWriting(buf, mHeaders->mItems[i].trueSize, &outBuf, &mHeaders->mItems[i]);
        packedBuffers.Insert(outBuf);
        if (i == mHeaders->mItemCount - 1)
        {
            if (i == 0)
                mHeaders->mItems[i].nextHeaderNo = sizeof(mHeader) + mHeaders->mItems[i].packedSize + SFS_COMPRESSED_HEADER_SIZE;
            else
                mHeaders->mItems[i].nextHeaderNo = mHeaders->mItems[i - 1].nextHeaderNo + mHeaders->mItems[i].packedSize + SFS_COMPRESSED_HEADER_SIZE;
        }

        free(buf);
        if (pSize < mHeaders->mItems[i].packedSize)
        {
            rewriteEnd = true;
        }
        if (rewriteEnd && (i < mHeaders->mItemCount - 1))
        {
            if (i > 0)
                mHeaders->mItems[i].nextHeaderNo = mHeaders->mItems[i - 1].nextHeaderNo + mHeaders->mItems[i].packedSize + SFS_COMPRESSED_HEADER_SIZE;
            else
                mHeaders->mItems[i].nextHeaderNo = mHeaders->mPositions[0] + mHeaders->mItems[i].packedSize + SFS_COMPRESSED_HEADER_SIZE;
        }

        i ++;
        curPos = i * mHeader.blockSize;
        mProgress = mProgress + 100.0f / mProgressMax;
    }
    i --;
    if (rewriteEnd)
        for (int d=1; d<mHeaders->mItemCount; d++)
            mHeaders->mPositions[d] = mHeaders->mItems[d - 1].nextHeaderNo;
    int d = 0;
    for (int j=startHeaderNo; j<i+1; j++)
    {
        DoOnProgress(mProgress);
        mFile->Seek(mHeaders->mPositions[j], SEEK_SET);
        mFile->Write(&mHeaders->mItems[j], SFS_COMPRESSED_HEADER_SIZE);
        mFile->Write(packedBuffers[d], mHeaders->mItems[j].packedSize);
        d ++;
    }
    mHeader.numBlocks = mHeaders->mItemCount;
    SaveHeader();

    for (int i=0; i<packedBuffers.Size(); i++)
        free(packedBuffers[i]);
    packedBuffers.Clear();
    result = size;
    CalculateRate();
    DoOnProgress(100.0f);
    
    return result;
}

__s64 SFSAdvancedFileStream::Seek(__s64 offset, int origin)
{
    if (mCompressionLevel == SFS_NONE)
        return mFile->Seek(offset, origin);

    switch (origin)
    {
        case SEEK_SET:
            mCurrentPos = offset;
            break;
        case SEEK_END:
            mCurrentPos = mTrueSize + offset;
            break;
        case SEEK_CUR:
            mCurrentPos += offset;
            break;
		default:
			assert(false);
    }
    if (mCurrentPos <= 0)
    {
        mCurrentPos = 0;
        mCurrentHeader = 0;
    }
    else
    {
        if (mTrueSize == 0)
            mCurrentHeader = 0;
        else
            mCurrentHeader = mHeaders->FindPosition(mCurrentPos);
    }
    return mCurrentPos;
}

bool SFSAdvancedFileStream::SaveToStream(Stream *stream)
{
    __s64 oldPos = Position();
    __s64 oldPos1 = stream->Position();
    Seek(0, SEEK_SET);
    __s64 outBytes = 0;
    mNoProgress = false;
    DoOnProgress(0.0f);
    __s64 inSize = Length();
    float progressMax = inSize;
    char *buf = (char *)malloc(mBlockSize);
    while (outBytes < inSize)
    {
        int outSize;
        if (inSize - outBytes > mBlockSize)
            outSize = mBlockSize;
        else
            outSize = inSize - outBytes;
        mNoProgress = true;
        Read(buf, outSize);
        stream->Write(buf, outSize);
        mNoProgress = false;
        outBytes += outSize;
        float progress = outBytes;
        DoOnProgress(progress / progressMax * 100.0f);
    }
    free(buf);
    Seek(oldPos, SEEK_SET);
    stream->Seek(oldPos1, SEEK_SET);
    DoOnProgress(100.0f);
    return outBytes == inSize;
}

bool SFSAdvancedFileStream::LoadFromStream(Stream *stream)
{
    __s64 oldPos = Position();
    __s64 oldPos1 = stream->Position();
    stream->Seek(0, SEEK_SET);
    SetLength(0);
    __s64 totalSize = stream->Length();
    Seek(0, SEEK_SET);
    mNoProgress = false;
    DoOnProgress(0.0f);
    char *buf = (char *)malloc(mBlockSize);
    float progressMax = totalSize;
    while (stream->Position() < totalSize)
    {
        int outSize;
        if (totalSize - stream->Position() > mBlockSize)
            outSize = mBlockSize;
        else
            outSize = totalSize - stream->Position();
        mNoProgress = true;
        stream->Read(buf, outSize);
        Write(buf, outSize);
        mNoProgress = false;
        float progress = stream->Position();
        DoOnProgress(progress / progressMax * 100.0f);
    }
    free(buf);
    Seek(oldPos, SEEK_SET);
    stream->Seek(oldPos1, SEEK_SET);
    DoOnProgress(100.0f);
    
    return true;
}

//------------------------------------------------------- SFSFileStream ------------------------------------------------------
int SFSFileStream::GetHandle()
{
    return mStream->GetHandle();
}

SingleFileSystem *SFSFileStream::GetSFSHandle()
{
    return mStream->GetSFSHandle();
}

SFSCompressionLevel SFSFileStream::GetCompressionLevel()
{
    return mStream->GetCompressionLevel();
}

void SFSFileStream::SetCompressionLevel(SFSCompressionLevel newCompressLevel)
{
    if (newCompressLevel == GetCompressionLevel())
        return;
    if (mMode == FM_OPEN_READ)
        return;
    SingleFileSystem *sfs = mStream->GetSFSHandle();
    delete mStream;
    int attr = sfs->FileGetAttr(mFileName);
    PasswordHeader passHeader;
    if (!sfs->GetPasswordHeader(mFileName, &passHeader))
        //SetCompressionLevel - can not retreive password header. Probably file does not exists.
        assert(false);
    char backupFileName[250];
    strcpy(backupFileName, mFileName);
    strcat(backupFileName, ".bak");
    sfs->RenameFile(mFileName, backupFileName);
    SFSAdvancedFileStream *cs = new SFSAdvancedFileStream(sfs, backupFileName, FM_OPEN_READ, mPassword.C_String());
    mStream = new SFSAdvancedFileStream(sfs, mFileName, FM_CREATE, mPassword.C_String(), newCompressLevel);
    delete mStream;
    sfs->SetPasswordHeader(mFileName, &passHeader);
    sfs->FileSetAttr(mFileName, attr);
    mStream = new SFSAdvancedFileStream(sfs, mFileName, FM_OPEN_WRITE, mPassword.C_String());
    //mStream->OnProgress = mOnProgress;
    mStream->LoadFromStream(cs);
    delete cs;
    delete mStream;
    mStream = new SFSAdvancedFileStream(sfs, mFileName, mMode, mPassword.C_String());
    sfs->DeleteFile(backupFileName);
}


void SFSFileStream::ChangeEncryption(const char *newPassword)
{
    if (mMode == FM_OPEN_READ)
        return;
    SFSCompressionLevel compressLevel = mStream->GetCompressionLevel();
    SingleFileSystem *sfs = mStream->GetSFSHandle();
    delete mStream;
    int attr = sfs->FileGetAttr(mFileName);
    char backupFileName[250];
    strcpy(backupFileName, mFileName);
    strcat(backupFileName, ".bak");
    sfs->RenameFile(mFileName, backupFileName);
    SFSAdvancedFileStream *cs = new SFSAdvancedFileStream(sfs, backupFileName, FM_OPEN_READ, mPassword.C_String());
    mStream = new SFSAdvancedFileStream(sfs, mFileName, FM_CREATE, newPassword, compressLevel);
    delete mStream;
    mPassword = newPassword;
    sfs->FileSetAttr(mFileName, attr);
    mStream = new SFSAdvancedFileStream(sfs, mFileName, FM_OPEN_WRITE, newPassword);
    //cStream->OnProgress = mOnProgress;
    mStream->LoadFromStream(cs);
    delete cs;
    delete mStream;
    mStream = new SFSAdvancedFileStream(sfs, mFileName, mMode, mPassword.C_String());
    sfs->DeleteFile(backupFileName);
}

int SFSFileStream::GetPackedSize()
{
    return mStream->GetPackedSize();
}

float SFSFileStream::GetCompressionRate()
{
    return mStream->GetCompressionRate();
}

bool SFSFileStream::IsEncrypted()
{
    return mStream->IsEncrypted();
}

bool SFSFileStream::SetLength(__s64 size)
{
    if (mReadOnly)
        //SetSize - file is in read only mode.
        assert(false);
    return mStream->SetLength(size);
}

SFSFileStream::SFSFileStream(SingleFileSystem *singleFileSystem, const char *fileName, __u16 mode, const char *password, SFSCompressionLevel compressLevel)
{
    strcpy(mFileName, fileName);
    mPassword = password;
    mReadOnly = false;
    if ((mode & (FM_CREATE | FM_OPEN_WRITE)) == 0)
        mReadOnly = true;
    if (mode == FM_CREATE)
        mStream = new SFSAdvancedFileStream(singleFileSystem, fileName, mode, password, compressLevel);
    else
        mStream = new SFSAdvancedFileStream(singleFileSystem, fileName, mode, password);
    //mStream->OnProgress = mOnProgress;
    mMode = mode;
}

SFSFileStream::~SFSFileStream()
{
    if (mStream != 0)
    {
        delete mStream;
        mStream = 0;
    }
}

bool SFSFileStream::CanRead()
{
	return mStream->CanRead();
}

bool SFSFileStream::CanWrite()
{
	return mStream->CanWrite();
}

bool SFSFileStream::CanSeek()
{
	return mStream->CanSeek();
}

void SFSFileStream::Close()
{
	mStream->Close();
}
        
size_t SFSFileStream::Read(void *ptr, size_t size)
{
    //mStream->OnProgress = mOnProgress;
    return mStream->Read(ptr, size);
}

size_t SFSFileStream::Write(const void *ptr, size_t size)
{
    if (mReadOnly)
        //Write - file is in read only mode.
        assert(false);
    //mStream->OnProgress = mOnProgress;
    return mStream->Write(ptr, size);
}

__s64 SFSFileStream::Seek(__s64 offset, int origin)
{
    return mStream->Seek(offset, origin);
}

bool SFSFileStream::SaveToStream(Stream *stream)
{
    //mStream.OnProgress = mOnProgress;
    return mStream->SaveToStream(stream);
}

bool SFSFileStream::LoadFromStream(Stream *stream)
{
    if (mReadOnly)
        //LoadFromStream - file is in read only mode.
        assert(false);
    //mStream->OnProgress = mOnProgress;
    return mStream->LoadFromStream(stream);
}

bool SFSFileStream::SaveToFile(const char *fileName)
{
	return mStream->SaveToFile(fileName);
}

bool SFSFileStream::LoadFromFile(const char *fileName)
{
	return mStream->LoadFromFile(fileName);
}
