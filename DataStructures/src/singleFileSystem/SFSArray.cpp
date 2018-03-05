#include "singleFileSystem/SFSArray.h"
#include "LinuxStrings.h"
#include "singleFileSystem/SFSUtils.h"

using namespace DataStructures;

//--------------------------------------- SFSHeadersArray ---------------------------------------
SFSHeadersArray::SFSHeadersArray()
{
    mAllocBy = 100; // default alloc
    mDeAllocBy = 100; // default alloc
    mMaxAllocBy = 10000; // max alloc
    mAllocItemCount = 0;
    mItemCount = 0;
    mItems = 0;
    mPositions = 0;
	SetSize(0);
}

SFSHeadersArray::~SFSHeadersArray()
{
    SetSize(0);
}

void SFSHeadersArray::SetSize(int newSize)
{
    if (newSize == 0)
    {
        mItemCount = 0;
        mAllocItemCount = 0;
        free(mItems);
        mItems = 0;
        free(mPositions);
        mPositions = 0;
        return;
    }
    
    if (newSize > mAllocItemCount)
    {
        mAllocBy = mAllocBy * 2;
        if (mAllocBy > mMaxAllocBy)
            mAllocBy = mMaxAllocBy;
        if (mAllocItemCount + mAllocBy > newSize)
            mAllocItemCount = mAllocItemCount + mAllocBy;
        else
            mAllocItemCount = newSize;
        mItems = (SFSHeader *)realloc(mItems, sizeof(SFSHeader) * mAllocItemCount);
        mPositions = (__s64 *)realloc(mPositions, sizeof(__s64) * mAllocItemCount);
    }
    else
        if (newSize < mItemCount)
            if (mAllocItemCount - newSize > mDeAllocBy)
            {
                mDeAllocBy = mDeAllocBy * 2;
                if (mDeAllocBy > mMaxAllocBy)
                    mDeAllocBy = mMaxAllocBy;
                mItems = (SFSHeader *)realloc(mItems, sizeof(SFSHeader) * newSize);
                mPositions = (__s64 *)realloc(mPositions, sizeof(__s64) * newSize);
                mAllocItemCount = newSize;
            }
    mItemCount = newSize;
}

inline int __compareS64(__s64 a, __s64 b)
{
    if (a == b)
        return 0;
    if (a < b)
        return 1;
    else
        return -1;
}

int SFSHeadersArray::FindPosition(__s64 pos)
{
    int i = mItemCount >> 1;
    int dx = i;
    int result = 0;
    if (mItemCount <= 0)
        return 0;
    int f = 0;
    int res = 2;
    while (true)
    {
        dx = dx >> 1;
        if (dx < 1)
            dx = 1;
        int oldRes = res;
        // compare, ascending
        res = __compareS64(mPositions[i], pos);
        if (res < 0)
        {
            //  element, specified by value should be higher then current element (+->0)
            i = i - dx;
        }
        else
            if (res > 0)
            {
                //  element, specified by value should be lower then current element (+->0)
                i = i + dx;
            }
            else
            {
                // values are equal
                result = i;
                break;
            }
        if ((i < 0) && (dx == 1))
        {
            // equal not found
            result = 0;
            break;
        }
        if ((i > mItemCount - 1) && (dx == 1))
        {
            // equal not found
            result = mItemCount;
            break;
        }
        
        if (i > mItemCount - 1)
            i = mItemCount - 1;
        if  (i < 0)
            i = 0;
        
        if ((dx == 1) && (f > 1))
        {
            // dx minimum
            // compare, ascending
            res = __compareS64(mPositions[i], pos);
            if ((res < 0) && (oldRes > 0))
                result = i;
            if ((res > 0) && (oldRes < 0))
                result = i + 1;
            if (res == oldRes)
                continue;
            break;
        }// last step
        if ((res != oldRes) && (dx == 1) && (oldRes != 2))
            f ++;
    }//while dx
    if (result >= mItemCount)
        result = mItemCount - 1;
    if (result > 0)
        if (mPositions[result] > pos)
            result --;
    if (result < 0)
        result = 0;
    return result;
}

void SFSHeadersArray::AppendItem(SFSHeader &value, __s64 pos)
{
    mItemCount ++;
    SetSize(mItemCount);
    mItems[mItemCount - 1] = value;
    mPositions[mItemCount - 1] = pos;
}

//--------------------------------------- IntegerArray ---------------------------------------
IntegerArray::IntegerArray(int size, int defaultAllocBy, int defaultMaxAllocBy)
{
    mAllocBy = defaultAllocBy; // default alloc
    mDeAllocBy = defaultAllocBy; // default dealloc
    mMaxAllocBy = defaultMaxAllocBy; // max alloc
    mAllocItemCount = 0;
    mItemCount = 0;
    mItems = 0;
	SetSize(size);
}

IntegerArray::~IntegerArray()
{
    if (mItems)
        free(mItems);
}

//------------------------------------------------------------------------------
// Set length of array to specified size
//------------------------------------------------------------------------------
void IntegerArray::SetSize(int newSize)
{
    mItemCount = newSize;
    if (mItemCount > 0)
        mItems = (int *)realloc(mItems, sizeof(int) * mItemCount);
    else
    {
        if (mItems)
            free(mItems);
        mItems = 0;
        return;
    }
    if (newSize == 0)
    {
        mItemCount = 0;
        mAllocItemCount = 0;
        if (mItems)
            free(mItems);
        mItems = 0;
        return;
    }
    if (newSize > mAllocItemCount)
    {
        mAllocBy = mAllocBy * 2;
        if (mAllocBy > mMaxAllocBy)
            mAllocBy = mMaxAllocBy;
        if (mAllocItemCount + mAllocBy > newSize)
            mAllocItemCount = mAllocItemCount + mAllocBy;
        else
            mAllocItemCount = newSize;
        mItems = (int *)realloc(mItems, sizeof(int) * mAllocItemCount);
    }
    else
        if (newSize < mItemCount)
            if (mAllocItemCount - newSize > mDeAllocBy)
            {
                mDeAllocBy = mDeAllocBy * 2;
                if (mDeAllocBy > mMaxAllocBy)
                    mDeAllocBy = mMaxAllocBy;
                mItems = (int *)realloc(mItems, sizeof(int) * newSize);
                mAllocItemCount = newSize;
            }
    
    mItemCount = newSize;
}

void IntegerArray::Append(int value)
{
    SetSize(mItemCount + 1);
    mItems[mItemCount - 1] = value;
}

void IntegerArray::Insert(int itemNo, int value)
{
    mItemCount ++;
    SetSize(mItemCount);
    
    if (mItemCount <= 1)
        mItems[0] = value;
    else
    {
        if (itemNo >= mItemCount - 1)
            mItems[mItemCount - 1] = value;
        else
        {
            memmove(&mItems[itemNo + 1], &mItems[itemNo], (mItemCount - itemNo - 1) * sizeof(int));
            mItems[itemNo] = value;
        }
    }
}

void IntegerArray::Delete(int itemNo)
{
 if (itemNo < mItemCount-1)
     memmove(&mItems[itemNo], &mItems[itemNo + 1], (mItemCount - itemNo - 1) * sizeof(int));
    mItemCount --;
    SetSize(mItemCount);
}

void IntegerArray::MoveTo(int itemNo, int newItemNo)
{
    if (itemNo == newItemNo)
        return;
    //if ((itemNo - newItemNo == 1) || (newItemNo - itemNo == 1))
	else
    {
        int value = mItems[itemNo];
        mItems[itemNo] = mItems[newItemNo];
        mItems[newItemNo] = value;
        return;
    }
    //if (itemNo > newItemNo)
    //{
    //    int value = mItems[itemNo];
    //    memmove(&mItems[newItemNo + 1], &mItems[newItemNo], (itemNo - newItemNo) * sizeof(int));
    //    mItems[newItemNo] = value;
    //}
    //else
    //{
    //    int value = mItems[itemNo];
    //    memmove(&mItems[newItemNo + 1], &mItems[newItemNo], (newItemNo - itemNo - 1) * sizeof(int));
    //    mItems[newItemNo - 1] = value;
    //}
}

void IntegerArray::CopyTo(int *ar, int itemNo, int count)
{
    if (mItemCount > 0)
        memmove(ar, &mItems[itemNo], sizeof(int) * count);
}

//--------------------------------------- SortedPtrArray ---------------------------------------
SortedPtrArray::SortedPtrArray(int size, int defaultAllocBy, int defaultMaxAllocBy)
{
    mUniqueKeyValue = -1;
    mAllocBy = defaultAllocBy; // default alloc
    mDeAllocBy = defaultAllocBy; // default dealloc
    mMaxAllocBy = defaultMaxAllocBy; // max alloc
    mAllocItemCount = 0;
    mKeyItems = 0;
    mValueItems = 0;
    mItemCount = 0;
	SetSize(size);
}

SortedPtrArray::~SortedPtrArray()
{
    if (mKeyItems)
        free(mKeyItems);
    if (mValueItems)
        free(mValueItems);
}

void SortedPtrArray::SetSize(int newSize)
{
    if (newSize == 0)
    {
        mItemCount = 0;
        mAllocItemCount = 0;
        if (mKeyItems)
            free(mKeyItems);
        if (mValueItems)
            free(mValueItems);
        mKeyItems = 0;
        mValueItems = 0;
        return;
    }
    if (newSize > mAllocItemCount)
    {
        mAllocBy = mAllocBy * 2;
        if (mAllocBy > mMaxAllocBy)
            mAllocBy = mMaxAllocBy;
        if (mAllocItemCount + mAllocBy > newSize)
            mAllocItemCount = mAllocItemCount + mAllocBy;
        else
            mAllocItemCount = newSize;
        mKeyItems = (int *)realloc(mKeyItems, mAllocItemCount * sizeof(int));
        mValueItems = (void **)realloc(mValueItems, sizeof(void *) * mAllocItemCount);
    }
    else
        if (newSize < mItemCount)
            if (mAllocItemCount - newSize > mDeAllocBy)
            {
                mDeAllocBy = mDeAllocBy * 2;
                if (mDeAllocBy > mMaxAllocBy)
                    mDeAllocBy = mMaxAllocBy;
                mKeyItems = (int *)realloc(mKeyItems, newSize * sizeof(int));
                mValueItems = (void **)realloc(mValueItems, sizeof(void *) * newSize);
                mAllocItemCount = newSize;
            }
    
    mItemCount = newSize;
}

int SortedPtrArray::FindPositionForInsert(int key)
{
    if (mItemCount <= 0)
        return 0;
    
    int result = 0;
    int i = mItemCount >> 1;
    int dx = i;
    result = mItemCount;
    if (mItemCount > 0)
    {
        int f = 0;
        int res = 2;
        while (true)
        {
            dx = dx >> 1;
            if (dx < 1)
                dx = 1;
            int oldRes = res;
            // compare, ascending
            if (mKeyItems[i] == key)
                res = 0;
            else
                if (mKeyItems[i] < key)
                    res = 1;
                else
                    res = -1;
            if (res < 0)
            {
                //  element, specified by value should be higher then current element (+->0)
                i = i - dx;
            }
            else
                if (res > 0)
                {
                    //  element, specified by value should be lower then current element (+->0)
                    i = i + dx;
                }
                else // values are equal
                {
                    result = i;
                    break;
                }
            if ((i < 0) && (dx == 1))
            {
                result = 0;
                break;
            }
            if ((i > mItemCount - 1) && (dx == 1))
            {
                result = mItemCount;
                break;
            }
            
            if  (i > mItemCount - 1)
                i = mItemCount - 1;
            if (i < 0)
                i = 0;
            
            if ((dx == 1) && (f > 1))
            {
                // dx minimum
                // compare, ascending
                if (mKeyItems[i] == key)
                    res = 0;
                else
                    if (mKeyItems[i] < key)
                        res = 1;
                    else
                        res = -1;
                
                if ((res < 0) && (oldRes > 0))
                    result = i;
                if ((res > 0) && (oldRes < 0))
                    result = i + 1;
                if (res == oldRes)
                    continue;
                break;
            }// last step
            if ((res != oldRes) && (dx == 1) && (oldRes != 2))
                f ++;
        }   //while dx
    }   // if itemCount > 0
    return result;
}


int SortedPtrArray::FindPosition(int key)
{
    int result = FindPositionForInsert(key);
    if ((result >= mItemCount) || (result < 0))
        result = -1;
    else
        if (mKeyItems[result] != key)
            result = -1;
    return result;
}

void *SortedPtrArray::Find(int key)
{
    int pos = FindPositionForInsert(key);
    if ((pos >= mItemCount) || (pos < 0))
        return 0;
    else
        if (mKeyItems[pos] != key)
            return 0;
        else
            return mValueItems[pos];
}

void SortedPtrArray::InsertByPosition(int itemNo, int key, void *value)
{
    mItemCount ++;
    SetSize(mItemCount);
    
    if (mItemCount <= 1)
    {
        mKeyItems[0] = key;
        mValueItems[0] = value;
    }
    else
        if (itemNo >= mItemCount - 1)
        {
            mKeyItems[mItemCount - 1] = key;
            mValueItems[mItemCount - 1] = value;
        }
        else
        {
            memmove(&mKeyItems[itemNo + 1], &mKeyItems[itemNo], (mItemCount - itemNo - 1) * sizeof(int));
            memmove(&mValueItems[itemNo + 1], &mValueItems[itemNo], (mItemCount - itemNo - 1) * sizeof(void *));
            mKeyItems[itemNo] = key;
            mValueItems[itemNo] = value;
        }
}

void SortedPtrArray::Insert(int key, void *value)
{
    if (mItemCount <= 0)
        InsertByPosition(0, key, value);
    else
        if (mItemCount == 1)
        {
            if (mKeyItems[0] <= key)
                InsertByPosition(1, key, value);
            else
                InsertByPosition(0, key, value);
        }
        else
        {
            int pos = FindPositionForInsert(key);
            InsertByPosition(pos, key, value);
        }
}

void SortedPtrArray::DeleteByPosition(int itemNo)
{
    if (itemNo < mItemCount - 1)
    {
        memmove(&mKeyItems[itemNo], &mKeyItems[itemNo+1], (mItemCount - itemNo - 1) * sizeof(int));
        memmove(&mValueItems[itemNo], &mValueItems[itemNo+1], (mItemCount - itemNo - 1) * sizeof(void *));
    }
    mItemCount --;
    SetSize(mItemCount);
}

void SortedPtrArray::Delete(int key)
{
    //no elements in array
    assert(mItemCount > 0);
    if (mItemCount == 1)
        DeleteByPosition(0);
    else
    {
        int pos = FindPosition(key);
        //element not found
        assert(pos >= 0);
        DeleteByPosition(pos);
    }
}

int SortedPtrArray::GetNextKeyValue()
{
    mUniqueKeyValue ++;
    return mUniqueKeyValue;
}

//--------------------------------------- DirArray ---------------------------------------
DirArray::DirArray(int size, int defaultAllocBy, int defaultMaxAllocBy)
{
    mAllocBy = defaultAllocBy; // default alloc
    mMaxAllocBy = defaultMaxAllocBy; // max alloc
    mAllocItemCount = 0;
    mNameIndex = new IntegerArray(0, 10, 100);
    mParentIndex = new IntegerArray(0, 10, 100);
    
    mFoundItemCount = 0;
    mItemCount = 0;
    mFoundItems = 0;
    mItems = 0;
    mIndexElementsCount = 0;
	SetSize(size);
}

DirArray::~DirArray()
{
    SetSize(0);
    mFoundItems = 0;
    mItems = 0;
    delete mNameIndex;
    delete mParentIndex;
}

void DirArray::SetSize(int newSize)
{
    if (newSize == 0)
    {
        for (int i=0; i<mFoundItemCount; i++)
            if (mFoundItems[i] != 0)
            {
                delete mFoundItems[i];
                mFoundItems[i] = 0;
            }
        if (mFoundItems)
            free(mFoundItems);
        mFoundItems = 0;
        mItemCount = 0;
        mFoundItemCount = 0;
        mIndexElementsCount = 0;
        mAllocItemCount = 0;
        if (mItems)
            free(mItems);
        mItems = 0;
        
        mNameIndex->SetSize(0);
        mParentIndex->SetSize(0);
        return;
    }
    
    if (newSize > mAllocItemCount)
    {
        int oldCount = mAllocItemCount;
        mAllocBy = mAllocBy * 2;
        if (mAllocBy > mMaxAllocBy)
            mAllocBy = mMaxAllocBy;
        if (mAllocItemCount + mAllocBy > newSize)
            mAllocItemCount = mAllocItemCount + mAllocBy;
        else
            mAllocItemCount = newSize;
        mItems = (DirectoryElement *)realloc(mItems, sizeof(DirectoryElement) * mAllocItemCount);
        for (int i=oldCount; i<mAllocItemCount; i++)
            memset(&mItems[i], 0, sizeof(DirectoryElement));
    }
    else
        if (newSize < mItemCount)
            //newSize < itemCount error.
            assert(false);
    
    mItemCount = newSize;
}

int __BuildCompare(DirectoryElement *items, IntegerArray *nameIndex, IntegerArray *parentIndex, int i, int j, bool byName)
{
    int result;
    if (byName)
        // by Name
        result = -_stricmp(items[nameIndex->mItems[i]].fileName, items[nameIndex->mItems[j]].fileName);
    else
    {
        // by parent
        if (items[parentIndex->mItems[i]].parentId == items[parentIndex->mItems[j]].parentId)
            result = 0;
        else
            if (items[parentIndex->mItems[i]].parentId < items[parentIndex->mItems[j]].parentId)
                result = 1;
            else
                result = -1;
    }
    return result;
}

void __QuickSort(DirectoryElement *items, IntegerArray *nameIndex, IntegerArray *parentIndex, int iLo, int iHi, bool byName)
{
    int lo = iLo;
    int hi = iHi;
    int mid = (lo + hi) / 2;
    while (lo <= hi)
    {
        while (__BuildCompare(items, nameIndex, parentIndex, lo, mid, byName) > 0)
            lo ++;
        while (__BuildCompare(items, nameIndex, parentIndex, hi, mid, byName) < 0)
            hi --;
        if (lo <= hi)
        {
            if (byName)
            {
                int t = nameIndex->mItems[lo];
                nameIndex->mItems[lo] = nameIndex->mItems[hi];
                nameIndex->mItems[hi] = t;
            }
            else
            {
                int t = parentIndex->mItems[lo];
                parentIndex->mItems[lo] = parentIndex->mItems[hi];
                parentIndex->mItems[hi] = t;
            }
            lo ++;
            hi --;
        }
    }
    if (hi > iLo)
        __QuickSort(items, nameIndex, parentIndex, iLo, hi, byName);
    if (lo < iHi)
        __QuickSort(items, nameIndex, parentIndex, lo, iHi, byName);
}

void DirArray::BuildIndexes()
{
    mNameIndex->SetSize(0);
    mParentIndex->SetSize(0);
    for (int k=0; k<mItemCount-1; k++)
        if (mItems[k].isDeleted == 0)
        {
            mNameIndex->Append(k);
            mParentIndex->Append(k);
        }
    if (mNameIndex->mItemCount <= 0)
        return;
    int k = mNameIndex->mItemCount - 1;
    bool byName = true;
    __QuickSort(mItems, mNameIndex, mParentIndex, 0, k, byName);
    // puzyrek
    for (int i=0; i<k+1; i++)
        for (int j=i; j<k+1; j++)
            if (__BuildCompare(mItems, mNameIndex, mParentIndex, i, j, byName) < 0)
            {
                int t = mNameIndex->mItems[i];
                mNameIndex->mItems[i] = mNameIndex->mItems[j];
                mNameIndex->mItems[j] = t;
            }
    byName = false;
    __QuickSort(mItems, mNameIndex, mParentIndex, 0, k, byName);
}

int __FindCompare(DirectoryElement *items, const char *str, IntegerArray *nameIndex, IntegerArray *parentIndex, int i, int parentId, bool byName)
{
    int result = 0;
    if (byName)
    {
        // by Name
        result = -_stricmp(items[nameIndex->mItems[i]].fileName, str);
        if (result > 0)
            result = 1;
        else
            if (result < 0)
                result = -1;
    }
    else
    {
        // by parent
        if (items[parentIndex->mItems[i]].parentId == parentId)
            result = 0;
        else
            if (items[parentIndex->mItems[i]].parentId < parentId)
                result = 1;
            else
                result = -1;
    }
    return result;
}

int DirArray::FindPositionForInsert(DirectoryElement &value, bool byName, bool first, bool insert)
{
    HexString str = value.fileName;
    int parentId = value.parentId;
    int i = mIndexElementsCount >> 1;
    int dx = i;
    int result = 0;
    if (mIndexElementsCount <= 0)
    {
        if (insert)
            result = 0;
        else
            result = -1;
        return result;
    }
    int f = 0;
    int res = 20000000;
    while (true)
    {
        dx = dx >> 1;
        if (dx < 1)
            dx = 1;
        int oldRes = res;
        // compare, ascending
        res = __FindCompare(mItems, str.C_String(), mNameIndex, mParentIndex, i, parentId, byName);
        if (res < 0)
        {
            //  element, specified by value should be higher then current element (+->0)
            i = i - dx;
        }
        else
            if (res > 0)
            {
                //  element, specified by value should be lower then current element (+->0)
                i = i + dx;
            }
            else
            {
                // values are equal
                result = i;
                break;
            }
        if ((i < 0) && (dx == 1))
        {
            // equal not found
            result = 0;
            break;
        }
        if ((i > mIndexElementsCount - 1) && (dx == 1))
        {
            // equal not found
            result = mIndexElementsCount;
            break;
        }
        
        if (i > mIndexElementsCount - 1)
            i = mIndexElementsCount - 1;
        if (i < 0)
            i = 0;
        
        if ((dx == 1) && (f > 1))
        {
            // dx minimum
            // compare, ascending
            res = __FindCompare(mItems, str.C_String(), mNameIndex, mParentIndex, i, parentId, byName);
            if ((res < 0) && (oldRes > 0))
                result = i;
            if ((res > 0) && (oldRes < 0))
                result = i + 1;
            if (res == oldRes)
                continue;
            break;
        }// last step
        //    if (sign(res) <> sign(oldRes)) and (dx = 1) and (oldRes <> 20000000) then
        if ((res != oldRes) && (dx == 1) && (oldRes != 20000000))
            f ++;
    }//while dx
    
    if ((result >= mIndexElementsCount) && (!insert))
        result = -1;
    if (result < 0)
    {
        if (insert)
            result = 0;
        else
            result = -1;
    }
    if ((!insert) && (result >= 0))
    {
        i = result;
        // compare, ascending
        res = __FindCompare(mItems, str.C_String(), mNameIndex, mParentIndex, i, parentId, byName);
        if (res != 0)
            result = -1;
    }
    
    if ((first && (!insert) && (result > 0) && (res == 0)))
    {
        // searching first equal value
        do
        {
            result --;
            i = result;
            res = __FindCompare(mItems, str.C_String(), mNameIndex, mParentIndex, i, parentId, byName);
            if (res != 0)
            {
                result ++;
                break;
            }
        } while (result > 0);
    }
    return result;
}

// recursive function for path finding
int DirArray::__FindFile(const char *fileName, int slen, int currentDir, int startSymbol)
{
    if (startSymbol >= slen)
        return currentDir;
    DirectoryElement el;
    int i = startSymbol;
    int curDir;
    while ((startSymbol < slen) && (fileName[startSymbol] != '/') && (fileName[startSymbol] != '\\'))
        startSymbol ++;
    if ((i == startSymbol) && (startSymbol != 0))
        // this means something like 'folder1\\folder2' - invalid path
        return ER_InvalidPath;
    else
        if (i == startSymbol)
        {
            curDir = -1;
        }
        else
        {
            // some file or folder found
            // checing '..'
            if ((startSymbol - i == 2) && (fileName[i] == '.') && (fileName[i + 1] == '.'))
            {
                // 'cd ..'
                if (currentDir >= mItemCount)
                    // invalid current dir
                    return ER_InvalidCurrentDir;
                else
                    if (currentDir < 0)
                        curDir = 0;
                    else
                        curDir = mItems[currentDir].parentId;
            }
            else
            {
                // find file by name
                int l = startSymbol - i;
                if (l >= 250)
                    l = 249;
                memcpy(el.fileName, &fileName[i], l);
                el.fileName[l] = 0;
                el.parentId = currentDir;
                int pos = FindPositionForInsert(el, true, true, false);
                if (pos < 0)
                    // invalid path
                    return ER_InvalidPath;
                // find by parent id
                bool ok = false;
                while (pos < mIndexElementsCount)
                {
                    if (mItems[mNameIndex->mItems[pos]].parentId == currentDir)
                    {
                        if (_stricmp(mItems[mNameIndex->mItems[pos]].fileName, el.fileName) == 0)
                        {
                            ok = true;
                            break;
                        }
                        else
                            break;
                    }
                    pos ++;
                }
                if (!ok)
                    // invalid current dir
                    return ER_InvalidCurrentDir;
                curDir = mNameIndex->mItems[pos];
            } // find file by name and currentDir
        }
    startSymbol ++;
    return __FindFile(fileName, slen, curDir, startSymbol);
}

int DirArray::FindFileByName(const char *fileName, int startDir)
{
    int result = ER_InvalidPath;
    if ((startDir < -1) || (startDir >= mItemCount))
        return result;
	if ((strcmp(fileName, "") == 0) || (strcmp(fileName, "\\") == 0) || (strcmp(fileName, "/") == 0))
    {
        // root directory found - for SetCurrentDirectory it is correct value
        result = ROOT_ID;
        return result;
    }
    int slen = (int)strlen(fileName);
    return __FindFile(fileName, slen, startDir);
}

HexString DirArray::GetFullFilePath(int itemNo)
{
    HexString result;
    if ((itemNo < 0) || (itemNo >= mItemCount))
        return "/";
    char buffer[250];
    memset(buffer, 0, sizeof(buffer));
    int i = itemNo;
    do
    {
        strcpy(buffer, mItems[i].fileName);
        result = HexString("/") + HexString(buffer) + result;
        i = mItems[i].parentId;
    } while ((i > ROOT_ID) && (i < mItemCount));
    return result;
}

void DirArray::PrepareSearchRecord(int itemNo, SearchRec &f)
{
    // prepare find structure ...
    f.findData.attrib = mItems[itemNo].attributes;
    //roc todo, file time not supported in linux file-searching
    /*
    FileTimeToLocalFileTime(Items[itemNo].LastModifiedTime, LocalFileTime);
    FileTimeToDosDateTime(LocalFileTime, LongRec(F.Time).Hi,
    LongRec(F.Time).Lo);
    */
    f.findData.size = mItems[itemNo].fileSize;
    strcpy(f.findData.name, mItems[itemNo].fileName);
    // finddata
    /*
    F.FindData.dwFileAttributes := F.Attr;
    F.FindData.CreationTime = Items[itemNo].CreationTime;
    F.FindData.ftLastAccessTime := Items[itemNo].LastAccessTime;
    F.FindData.ftLastWriteTime := Items[itemNo].LastModifiedTime;
    F.FindData.nFileSizeHigh := 0;
    F.FindData.nFileSizeLow := F.Size;
    Move(Items[itemNo].FileName,F.FindData.cFileName,MAX_PATH);
    */
}

int DirArray::FindFirst(const char *path, int attr, SearchRec &f, int currentDir)
{
    const int faSpecial = _A_HIDDEN | _A_SYSTEM | _A_VOLID | _A_SUBDIR;
    int result = ER_Ok;
    // extracting path and pattern
    char filePath[250];
    filePath[0] = 0;
    char pattern[250];
    pattern[0] = 0;
    f.findHandle = (__u32)(-1);
    // prepare find structure ..
    int id = -1;
    for (int i=0; i<mFoundItemCount; i++)
        if (mFoundItems[i] == 0)
        {
            id = i;
            break;
        }
    if (id < 0)
    {
        id = mFoundItemCount;
        mFoundItemCount ++;
        mFoundItems = (IntegerArray **)realloc(mFoundItems, mFoundItemCount * sizeof(IntegerArray *));
    }
    // bugs were here
    mFoundItems[id] = new IntegerArray(0, 10, 10);

    mFoundItems[id]->mCurrentItem = 0;
    f.findHandle = (__u32)id;
    f.exclusiveAttr = (~attr) & faSpecial;
    // searching file...
    int i;
    if (!ExtractPathAndPattern(path, filePath, pattern))
    {
        strcpy(pattern, path);
        if (currentDir == ROOT_ID)
            strcpy(filePath, "/");
        else
            strcpy(filePath, mItems[currentDir].fileName);
        i = 0;
    }
    else
        i = strlen(filePath);
    
    int curDir;
    if (i == 0)
    {
        if ((path[0] == '/') || (path[0] == '\\'))
            curDir = ROOT_ID;
        else
            curDir = currentDir;
    }
    else
        curDir = FindFileByName(filePath, currentDir);

    if (curDir <= ER_InvalidPath)
        result = ER_InvalidPath;
    else
    {
        DirectoryElement el;
        int itemId;

        el.parentId = curDir;
        i = FindPositionForInsert(el, false, true, false);
        if (i < 0)
            result = ER_FileNotFound;
        else
        {
            // some files may be found
            do
            {
                // get element
                itemId = mParentIndex->mItems[i];
                if (mItems[itemId].parentId != curDir)
                    break;
                // check element
                bool ok;
                if ((mItems[itemId].attributes & f.exclusiveAttr) != 0)
                    ok = false;
                else
                    ok = IsStrMatchPattern(mItems[itemId].fileName, pattern, true);
                if (ok)
                    mFoundItems[id]->Append(itemId);
                i ++;
            } while (i < mIndexElementsCount);
            if (mFoundItems[id]->mItemCount > 0)
                PrepareSearchRecord(mFoundItems[id]->mItems[0], f);
            else
                result = ER_FileNotFound;
        }
    } // path Ok
    return result;
}

int DirArray::FindNext(SearchRec &f)
{
    int result = ER_Ok;
    if (f.findHandle >= (__u32)mFoundItemCount)
        return ER_FileNotFound;
    if (mFoundItems[f.findHandle] == 0)
        result = ER_InvalidSearchRec;
    else
        if (mFoundItems[f.findHandle]->mCurrentItem >= mFoundItems[f.findHandle]->mItemCount - 1)
            result = ER_FileNotFound;
        else
        {
            mFoundItems[f.findHandle]->mCurrentItem ++;
            PrepareSearchRecord(mFoundItems[f.findHandle]->mItems[mFoundItems[f.findHandle]->mCurrentItem], f);
        }
    return result;
}

void DirArray::FindClose(SearchRec &f)
{
    if (f.findHandle < (__u32)mFoundItemCount)
        if (mFoundItems[f.findHandle] != 0)
        {
            delete mFoundItems[f.findHandle];
            mFoundItems[f.findHandle] = 0;
        }
}

void DirArray::AppendItem(DirectoryElement &value)
{
    mItemCount ++;
    SetSize(mItemCount);
    mItems[mItemCount-1] = value;
    // deleted files will not be added to indexes
    if (value.isDeleted == 0)
    {
        // update filename index
        int i = FindPositionForInsert(value, true, false, true);
        mNameIndex->Insert(i, mItemCount - 1);
        // update filename index
        i = FindPositionForInsert(value, false, false, true);
        mParentIndex->Insert(i, mItemCount - 1);
        mIndexElementsCount ++;
    }
    // here will be writing new element to disk
}

void DirArray::UpdateItem(DirectoryElement &value, int position)
{
    if ((position < 0) || (position >= mItemCount))
        //UpdateItem - invalid itemNo
        assert(false);
    DirectoryElement oldValue = mItems[position];
    mItems[position] = value;
    // both new and old element are deleted
    if ((value.isDeleted != 0) && (oldValue.isDeleted != 0))
        return;
    int oldNamePos = -1;
    int newNamePos = -1;
    int oldParentPos = -1;
    int newParentPos = -1;
    if (oldValue.isDeleted == 0)
    {
        // find old position in name index
        oldNamePos = FindPositionForInsert(oldValue, true, true, false);
        bool ok = false;
        while (oldNamePos < mIndexElementsCount)
        {
            if (mNameIndex->mItems[oldNamePos] == position)
            {
                ok = true;
                break;
            }
            oldNamePos ++;
        }
        if (!ok)
            assert(false);
            //UpdateItem - old name position not found.
        // find old position in name index
        oldParentPos = FindPositionForInsert(oldValue, false, true, false);
        ok = false;
        while (oldParentPos < mIndexElementsCount)
        {
            if (mParentIndex->mItems[oldParentPos] == position)
            {
                ok = true;
                break;
            }
            oldParentPos ++;
        }
        if (!ok)
            assert(false);
        //UpdateItem - old parentID position not found.
    }

    // deleted files will not be added to indexes
    if (value.isDeleted == 0)
    {
        // find new value position in name index
        newNamePos = FindPositionForInsert(value, true, false, true);
        // find new value position parentID index
        newParentPos = FindPositionForInsert(value, false, false, true);
    }
    if (oldValue.isDeleted != 0)
    {
        // new element is not deleted, old element is deleted
        // inserting into indexes
        mNameIndex->Insert(newNamePos, position);
        mParentIndex->Insert(newParentPos, position);
        mIndexElementsCount ++;
    }
    else
        if (value.isDeleted != 0)
        {
            // old element is not deleted, new element is deleted
            // deleteing from indexes
            mNameIndex->Delete(oldNamePos);
            mParentIndex->Delete(oldParentPos);
            mIndexElementsCount --;
        }
        else
        {
            // both new and old element are not deleted
            // updating indexes
            if ((newNamePos != oldNamePos) && (newNamePos != (oldNamePos + 1)))
                mNameIndex->MoveTo(oldNamePos, newNamePos);
            if ((newParentPos != oldParentPos) && (newParentPos != (oldParentPos + 1)))
                mParentIndex->MoveTo(oldParentPos, newParentPos);
        }
    // here will be writing new element to disk
}

DirectoryElement &DirArray::ReadItem(int position)
{
    // in multi-user version
    // here will be read from disk of specified element
    if ((position < 0) || (position >= mItemCount))
        //ReadItem - invalid itemNo
        assert(false);
    return mItems[position];
}
