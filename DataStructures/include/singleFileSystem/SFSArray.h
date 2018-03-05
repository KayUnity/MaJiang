#ifndef __SFS_ARRAY_H
#define __SFS_ARRAY_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "HexString.h"
#include "fileUtils/_FindFirst.h"

namespace DataStructures
{
    class SFSHeadersArray
    {
    private:
        int mAllocBy;
        int mDeAllocBy;
        int mMaxAllocBy;
        int mAllocItemCount;
    public:
        SFSHeader *mItems;
        __s64 *mPositions; // block positions
        int mItemCount; // all files quantity (including deleted files)
    public:
        SFSHeadersArray();
        virtual ~SFSHeadersArray();
        
        void SetSize(int newSize);
        void AppendItem(SFSHeader &value, __s64 pos);
        int FindPosition(__s64 pos);
    };

    class IntegerArray
    {
    public:
        int *mItems;
        int mItemCount;
        int mCurrentItem; // used by findnext
        int mAllocBy;
        int mDeAllocBy;
        int mMaxAllocBy;
        int mAllocItemCount;
        
        IntegerArray(int size = 0, int defaultAllocBy = 1000, int defaultMaxAllocBy = 10000);
        virtual ~IntegerArray();
        
        void SetSize(int newSize);
        void Append(int value);
        void Insert(int itemNo, int value);
        void Delete(int itemNo);
        void MoveTo(int itemNo, int newItemNo);
        void CopyTo(int *ar, int itemNo, int count);
    };

    class SortedPtrArray
    {
    private:
        int mUniqueKeyValue;
        int mAllocBy;
        int mDeAllocBy;
        int mMaxAllocBy;
        int mAllocItemCount;
        
        int FindPositionForInsert(int key);
        int FindPosition(int key);
        void InsertByPosition(int itemNo, int key, void *value);
        void DeleteByPosition(int itemNo);
        
    public:
        int *mKeyItems;
        void **mValueItems;
        int mItemCount;
        
        SortedPtrArray(int size = 0, int defaultAllocBy = 1000, int defaultMaxAllocBy = 10000);
        virtual ~SortedPtrArray();
        
        void SetSize(int newSize);
        void *Find(int key);
        void Insert(int key, void *value);
        void Delete(int key);
        int GetNextKeyValue();
    };

    class DirArray
    {
    private:
        int mAllocBy;
        int mMaxAllocBy;
        int mAllocItemCount;

        int __FindFile(const char *fileName, int slen, int currentDir, int startSymbol = 0);
    public:
        IntegerArray **mFoundItems;
        DirectoryElement *mItems;
        IntegerArray *mNameIndex;
        IntegerArray *mParentIndex;
        int mFoundItemCount; // number of foundItems arrays
        int mItemCount; // all files quantity (including deleted files)
        int mIndexElementsCount; // double files qunatity (excluding deleted)
        
        void BuildIndexes();
    public:
        DirArray(int size = 0, int defaultAllocBy = 2, int defaultMaxAllocBy = 200);
        virtual ~DirArray();
        void SetSize(int newSize);
        int FindPositionForInsert(DirectoryElement &value, bool byName = true, bool first = false, bool insert = true);
        // Find file by name (with path)
        // returns -1 if root directory found
        // returns <-1 if file not found
        // >= 0 - directory element number if fili found
        int FindFileByName(const char *fileName, int startDir = ROOT_ID);
        // returns full file path (form root, '/folder1/folder2')
        HexString GetFullFilePath(int itemNo);
        // prepares searchRec
        void PrepareSearchRecord(int itemNo, SearchRec &f);
        // find file by pattern using '*', '?'
        // returns 0 if file was found, otherwise returns error code
        int FindFirst(const char *path, int attr, SearchRec &f, int currentDir = ROOT_ID);
        int FindNext(SearchRec &f);
        void FindClose(SearchRec &f);
        // edit / read items
        void AppendItem(DirectoryElement &value);
        void UpdateItem(DirectoryElement &value, int position);
        DirectoryElement &ReadItem(int position);
    };

}
#endif