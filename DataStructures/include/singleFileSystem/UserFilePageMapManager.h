#ifndef __USER_FILE_PAGE_MAP_MANAGER_H
#define __USER_FILE_PAGE_MAP_MANAGER_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "singleFileSystem/SFSArray.h"
#include "HexString.h"

namespace DataStructures
{
    class PageFileManager;
    class FreeSpaceManager;
    
    // User Files Page Maps
    class UserFilePageMapManager
    {
    public:
        PageFileManager *mPFMHandle; // page manager
        int mLastError; // error No
    private:
        FreeSpaceManager *mPSMHandle; // free space manager
        SortedPtrArray *mUFPMMaps;   // pointers to UFPM map
        SortedPtrArray *mUFMaps;   // pointers to user files maps
        int mPagesPerMapPage;           // how many user file pages are addressed by one map page
        IntegerArray *mTempPages;         // internal temp array
        
        // get quantity of pages covering specified size
        int GetCoverPageCount(__s64 size);
        // get file maps - find or load (file is indentified by FirstMapPageNo)
        void GetMaps(DirectoryElement *fileRec, IntegerArray **ufpmMap, IntegerArray **ufMap);
        // append pages to the end of file (file is indentified by FirstMapPageNo)
        bool AppendPages(DirectoryElement *fileRec, int pageCount, bool writeAppendedPages);
        // delete pages from the end of file (file is indentified by FirstMapPageNo)
        void DeletePagesFromEOF(DirectoryElement *fileRec, int pageCount);
        // save UFPM pages
        void SaveMapPages(int itemNo, int itemCount, IntegerArray *ufpmMap, IntegerArray *ufMap);
        
    public:
        // constructor
        UserFilePageMapManager(PageFileManager *pfmHandle1, FreeSpaceManager *fsmHandle1);
        // destructor
        virtual ~UserFilePageMapManager();
        
        // get pages from file (FirstMapPageNo) starting from Offset to cover Size
        // can allocate additional pages, returns list of FF pages
        // FirstMapPageNo=-1 corresponds to the new created file
        bool GetPages(DirectoryElement *fileRec, __s64 offset, __s64 size, bool isAllocateAllowed, IntegerArray *pages);
        // set size of file (FirstMapPageNo)
        // FirstMapPageNo=-1 corresponds to the new created file
        bool SetSize(DirectoryElement *fileRec, __s64 newSize, bool writeAppendedPages = true);
    };

}

#endif