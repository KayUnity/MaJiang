#include "singleFileSystem/FreeSpaceManager.h"
#include "singleFileSystem/PageFileManager.h"
#include "singleFileSystem/SFSArray.h"
#include "streams/DS_Stream.h"
#include "singleFileSystem/SFSUtils.h"

using namespace DataStructures;

void FreeSpaceManager::LoadPFS()
{
    mPFSPageMap->SetSize(0);
    mPFS = 0;
    int pageCount = mPMHandle->mHeader.pfsPageCount;
    if (pageCount <= 0)
        return;
    mPFS = (char *)malloc(pageCount * mPFSPageSize);
    int i = 0;
    int curPage = 1;
    while (i < pageCount)
    {
        mPFSPageMap->Append(curPage);
        FFPage buf;
        buf.pData = mPFS + i * mPFSPageSize;
        if (!mPMHandle->ReadPage(&buf, curPage, mPFSPageSize))
            //LoadPFS - can not load page
            assert(false);
        curPage += mPFSExtentsPerPage * mExtentPageCount;
        i ++;
    }
}

void FreeSpaceManager::LoadGAM()
{
    mGAMPageMap->SetSize(0);
    mGAM = 0;
	mSGAM = 0;
    int pageCount = mPMHandle->mHeader.gamPageCount;
    if (pageCount <= 0)
        return;
    mGAM = (char *)malloc(pageCount * mGAMPageSize);
    mSGAM = (char *)malloc(pageCount * mGAMPageSize);
    int i = 0;
    int curPage = 2;
    while (i < pageCount)
    {
        mGAMPageMap->Append(curPage);
        FFPage buf;
        buf.pData = mGAM + i * mGAMPageSize;
        if (!mPMHandle->ReadPage(&buf, curPage, mGAMPageSize))
            //LoadGAM - can not load page
            assert(false);
        buf.pData = mSGAM + i * mGAMPageSize;
        if (!mPMHandle->ReadPage(&buf, curPage + 1, mGAMPageSize))
            //LoadGAM - can not load SGAM page
            assert(false);
        curPage += mGAMExtentsPerPage * mExtentPageCount;
        i ++;
    }
}

void FreeSpaceManager::SavePFS(IntegerArray *pages)
{
    if (pages->mItemCount != mPFSPageMap->mItemCount)
        //SavePFS - pages.ItemCount <> PFSPageMap.ItemCount!
        assert(false);
    FFPage buf;
    buf.pageHeader.pageType = PFS_PAGE;
    buf.pageHeader.encType = 0;
    buf.pageHeader.crcType = 0;
    for (int i=0; i<pages->mItemCount; i++)
    {
        if (pages->mItems[i] != 1)
            continue;
        int curPage = mPFSPageMap->mItems[i];
        buf.pData = mPFS + i * mPFSPageSize;
        if (!mPMHandle->WritePage(&buf, curPage, mPFSPageSize))
            //SavePFS - can not save page
            assert(false);
    }
}

void FreeSpaceManager::SaveGAM(IntegerArray *pages)
{
    if (pages->mItemCount != mGAMPageMap->mItemCount)
        //SaveGAM - pages.ItemCount != GAMPageMap.ItemCount!
        assert(false);
    FFPage buf;
    buf.pageHeader.encType = 0;
    buf.pageHeader.crcType = 0;
    for (int i=0; i<pages->mItemCount; i++)
    {
        if (pages->mItems[i] != 1)
            continue;
        int curPage = mGAMPageMap->mItems[i];
        buf.pageHeader.pageType = GAM_PAGE;
        buf.pData = mGAM + i * mGAMPageSize;
        if (!mPMHandle->WritePage(&buf, curPage, mGAMPageSize))
            //SaveGAM - can not save page
            assert(false);
        buf.pData = mSGAM + i * mGAMPageSize;
        buf.pageHeader.pageType = SGAM_PAGE;
        if (!mPMHandle->WritePage(&buf, curPage + 1, mGAMPageSize))
            //SaveGAM - can not save SGAM page
            assert(false);
    }
}

int FreeSpaceManager::GetGAMPageNo(int extentNo)
{
    return extentNo / mGAMExtentsPerPage;
}

int FreeSpaceManager::GetPFSPageNo(int extentNo)
{
    return extentNo / mPFSExtentsPerPage;
}

FreeSpaceManager::FreeSpaceManager(PageFileManager *pageFileManager)
{
    mPMHandle = pageFileManager;
    mPageSize = mPMHandle->GetPageDataSize();
    mExtentPageCount = mPMHandle->mHeader.extentPageCount;
    // total number of pages
    mPageCount = mPMHandle->mHeader.totalPageCount - mPMHandle->mHeader.hdrPageCount;
    // total number of extents in file
    // extents does not contain GAM,SGAM,PFS,HDR pages
    mExtentCount = (mPMHandle->mHeader.totalPageCount - mPMHandle->mHeader.hdrPageCount) / mExtentPageCount;
    mPFSPageSize = (mPageSize * 8 - (mPageSize * 8) % mExtentPageCount) / 8;
    mPFSExtentsPerPage = mPFSPageSize * 8 / mExtentPageCount;
    mGAMPageSize = (mPageSize * 8 - (mPageSize * 8) % mPFSExtentsPerPage) / 8;
    mGAMExtentsPerPage = mGAMPageSize * 8;

 	// check some data
    if (mPMHandle->mHeader.pfsPageCount * mPFSExtentsPerPage < mExtentCount)
        //PFSPageCount * FPageSize < FExtentCount
        assert(false);
    if (mPMHandle->mHeader.gamPageCount * mGAMExtentsPerPage < mExtentCount)
        //GAMPageCount * FPageSize < FExtentCount
        assert(false);
    mPFSPageMap = new IntegerArray(0, 10, 10);
    mGAMPageMap = new IntegerArray(0, 10, 10);

    LoadPFS();
    LoadGAM();
    mMixedExtentCount = 0;
    mFreeExtentCount = 0;
    int segment = 0;
    int offset = 0;
    unsigned char x = 1;
    while ((segment * 8 + offset) < mExtentCount)
    {
        if ((*(unsigned char *)(mGAM + segment) & x) != 0)
            mFreeExtentCount ++;
        if ((*(unsigned char *)(mSGAM + segment) & x) != 0)
            mMixedExtentCount ++;
        offset ++;
        if (offset == 8)
        {
            segment ++;
            offset = 0;
            x = 1;
        }
        else
            x = x << 1;
    }
}

FreeSpaceManager::~FreeSpaceManager()
{
    delete mPFSPageMap;
    delete mGAMPageMap;
    if (mPMHandle->mHeader.pfsPageCount > 0)
        free(mPFS);
    if (mPMHandle->mHeader.gamPageCount > 0)
        free(mGAM);
    if (mPMHandle->mHeader.gamPageCount > 0)
        free(mSGAM);
}

void FreeSpaceManager::UseExtent(int curExtent, IntegerArray *pages, int pageCount, IntegerArray *pfsPages, IntegerArray *gamPages, bool uniform)
{
    int curPage = curExtent * mExtentPageCount;
    int k = curPage;
    while ((k < curPage + mExtentPageCount) && (pages->mItemCount < pageCount))
    {
        if (GetBit(mPFS, k))
        {
            pfsPages->mItems[GetPFSPageNo(curExtent)] = 1;
            SetBit(mPFS, k, false);
            pages->Append(k + mPMHandle->mHeader.hdrPageCount);
            // extent was free
            if (k == 0)
            {
                mFreeExtentCount --;
                if (uniform)
                {
                    // mark Current extent as uniform
                    SetBit(mGAM, curExtent, false);
                    SetBit(mSGAM, curExtent, false);
                }
                else
                {
                    // mark Current extent as mixed
                    SetBit(mGAM, curExtent, false);
                    SetBit(mSGAM, curExtent, true);
                    mMixedExtentCount ++;
                }
                gamPages->mItems[GetGAMPageNo(curExtent)] = 1;
            }
            // extent full
            if (k == curPage + mExtentPageCount - 1)
            {
                if (GetBit(mSGAM, curExtent))
                    mMixedExtentCount --;
                // mark Current extent as full
                SetBit(mGAM, curExtent, false);
                SetBit(mSGAM, curExtent, false);
                gamPages->mItems[GetGAMPageNo(curExtent)] = 1;
            }
        }
        k ++;
    }
}

void FreeSpaceManager::AddExtents(int &oldPages, int &newPages, int &newExtents, int restPages, IntegerArray *pfsPages, IntegerArray *gamPages, bool uniform)
{
    oldPages = mPMHandle->mHeader.totalPageCount - mPMHandle->mHeader.hdrPageCount;
    int oldExtents = mExtentCount;
    newPages = 0;
    newExtents = 0;
    while (newPages < restPages)
    {
        bool pfs = false;
        bool gam = false;
        if (newExtents + oldExtents == mPFSExtentsPerPage * mPMHandle->mHeader.pfsPageCount)
            pfs = true;
        if (newExtents + oldExtents == mGAMExtentsPerPage * mPMHandle->mHeader.gamPageCount)
            gam = true;
        if (gam || pfs)
        {
            mPMHandle->mHeader.pfsPageCount ++;
            mPFS = (char *)realloc(mPFS, mPMHandle->mHeader.pfsPageCount * mPFSPageSize);
            // mark all pages in new pfs page as free
            memset(mPFS + (mPMHandle->mHeader.pfsPageCount - 1) * mGAMPageSize, 0xff, mGAMPageSize);
    
            int i;
            if (mPFSPageMap->mItemCount > 0)
                i = mPFSPageMap->mItems[mPFSPageMap->mItemCount - 1] + mPFSExtentsPerPage * mExtentPageCount;
            else
                // first PFS page number = 1
                i = 1;
            mPFSPageMap->Append(i);
            // new allocated page will be saved
            pfsPages->Append(1);
            // mark new pfs page as full
            i = (oldExtents + newExtents) * mExtentPageCount;
            SetBit(mPFS, i, false);
            if (gam)
            {
                mPMHandle->mHeader.gamPageCount ++;
                mGAM = (char *)realloc(mGAM, mPMHandle->mHeader.gamPageCount * mGAMPageSize);
                mSGAM = (char *)realloc(mSGAM, mPMHandle->mHeader.gamPageCount * mGAMPageSize);
                // mark all extents in new GAM/SGAM page as free extents
                memset(mGAM + (mPMHandle->mHeader.gamPageCount - 1) * mGAMPageSize, 0xff, mGAMPageSize);
                memset(mSGAM + (mPMHandle->mHeader.gamPageCount - 1) * mGAMPageSize, 0x00, mGAMPageSize);
                // mark new extent as mixed
                SetBit(mGAM, oldExtents+newExtents, false);
                SetBit(mSGAM, oldExtents+newExtents, true);
                // mark new GAM page as full
                SetBit(mPFS, i + 1, false);
                // mark new SGAM page as full
                SetBit(mPFS, i + 2, false);
                if (mGAMPageMap->mItemCount > 0)
                    i = mGAMPageMap->mItems[mGAMPageMap->mItemCount - 1] + mGAMExtentsPerPage * mExtentPageCount;
                else
                    // first GAM page number = 2
                    i = 2;
                mGAMPageMap->Append(i);
                // new allocated page will be saved
                gamPages->Append(1);
                if (!uniform)
                    newPages += mExtentPageCount - 3;
            }
            else
                if (!uniform)
                    newPages += mExtentPageCount - 1;
            mMixedExtentCount ++;
        }
        else
        {
            // add pages of new free extent
            newPages += mExtentPageCount;
            mFreeExtentCount ++;
        }
        newExtents ++;
    }
    mExtentCount = oldExtents + newExtents;
}

bool FreeSpaceManager::GetPages(int pageCount, int startPageNo, bool uniform, IntegerArray *pages)
{
    int newPages;
    int newExtents;
    
    bool result = true;
    pages->SetSize(0);
    IntegerArray *pfsPages = new IntegerArray(mPFSPageMap->mItemCount, 1, 10);
    IntegerArray *gamPages = new IntegerArray(mGAMPageMap->mItemCount, 1, 10);
    // clear save pages flags
    for (int i=0; i<pfsPages->mItemCount; i++)
        pfsPages->mItems[i] = 0;
    for (int i=0; i<gamPages->mItemCount; i++)
        gamPages->mItems[i] = 0;
    // check Current extent for free pages
    int curPage = startPageNo - mPMHandle->mHeader.hdrPageCount;
    int curExtent = curPage / mExtentPageCount;
    //if not existing page is specified
    if (curExtent < mExtentCount)
        UseExtent(curExtent, pages, pageCount, pfsPages, gamPages, uniform);
    else
        curExtent = 0;

    // try to use existing free pages
    if ((uniform && (mFreeExtentCount > 0)) || ((!uniform) && ((mMixedExtentCount > 0) || (mFreeExtentCount > 0))))
    {
        curExtent = 0;
        while ((curExtent < mExtentCount) && (pages->mItemCount < pageCount))
        {
            // if extent is free or mixed try to use it
            if ((GetBit(mGAM, curExtent) || (GetBit(mSGAM, curExtent) && (!uniform))))
                UseExtent(curExtent, pages, pageCount, pfsPages, gamPages, uniform);
            curExtent ++;
        }
    }
 
    // if some pages must be added
    if (pages->mItemCount < pageCount)
    {
        // calculate rest number of extents
        // these extents will be added as free to the file
        int restPages = pageCount - pages->mItemCount;
        // add extents to the file
        int oldPages;
        AddExtents(oldPages, newPages, newExtents, restPages, pfsPages, gamPages, uniform);
        if (newExtents <= 0)
            // AddExtents does not append any extents.
            assert(false);
        result = mPMHandle->AppendPages(newExtents * mExtentPageCount);
        if (result)
        {
            while ((curExtent < mExtentCount) && (pages->mItemCount < pageCount))
            {
                // if extent is free or mixed try to use it
                if ((GetBit(mGAM, curExtent) || (GetBit(mSGAM, curExtent) && (!uniform))))
                    UseExtent(curExtent, pages, pageCount, pfsPages, gamPages, uniform);
                curExtent ++;
            }
            if (pages->mItemCount < pageCount)
                //AddExtents appends too small pages.
                assert(false);
        }
    }

    if (result)
    {
        // try to extend file
        int k = 0;
        for (int i=0; i<pages->mItemCount; i++)
            if (pages->mItems[i] > k)
                k = pages->mItems[i];

        if (k > mPMHandle->mHeader.totalPageCount - 1)
            result = mPMHandle->AppendPages(k - mPMHandle->mHeader.totalPageCount + 1);
        if (result)
        {
            SaveGAM(gamPages);
            SavePFS(pfsPages);
        }
    }
    delete gamPages;
    delete pfsPages;
    
    return result;
}

void FreeSpaceManager::FreeExtent(int curExtent, IntegerArray *gamPages)
{
    int k = curExtent * mExtentPageCount;
    int curPage = k;
    bool bFree = true;
    while (k < curPage + mExtentPageCount)
    {
        if (!GetBit(mPFS, k))
        {
            bFree = false;
            break;
        }
        k ++;
    }
    if (bFree)
    {
        if (GetBit(mSGAM, curExtent))
            mMixedExtentCount --;
        // mark Current extent as uniform
        SetBit(mGAM, curExtent, true);
        SetBit(mSGAM, curExtent, false);
        gamPages->mItems[GetGAMPageNo(curExtent)] = 1;
        mFreeExtentCount ++;
    }
}

void FreeSpaceManager::FreePages(IntegerArray *pages)
{
    // this is an array of extent flags
    // each extent flag = 1 if any page belonging this extent was marked as free
    IntegerArray *freeExtents = new IntegerArray(mExtentCount, 1, 10);
    // clear used extents flags
    for (int i=0; i<freeExtents->mItemCount; i++)
        freeExtents->mItems[i] = 0;
    IntegerArray *pfsPages = new IntegerArray(mPFSPageMap->mItemCount, 1, 10);
    IntegerArray *gamPages = new IntegerArray(mGAMPageMap->mItemCount, 1, 10);
    // clear save pages flags
    for (int i=0; i<pfsPages->mItemCount; i++)
        pfsPages->mItems[i] = 0;
    for (int i=0; i<gamPages->mItemCount; i++)
        gamPages->mItems[i] = 0;
    
    int curPage;
    int curExtent;
    // free pages
    for (int i=0; i<pages->mItemCount; i++)
    {
        curPage = pages->mItems[i];
        // if this page does not exists
        if (curPage >= mPMHandle->mHeader.totalPageCount)
            continue;
        // dec not addressed page count
        curPage -= mPMHandle->mHeader.hdrPageCount;
        // mark current page as free
        SetBit(mPFS, curPage, true);
        // set extent flag
        curExtent = curPage / mExtentPageCount;
        freeExtents->mItems[curExtent] = 1;
        // mark PFS page for saving
        pfsPages->mItems[GetPFSPageNo(curExtent)] = 1;
    }
    // free extents
    for (int i=0; i<freeExtents->mItemCount; i++)
    {
        if (freeExtents->mItems[i] != 1)
            continue;
        curExtent = i;
        FreeExtent(curPage, gamPages);
    }
    SaveGAM(gamPages);
    SavePFS(pfsPages);
    delete gamPages;
    delete pfsPages;
    delete freeExtents;
}

__s64 FreeSpaceManager::GetFreePageCount()
{
    return mFreeExtentCount * mExtentPageCount * mPageSize;
}

