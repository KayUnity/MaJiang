#ifndef __SINGLE_FILE_SYSTEM_H
#define __SINGLE_FILE_SYSTEM_H

#include "singleFileSystem/SingleFileSystemStructures.h"
#include "singleFileSystem/SFSArray.h"
#include "HexString.h"
#include "streams/DS_Stream.h"
#include "singleFileSystem/SFSFileStreams.h"

namespace DataStructures
{
    
    class PageFileManager;
    class DirManager;
    class FreeSpaceManager;
    class UserFilePageMapManager;
    
    class SingleFileSystem
    {
    protected:
        PageFileManager *mPFMHandle;
        DirManager *mDMHandle;
        FreeSpaceManager *mFSMHandle;
        UserFilePageMapManager *mUFMHandle;
        SortedPtrArray *mFileHandles;
        bool mReadOnly;
        bool mExclusive;
        bool mCancel;
        HexString mPassword;
        HexString mFileName;
        __u16 mOpenMode;
        
    private:
        float mProgress;
        float mProgressMax;
        /*
         FOnProgress: TSFSProgressEvent;
         FOnFileProgress: TSFSFileProgressEvent;
         FOnOverwritePrompt: TSFSOverwritePromptEvent;
         FOnDiskFull: TSFSDiskFullEvent;
         FOnPassword: TSFSOnPasswordEvent;
         */
        bool mInMemory;
        HexString mCurrentFileName;
        void CloseAllFiles();
        void InternalCreate();
        
        void DoOnProgress(float progress);
        void DoOnDiskFull(void *sender);
        void DoOnPassword(const char *fileName, char *newPassword, bool &skipFile);
        void DoOnOverwritePrompt(const char *existingFileName, const char *newFileName, bool &overwrite);
        
        void DoOnFileProgress(void *sender, float percentDone);
        void InternalReopen(Stream *stream);
        SFSCompressionLevel GetCompressionLevel();
        void SetCompressionLevel(SFSCompressionLevel newLevel);
        void SetInMemory(bool value);
    public:
        int mLastError;
        
        SingleFileSystem(const char *fileName, __u16 mode, const char *password = "", SFSCompressionLevel defaultCompressionLevel = SFS_NONE);
        SingleFileSystem(const char *fileName, __u16 mode, bool inMemory, const char *password = "", SFSCompressionLevel defaultCompressionLevel = SFS_NONE, int pageSize = DEFAULT_PAGE_SIZE, int extentPageCount = DEFAULT_EXTENT_PAGE_COUNT);
        
        virtual ~SingleFileSystem();
        
        bool InternalRepair(char *log, bool deleteCorruptedFiles = false, bool changeEncryption = false, const char *newPassword = "");
        bool Repair(char *log, bool deleteCorruptedFiles = false);
        bool IsPasswordValid(const char *fileName, const char *password);
        
        bool IsFileEncrypted(const char *fileName);
        
        int FileCreate(const char *fileName, const char *password = "");
        int FileOpen(const char *fileName, __u32 mode, const char *password = "");
        void FileClose(int handle);
        int FileRead(int handle, char *buffer, int count);
        int FileWrite(int handle, const char *buffer, int count);
        __s64 FileSeek(int handle, const __s64 offset, int origin);
        void FlushFileBuffers(int handle);
        __s64 FileSetSize(int handle, __s64 size);
        __s64 FileGetSize(int handle);
        bool DeleteFile(const char *fileName);
        bool RenameFile(const char *oldName, const char *newName);
        bool CopyFile(const char *oldName, const char *newName, const char *password = "");
        bool MoveFile(const char *oldName, const char *newName, const char *password = "");
        bool FileExists(const char *fileName);
        int FileGetAttr(const char *fileName);
        int FileSetAttr(const char *fileName, int attr);
        int FileAge(const char *fileName);
        int FileGetDate(int handle);
        int FileSetDate(int handle, int age);
        __s64 FileGetPosition(int handle);

        bool GetPasswordHeader(const char *fileName, PasswordHeader *passHeader);
        void SetPasswordHeader(const char *fileName, PasswordHeader *passHeader);
        bool IsEncrypted();
        HexString GetControlQuestion();
    private:
        int InternalFileOpen(int fileId);
        int InternalFileRead(int handle, char *buffer, int count, DirectoryElement *el, bool changeEncryption, bool deleteCorruptedFiles);
        int InternalFileWrite(int handle, const char *buffer, int count, SingleFileSystem *newFile, DirectoryElement *el, bool changeEncryption, const char *newPassword);
        
    public:
        int FindFirst(const char *path, int attr, SearchRec &f);
        int FindNext(SearchRec &f);
        void FindClose(SearchRec &f);
        bool ChangeEncryption(const char *newPassword = "");
        bool ChangeFilesEncryption(const char *fileMask, const char *oldPassword = "", const char *newPassword = "");
        __s64 DiskSize();
        HexString GetCurrentDir();
        bool SetCurrentDir(const char *dir);
        bool RemoveDir(const char *dir);
        bool CreateDir(const char *dir);
        __s64 DiskFree();
        int ImportFiles(const char *sourcePath, const char *destPath = "", int attr = 0xff, bool recursive = true, SFSOverwriteMode overwriteMode = OM_PROMPT, bool encryptFiles = false);
        int ImportFolder(const char *sourcePath, const char *destPath = "", bool recursive = true, SFSOverwriteMode overwriteMode = OM_PROMPT, bool encryptFiles = false);
        int ExportFiles(const char *sourcePath, const char *destPath = "", int attr = 0xff, bool recursive = true, SFSOverwriteMode overwriteMode = OM_PROMPT);
        int ExportFolder(const char *sourcePath, const char *destPath = "", bool recursive = true, SFSOverwriteMode overwriteMode = OM_PROMPT);
        bool IsFolderEmpty(const char *dir);
        int DeleteFiles(const char *path, int attr = 0xff, bool recursive = true);
        int DeleteFolder(const char *dir);
        
        //bool RunApplication(const char *fileName, const char *parameters = "", const char *directory = "", int showCmd = SW_SHOW_NORMAL);
        
        int LoadLibrary(const char *fileName);
        
        void LoadFromStream(Stream *stream);
        void LoadFromFile(const char *fileName);
        void SaveToStream(Stream *stream);
        void SaveToFile(const char *fileName);
        
    private:
        void ImportFile(const char *path, const char *file, const char *startPath, SFSOverwriteMode overwriteMode, bool encryptFiles);
        void ExportFile(const char *path, const char *file, const char *startPath, SFSOverwriteMode overwriteMode);
        void ProcessFilesForImport(const char *startPath, const char *path, const char *mask, int attr, int exclusive, void *list1, void *list2, int &result, bool recursive);
        void ProcessFilesForExport(const char *startPath, const char *path, const char *mask, int attr, int exclusive, void *list1, void *list2, int &result, bool recursive);
        void ProcessFilesForDelete(const char *path, const char *mask, int attr, int exclusive, bool recursive, int &result);

    public:
        bool ForceDirectories(const char *dir);
        bool DirectoryExists(const char *name);
    };

}
#endif
