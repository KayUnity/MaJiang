#include "singleFileSystem/SingleFileSystem.h"
#include "singleFileSystem/FreeSpaceManager.h"
#include "singleFileSystem/DirManager.h"
#include "singleFileSystem/UserFilePageMapManager.h"
#include "singleFileSystem/PageFileManager.h"
#include "fileUtils/_fileExists.h"
#include "singleFileSystem/SFSUtils.h"
#include "singleFileSystem/DS_HugeFile.h"
#include "FormatString.h"
#include "DS_List.h"
#include "fileUtils/_splitpath.h"
#include "fileUtils/_FindFirst.h"
#include "streams/DS_FileStream.h"
#include "singleFileSystem/SFSFileStreams.h"

using namespace DataStructures;

void SingleFileSystem::DoOnProgress(float progress)
{
    //roc todo
    /*
    if (FOnProgress)
        FOnProgress(Self, Progress, FCancel);
    */
}

void SingleFileSystem::CloseAllFiles()
{
    if (mFileHandles != 0)
        for (int i=0; i<mFileHandles->mItemCount; i++)
        {
            if (mFileHandles->mValueItems[i] != 0)
                delete (UserFileHandle *)mFileHandles->mValueItems[i];
        }
}

void SingleFileSystem::InternalCreate()
{
    mFSMHandle = new FreeSpaceManager(mPFMHandle);
    mDMHandle = new DirManager(mPFMHandle, mFSMHandle);
    mUFMHandle = new UserFilePageMapManager(mPFMHandle, mFSMHandle);
    mFileHandles = new SortedPtrArray(0, 10, 1000);
    mReadOnly = mPFMHandle->mReadOnly;
    mExclusive = mPFMHandle->mExclusive;
    if (!IsEncrypted())
        mPassword = "";
}

void SingleFileSystem::DoOnDiskFull(void *sender)
{
    //roc todo
    /*
    if (FOnDiskFull)
        FOnDiskFull(self);
    */
}

void SingleFileSystem::DoOnPassword(const char *fileName, char *newPassword, bool &skipFile)
{
    //roc todo
    /*
    if Assigned(FOnPassword) then
        FOnPassword(Self, FileName, NewPassword, SkipFile);
    else
    {
        SkipFile = (!InputQuery(Format(SPasswordTitle, [FileName]), SPasswordPrompt, NewPassword));
    }
    */
}

void SingleFileSystem::DoOnOverwritePrompt(const char *existingFileName, const char *newFileName, bool &overwrite)
{
    //roc todo
    /*
    if (FOnOverwritePrompt)
        FOnOverwritePrompt(self, ExistingFileName, NewFileName, bOverwrite);
    */
}

void SingleFileSystem::DoOnFileProgress(void *sender, float percentDone)
{
    //roc todo
    /*
    if (FOnFileProgress)
        FOnFileProgress(Self, PercentDone, FCurrentFileName);
    */
}

void SingleFileSystem::InternalReopen(Stream *stream)
{
    delete mUFMHandle;
    delete mDMHandle;
    delete mFSMHandle;
    delete mPFMHandle;
    delete mFileHandles;
    if (!mInMemory)
        if (mOpenMode == FM_CREATE)
            mOpenMode = FM_OPEN_READ_WRITE;
    mPFMHandle = new PageFileManager(mFileName.C_String(), FileModeToString(mOpenMode), mPassword.C_String(), mInMemory);
    mPFMHandle->LoadFromStream(stream);
    InternalCreate();
    mDMHandle->Load();
}

SFSCompressionLevel SingleFileSystem::GetCompressionLevel()
{
    return (SFSCompressionLevel)mPFMHandle->mHeader.compressionLevel;
}

void SingleFileSystem::SetCompressionLevel(SFSCompressionLevel newLevel)
{
    mPFMHandle->mHeader.compressionLevel = (unsigned char)newLevel;
    mPFMHandle->SaveSFHeader();
}

void SingleFileSystem::SetInMemory(bool value)
{
    mPFMHandle->mInMemory = value;
}

SingleFileSystem::SingleFileSystem(const char *fileName, __u16 mode, const char *password, SFSCompressionLevel defaultCompressionLevel)
{
	mInMemory = false;

    if (mode != FM_CREATE)
        if (!DataStructures::IsPasswordValid(fileName, password))
            //invalid password specified, file name;
            assert(false);
    mOpenMode = mode;
    mFileName = fileName;
    mPFMHandle = new PageFileManager(fileName, FileModeToString(mode), password);
    if (mode == FM_CREATE)
        SetCompressionLevel(defaultCompressionLevel);
    mPassword = password;
    InternalCreate();
}

SingleFileSystem::SingleFileSystem(const char *fileName, __u16 mode, bool inMemory, const char *password, SFSCompressionLevel defaultCompressionLevel, int pageSize, int extentPageCount)
{
    mInMemory = inMemory;
    mFileName = fileName;
    mOpenMode = mode;
    //if (mInMemory && (mode == FM_CREATE))
    //    if (FileExists(fileName))
    //        mOpenMode = FM_OPEN_READ_WRITE;

    if (mOpenMode != FM_CREATE)
        if (!IsPasswordValid(fileName, password))
          //invalid password specified
          assert(false);

    mPFMHandle = new PageFileManager(fileName, FileModeToString(mOpenMode), password, inMemory, pageSize, extentPageCount);
    if (mOpenMode == FM_CREATE)
        SetCompressionLevel(defaultCompressionLevel);
    mPassword = password;
    InternalCreate();
}

SingleFileSystem::~SingleFileSystem()
{
    CloseAllFiles();
    if (mPFMHandle != 0)
        mPFMHandle->FlushFileBuffers();
    delete mUFMHandle;
    delete mDMHandle;
    delete mFSMHandle;
    delete mPFMHandle;
    delete mFileHandles;
}

int SingleFileSystem::InternalFileOpen(int fileId)
{
    mDMHandle->mOpenedFiles->mItems[fileId] ++;
    UserFileHandle *ufHandle = new UserFileHandle();
    ufHandle->fileId = fileId;
    ufHandle->position = 0;
    strcpy(ufHandle->key, "");
    ufHandle->mode = FM_OPEN_READ;
    int result = mFileHandles->GetNextKeyValue();
    mFileHandles->Insert(result, ufHandle);
    return result;
}

int SingleFileSystem::InternalFileRead(int handle, char *buffer, int count, DirectoryElement *el, bool changeEncryption, bool deleteCorruptedFiles)
{
    int result = 0;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    int size;
    if (ufHandle->position + count >= el->fileSize)
        size = el->fileSize - ufHandle->position;
    else
        size = count;
    if (size <= 0)
        return result;
    IntegerArray *pages = new IntegerArray(0, 1, 100);
    mUFMHandle->GetPages(el, ufHandle->position, size, false, pages);
    if (pages->mItemCount <= 0)
    {
        delete pages;
        return result;
    }
    FFPage buf;
    buf.pData = (char *)malloc(mPFMHandle->GetPageDataSize());
    buf.pageHeader.pageType = UF_PAGE;
    buf.pageHeader.crcType = CRC_FAST;
    char s[MAX_PASSWORD_LENGTH];
    bool ignore;
    if (changeEncryption && IsEncrypted() && (el->encMethod == 0))
    {
        strcpy(s, "");
        ignore = false;
    }
    else
    {
        strcpy(s, "");
        ignore = true;
    }
    for (int i=0; i<pages->mItemCount; i++)
    {
        if (count - result > mPFMHandle->GetPageDataSize())
            size = mPFMHandle->GetPageDataSize();
        else
            size = count - result;
        if (!mPFMHandle->ReadPage(&buf, pages->mItems[i], mPFMHandle->GetPageDataSize(), s, ignore))
            if (deleteCorruptedFiles)
            {
                result = mPFMHandle->mLastError;
                free(buf.pData);
                delete pages;
                return result;
            }
        memcpy(buffer + result, buf.pData, size);
        result += size;
    }
    ufHandle->position += result;
    free(buf.pData);
    delete pages;
    
    return result;
}

int SingleFileSystem::InternalFileWrite(int handle, const char *buffer, int count, SingleFileSystem *newFile, DirectoryElement *el, bool changeEncryption, const char *newPassword)
{
    mLastError = ER_Ok;
    int result = 0;
    if (count <= 0)
        return result;
    UserFileHandle *ufHandle = (UserFileHandle *)newFile->mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    DirectoryElement el1;
    newFile->mDMHandle->ReadItem(ufHandle->fileId, &el1);
    IntegerArray *pages = new IntegerArray(0, 1, 100);
    try
    {
        newFile->mUFMHandle->GetPages(&el1, ufHandle->position, count, true, pages);
    }
    catch(...)
    {
        delete pages;
        return result;
    }
    if (pages->mItemCount <= 0)
    {
        delete pages;
        return result;
    }
    FFPage buf;
    buf.pData = (char *)malloc(newFile->mPFMHandle->GetPageDataSize());
    buf.pageHeader.pageType = UF_PAGE;
    buf.pageHeader.crcType = CRC_FAST;
    int size;
    char s[MAX_PASSWORD_LENGTH];
	memset(s, 0, sizeof(s));
    for (int i=0; i<pages->mItemCount; i++)
    {
        if (count - result > newFile->mPFMHandle->GetPageDataSize())
            size = newFile->mPFMHandle->GetPageDataSize();
        else
            size = count - result;

        memcpy(buf.pData, buffer + result, mPFMHandle->GetPageDataSize());
    
        if (changeEncryption && (el->encMethod == 0) && (newPassword && strlen(newPassword)))
        {
            buf.pageHeader.encType = 1;
            strcpy(s, "");
        }
        else
        {
            buf.pageHeader.encType = 0;
            strcpy(s, "");
        }
        if (!newFile->mPFMHandle->WritePage(&buf, pages->mItems[i], newFile->mPFMHandle->GetPageDataSize(), s))
        {
            mLastError = newFile->mPFMHandle->mLastError;
            break;
        }
        if (el1.encMethod != 0)
            buf.pageHeader.encType = el1.encMethod;
        else
            buf.pageHeader.encType = newFile->mPFMHandle->mHeader.encMethod;
        
        if (!newFile->mPFMHandle->WritePage(&buf, pages->mItems[i], 0, ""))
        {
            mLastError = newFile->mPFMHandle->mLastError;
            break;
        }
        result += size;
    }
    
    ufHandle->position += result;
    free(buf.pData);
    if ((ufHandle->position > el1.fileSize) || (el1.fileSize == 0))
    {
        el1.fileSize = ufHandle->position;
    }
    newFile->mDMHandle->WriteItem(ufHandle->fileId, el1);
    delete pages;
    
    return result;
}

bool SingleFileSystem::InternalRepair(char *log, bool deleteCorruptedFiles, bool changeEncryption, const char *newPassword)
{
    HexString oldDir = GetCurrentDir();
    strcpy(log, "");
    bool result = false;
    mCancel = false;
    if (mInMemory)
      return result;

    //if (mPFMHandle->DiskFree() < DiskSize())
    //{
    //    strcpy(log, "Not enough space.");
    //    DoOnDiskFull(this);
    //    return result;
    //}
    mLastError = ER_Ok;
	HexString oldName = mPFMHandle->mFileName;
	HexString newName = oldName + "_new";
    SingleFileSystem *newFile;
    if (changeEncryption)
    {
        newFile = new SingleFileSystem(newName.C_String(), FM_CREATE, mInMemory, newPassword, GetCompressionLevel(), mPFMHandle->mHeader.pageSize, mPFMHandle->mHeader.extentPageCount);
        if (!newFile)
            return result;
	}
    else
    {
        newFile = new SingleFileSystem(newName.C_String(), FM_CREATE, mInMemory, mPassword.C_String(), GetCompressionLevel(), mPFMHandle->mHeader.pageSize, mPFMHandle->mHeader.extentPageCount);
        if (newFile == 0)
            return result;
        newFile->mPFMHandle->mHeader.passwordHeader = mPFMHandle->mHeader.passwordHeader;
        newFile->mPFMHandle->mHeader.encMethod = mPFMHandle->mHeader.encMethod;
        memcpy(newFile->mPFMHandle->mKey, mPFMHandle->mKey, MAX_PASSWORD_LENGTH);
    }
    char *buf = (char *)malloc(mPFMHandle->GetPageDataSize());
    int size = mPFMHandle->GetPageDataSize();
    if (buf == 0)
    {
        mLastError = ER_NoMemory;
        return false;
    }
    try
    {
        newFile->mPFMHandle->SaveSFHeader();
        mProgressMax = mDMHandle->mDir->mItemCount;
        mProgress = 0.0f;
        for (int i=0; i<mDMHandle->mDir->mItemCount; i++)
        {
            DoOnProgress(mProgress / mProgressMax * 100.0);
            if (mCancel)
            {
                newFile->mPFMHandle->mSFSFile->Close();
                newFile->mPFMHandle->mSFSFile->DeleteFile();
                delete newFile;
                strcat(log, "RepairFile cancelled. Original file restored.\n\t");
                return result;
            }

            mProgress = i + 1;
            DirectoryElement el;
            mDMHandle->ReadItem(i, &el);
            if (el.isDeleted != 0)
                continue;
            if (el.isFolder != 0)
            {
                HexString fullPath = mDMHandle->GetFullFilePath(i);
                newFile->ForceDirectories(fullPath.C_String());
                if (!newFile->DirectoryExists(fullPath.C_String()))
                {
                    strcat(log, "Directory ");
                    strcat(log, fullPath.C_String());
                    strcat(log, " deleted. \n\t");
                    continue;
                }
            }
            else
            {
                HexString fullPath = mDMHandle->GetFullFilePath(el.parentId);
                newFile->ForceDirectories(fullPath.C_String());
                if (!newFile->DirectoryExists(fullPath.C_String()))
                {
                    strcat(log, "Directory ");
                    strcat(log, fullPath.C_String());
                    strcat(log, " deleted. \n\t");
                    continue;
                }
                fullPath = mDMHandle->GetFullFilePath(i);
                int frHandle = InternalFileOpen(i);
                if (frHandle <= NONE)
                {
                    strcat(log, "file ");
                    strcat(log, fullPath.C_String());
                    strcat(log, " deleted. \n\t");
                    continue;
                }
                int fwHandle = newFile->FileOpen(fullPath.C_String(), FM_CREATE);
                UserFileHandle *ufHandle = (UserFileHandle *)newFile->mFileHandles->Find(fwHandle);
                if (ufHandle == 0)
                {
                    strcat(log, "file ");
                    strcat(log, fullPath.C_String());
                    strcat(log, " deleted. \n\t");
                    continue;
                }
                int fileId = ufHandle->fileId;
                DirectoryElement newEl;
                newFile->mDMHandle->ReadItem(fileId, &newEl);
                newEl.passwordHeader = el.passwordHeader;
                newEl.encMethod = el.encMethod;
                newEl.attributes = el.attributes;
                newEl.creationTime = el.creationTime;
                newEl.lastModifiedTime = el.lastModifiedTime;
                newEl.lastAccessTime = el.lastAccessTime;
                newFile->mDMHandle->WriteItem(fileId, newEl);
                int pageCount = el.fileSize / size;
                if ((el.fileSize % size) != 0)
                    pageCount ++;
                for (int j=0; j<pageCount; j++)
                {
                    memset(buf, 0, size);
                    int k = InternalFileRead(frHandle, buf, mPFMHandle->GetPageDataSize(), &el, changeEncryption, deleteCorruptedFiles);
                    if (deleteCorruptedFiles)
                    {
                        if ((mLastError != ER_Ok) || ((k != size) && (!((j == pageCount - 1) && (k == el.fileSize % size)))))
                        {
                            strcat(log, "file ");
                            strcat(log, fullPath.C_String());
                            strcat(log, " deleted. \n\t");
                            break;
                        }
                    }
                    if (j < pageCount - 1)
                        k = size;
                    else
                        k = el.fileSize % size;
                    if (InternalFileWrite(fwHandle, buf, k, newFile, &el, changeEncryption, newPassword) != k)
                    {
                        try
                        {
                            DoOnDiskFull(this);
                        }
                        catch(...)
                        {
                        }
                        //Not enough space
                        assert(false);;
                    }
                }
                newFile->mDMHandle->ReadItem(fileId, &newEl);
                if (deleteCorruptedFiles)
                    if (el.fileSize != newEl.fileSize)
                        assert(false);
                FileClose(frHandle);
                newFile->FileClose(fwHandle);
            }
        }
        free(buf);
    }
    catch(...)
    {
        strcat(log, "Critical error occured. Original file restored. \n\t");
        newFile->mPFMHandle->mSFSFile->Close();
        newFile->mPFMHandle->mSFSFile->DeleteFile();
        delete newFile;
        return result;
    }
    newFile->mPFMHandle->mSFSFile->Close();
    mPFMHandle->mSFSFile->Close();

    int i = 0;
    HexString s = FormatString("%s%i", mPFMHandle->mFileName.C_String(), i);

    do
    {
        while (FileExists(s.C_String()))
        {
            i ++;
            s = FormatString("%s%i", mPFMHandle->mFileName.C_String(), i);
        }
    }
    while (mPFMHandle->RenameFile(s) != true);

    if (!newFile->mPFMHandle->RenameFile(mPFMHandle->mFileName))
    {
        mPFMHandle->RenameFile(mPFMHandle->mFileName);
        mPFMHandle->mSFSFile->Open(false, true);
        delete newFile;
        mLastError = ER_RenameFileError;
        return result;
    }
    mPFMHandle->DeleteFile();

    delete mUFMHandle;
    delete mDMHandle;
    delete mFSMHandle;
    delete mPFMHandle;
    delete mFileHandles;
    delete newFile;

    if (changeEncryption)
        mPassword = newPassword;

	mPFMHandle = new PageFileManager(oldName.C_String(), "rb+", mPassword.C_String(), mInMemory);
    InternalCreate();
    mDMHandle->Load();
    DoOnProgress(100.0f);
    SetCurrentDir(oldDir.C_String());
    
    return true;
}

bool SingleFileSystem::Repair(char *log, bool deleteCorruptedFiles)
{
    if (mReadOnly)
        return false;
    return InternalRepair(log, deleteCorruptedFiles, false);
}

bool SingleFileSystem::IsPasswordValid(const char *fileName, const char *password)
{
    return mDMHandle->IsPasswordValid(fileName, password);
}

bool SingleFileSystem::GetPasswordHeader(const char *fileName, PasswordHeader *passHeader)
{
    return mDMHandle->GetPasswordHeader(fileName, passHeader);
}

void SingleFileSystem::SetPasswordHeader(const char *fileName, PasswordHeader *passHeader)
{
    mDMHandle->SetPasswordHeader(fileName, passHeader);
}

bool SingleFileSystem::IsEncrypted()
{
    return mPFMHandle->mHeader.encMethod != ENC_NONE;
}

bool SingleFileSystem::IsFileEncrypted(const char *fileName)
{
    return mDMHandle->IsFileEncrypted(fileName);
}

int SingleFileSystem::FileCreate(const char *fileName, const char *password)
{
    if (FileExists(fileName))
        if (!DeleteFile(fileName))
            return ER_FileNotDeleted;
    int result = mDMHandle->FileCreate(fileName, password);
    if (result < 0)
        mLastError = mDMHandle->mLastError;
    if ((result < 0) && (mLastError == ER_DiskFull))
        DoOnDiskFull(this);

    return result;
}

int SingleFileSystem::FileOpen(const char *fileName, __u32 mode, const char *password)
{
    if (mode == FM_CREATE)
        FileCreate(fileName, password);
    else
        if (FileExists(fileName))
            if (!mDMHandle->IsPasswordValid(fileName, password))
            {
                bool ok;
                bool skipFile = true;
                do
                {
                    DoOnPassword(fileName, (char *)password, skipFile);
                    ok = mDMHandle->IsPasswordValid(fileName, password);
                    if (skipFile)
                        break;
                } while (ok);

                if (!ok)
                    return NONE;
            }
    char key[MAX_PASSWORD_LENGTH];
    int result = mDMHandle->FileOpen(fileName, password, key);
    if (result <= NONE)
        return result;
    
    UserFileHandle *ufHandle = new UserFileHandle;
    ufHandle->fileId = result;
    ufHandle->position = 0;
    strcpy(ufHandle->key, key);
    ufHandle->mode = mode;
    result = mFileHandles->GetNextKeyValue();
    mFileHandles->Insert(result, ufHandle);

    return result;
}

void SingleFileSystem::FileClose(int handle)
{
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle != 0)
    {
        mDMHandle->FileClose(ufHandle->fileId);
        delete ufHandle;
        mFileHandles->Delete(handle);
    }
}

int SingleFileSystem::FileRead(int handle, char *buffer, int count)
{
    mLastError = ER_Ok;
    int result = 0;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    int size;
    if (ufHandle->position + count >= el.fileSize)
        size = el.fileSize - ufHandle->position;
    else
        size = count;
    if (size <= 0)
        return result;
    if (count > size)
        count = size;
    IntegerArray *pages = new IntegerArray(0, 1, 100);
    mUFMHandle->GetPages(&el, ufHandle->position, size, false, pages);
    if (pages->mItemCount <= 0)
        //FileRead - no pages were found by UFM.
        assert(false);
    FFPage buf;
    buf.pData = (char *)malloc(mPFMHandle->GetPageDataSize());
    buf.pageHeader.pageType = UF_PAGE;
    if (el.encMethod == ENC_NONE)
        if (mPFMHandle->mHeader.encMethod != ENC_NONE)
            buf.pageHeader.encType = mPFMHandle->mHeader.encMethod;
    buf.pageHeader.crcType = CRC_FAST;
    char key[MAX_PASSWORD_LENGTH];
    strcpy(key, "");
    if (el.encMethod != ENC_NONE)
        strcpy(key, ufHandle->key);
    else
        if (mPFMHandle->mHeader.encMethod != ENC_NONE)
            memcpy(key, mPFMHandle->mKey, MAX_PASSWORD_LENGTH);
    int offset;
    for (int i=0; i<pages->mItemCount; i++)
    {
        offset = 0;
        if (i == 0)
            offset = (ufHandle->position + mPFMHandle->GetPageDataSize()) % mPFMHandle->GetPageDataSize();

        if (count - result > mPFMHandle->GetPageDataSize() - offset)
            size = mPFMHandle->GetPageDataSize() - offset;
        else
            size = count - result;
        if (!mPFMHandle->ReadPage(&buf, pages->mItems[i], mPFMHandle->GetPageDataSize(), key))
        {
            mLastError = mPFMHandle->mLastError;
            break;
        }
        memcpy(buffer + result, (char *)buf.pData + offset, size);
        result += size;
    }
    ufHandle->position += result;
    free(buf.pData);
    delete pages;
    
    return result;
}

int SingleFileSystem::FileWrite(int handle, const char *buffer, int count)
{
    mLastError = ER_Ok;
    int result = 0;
    if (count <= 0)
        return result;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
	if (ufHandle->mode != FM_OPEN_READ_WRITE)
		 if ((ufHandle->mode & FM_OPEN_READ) != 0)
			return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    IntegerArray *pages = new IntegerArray(0, 1, 100);
    if (!mUFMHandle->GetPages(&el, ufHandle->position, count, true, pages))
    {
        delete pages;
        if (mUFMHandle->mLastError == ER_DiskFull)
            DoOnDiskFull(this);
        mLastError = mUFMHandle->mLastError;
        return result;
    }

    if (pages->mItemCount <= 0)
    {
        delete pages;
        return result;
    }

    FFPage buf;
    buf.pData = (char *)malloc(mPFMHandle->GetPageDataSize());
    buf.pageHeader.pageType = UF_PAGE;
    buf.pageHeader.encType = el.encMethod;
    if (el.encMethod == ENC_NONE)
        if (mPFMHandle->mHeader.encMethod != ENC_NONE)
            buf.pageHeader.encType = mPFMHandle->mHeader.encMethod;

    buf.pageHeader.crcType = CRC_FAST;
    char key[MAX_PASSWORD_LENGTH];
    strcpy(key, "");
    if (el.encMethod != ENC_NONE)
        strcpy(key, ufHandle->key);
    else
        if (mPFMHandle->mHeader.encMethod != ENC_NONE)
            memcpy(key, mPFMHandle->mKey, MAX_PASSWORD_LENGTH);
    for (int i=0; i<pages->mItemCount; i++)
    {
        int offset = 0;
        int size;
        if (i == 0)
            offset = (ufHandle->position + mPFMHandle->GetPageDataSize()) % mPFMHandle->GetPageDataSize();
        if (count - result > mPFMHandle->GetPageDataSize() - offset)
            size = mPFMHandle->GetPageDataSize() - offset;
        else
            size = count - result;

        if (!mPFMHandle->ReadPage(&buf, pages->mItems[i], mPFMHandle->GetPageDataSize(), key))
        {
            mLastError = mPFMHandle->mLastError;
            break;
        }

        memcpy((char *)buf.pData + offset, buffer + result, size);
        buf.pageHeader.pageType = UF_PAGE;
        buf.pageHeader.encType = el.encMethod;
        if (el.encMethod == ENC_NONE)
            if (mPFMHandle->mHeader.encMethod != ENC_NONE)
                buf.pageHeader.encType = mPFMHandle->mHeader.encMethod;

        buf.pageHeader.crcType = CRC_FAST;

        if (!mPFMHandle->WritePage(&buf, pages->mItems[i], mPFMHandle->GetPageDataSize(), key))
        {
            mLastError = mPFMHandle->mLastError;
            break;
        }
        result += size;
    }
    ufHandle->position += result;
    free(buf.pData);
    if ((ufHandle->position > el.fileSize) || (el.fileSize == 0))
        el.fileSize = ufHandle->position;
	//roc todo
    //SetCurrentTime(el.lastModifiedTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.lastModifiedTime = nowtime;
    mDMHandle->WriteItem(ufHandle->fileId, el);
    delete pages;
    
    return result;
}

__s64 SingleFileSystem::FileSeek(int handle, const __s64 offset, int origin)
{
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return 0;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    switch (origin)
    {
        case SEEK_SET:
            ufHandle->position = offset;
            break;
        case SEEK_END:
            ufHandle->position = el.fileSize + offset;
            break;
        case SEEK_CUR:
            ufHandle->position += offset;
    }
    return ufHandle->position;
}

void SingleFileSystem::FlushFileBuffers(int handle)
{
    mPFMHandle->FlushFileBuffers();
}

__s64 SingleFileSystem::FileSetSize(int handle, __s64 size)
{
    __s64 result = ER_InvalidHandle;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    if ((ufHandle->mode & FM_OPEN_READ) != 0)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    if (mUFMHandle->SetSize(&el, size, true))
    {
        el.fileSize = size;
		//roc todo
        //SetCurrentTime(el.lastModifiedTime);
		time_t nowtime;
		nowtime = time(NULL);
		el.lastModifiedTime = nowtime;
        mDMHandle->WriteItem(ufHandle->fileId, el);
        ufHandle->position = size;
        result = size;
    }
    else
    {
        if (mUFMHandle->mLastError == ER_DiskFull)
            DoOnDiskFull(this);
        mLastError = mUFMHandle->mLastError;
        result = mUFMHandle->mLastError;
    }
    
    return result;
}

__s64 SingleFileSystem::FileGetPosition(int handle)
{
    __s64 result = ER_InvalidHandle;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    return ufHandle->position;
}

__s64 SingleFileSystem::FileGetSize(int handle)
{
    __s64 result = ER_InvalidHandle;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    return el.fileSize;
}

bool SingleFileSystem::DeleteFile(const char *fileName)
{
    int fileId = mDMHandle->FindByName(fileName);
    if (fileId <= NONE)
        return false;
    if (mDMHandle->GetOpenFiles(fileId) > 0)
        return false;
    DirectoryElement el;
    mDMHandle->ReadItem(fileId, &el);
    el.isDeleted = 1;
	//roc todo
    //SetCurrentTime(el.lastModifiedTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.lastModifiedTime = nowtime;
    el.lastAccessTime = el.lastModifiedTime;
    mUFMHandle->SetSize(&el, 0, true);
    el.fileSize = 0;
    mDMHandle->WriteItem(fileId, el);
    return true;
}

bool SingleFileSystem::RenameFile(const char *oldName, const char *newName)
{
    return mDMHandle->RenameFile(oldName, newName);
}

bool SingleFileSystem::CopyFile(const char *oldName, const char *newName, const char *password)
{
    int blockSize = DEFAULT_COPY_BLOCK_SIZE;
    bool result = false;

    PasswordHeader passwordHeader;
    GetPasswordHeader(oldName, &passwordHeader);
    DeleteFile(newName);
    int outHandle = FileCreate(newName, password);
    if (outHandle < 0)
        return result;
    FileClose(outHandle);

    SetPasswordHeader(newName, &passwordHeader);

    __u16 attr = FileGetAttr(oldName);
    FileSetAttr(newName, attr);

    int inHandle = FileOpen(oldName, FM_OPEN_READ, password);
    if (inHandle < 0)
        return result;

    outHandle = FileOpen(newName, FM_OPEN_READ_WRITE, password);
    if (outHandle < 0)
        return result;

    DoOnFileProgress(this, 0);
    char *buf = (char *)malloc(blockSize);
    int count = 0;
    int size = FileSeek(inHandle, 0, SEEK_END);
    FileSeek(inHandle, 0, SEEK_SET);
    result = true;
    while (count < size)
    {
        int size1;
        if (size - count > blockSize)
            size1 = blockSize;
        else
            size1 = size - count;
        result = false;
        if (FileRead(inHandle, buf, size1) != size1)
            break;
        if (FileWrite(outHandle, buf, size1) != size1)
            break;
        result = true;
        count += size1;
        mProgressMax = size;
        mProgress = count;
        DoOnFileProgress(this, mProgress / mProgressMax * 100.0);
    }

    free(buf);

    FileClose(inHandle);
    FileClose(outHandle);

    DoOnFileProgress(this, 100.0f);
    
    return result;
}

bool SingleFileSystem::MoveFile(const char *oldName, const char *newName, const char *password)
{
    bool result = CopyFile(oldName, newName, password);
    if (result)
        result = DeleteFile(oldName);
    return result;
}

bool SingleFileSystem::FileExists(const char *fileName)
{
    return mDMHandle->FindByName(fileName) >= ER_Ok;
}

int SingleFileSystem::FileGetAttr(const char *fileName)
{
    int fileId = mDMHandle->FindByName(fileName);
    if (fileId <= NONE)
        return 0;
    DirectoryElement el;
    mDMHandle->ReadItem(fileId, &el);
    return el.attributes;
}

int SingleFileSystem::FileSetAttr(const char *fileName, int attr)
{
    int fileId = mDMHandle->FindByName(fileName);
    if (fileId <= NONE)
        return 0;
    DirectoryElement el;
    mDMHandle->ReadItem(fileId, &el);
    el.attributes = attr;
    mDMHandle->WriteItem(fileId, el);
    return 0;
}

int SingleFileSystem::FileAge(const char *fileName)
{
    int result = NONE;
    int fileId = mDMHandle->FindByName(fileName);
    if (fileId <= NONE)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(fileId, &el);
    //roc todo
    /*
    FileTimeToLocalFileTime(el->lastModifiedTime, localFileTime);
    if (FileTimeToDosDateTime(LocalFileTime, LongRec(Result).Hi, LongRec(Result).Lo))
        return result;
    */
    return result;
}

int SingleFileSystem::FileGetDate(int handle)
{
    int result = NONE;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    //roc todo
    /*
    FileTimeToLocalFileTime(el.LastModifiedTime, LocalFileTime);
    if FileTimeToDosDateTime(LocalFileTime, LongRec(Result).Hi,
                             LongRec(Result).Lo) then Exit;
    Result := None;
    */
	return el.lastModifiedTime;
    //return result;
}

int SingleFileSystem::FileSetDate(int handle, int age)
{
    int result = NONE;
    UserFileHandle *ufHandle = (UserFileHandle *)mFileHandles->Find(handle);
    if (ufHandle == 0)
        return result;
    DirectoryElement el;
    mDMHandle->ReadItem(ufHandle->fileId, &el);
    result = 0;
    /*
    if DosDateTimeToFileTime(LongRec(Age).Hi, LongRec(Age).Lo, LocalFileTime) and
        LocalFileTimeToFileTime(LocalFileTime, FileTime) then
    {
        el.LastModifiedTime := FileTime;
        DMHandle.WriteItem(UFHandle^.FileID, el);
        return result;
    }
    result = None;
    */
	time_t nowtime;
	nowtime = time(NULL);
	el.lastModifiedTime = nowtime;
    return result;
}

int SingleFileSystem::FindFirst(const char *path, int attr, SearchRec &f)
{
    return mDMHandle->FindFirst(path, attr, f);
}

int SingleFileSystem::FindNext(SearchRec &f)
{
    return mDMHandle->FindNext(f);
}

void SingleFileSystem::FindClose(SearchRec &f)
{
    mDMHandle->FindClose(f);
}

bool SingleFileSystem::ChangeEncryption(const char *newPassword)
{
    if (mReadOnly)
        return false;
    bool result = false;
    if (mPassword == newPassword)
    {
        DoOnProgress(0);
        CreatePasswordHeader(&mPFMHandle->mHeader.passwordHeader, mPassword.C_String());
        mPFMHandle->SaveSFHeader();
        if (mPFMHandle->mHeader.encMethod != ENC_NONE)
            CheckPassword(&mPFMHandle->mHeader.passwordHeader, mPassword.C_String(), mPFMHandle->mKey);
        DoOnProgress(100);
        result = true;
    }
    else
    {
        char log[10240];
        result = InternalRepair(log, false, true, newPassword);
    }
    return result;
}

bool SingleFileSystem::ChangeFilesEncryption(const char *fileMask, const char *oldPassword, const char *newPassword)
{
    bool result = false;
    SearchRec sr;
    FindFirst(fileMask, 0xff & (~_A_SUBDIR), sr);
    do
    {
        bool skipFile = false;
        char pwd[MAX_PASSWORD_LENGTH];
		memset(pwd, 0, MAX_PASSWORD_LENGTH);
        strcpy(pwd, oldPassword);

		while (!IsPasswordValid(sr.findData.name, pwd) && !skipFile)
			DoOnPassword(sr.findData.name, pwd, skipFile);

        if (!skipFile)
        {
            SFSFileStream *stream = new SFSFileStream(this, sr.findData.name, FM_OPEN_READ_WRITE, pwd);
            try
            {
                stream->ChangeEncryption(newPassword);
                result = true;
            }
            catch(...)
            {
            }
            delete stream;
        }
    }
    while (FindNext(sr) == 0);
	FindClose(sr);
    return result;
}

HexString SingleFileSystem::GetCurrentDir()
{
    return mDMHandle->GetCurrentDir();
}

bool SingleFileSystem::SetCurrentDir(const char *dir)
{
    return mDMHandle->SetCurrentDir(dir);
}

bool SingleFileSystem::RemoveDir(const char *dir)
{
    return mDMHandle->RemoveDir(dir);
}

bool SingleFileSystem::CreateDir(const char *dir)
{
    bool result = mDMHandle->CreateDir(dir);
    if (!result)
        mLastError = mDMHandle->mLastError;
    if (!result && (mLastError == ER_DiskFull))
        DoOnDiskFull(this);
    return result;
}

bool SingleFileSystem::ForceDirectories(const char *dir)
{
    bool result = mDMHandle->ForceDirectories(dir);
    if (!result)
        mLastError = mDMHandle->mLastError;
    if (!result && (mLastError == ER_DiskFull))
        DoOnDiskFull(this);
    return result;
}

bool SingleFileSystem::DirectoryExists(const char *name)
{
    return mDMHandle->DirectoryExists(name);
}

__s64 SingleFileSystem::DiskSize()
{
    return mPFMHandle->mSFSFile->GetSize();
}

__s64 SingleFileSystem::DiskFree()
{
    return mFSMHandle->GetFreePageCount();
}

HexString Copy(const char *str, int index)
{
    char newStr[250];
    strcpy(newStr, str + index);
    return HexString(newStr);
}

void SingleFileSystem::ProcessFilesForExport(const char *startPath, const char *path, const char *mask, int attr, int exclusive, void *list1, void *list2, int &result, bool recursive)
{
    DataStructures::List<HexString> *pathList = (DataStructures::List<HexString> *)list1;
    DataStructures::List<HexString> *fileList = (DataStructures::List<HexString> *)list2;
    
    mCancel = false;
    
    HexString name = path;
    name += mask;
    SearchRec sr;

    long handle = FindFirst(name.C_String(), attr, sr);
    if (handle == 0)
    {
        do
        {
            if (!strcmp(sr.findData.name, "..") || !strcmp(sr.findData.name, "."))
                continue;
            if ((sr.findData.attrib & (~attr & exclusive)) == 0)
                if (IsStrMatchPattern(sr.findData.name, mask, true))
                    if ((sr.findData.attrib & _A_SUBDIR) == 0)
                    {
                        result ++;
                        fileList->Insert(HexString(sr.findData.name));
                        if (!strcmp(path, startPath))
                            pathList->Insert(HexString(""));
                        else
                            pathList->Insert(Copy(path, strlen(startPath)));
                    }
            if ((sr.findData.attrib & _A_SUBDIR) != 0)
                if (recursive)
                {
                    if (!strcmp(path, startPath))
                        pathList->Insert(HexString(sr.findData.name) + "/");
                    else
                        pathList->Insert(Copy(path, strlen(startPath)) + sr.findData.name + "/");
                    fileList->Insert("");
                    HexString newPath = path;
                    newPath += sr.findData.name;
                    newPath += "/";
                    ProcessFilesForExport(startPath, newPath.C_String(), mask, attr, exclusive, list1, list2, result, recursive);
                }
        } while (FindNext(sr) == 0);
    }
    FindClose(sr);
}

void SingleFileSystem::ProcessFilesForImport(const char *startPath, const char *path, const char *mask, int attr, int exclusive, void *list1, void *list2, int &result, bool recursive)
{
	DataStructures::List<HexString> *pathList = (DataStructures::List<HexString> *)list1;
	DataStructures::List<HexString> *fileList = (DataStructures::List<HexString> *)list2;

	mCancel = false;

	HexString name = path;
	name += mask;
	_finddata_t sr;

    long handle = _findfirst(name.C_String(), &sr);
    if (handle >= 0)
    {
        do
        {
            if (!strcmp(sr.name, "..") || !strcmp(sr.name, ".") || !strcmp(sr.name, ".DS_Store"))
                continue;
            if ((sr.attrib & (~attr & exclusive)) == 0)
                if (IsStrMatchPattern(sr.name, mask, true))
                    if ((sr.attrib & _A_SUBDIR) == 0)
                    {
                        result ++;
                        fileList->Insert(HexString(sr.name));
                        if (!strcmp(path, startPath))
                            pathList->Insert(HexString(""));
                        else
                            pathList->Insert(Copy(path, strlen(startPath)));
                    }
            if ((sr.attrib & _A_SUBDIR) != 0)
                if (recursive)
                {
                    if (!strcmp(path, startPath))
                        pathList->Insert(HexString(sr.name) + "/");
                    else
                        pathList->Insert(Copy(path, strlen(startPath)) + sr.name + "/");
                    fileList->Insert("");
                    HexString newPath = path;
                    newPath += sr.name;
                    newPath += "/";
                    ProcessFilesForImport(startPath, newPath.C_String(), mask, attr, exclusive, list1, list2, result, recursive);
                }
        } while (_findnext(handle, &sr) == 0);
    }
    _findclose(handle);
}

void SingleFileSystem::ImportFile(const char *path, const char *file, const char *startPath, SFSOverwriteMode overwriteMode, bool encryptFiles)
{
    char myPath[250];
    strcpy(myPath, path);
    ForceDirectories(ExcludeTrailingBackslash(myPath));
    if (!strlen(file))
        return;
    char name[250];
    strcpy(name, path);
    strcat(name, file);
    bool overwrite = false;

    HexString folder = startPath;
    folder += name;
    
    if (FileExists(name))
    {
        if (overwriteMode == OM_NEVER)
            return;
        if (overwriteMode == OM_PROMPT)
        {
            DoOnOverwritePrompt(name, folder, overwrite);
            if (!overwrite)
                return;
        }
    }
    
    mCurrentFileName = name;
    char password[MAX_PASSWORD_LENGTH];
	memset(password, 0, MAX_PASSWORD_LENGTH);
    
    bool skipFile = true;
    if (encryptFiles)
        DoOnPassword(folder, password, skipFile);
    if (skipFile)
        strcpy(password, "");

    SFSFileStream *fs = new SFSFileStream(this, name, FM_CREATE, password);
    //fs->OnProgress = DoOnFileProgress;
	fs->LoadFromFile(folder.C_String());
	//roc todo
	//FileSetAttr(name, FileGetAttr(folder));
    delete fs;
}

int SingleFileSystem::ImportFiles(const char *sourcePath, const char *destPath, int attr, bool recursive, SFSOverwriteMode overwriteMode, bool encryptFiles)
{
    const __u32 faSpecial = _A_HIDDEN | _A_SYSTEM | _A_SUBDIR | FA_LABEL;
    int result = 0;
    HexString oldPath = GetCurrentDir();
    if (destPath && (strlen(destPath) > 0))
    {
        ForceDirectories(destPath);
        if (!SetCurrentDir(destPath))
        {
            SetCurrentDir(oldPath.C_String());
            return result;
        }
    }
    mCancel = false;
    mProgress = 0;
    DoOnProgress(0);
    char drive[32];
    char folder[250];
    char fileName[250];
    char extension[32];
    _splitpath(sourcePath, drive, folder, fileName, extension);
    HexString mask = fileName;
    mask += extension;
    if (!mask.GetLength())
        mask = "*.*";
    HexString startPath = drive;
    startPath += folder;
    if (startPath.GetLength() > 0)
		if (!_directoryExists(startPath.C_String()))
            return result;
    DataStructures::List<HexString> fileList;
    DataStructures::List<HexString> pathList;
    ProcessFilesForImport(startPath.C_String(), startPath.C_String(), mask.C_String(), attr, faSpecial, &pathList, &fileList, result, recursive);
    mProgressMax = fileList.Size();
    for (int i=0; i<fileList.Size(); i++)
    {
        try
        {
            result = i + 1;
            ImportFile(pathList[i].C_String(), fileList[i].C_String(), startPath.C_String(), overwriteMode, encryptFiles);
        }
        catch(...)
        {
            mCancel = true;
        }
        mProgress = i + 1;
        DoOnProgress(mProgress / mProgressMax * 100.0f);
        if (mCancel)
            break;
    }
    pathList.Clear();
    fileList.Clear();
    DoOnProgress(100.0f);
    if (destPath && (strlen(destPath) > 0))
        SetCurrentDir(oldPath);
  
    return result;
}

int SingleFileSystem::ImportFolder(const char *sourcePath, const char *destPath, bool recursive, SFSOverwriteMode overwriteMode, bool encryptFiles)
{
    if (!_directoryExists(sourcePath))
        return 0;
    int result = 0;
    HexString oldPath = GetCurrentDir();
    if (destPath && strlen(destPath))
    {
        ForceDirectories(destPath);
        if (!SetCurrentDir(destPath))
        {
            SetCurrentDir(oldPath.C_String());
            return result;
        }
    }

    char drive[32];
    char folder[250];
    char fileName[250];
    char extension[32];
    char myPath[256];
    strcpy(myPath, sourcePath);
    _splitpath(ExcludeTrailingBackslash(myPath), drive, folder, fileName, extension);
    HexString s = fileName;
    s += extension;
    if (!DirectoryExists(s.C_String()))
        if (!CreateDir(s.C_String()))
        {
            SetCurrentDir(oldPath.C_String());
            return result;
        }
    if (!SetCurrentDir(s.C_String()))
    {
        SetCurrentDir(oldPath.C_String());
        return result;
    }
    result = ImportFiles((HexString(IncludeTrailingBackslash(myPath)) + "*.*").C_String(), "", 0xff, recursive, overwriteMode, encryptFiles);
    SetCurrentDir(oldPath.C_String());

    return result;
}

void SingleFileSystem::ExportFile(const char *path, const char *file, const char *startPath, SFSOverwriteMode overwriteMode)
{
    char name[250];
    if (!getcwd(name, sizeof(name)))
        return;
    ExcludeTrailingBackslash(name);
    strcat(name, "/");
    char myPath[255];
    strcpy(myPath, path);
    ExcludeTrailingBackslash(myPath);
    strcat(name, myPath);
    
    __mkdir(name);
    if (!file || !strlen(file))
        return;
    strcpy(name, path);
    strcat(name, file);
    bool overwrite = false;
    if (_fileExists(name))
    {
        if (overwriteMode == OM_NEVER)
            return;
        if (overwriteMode == OM_PROMPT)
        {
            char p[250];
            strcpy(p, startPath);
            strcat(p, file);
            DoOnOverwritePrompt(name, p, overwrite);
            if (!overwrite)
                return;
        }
        //roc todo
        //SysUtils.FileSetAttr(name, 0);
        remove(name);
    }
    
    mCurrentFileName = name;
    char newName[250];
    strcpy(newName, startPath);
    strcat(newName, name);
    SFSFileStream *fs = new SFSFileStream(this, newName, FM_OPEN_READ, mPassword.C_String());
    //roc todo
    //fs->OnProgress = DoOnFileProgress;
    if (_fileExists(name))
        remove(name);
    if (_fileExists(name))
        //Error in TSingleFileSystem.ExportFiles, cannot be overwritten.
        assert(false);
    fs->SaveToFile(name);
    //roc todo
    //SysUtils.FileSetAttr(name, FileGetAttr(startPath + name));
    delete fs;
}

int SingleFileSystem::ExportFiles(const char *sourcePath, const char *destPath, int attr, bool recursive, SFSOverwriteMode overwriteMode)
{
    const __u32 faSpecial = _A_HIDDEN | _A_SYSTEM | _A_SUBDIR | FA_LABEL;
    int result = 0;
    char oldPath[250];
    if (!getcwd(oldPath, sizeof(oldPath)))
        return result;
    if (destPath && strlen(destPath))
    {
        __mkdir(destPath);
        if (chdir(destPath) < 0)
        {
            chdir(oldPath);
            return result;
        }
    }

    mCancel = false;
    mProgress = 0;
    DoOnProgress(0);
    char drive[32];
    char folder[250];
    char fileName[250];
    char extension[32];
    _splitpath(sourcePath, drive, folder, fileName, extension);
    HexString mask = fileName;
    mask += extension;
    if (!mask.GetLength())
        mask = "*.*";
    HexString startPath = drive;
    startPath += folder;
    if (startPath.GetLength())
        if (!DirectoryExists(startPath.C_String()))
            return result;

    DataStructures::List<HexString> fileList;
    DataStructures::List<HexString> pathList;

    ProcessFilesForExport(startPath.C_String(), startPath.C_String(), mask.C_String(), attr, faSpecial, &pathList, &fileList, result, recursive);
    mProgressMax = fileList.Size();
    for (int i=0; i<fileList.Size(); i++)
    {
        ExportFile(pathList[i].C_String(), fileList[i].C_String(), startPath.C_String(), overwriteMode);
        mProgress = i + 1;
        DoOnProgress(mProgress / mProgressMax * 100.0f);
        if (mCancel)
            break;
    }
    pathList.Clear();
    fileList.Clear();
    if (strlen(destPath))
        chdir(oldPath);
    DoOnProgress(100.0f);
    
    return result;
}

int SingleFileSystem::ExportFolder(const char *sourcePath, const char *destPath, bool recursive, SFSOverwriteMode overwriteMode)
{
    int result = 0;
    if (!DirectoryExists(sourcePath))
        return result;
    char oldPath[250];
    getcwd(oldPath, sizeof(oldPath));
    if (strlen(destPath))
    {
        __mkdir(destPath);
        if (chdir(destPath) < 0)
        {
            chdir(oldPath);
            return result;
        }
    }

    char drive[32];
    char folder[250];
    char fileName[250];
    char extension[32];
    char myPath[250];
    strcpy(myPath, sourcePath);
    ExcludeTrailingBackslash(myPath);
    _splitpath(myPath, drive, folder, fileName, extension);
	HexString s = fileName;
	s += extension;
	//if (s == "")
	//{
	//	s = "/";
	//}
    if (s.GetLength() && s.StrCmp("/") && s.StrCmp("\\"))
        __mkdir(s.C_String());
		/*
        {
            chdir(oldPath);
            return result;
        }
		*/
    if (chdir(s.C_String()) < 0)
    {
        chdir(oldPath);
        return result;
    }
    strcat(myPath, "/*.*");
    result = ExportFiles(myPath, "", 0xff, recursive, overwriteMode);
    chdir(oldPath);
    
    return result;
}

bool SingleFileSystem::IsFolderEmpty(const char *dir)
{
    bool result = true;
    HexString oldPath = GetCurrentDir();
    if (SetCurrentDir(dir))
    {
        SearchRec sr;
        if (FindFirst("*.*", 0xff, sr) == 0)
            result = false;
		FindClose(sr);
    }
    SetCurrentDir(oldPath.C_String());
    
    return result;
}

void SingleFileSystem::ProcessFilesForDelete(const char *path, const char *mask, int attr, int exclusive, bool recursive, int &result)
{
    SearchRec sr;
    mCancel = false;
    char name[255];
    strcpy(name, path);
    strcat(name, mask);
    if (FindFirst(name, attr, sr) == 0)
    {
        do
        {
            if ((sr.findData.attrib & _A_SUBDIR) == 0)
            {
                if ((sr.findData.attrib & (~attr & exclusive)) == 0)
                    if (IsStrMatchPattern(sr.findData.name, mask, true))
                    {
                        result ++;
                        strcpy(name, path);
                        strcat(name, sr.findData.name);
                        DeleteFile(name);
                    }
            }
            else
            {
                strcpy(name, path);
                strcat(name, sr.findData.name);
                if (recursive)
                {
                    char processName[250];
                    strcpy(processName, name);
                    strcat(processName, "/");
                    ProcessFilesForDelete(processName, mask, attr, exclusive, recursive, result);
                }
                if (!RemoveDir(name))
                    //DeleteFiles - could not delete directory
                    assert(false);
            }
        } while (FindNext(sr) == 0);
        FindClose(sr);
    }
}

int SingleFileSystem::DeleteFiles(const char *path, int attr, bool recursive)
{
    const __u32 faSpecial = _A_HIDDEN | _A_SYSTEM | _A_SUBDIR | FA_LABEL;
    int result = 0;
    char sourcePath[250];
    strcpy(sourcePath, path);
    ExcludeTrailingBackslash(sourcePath);
    char drive[32];
    char folder[250];
    char fileName[250];
    char extension[32];
    _splitpath(sourcePath, drive, folder, fileName, extension);
    char startPath[250];
    strcpy(startPath, drive);
    strcat(startPath, folder);
    char mask[250];
    strcpy(mask, fileName);
    strcat(mask, extension);
    if (!strlen(mask))
        strcpy(mask, "*.*");
    ProcessFilesForDelete(startPath, mask, attr, faSpecial, recursive, result);

    return result;
}


int SingleFileSystem::DeleteFolder(const char *dir)
{
    int result = 0;
    if (!DirectoryExists(dir))
        return result;
    if (IsFolderEmpty(dir))
    {
        RemoveDir(dir);
        return 1;
    }
    HexString oldPath = GetCurrentDir();
    char path[250];
    strcpy(path, dir);
    ExcludeTrailingBackslash(path);
    strcat(path, "/*.*");
    result = DeleteFiles(path);
    SetCurrentDir(oldPath.C_String());
    
	if (RemoveDir(dir)) return 1;

	return result;
}

//roc todo
/*
bool SingleFileSystem::RunApplication(const char *fileName, const char *parameters, const char *directory, int showCmd)
{
    ;
    char s1[250];
    if (!FileExists(fileName))
        return false;
    HexString s = GetTemporaryDirectory();
    char drive[32];
    char folder[250];
    char filename[250];
    char extension[32];
    _splitpath(fileName, drive, folder, filename, extension);
    HexString s1 = filename;
    s1 += extension;
    ExportFiles(fileName, s.C_String(), FM_CREATE, false, OM_ALWAYS);
    if (!_fileExists(s1.C_String()))
        return false;
    if (strlen(directory))
        s = directory;
    return ShellExecute(0, "Open", s1.C_String(), parameters, s.C_String(), showCmd) != 0;
}
*/

int SingleFileSystem::LoadLibrary(const char *fileName)
{
    if (!FileExists(fileName))
        return 0;
    HexString s = GetTemporaryDirectory();
        ExportFiles(fileName, s.C_String(), FM_CREATE, false, OM_ALWAYS);
    char drive[32];
    char folder[250];
    char filename[250];
    char extension[32];
    _splitpath(fileName, drive, folder, filename, extension);
    HexString s1 = s + filename;
    s1 += extension;
    if (!_fileExists(s1.C_String()))
        return 0;

    return LoadLibrary(s1.C_String());
}

void SingleFileSystem::LoadFromStream(Stream *stream)
{
    if (mReadOnly)
        //LoadFromStream - file is in read only mode
        assert(false);
    InternalReopen(stream);
}

void SingleFileSystem::LoadFromFile(const char *fileName)
{
    FileStream *stream = new FileStream(fileName, "rb");
    try
    {
        LoadFromStream(stream);
    }
    catch(...)
    {
    }
    delete stream;
}

void SingleFileSystem::SaveToStream(Stream *stream)
{
    mPFMHandle->mSFSFile->SaveToStream(stream);
}

void SingleFileSystem::SaveToFile(const char *fileName)
{
    FileStream *stream = new FileStream(fileName, "wb+");
    try
    {
        SaveToStream(stream);
    }
    catch(...)
    {
    }
    delete stream;
}

