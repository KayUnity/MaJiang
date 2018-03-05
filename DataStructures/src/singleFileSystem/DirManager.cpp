#include "singleFileSystem/DirManager.h"
#include "singleFileSystem/PageFileManager.h"
#include "singleFileSystem/FreeSpaceManager.h"
#include "LinuxStrings.h"
#include "streams/DS_Stream.h"
#include "singleFileSystem/SFSUtils.h"
#include "fileUtils/_FindFirst.h"


using namespace DataStructures;

void DirManager::Load()
{
    FFPage buf;
    buf.pageHeader.pageType = DIR_PAGE;
    buf.pageHeader.crcType = 0;
    buf.pageHeader.encType = mFMHandle->mHeader.encMethod;

    char *buffer = (char *)malloc(mFMHandle->mHeader.dirPageCount * mDirElementsPerPage * SFS_DIRECTORY_ELEMENT_SIZE);

    int i = 0;
    mDirPageMap->SetSize(0);
    int curPage = mFMHandle->mHeader.dirFirstPageNo;
    while (i < mFMHandle->mHeader.dirPageCount)
    {
        mDirPageMap->Append(curPage);
        //buf.pData = &mDir->Items[i * mDIRElementsPerPage];
        int size = mDirPageSize;
        buf.pData = buffer + i * mDirElementsPerPage * SFS_DIRECTORY_ELEMENT_SIZE;
        if (!mFMHandle->ReadPage(&buf, curPage, size))
            //Load - can not load page
            assert(false);
        curPage = buf.pageHeader.nextPageNo;
        i ++;
    }

    mDir->SetSize(0);
    mOpenedFiles->SetSize(0);
    for (int i=0; i<mFMHandle->mHeader.dirElementsCount; i++)
    {
        mDir->AppendItem(*((DirectoryElement *)(buffer + i * SFS_DIRECTORY_ELEMENT_SIZE)));
        mOpenedFiles->Append(0);
    }
    free(buffer);
}

bool DirManager::AddItem(DirectoryElement &item)
{
    bool result = true;
    // search for deleted directory elements
    for (int i=0; i<mDir->mItemCount; i++)
    {
        DirectoryElement &el = mDir->ReadItem(i);
        if (el.isDeleted != 0)
        {
            // deleted element found
            WriteItem(i, item);
            return result;
        }
    }

    FFPage buf;
    buf.pageHeader.pageType = DIR_PAGE;
    buf.pageHeader.crcType = 0;
    buf.pageHeader.encType = mFMHandle->mHeader.encMethod;
    buf.pData = 0;
    if (mFMHandle->mHeader.dirPageCount * mDirElementsPerPage == mFMHandle->mHeader.dirElementsCount)
    {
        int startPage;
        // try to add page
        bool uniform = false;
        if (mFMHandle->mHeader.dirPageCount == 0)
        {
            startPage = mFMHandle->mHeader.hdrPageCount;
        }
        else
        {
            if (mFMHandle->mHeader.dirPageCount > UNIFORM_MIN_PAGE_COUNT)
                uniform = true;
            startPage = mDirPageMap->mItems[mDirPageMap->mItemCount - 1];
        }
        IntegerArray *pages = new IntegerArray(1, 1, 1);
        if (!mSMHandle->GetPages(1, startPage, uniform, pages))
        {
            mLastError = ER_DiskFull;
            delete pages;
            result = false;
            //assert for debuging
            assert(false);
            return result;
        }

        // Appending new page
        int curPage;
        if (mFMHandle->mHeader.dirPageCount == 0)
        {
            curPage = pages->mItems[0];
            mFMHandle->mHeader.dirFirstPageNo = curPage;
        }
        else
        {
            // load last page header
            curPage = mDirPageMap->mItems[mDirPageMap->mItemCount - 1];
            if (!mFMHandle->ReadPage(&buf, curPage, 0))
                //AddItem - can not load last page
                assert(false);
            buf.pageHeader.nextPageNo = pages->mItems[0];
            if (!mFMHandle->WritePage(&buf, curPage, 0))
                //AddItem - can not save last page
                assert(false);
        }
        curPage = pages->mItems[0];
        delete pages;
        // append new page
        mDirPageMap->Append(curPage);
        mFMHandle->mHeader.dirPageCount ++;
    }
    // save last page
    mFMHandle->mHeader.dirElementsCount ++;
    mDir->AppendItem(item);
    mOpenedFiles->Append(0);
    int i = (mFMHandle->mHeader.dirElementsCount - 1) / mDirElementsPerPage;
    int curPage = mDirPageMap->mItems[mDirPageMap->mItemCount - 1];
    if ((mFMHandle->mHeader.dirElementsCount - 1) % mDirElementsPerPage != 0)
        if (!mFMHandle->ReadPage(&buf, curPage, 0))
            //AddItem - can not load  page,
            assert(false);

    int size = mDirPageSize;
    buf.pData = malloc(size);
    memcpy(buf.pData, &mDir->mItems[i * mDirElementsPerPage], size);
    if (!mFMHandle->WritePage(&buf, curPage, size))
        //AddItem - can not save page
        assert(false);
    free(buf.pData);
    mFMHandle->SaveSFHeader();
    
    return result;
}

void DirManager::ReadItem(int itemNo, DirectoryElement *item)
{
    *item = mDir->ReadItem(itemNo);
}

void DirManager::WriteItem(int itemNo, DirectoryElement &item)
{
    mDir->UpdateItem(item, itemNo);
    // if file deleted it means that it is closed
     if (item.isDeleted != 0)
        mOpenedFiles->mItems[itemNo] = 0;
    // save current page
    int i = itemNo / mDirElementsPerPage;
    int curPage = mDirPageMap->mItems[i];
    FFPage buf;
    if (!mFMHandle->ReadPage(&buf, curPage, 0))
        //WriteItem - can not load  page
        assert(false);

    int size = mDirPageSize;
    buf.pData = malloc(size);
    memcpy(buf.pData, &mDir->mItems[i * mDirElementsPerPage], size);
    if (!mFMHandle->WritePage(&buf, curPage, size))
        //WriteItem - can not save page
        assert(false);
    free(buf.pData);
}

int DirManager::FindByName(const char *fileName)
{
    return mDir->FindFileByName(fileName, mCurrentDir);
}

HexString DirManager::GetFullFilePath(int itemNo)
{
    return mDir->GetFullFilePath(itemNo);
}

int DirManager::FileCreate(const char *fileName, const char *password)
{
    int result = NONE;
    if (!fileName || !strlen(fileName))
        return result;
    if (!strcmp(fileName, "\\") || !strcmp(fileName, "/"))
        return result;
    DirectoryElement el;
    InitDirectoryElement(&el);
    
    char path[250];
    char dirName[250];
    int dirId;
    if (!ExtractPathAndPattern(fileName, path, dirName))
    {
        dirId = mCurrentDir;
        DataStructures::SetFileName(fileName, &el);
    }
    else
    {
        dirId = mDir->FindFileByName(path, mCurrentDir);
        DataStructures::SetFileName(dirName, &el);
    }
 
    if (dirId <= ER_InvalidPath)
        return result;
    // filling new file structure
    el.parentId = dirId;
    if (password && (strlen(password) > 0))
    {
        el.encMethod = ENC_RIJNDAEL;
        CreatePasswordHeader(&el.passwordHeader, password);
    }

    el.firstMapPageNo = NONE;
    el.fileSize = 0;
	//roc todo
    //SetCurrentTime(el.creationTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.creationTime = nowtime;
    el.lastModifiedTime = el.creationTime;
    el.lastAccessTime = el.creationTime;
    el.attributes = 0;
    el.isFolder = 0;
    if (AddItem(el))
        result = mDir->mItemCount - 1;
    
    return result;
}

int DirManager::FileOpen(const char *fileName, const char *password, char *key)
{
    // find existing file or folder
    int result = FindByName(fileName);
    DirectoryElement el;
    if (result <= NONE)
        return NONE;
    else
        // if folder was found return error
        el = mDir->ReadItem(result);
    if (el.isFolder != 0)
        return NONE;
    // inc number of opened file handles
    mOpenedFiles->mItems[result] ++;
    // prepare key value for read/write pages
    strcpy(key, "");
    if (el.encMethod != ENC_NONE)
        CheckPassword(&el.passwordHeader, password, key);
    if (!mFMHandle->mReadOnly)
    {
		//roc todo
        //SetCurrentTime(el.lastAccessTime);
		time_t nowtime;
		nowtime = time(NULL);
		el.lastAccessTime = nowtime;
        WriteItem(result, el);
    }
    return result;
}

void DirManager::FileClose(int itemNo)
{
    // dec number of opened file handles
    if ((itemNo >= 0) && (itemNo < mOpenedFiles->mItemCount))
        mOpenedFiles->mItems[itemNo] --;
}

bool DirManager::RenameFile(const char *oldName, const char *newName)
{
    bool result = false;
    // check if file exists
    int fileId = FindByName(newName);
    if ((fileId >= ER_Ok) && (_stricmp(oldName, newName) != 0))
        return result;

    fileId = FindByName(oldName);
    if (fileId <= NONE)
        return result;
    if (GetOpenFiles(fileId) > 0)
        return result;

    char path[250];
    char dirName[250];
    char name[250];
    int dirId;
    if (!ExtractPathAndPattern(newName, path, dirName))
    {
        dirId = mCurrentDir;
        strcpy(name, newName);
    }
    else
    {
        dirId = mDir->FindFileByName(path, mCurrentDir);
        name[strlen(dirName)] = 0;
        strcpy(name, dirName);
        if (dirId <= ER_InvalidPath)
        {
            if (!ForceDirectories(path))
                return result;
            dirId = mDir->FindFileByName(path, mCurrentDir);
            if (dirId <= ER_InvalidPath)
                return result;
        }
    }
    // mark old file as deleted
    DirectoryElement el;
    ReadItem(fileId, &el);
    el.isDeleted = 1;
    WriteItem(fileId, el);
    // filling new file structure
    el.parentId = dirId;
    el.isDeleted = 0;
    SetFileName(name, &el);
	//roc todo
    //SetCurrentTime(el.lastModifiedTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.lastModifiedTime = nowtime;
    el.lastAccessTime = el.lastModifiedTime;
    // write new element
    WriteItem(fileId, el);
    result = true;
    
    return result;
}

int DirManager::GetOpenFiles(int itemNo)
{
    if ((itemNo >= 0) && (itemNo < mOpenedFiles->mItemCount))
        return mOpenedFiles->mItems[itemNo];
    else
        return NONE;
}

bool DirManager::IsPasswordValid(const char *fileName, const char *password)
{
    int fileId = FindByName(fileName);
    if ((fileId < ER_Ok) || (fileId >= mDir->mItemCount))
        return false;
    DirectoryElement &el = mDir->ReadItem(fileId);
    char key[MAX_PASSWORD_LENGTH];
    if (el.encMethod != ENC_NONE)
        if (!CheckPassword(&el.passwordHeader, password, key))
            return false;
    return true;
}

bool DirManager::GetPasswordHeader(const char *fileName, PasswordHeader *passHeader)
{
    int fileId = FindByName(fileName);
    if ((fileId < ER_Ok) || (fileId >= mDir->mItemCount))
        return false;
    DirectoryElement &el = mDir->ReadItem(fileId);
    *passHeader = el.passwordHeader;
    return true;
}

void DirManager::SetPasswordHeader(const char *fileName, PasswordHeader *passHeader)
{
    int fileId = FindByName(fileName);
    if ((fileId < ER_Ok) || (fileId >= mDir->mItemCount))
        return;
    DirectoryElement &el = mDir->ReadItem(fileId);
    el.passwordHeader = *passHeader;
    WriteItem(fileId, el);
}

bool DirManager::IsFileEncrypted(const char *fileName)
{
    int fileId = FindByName(fileName);
    if ((fileId < ER_Ok) || (fileId >= mDir->mItemCount))
        return false;
    DirectoryElement &el = mDir->ReadItem(fileId);
    if (el.encMethod != ENC_NONE)
        return true;
    else
        return false;
}

DirManager::DirManager(PageFileManager *pageFileManager, FreeSpaceManager *freeSpaceManager)
{
    mFMHandle = pageFileManager;
    mSMHandle = freeSpaceManager;

    mDirPageMap = new IntegerArray(0, 10, 100);

    mCurrentDir = -1; // root
    mCurrentPath = "/";
    mDirElementsPerPage = mFMHandle->GetPageDataSize() / SFS_DIRECTORY_ELEMENT_SIZE;
    mDirPageSize = mDirElementsPerPage * SFS_DIRECTORY_ELEMENT_SIZE;
    mDir = new DirArray(0, mDirElementsPerPage, mDirElementsPerPage * 10);
    mOpenedFiles = new IntegerArray(0, 10, 100);
    Load();
}

DirManager::~DirManager()
{
    delete mDirPageMap;
    delete mDir;
    delete mOpenedFiles;
}

int DirManager::FindFirst(const char *path, int attr, SearchRec &f)
{
    if ((path[0] == '\\') || (path[0] == '/'))
        return mDir->FindFirst(path, attr, f, ROOT_ID);
    else
        return mDir->FindFirst(path, attr, f, mCurrentDir);
}

int DirManager::FindNext(SearchRec &f)
{
    return mDir->FindNext(f);
}

void DirManager::FindClose(SearchRec &f)
{
    return mDir->FindClose(f);
}

HexString DirManager::GetCurrentDir()
{
    return mCurrentPath;
}

bool DirManager::SetCurrentDir(const char *dir)
{
    if (!dir || !strlen(dir))
        return false;
    int res = mDir->FindFileByName(dir, mCurrentDir);
    if (res <= ER_InvalidPath)
        return false;
    mCurrentDir = res;
    mCurrentPath = mDir->GetFullFilePath(mCurrentDir);
    return true;
}

bool DirManager::RemoveDir(const char *dir)
{
    bool result = false;
    int dirId = mDir->FindFileByName(dir, mCurrentDir);
    if (dirId <= ER_InvalidPath)
        return false;
    // check if is not empty
    HexString name;
    if ((dir[strlen(dir) - 1] == '\\') || (dir[strlen(dir) - 1] == '/'))
        name = HexString(dir) + "*.*";
    else
        name = HexString(dir) + "/*.*";

    int res;
    SearchRec sr;
    if ((dir[0] == '/') || (dir[0] == '\\'))
        res = mDir->FindFirst(name.C_String(), 0xff, sr, ROOT_ID);
    else
        res = mDir->FindFirst(name.C_String(), 0xff, sr, mCurrentDir);
    mDir->FindClose(sr);
    if (res == ER_Ok)
        return result;

    DirectoryElement el = mDir->ReadItem(dirId);
    if (el.isFolder != 1)
        return result;
    el.isDeleted = 1;
	//roc todo
    //SetCurrentTime(el.lastModifiedTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.lastModifiedTime = nowtime;
    el.lastAccessTime = el.lastModifiedTime;
    WriteItem(dirId, el);
    return true;
}

bool DirManager::CreateDir(const char *dir)
{
    if (!dir || !strlen(dir))
        return false;
    if (!strcmp(dir, "\\") || !strcmp(dir, "/"))
        return false;
    if (DirectoryExists(dir))
        return false;

    DirectoryElement el;
    InitDirectoryElement(&el);

    char path[250];
    char dirName[250];
    int dirId;
    if (!ExtractPathAndPattern(dir, path, dirName))
    {
        dirId = mCurrentDir;
        SetFileName(dir, &el);
    }
    else
    {
        dirId = mDir->FindFileByName(path, mCurrentDir);
        SetFileName(dirName, &el);
    }
    if (dirId <= ER_InvalidPath)
        return false;
    // filling directory structure
    el.parentId = dirId;
    el.firstMapPageNo = NONE;
	//roc todo
    //SetCurrentTime(el.creationTime);
	time_t nowtime;
	nowtime = time(NULL);
	el.creationTime = nowtime;
    el.lastModifiedTime = el.creationTime;
    el.lastAccessTime = el.creationTime;
    el.attributes = _A_SUBDIR;
    el.isFolder = 1;

    return AddItem(el);
}

bool DirManager::ForceDirectories(const char *dir)
{
    if (!dir || !strlen(dir))
        return false;

    bool result;
    char path[250];
    char dirName[250];
    if (!ExtractPathAndPattern(dir, path, dirName))
    {
        result = true;
        if (!DirectoryExists(dir))
            result = CreateDir(dir);
        return result;
    }
    else
        if (!strlen(path))
        {
            result = true;
            if (!DirectoryExists(dir))
                result = CreateDir(dir);
            return result;
        }

    if (!DirectoryExists(path))
        if (!ForceDirectories(path))
            //ForceDirectories - ForceDirectories error!
            result = true;
    if (!DirectoryExists((HexString(path) + "/" + HexString(dirName)).C_String()))
        result = CreateDir((HexString(path) + "/" + dirName).C_String());

    return result;
}

bool DirManager::DirectoryExists(const char *name)
{
    // if check for root folder - it always exists
    if (!strcmp(name, "\\") || !strcmp(name, "/"))
        return true;
    int res;
    SearchRec sr;
    if ((name[0] == '\\') || (name[0] == '/'))
        res = mDir->FindFirst(name, _A_SUBDIR, sr, ROOT_ID);
    else
        res = mDir->FindFirst(name, _A_SUBDIR, sr, mCurrentDir);
    mDir->FindClose(sr);
    if (res != ER_Ok)
        return false;
    return true;
}
