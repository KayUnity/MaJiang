#ifndef BASE_FILESYSTEM_H_
#define BASE_FILESYSTEM_H_

#include <vector>
#include <map>
#include "streams/DS_Stream.h"
#include <string>
#include "fileUtils/_FindFirst.h"

namespace DataStructures
{

    //Defines a set of functions for interacting with the device filesystem.
    class BaseFileSystem
    {
    public:
        // Mode flags for opening a stream.
        enum StreamMode
        {
            READ = 1,
            WRITE = 2
        };
        
        enum DialogMode
        {
            OPEN,
            SAVE
        };

        BaseFileSystem();
        ~BaseFileSystem();
        
        static void SetResourcePath(const char *path);
        static const char *GetResourcePath();
        
        static void Initialize(const char *path);
        static void Finitialize();
        
        static const char *ResolvePath(const char *path);
        static void GetFullPath(const char *path, std::string &fullPath);
        
        static bool ListFiles(const char *dirPath, std::vector<std::string> &files);
        
        static bool FileExists(const char *filePath);

        static long FindFirst(const char *name, _finddata_t *f);
        static int FindNext(long h, _finddata_t *f);
        static int FindClose(long h);
        
        static DataStructures::Stream *Open(const char *path, size_t streamMode = READ, bool cachedInMemory = false);
        static DataStructures::Stream *ReOpen(DataStructures::Stream *original, const char *path, size_t streamMode, bool cachedInMemory = false);
        static FILE *OpenFile(const char *filePath, const char *mode);
        
        static char *ReadAll(const char *filePath, int *fileSize = NULL);
        
        static bool IsAbsolutePath(const char *filePath);
        
        static void CreateFileFromAsset(const char *path);
        
        static std::string GetDirectoryName(const char *path);
        static std::string GetFileName(const char *path);
        static std::string GetExtension(const char *path);
        static std::string GetFileNameWithExtension(const char *path);
        
    protected:
        static std::map<std::string, std::string> &GetAliases();
    };
    
}

#endif
