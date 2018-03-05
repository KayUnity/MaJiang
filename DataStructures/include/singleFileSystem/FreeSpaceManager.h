#ifndef __FREE_SPACE_MANAGER_H
#define __FREE_SPACE_MANAGER_H

#include "SingleFileSystemStructures.h"
#include "HexString.h"

namespace DataStructures
{
    class PageFileManager;
    class IntegerArray;
    
    // free space manager
    class FreeSpaceManager
    {
    public:
        PageFileManager *mPMHandle; // page manager
    private:
        char *mPFS; // Page Free Space bits (all pages)
        char *mGAM; // Global Allocation Map bits (all pages)
        char *mSGAM; // Shared Global Allocation Map bits (all pages)
        IntegerArray *mPFSPageMap; // PFS page map
        IntegerArray *mGAMPageMap; // GAM page map
        int mPageSize;  // double page size (FHeader.PageSize - sizeof(TPageHeader)
        int mGAMPageSize; // GAM page size in bytes (only used by bitmap)
        int mGAMExtentsPerPage; // numer of extents per 1 page (maximum)
        int mPFSExtentsPerPage; // numer of extents per 1 page (maximum)
        int mPFSPageSize; // PFS page size in bytes (only used by bitmap)
        int mExtentPageCount; // number of pages per one extent
        int mFreeExtentCount; // number of free extents
        int mMixedExtentCount; // number of mixed extents
        int mExtentCount; // number of existing extents
        int mPageCount; // number of pages in PFS
    private:
        // load procedures
        void LoadPFS();
        void LoadGAM();
        // save PFS pages
        void SavePFS(IntegerArray *pages);
        // save GAM pages
        void SaveGAM(IntegerArray *pages);
        // returns number of GAM page
        int GetGAMPageNo(int extentNo);
        // returns number of PFS page
        int GetPFSPageNo(int extentNo);
    public:
        // constructor
        FreeSpaceManager(PageFileManager *pageFileManager);
        // destructor
        virtual ~FreeSpaceManager();
        // get pages sequence
        bool GetPages(int pageCount, int startPageNo, bool uniform, IntegerArray *pages);
        // free pages sequence
        void FreePages(IntegerArray *pages);
        // returns free page count
        __s64 GetFreePageCount();
    private:
        void UseExtent(int curExtent, IntegerArray *pages, int pageCount, IntegerArray *pfsPages, IntegerArray *gamPages, bool uniform);
        void AddExtents(int &oldPages, int &newPages, int &newExtents, int restPages, IntegerArray *pfsPages, IntegerArray *gamPages, bool uniform);
        void FreeExtent(int curExtent, IntegerArray *gamPages);
    };

}

#endif