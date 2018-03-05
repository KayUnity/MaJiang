#include "singleFileSystem/PageFileManager.h"
#include "singleFileSystem/DS_HugeFile.h"
#include "FormatString.h"
#include "fileUtils/_splitpath.h"
#include "fileUtils/_fileExists.h"
#include "singleFileSystem/SFSUtils.h"
#include "encription/AES.h"

#if defined WIN32 || defined WIN64
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

using namespace DataStructures;

// constructor
PageFileManager::PageFileManager(const char *fileName, const char *mode, const char *password, bool isInMemory,
    int pageSize, int extentPageCount, __s64 partFileSize)
{
	memset(mKey, 0, MAX_PASSWORD_LENGTH);
    if (pageSize < MIN_PAGE_SIZE)
        //page size should be >= MIN_PAGE_SIZE
        assert(false);
    if (extentPageCount <= 0)
        //extent page count should be >= 1 Page.
        assert(false);
    mFileName = fileName;
    mInMemory = isInMemory;

    char drive[32];
    char folder[250];
    char filename[250];
    char extension[32];
    _splitpath(fileName, drive, folder, filename, extension);
    HexString databaseName = drive;
    databaseName += folder;
    mReadOnly = false;
    mPassword = password;
    HexString myMode = mode;
    if ((myMode.FindIString("r") >=0) && (myMode.FindIString("+") < 0))
        mReadOnly = true;
    mExclusive = false;
    //roc todo
    //if ((mode & fmShareExclusive) == fmShareExclusive)
        mExclusive = true;
    if (myMode.FindIString("w") >= 0)
    {
        // create Single file
        mSFSFile = new HugeFile(fileName, mReadOnly, mExclusive, mInMemory, partFileSize);
        mSFSFile->Open(true);
        memset(&mHeader, 0, sizeof(SingleFileHeader));
        memcpy(mHeader.signature, SFS_SIGNATURE, SFS_SIGNATURE_SIZE);
        mHeader.version = SFS_CURRENT_VERSION;
        mHeader.pageSize = pageSize; // since v.2.10 - to allow 600 bytes pages
        mHeader.extentPageCount = extentPageCount;
   
        mHeader.hdrPageCount = 1;
        mHeader.totalPageCount = 1;
        mHeader.encMethod = ENC_NONE;
        if (password && strlen(password) > 0)
        {
            mHeader.encMethod = ENC_RIJNDAEL;
            CreatePasswordHeader(&mHeader.passwordHeader, password);
        }
        SaveSFHeader();
    }
    else
    {
        if (!_fileExists(fileName))
            //file not found
            assert(false);
        mSFSFile = new HugeFile(fileName, mReadOnly, mExclusive, mInMemory);
        // open file and ignore corruption errors
        if (!mSFSFile->Open(false, true))
        {
            delete mSFSFile;
            mSFSFile = 0;
            //file has corrupted header
            assert(false);
        }
        LoadSFHeader();
    }
    // decode default key for user files
    if (mHeader.encMethod != ENC_NONE)
    {
		CheckPassword(&mHeader.passwordHeader, mPassword, mKey);
    }
}

PageFileManager::~PageFileManager()
{
    if (mSFSFile)
        delete mSFSFile;
    mSFSFile = 0;
}

__s64 PageFileManager::PageNoToOffset(int pageNo)
{
    return pageNo * mHeader.pageSize;
}

bool PageFileManager::EncodeBuffer(char *buffer, long size, unsigned char encType, const char *password)
{
    bool unknownEncType = false;
    switch (encType)
    {
        case 0: ;// no encoding
            break;
        case 1:
			{
				AESContext encContext;
				AESContext decContext;
				init_AES((unsigned char *)password, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
				blockEncrypt(&encContext, (unsigned char *)buffer, (int)size, (unsigned char *)buffer);
			}
            break;
        default:
            unknownEncType = true;
    }
    // unknown type?
    if (unknownEncType)
        //Unknown encryption type');
        assert(false);
    return true;
}

bool PageFileManager::DecodeBuffer(char *buffer, long size, unsigned char encType, const char *password)
{
    bool unknownEncType = false;
    switch (encType)
    {
        case 0: ;// no encoding
            break;
        case 1:
 			{
				AESContext encContext;
				AESContext decContext;
				init_AES((unsigned char *)password, MAX_PASSWORD_LENGTH >> 2, &encContext, &decContext);
				blockDecrypt(&decContext, (unsigned char *)buffer, (int)size, (unsigned char *)buffer);
			}
            break;
        default:
            unknownEncType = true;
    }
    // unknown type?
    if (unknownEncType)
        return false;
    return true;
}

bool PageFileManager::InternalLoadHeader(__s64 fileOffset)
{
    bool ok = true;
    mSFSFile->Seek(fileOffset, SEEK_SET);
    mSFSFile->ReadBuffer(&mHeader, SINGLE_FILE_HEADER_SIZE);
    // check signature
    if (memcmp(mHeader.signature, SFS_SIGNATURE, SFS_SIGNATURE_SIZE) != 0)
        ok = false;
    else
    {
        // check crc
        int size = SINGLE_FILE_HEADER_SIZE - sizeof(mHeader.signature) - sizeof(__u32);
        __u32 crc = CountCRC((char *)&mHeader.version, size, 0);
        if (mHeader.crc !=  crc)
            ok = false;
    }
    return ok;
}

long int PageFileManager::GetPageDataSize()
{
    return mHeader.pageSize - SFS_PAGE_HEADER_SIZE;
}

void PageFileManager::SetInMemory(bool value)
{
    mSFSFile->SetInMemory(value);
}

void PageFileManager::LoadSFHeader()
{
    // try to open header (it is always placed at the beginning of Single file)
    if (!InternalLoadHeader(0))
        //LoadHeader - Invalid SFS file, header is corrupted
        assert(false);
}

void PageFileManager::SaveSFHeader()
{
    if (mReadOnly)
        return;
    // count header crc
    int size = SINGLE_FILE_HEADER_SIZE - sizeof(mHeader.signature) - sizeof(__u32);
    mHeader.crc = CountCRC((char *)&mHeader.version, size, CRC_FAST);
    // seek to beginnig of header
    mSFSFile->Seek(0, SEEK_SET);
    // write primary Single file header
    mSFSFile->WriteBuffer(&mHeader, SINGLE_FILE_HEADER_SIZE);
}

void PageFileManager::AllocPageBuffer(FFPage *ffPage)
{
    ffPage->pageHeader.nextPageNo = -1; // no next page
    ffPage->pageHeader.encType = 0; // no encryption
    ffPage->pageHeader.crcType = 0; // fast CRC
    ffPage->pData = malloc(GetPageDataSize());
}

void PageFileManager::FreePageBuffer(FFPage *ffPage)
{
    free(ffPage->pData);
}

bool PageFileManager::ReadPage(FFPage *buffer, int pageNo, int size, const char *password, bool ignoreEncrypted)
{
    mLastError = ER_Ok;
    char key[MAX_PASSWORD_LENGTH];
    if (!password || !strlen(password))
        memcpy(key, mKey, MAX_PASSWORD_LENGTH);
    else
        memcpy(key, password, MAX_PASSWORD_LENGTH);
    if (size < 0)
        size = (int)GetPageDataSize();
    // calc offset
    __s64 offset = PageNoToOffset(pageNo);
    // seek
    mSFSFile->Seek(offset, SEEK_SET);
    // read page header
    bool result = mSFSFile->Read(&buffer->pageHeader, SFS_PAGE_HEADER_SIZE) == SFS_PAGE_HEADER_SIZE;
    if (!result)
    {
        mLastError = ER_ReadPageHeaderError;
        return result;
    }
    // read header only
    if (size == 0)
        return result;
    // if OK
    if (buffer->pData == 0)
        //ReadPage - pData is ULL
        assert(false);
    result = mSFSFile->Read(buffer->pData, size) == size;
    if (!result)
    {
        mLastError = ER_ReadPageError;
        return result;
    }
    // check CRC
    result = CheckCRC((char *)buffer->pData, size, buffer->pageHeader.crcType, buffer->pageHeader.crc);
    if (!result)
    {
        mLastError = ER_CRCError;
        assert(result);
    }
    // decrypt if necessary
    if (!ignoreEncrypted)
        result = DecodeBuffer((char *)buffer->pData, size, buffer->pageHeader.encType, key);
    if (!result)
        mLastError = ER_DecodeError;
    
    return result;
}

bool PageFileManager::WritePage(FFPage *buffer, int pageNo, int size, const char *password)
{
    mLastError = ER_Ok;
    char key[MAX_PASSWORD_LENGTH];
    if (!password || !strlen(password))
        memcpy(key, mKey, MAX_PASSWORD_LENGTH);
    else
        memcpy(key, password, MAX_PASSWORD_LENGTH);

	if (size < 0)
        size = (int)GetPageDataSize();
    
    bool result = true;
    // page data presents?
    if ((buffer->pData != 0) && (size > 0))
    {
        // encrypt
        result = EncodeBuffer((char *)buffer->pData, size, buffer->pageHeader.encType, key);
        if (!result)
        {
            mLastError = ER_EncodeError;
            return false;
        }
         // calc CRC
        buffer->pageHeader.crc = CountCRC((char *)buffer->pData, size, buffer->pageHeader.crcType);
    }
    // calc offset
    __s64 offset = PageNoToOffset(pageNo);

    // seek
    mSFSFile->Seek(offset, SEEK_SET);
    // write page header
    result = mSFSFile->Write(&buffer->pageHeader, SFS_PAGE_HEADER_SIZE) == SFS_PAGE_HEADER_SIZE;

    if (!result)
    {
        mLastError = ER_WritePageHeaderError;
        return result;
    }

    if ((buffer->pData != 0) && (size > 0))
        result = mSFSFile->Write(buffer->pData, size) == size;

    if (!result)
        mLastError = ER_WritePageError;
    
    return result;
}

bool PageFileManager::AppendPages(int qty)
{
    bool result = false;
    mSFSFile->SetSize((mHeader.totalPageCount + qty) * mHeader.pageSize);
    if (mSFSFile->GetSize() == (mHeader.totalPageCount + qty) * mHeader.pageSize)
        result = true;
    if (result)
    {
        mHeader.totalPageCount += qty;
        SaveSFHeader();
    }
    return result;
}

void PageFileManager::DeletePagesFromEOF(int qty)
{
    // file size
    __s64 fileSize = mSFSFile->GetSize();
    // get total pages count in file
    int pageCount = (int)(fileSize / mHeader.pageSize);
    if ((fileSize % mHeader.pageSize) > 0)
        pageCount ++;
    // calc new file size
    fileSize = (pageCount - qty) * mHeader.pageSize;
    if (pageCount < qty)
        //DeletePagesFromEOF - attempt to delete more pages than exists.
        assert(false);
    // resize file
    mSFSFile->SetSize(fileSize);
}

bool PageFileManager::RenameFile(const char *newName)
{
    return mSFSFile->RenameFile(newName);
}

bool PageFileManager::DeleteFile()
{
    return mSFSFile->DeleteFile();
}

void PageFileManager::FlushFileBuffers()
{
    mSFSFile->FlushBuffers();
}

void PageFileManager::LoadFromStream(Stream *stream)
{
    if (!mSFSFile)
        //LoadFromStream - SFSFile = NULL
        assert(false);
    mSFSFile->LoadFromStream(stream);
    LoadSFHeader();
    // decode default key for user files
    if (mHeader.encMethod != ENC_NONE)
		CheckPassword(&mHeader.passwordHeader, mPassword, mKey);
}

__s64 PageFileManager::DiskFree()
{
    //return heap size of diskspace
#if defined WIN32 || defined WIN64
	DWORD sectorsPerCluster;
	DWORD bytesPerSector;
	DWORD numberOfFreeClusters;
	DWORD totalNumberOfClusters;
	bool ret = GetDiskFreeSpace(L"/", &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);
    if (ret)
        return numberOfFreeClusters * sectorsPerCluster * bytesPerSector;
    else
        return 0;
#else
    struct statvfs st;
    int ret = statvfs("/", &st);
    if (ret == 0)
        return st.f_bsize * st.f_bfree;
    else
        return 0;
#endif
}
