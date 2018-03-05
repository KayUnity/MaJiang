#include "singleFileSystem/UserFilePageMapManager.h"
#include "singleFileSystem/PageFileManager.h"
#include "singleFileSystem/FreeSpaceManager.h"
#include "singleFileSystem/SFSUtils.h"

using namespace DataStructures;

UserFilePageMapManager::UserFilePageMapManager(PageFileManager *pfmHandle1, FreeSpaceManager *fsmHandle1)
{
    mPFMHandle = pfmHandle1; // page file manager
    mPSMHandle = fsmHandle1; // free space manager

    // pointers to UFPM map
    mUFPMMaps = new SortedPtrArray(0, 10, 25);
    // pointers to file page map
    mUFMaps = new SortedPtrArray(0, 10, 25);
    // how many user file pages are addressed by one map page
    mPagesPerMapPage = mPFMHandle->GetPageDataSize() / sizeof(int);
    // temp array
    mTempPages = new IntegerArray(10000);
}

UserFilePageMapManager::~UserFilePageMapManager()
{
    for (int i=0; i<mUFPMMaps->mItemCount; i++)
        if (mUFPMMaps->mValueItems[i] != 0)
            delete (IntegerArray *)mUFPMMaps->mValueItems[i];
    delete mUFPMMaps;
    for (int i=0; i<mUFMaps->mItemCount; i++)
        if (mUFMaps->mValueItems[i] != 0)
            delete (IntegerArray *)mUFMaps->mValueItems[i];
    delete mUFMaps;
    delete mTempPages;
}

int UserFilePageMapManager::GetCoverPageCount(__s64 size)
{
     int result = size / mPFMHandle->GetPageDataSize();
     if ((size % mPFMHandle->GetPageDataSize()) != 0)
         result ++;
    return result;
}

void UserFilePageMapManager::GetMaps(DirectoryElement *fileRec, IntegerArray **ufpmMap, IntegerArray **ufMap)
{
    if (fileRec->firstMapPageNo == NONE)
        //GetMaps - Invalid parameter FileRec.FirstMapPageNo
        assert(false);

    // check for maps overflow
    if (mUFPMMaps->mItemCount > 100)
    {
        for (int i=0; i<mUFPMMaps->mItemCount; i++)
            if (mUFPMMaps->mValueItems[i] != 0)
                delete (IntegerArray *)mUFPMMaps->mValueItems[i];
        mUFPMMaps->SetSize(0);
    }
    if (mUFMaps->mItemCount > 100)
    {
        for (int i=0; i< mUFMaps->mItemCount; i++)
            if (mUFMaps->mValueItems[i] != 0)
                delete (IntegerArray *)mUFMaps->mValueItems[i];
        mUFMaps->SetSize(0);
    }

    // try to find existing maps (UFPMMap, UFMap)
    *ufpmMap = (IntegerArray *)mUFPMMaps->Find(fileRec->firstMapPageNo);
    *ufMap = (IntegerArray *)mUFMaps->Find(fileRec->firstMapPageNo);
    // if not found - try to load
    if (*ufpmMap == 0)
    {
        *ufpmMap = new IntegerArray(0, 10, 25);
        *ufMap = new IntegerArray(0, 10, 25);

        FFPage ffPage;
        // allocate buffer for page
        mPFMHandle->AllocPageBuffer(&ffPage);
        // quantity of file pages
        int filePageCount = GetCoverPageCount(fileRec->fileSize);
        // quantity of map pages
        int mapPageCount = filePageCount / mPagesPerMapPage;
        if ((filePageCount % mPagesPerMapPage) != 0)
            mapPageCount ++;
        // starting from first map page No
        int mapPageNo = fileRec->firstMapPageNo;
        // load all map pages
        int filePageNo = 0;
        if (mapPageCount == 0)
            (*ufpmMap)->Append(mapPageNo);
        else
            for (int i=0; i<mapPageCount; i++)
            {
                // read page
                if (!mPFMHandle->ReadPage(&ffPage, mapPageNo))
                    //GetMaps - Error on reading map page.
                    assert(false);
                // store map page No
                (*ufpmMap)->Append(mapPageNo);
                // store file pages No
                for (int j=0; j<mPagesPerMapPage; j++)
                {
                    if (filePageNo < filePageCount)
                        (*ufMap)->Append(*(int *)((char *)ffPage.pData + j * sizeof(int)));
                    filePageNo ++;
                }
                // get No of next map page
                mapPageNo = ffPage.pageHeader.nextPageNo;
                if ((filePageNo < filePageCount) && (mapPageNo == NONE))
                    //GetMaps - Invalid page No in map chain.
                    assert(false);
            }
        // add maps to global list of maps
        mUFPMMaps->Insert(fileRec->firstMapPageNo, *ufpmMap);
        mUFMaps->Insert(fileRec->firstMapPageNo, *ufMap);
        // free page buffer
        mPFMHandle->FreePageBuffer(&ffPage);
    }
    if (*ufpmMap == 0)
        //GetMaps - Cannot find UFPM page map
        assert(false);
    if (*ufMap == 0)
        //GetMaps - Cannot find user file page map
        assert(false);
    if ((*ufpmMap)->mItems[0] != fileRec->firstMapPageNo)
        //GetMaps - UFPMMap.Items[0] <> FirstMapPageNo
        assert(false);
}

bool UserFilePageMapManager::AppendPages(DirectoryElement *fileRec, int pageCount, bool writeAppendedPages)
{
    bool result = true;
    if (pageCount <= 0)
        //AppendPages - PageCount <= 0
        assert(false);
    // is new file (no pages in map at all)
    IntegerArray *ufpmMap = 0;
    IntegerArray *ufMap = 0;
    int saveItemNo;
    int saveItemCount;
    if (fileRec->firstMapPageNo == NONE)
    {
        // calc map pages count required for the appended pages
        int mapPageCount = pageCount / mPagesPerMapPage;
        if ((pageCount % mPagesPerMapPage) != 0)
            mapPageCount ++;
        // allocate pages for map extension
        ufpmMap = new IntegerArray(0, 10, 25);
        if (!mPSMHandle->GetPages(mapPageCount, 1, false, ufpmMap))
        {
            delete ufpmMap;
            mLastError = ER_DiskFull;
            return false;
        }
        // set first map page No
        fileRec->firstMapPageNo = ufpmMap->mItems[0];
        // add new UFPM page map
        mUFPMMaps->Insert(fileRec->firstMapPageNo, ufpmMap);
        ufMap = new IntegerArray(0, 10, 25);
        // add new user file page map
        mUFMaps->Insert(fileRec->firstMapPageNo, ufMap);
        // set what map pages will be saved
        saveItemNo = 0;
        saveItemCount = ufpmMap->mItemCount;
    }
    else
    {
	    ufpmMap = 0;
		ufMap = 0;
        // find or load maps
        GetMaps(fileRec, &ufpmMap, &ufMap);
        // calc count of additional required map pages
        int pc = ((ufMap->mItemCount + pageCount) / mPagesPerMapPage) - ufpmMap->mItemCount;
        if (((ufMap->mItemCount + pageCount) % mPagesPerMapPage) != 0)
            pc ++;
        if (pc > 0)
        {
            // allocate pages for map extension near the last map page
            if (!mPSMHandle->GetPages(pc, ufpmMap->mItems[ufpmMap->mItemCount - 1] + 1, false, mTempPages))
            {
                mLastError = ER_DiskFull;
                return false;
            }
            // extend UFPM in memory
            for (int i=0; i<mTempPages->mItemCount; i++)
                ufpmMap->Append(mTempPages->mItems[i]);
            // always save last map page (as it has link to the next page)
            saveItemNo = ufpmMap->mItemCount - pc - 1;
            saveItemCount = pc + 1;
        }
        else
        {
            // no additional map pages required - save only last page
            saveItemNo = ufpmMap->mItemCount - 1;
            saveItemCount = 1;
        }
    }

    int desiredPageNo;
    // desired start page is next to the EOF page
    if (ufMap->mItemCount > 0)
        desiredPageNo = ufMap->mItems[ufMap->mItemCount - 1] + 1;
    else
        desiredPageNo = 1;
    bool isUniform;
    // allocate uniform extent?
    if (ufMap->mItemCount + pageCount > mPFMHandle->mHeader.extentPageCount)
        isUniform = true;
    else
        isUniform = false;
    // allocate pages for user file map extension
    if (!mPSMHandle->GetPages(pageCount, desiredPageNo, isUniform, mTempPages))
    {
        mLastError = ER_DiskFull;
        return false;
    }
    // extend user file map in memory
    for (int i=0; i<mTempPages->mItemCount; i++)
        ufMap->Append(mTempPages->mItems[i]);

    // save map pages
    SaveMapPages(saveItemNo, saveItemCount, ufpmMap, ufMap);

    // if specified - save empty headers for appended pages
    if (writeAppendedPages)
    {
        FFPage ffPage;
        // allocate buffer for page
        mPFMHandle->AllocPageBuffer(&ffPage);
        ffPage.pageHeader.pageType = UF_PAGE;
        ffPage.pageHeader.encType = 0; // none
        ffPage.pageHeader.crcType = 0; // fast
        // write appended pages with trash
        for (int i=0; i<mTempPages->mItemCount; i++)
            mPFMHandle->WritePage(&ffPage, mTempPages->mItems[i]);
        // free page buffer
        mPFMHandle->FreePageBuffer(&ffPage);
    }
    return result;
}

void UserFilePageMapManager::DeletePagesFromEOF(DirectoryElement *fileRec, int pageCount)
{
    if (fileRec->firstMapPageNo == NONE)
        //DeletePagesFromEOF - File map not exists.
        assert(false);

    IntegerArray *ufpmMap = 0;
    IntegerArray *ufMap = 0;
    // find or load maps
    GetMaps(fileRec, &ufpmMap, &ufMap);

    // get new map pages count
    int newMapPageCount = (ufMap->mItemCount - pageCount) / mPagesPerMapPage;
    if ((ufMap->mItemCount - pageCount) % mPagesPerMapPage > 0)
        newMapPageCount ++;

    // init array of pages to free
    mTempPages->SetSize(0);

    // if map pages count decreased
    if (newMapPageCount < ufpmMap->mItemCount)
    {
        // fill map pages to free
        for (int i=newMapPageCount; i<ufpmMap->mItemCount; i++)
            mTempPages->Append(ufpmMap->mItems[i]);
        // remove pages from memory array
        ufpmMap->SetSize(newMapPageCount);
    }

    //--- free user file pages ---
    // fill pages to free
    for (int i=ufMap->mItemCount - pageCount; i<ufMap->mItemCount; i++)
        mTempPages->Append(ufMap->mItems[i]);
    // remove pages from memory array
    ufMap->SetSize(ufMap->mItemCount - pageCount);

    // free pages
    mPSMHandle->FreePages(mTempPages);

    // save last map page if size > 0
    if (newMapPageCount > 0)
        SaveMapPages(newMapPageCount - 1, 1, ufpmMap, ufMap);
    else
    {
        // file becomes of zero size - delete maps
        delete ufpmMap;
        delete ufMap;
        mUFPMMaps->Delete(fileRec->firstMapPageNo);
        mUFMaps->Delete(fileRec->firstMapPageNo);
        fileRec->firstMapPageNo = NONE;
    }
}

void UserFilePageMapManager::SaveMapPages(int itemNo, int itemCount, IntegerArray *ufpmMap, IntegerArray *ufMap)
{
    // allocate buffer for page
    FFPage ffPage;
    mPFMHandle->AllocPageBuffer(&ffPage);

    ffPage.pageHeader.pageType = UFPM_PAGE;
    ffPage.pageHeader.encType = 0; // none
    ffPage.pageHeader.crcType = 0; // fast
    // save all map pages
    for (int i=itemNo; i<itemNo + itemCount; i++)
    {
        if (i + 1 < ufpmMap->mItemCount)
            ffPage.pageHeader.nextPageNo = ufpmMap->mItems[i + 1];
        else
            ffPage.pageHeader.nextPageNo = -1;
        // copy data from TIntegerArray to FF page
        int usedBytes;
        if ((i + 1 ) * mPagesPerMapPage <= ufMap->mItemCount)
            usedBytes = mPagesPerMapPage * sizeof(int);
        else
            usedBytes = (ufMap->mItemCount - i * mPagesPerMapPage) * sizeof(int);
        memcpy(ffPage.pData, &ufMap->mItems[i * mPagesPerMapPage], usedBytes);
        // write page
        mPFMHandle->WritePage(&ffPage, ufpmMap->mItems[i]);
    }

    // free page buffer
    mPFMHandle->FreePageBuffer(&ffPage);
}

bool UserFilePageMapManager::GetPages(DirectoryElement *fileRec, __s64 offset, __s64 size, bool isAllocateAllowed, IntegerArray *pages)
{
    IntegerArray *ufpmMap = 0;
    IntegerArray *ufMap = 0;
    bool result = true;
    //--- append if necessary ---
    // new file?
    if (fileRec->firstMapPageNo == NONE)
    {
        // append pages allowed?
        if (isAllocateAllowed)
        {
            result = SetSize(fileRec, offset + size, true);
            // find or load page maps
            if (result)
                GetMaps(fileRec, &ufpmMap, &ufMap);
        }
    }
    else
    {
        // find or load page maps
        GetMaps(fileRec, &ufpmMap, &ufMap);
    }
   
    // if new size exceeds alocated pages summary size - append pages
    if ((offset + size > fileRec->fileSize) && (isAllocateAllowed))
        result = SetSize(fileRec, offset + size, true);

    if (!result)
        return result;

    // clear pages parameter items
    pages->SetSize(0);
    // if file exists
    if (fileRec->firstMapPageNo != NONE)
    {
        int startPageNo = offset / mPFMHandle->GetPageDataSize();
        int endPageNo = (offset + size) / mPFMHandle->GetPageDataSize();
        // build requested pages list
        int i = startPageNo;
        while ((i <= endPageNo) && (i < ufMap->mItemCount))
        {
            pages->Append(ufMap->mItems[i]);
            i ++;
        }
    }
    return result;
}

bool UserFilePageMapManager::SetSize(DirectoryElement *fileRec, __s64 newSize, bool writeAppendedPages)
{
    IntegerArray *ufpmMap = 0;
    IntegerArray *ufMap = 0;
    
    bool result = true;
    int appendPageCount;
    //--- append if necessary ---
    // new file?
    if (fileRec->firstMapPageNo == NONE)
    {
        if (newSize != 0)
        {
            appendPageCount = GetCoverPageCount(newSize);
            result = AppendPages(fileRec, appendPageCount, writeAppendedPages);
        }
    }// zero size file
    else
    {
        // find or load page maps
        GetMaps(fileRec, &ufpmMap, &ufMap);
        // new file page count
        int newPageCount = GetCoverPageCount(newSize);
        // append or delete?
        if (newSize > fileRec->fileSize)
        {
            // append
            appendPageCount = newPageCount - ufMap->mItemCount;
            if (appendPageCount > 0)
                result = AppendPages(fileRec, appendPageCount, writeAppendedPages);
        }
        else
            if (newSize < fileRec->fileSize)
            {
                // delete
                int deletePageCount = ufMap->mItemCount - newPageCount;
                if (deletePageCount > 0)
                    DeletePagesFromEOF(fileRec, deletePageCount);
            }
    }
    return result;
}

