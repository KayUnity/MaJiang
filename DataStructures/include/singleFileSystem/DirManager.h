#ifndef __DIR_MANAGER_H
#define __DIR_MANAGER_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "singleFileSystem/SFSArray.h"
#include "HexString.h"

namespace DataStructures
{
    class FreeSpaceManager;
    class PageFileManager;
    
    class DirManager
    {
    private:
        FreeSpaceManager *mSMHandle; // space manager
        IntegerArray *mDirPageMap;
        int mDirElementsPerPage;
        int mDirPageSize;
    public:
        PageFileManager *mFMHandle; // page manager
    public:
        // protected
        DirArray *mDir;
        IntegerArray *mOpenedFiles; // each element in this array corresponds number of
        // opened file handles for this file
        // default value = 0
        int mLastError; // code of last error
        
        //  private
        // load dir
        void Load();
        // appends element to DIR
        // if it was unable to Append this element - returns false
        bool AddItem(DirectoryElement &item);
        // read item
        void ReadItem(int itemNo, DirectoryElement *item);
        // write item
        void WriteItem(int itemNo, DirectoryElement &item);
        //  protected
        // find by name, returns element number or erFileNotFound if no element were found
        int FindByName(const char *fileName);
        // returns full file path (form root, '\folder1\folder2')
        HexString GetFullFilePath(int itemNo);
        // creates file, returns returns erFileNotFound if file can not be created;
        // if file created successfully return value will be index of directory element
        int FileCreate(const char *fileName, const char *password = "");
        // open file if it is possible; returns erFileNotFound if file does not exists
        // returns Key if file is encrypted
        int FileOpen(const char *fileName, const char *password, char *key);
        // file close (itemNo - index of directory element for the file)
        void FileClose(int itemNo);
        // renames file
        bool RenameFile(const char *oldName, const char *newName);
        // returns number of opened files for specified directory element
        int GetOpenFiles(int itemNo);
        // returns true if Single file password is valid
        bool IsPasswordValid(const char *fileName, const char *password);
        // returns password header
        bool GetPasswordHeader(const char *fileName, PasswordHeader *passHeader);
        // sets password header
        void SetPasswordHeader(const char *fileName, PasswordHeader *passHeader);
        // returns true if file is encrypted by its own password
        bool IsFileEncrypted(const char *fileName);
    public:
        HexString mCurrentPath;
        int mCurrentDir;
        
        // constructor
        DirManager(PageFileManager *pageFileManager, FreeSpaceManager *freeSpaceManager);
        // destructor
        virtual ~DirManager();
        //----------------------- User Interface ----------------------------------
        // find file by pattern using '*', '?'
        int FindFirst(const char *path, int attr, SearchRec &f);
        int FindNext(SearchRec &f);
        void FindClose(SearchRec &f);
        // returns current directory name ('\' if root directory )
        HexString GetCurrentDir();
        // return value set to True if directory successfully changed
        bool SetCurrentDir(const char *dir);
        // removes directory
        bool RemoveDir(const char *dir);
        // creates directory
        bool CreateDir(const char *dir);
        // Creates all the directories along a directory path if they do not already exist
        bool ForceDirectories(const char *dir);
        // determines whether a specified directory exists.
        bool DirectoryExists(const char *name);
    };
        
}

#endif