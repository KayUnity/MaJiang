#include "fileUtils/BaseFileSystem.h"
#include "streams/DS_FileStream.h"
#include "streams/DS_MemoryStream.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <direct.h>
	#include <algorithm>    // std::replace
	#include <locale>       // std::locale, std::toupper
    #define gp_stat _stat
    #define gp_stat_struct struct stat
#else
    #define __EXT_POSIX2
    #include <libgen.h>
    #include <dirent.h>
    #define gp_stat stat
    #define gp_stat_struct struct stat
    #include "fileUtils/_splitpath.h"
    #include <memory>
#endif

#ifdef __ANDROID__
#include <android/asset_manager.h>
extern AAssetManager* __assetManager;
#endif

#include "singleFileSystem/SingleFileSystem.h"

static DataStructures::SingleFileSystem *__singleFileSystem = 0;

using namespace DataStructures;

#ifdef __ANDROID__
#include <unistd.h>

static void makepath(std::string path, int mode)
{
    std::vector<std::string> dirs;
    while (path.length() > 0)
    {
        int index = path.find('/');
        std::string dir = (index == -1 ) ? path : path.substr(0, index);
        if (dir.length() > 0)
            dirs.push_back(dir);
        
        if (index + 1 >= path.length() || index == -1)
            break;
            
        path = path.substr(index + 1);
    }
    
    struct stat s;
    std::string dirPath;
    for (unsigned int i = 0; i < dirs.size(); i++)
    {
        dirPath += "/";
        dirPath += dirs[i];
        if (stat(dirPath.c_str(), &s) != 0)
        {
            // Directory does not exist.
            if (mkdir(dirPath.c_str(), 0777) != 0)
            {
                GP_ERROR("Failed to create directory: '%s'", dirPath.c_str());
                return;
            }
        }
    }
    
    return;
}

//Returns true if the file exists in the android read-only asset directory.
static bool androidFileExists(const char *filePath)
{
    AAsset* asset = AAssetManager_open(__assetManager, filePath, AASSET_MODE_RANDOM);
    if (asset)
    {
        int lenght = AAsset_getLength(asset);
        AAsset_close(asset);
        return length > 0;
    }
    return false;
}

#endif

static std::string __resourcePath("./");
static std::map<std::string, std::string> __aliases;


//Gets the fully resolved path.
//If the path is relative then it will be prefixed with the resource path.
//Aliases will be converted to a relative path.
static void getFullPath(const char *path, std::string& fullPath)
{
    if (BaseFileSystem::IsAbsolutePath(path))
    {
        fullPath.assign(path);
    }
    else
    {
        fullPath.assign(__resourcePath);
        fullPath += BaseFileSystem::ResolvePath(path);
    }
}

/////////////////////////////

BaseFileSystem::BaseFileSystem()
{
}

BaseFileSystem::~BaseFileSystem()
{
}

std::map<std::string, std::string> &BaseFileSystem::GetAliases()
{
    return __aliases;
}

void BaseFileSystem::Initialize(const char *path)
{
    std::string fullPath;
    getFullPath(path, fullPath);
    if (!__singleFileSystem && FileExists(fullPath.c_str()))
    {
        __singleFileSystem = new DataStructures::SingleFileSystem(fullPath.c_str(), FM_OPEN_READ, "");
    }
}

void BaseFileSystem::GetFullPath(const char *path, std::string &fullPath)
{
    getFullPath(path, fullPath);
}

void BaseFileSystem::Finitialize()
{
    if (__singleFileSystem)
        delete __singleFileSystem;
    __singleFileSystem = 0;
}

void BaseFileSystem::SetResourcePath(const char *path)
{
    __resourcePath = path == NULL ? "" : path;
}

const char *BaseFileSystem::GetResourcePath()
{
    return __resourcePath.c_str();
}

const char *BaseFileSystem::ResolvePath(const char *path)
{
    assert(path);

    size_t len = strlen(path);
    if (len > 1 && path[0] == '@')
    {
        std::string alias(path + 1);
        std::map<std::string, std::string>::const_iterator itr = __aliases.find(alias);
        if (itr == __aliases.end())
            return path; // no matching alias found
        return itr->second.c_str();
    }

    return path;
}

bool BaseFileSystem::ListFiles(const char *dirPath, std::vector<std::string> &files)
{
    //first of all, try get file list from single file system
    if (__singleFileSystem)
    {
        std::string path(BaseFileSystem::GetResourcePath());
        if (dirPath && strlen(dirPath) > 0)
        {
            path.append(dirPath);
        }
        std::string wPath = path + "/*.*";
        DataStructures::SearchRec sr;
        if (__singleFileSystem->FindFirst(wPath.c_str(), 0xff, sr) == 0)
        {
            while (true)
            {
                //not a folder
                if ((sr.findData.attrib & (_A_VOLID | _A_SUBDIR)) == 0)
                {
                    std::string filepath(path);
                    filepath.append("/");
                    filepath.append(sr.findData.name);
                    files.push_back(filepath);
                }
                if (__singleFileSystem->FindNext(sr) != 0)
                    break;
            }
            __singleFileSystem->FindClose(sr);
        }
        
    }
#ifdef WIN32
    std::string path(BaseFileSystem::GetResourcePath());
    if (dirPath && strlen(dirPath) > 0)
    {
        path.append(dirPath);
    }
    path.append("/*");
    // Convert char to wchar
    std::basic_string<TCHAR> wPath;
    wPath.assign(path.begin(), path.end());

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(wPath.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        return false;
    }
    do
    {
        // Add to the list if this is not a directory
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            // Convert wchar to char
            std::basic_string<TCHAR> wfilename(FindFileData.cFileName);
            std::string filename;
            filename.assign(wfilename.begin(), wfilename.end());
            files.push_back(filename);
        }
    } while (FindNextFile(hFind, &FindFileData) != 0);

    FindClose((long)hFind);
    return true;
#else
    std::string path(BaseFileSystem::GetResourcePath());
    if (dirPath && strlen(dirPath) > 0)
    {
        path.append(dirPath);
    }
    path.append("/.");
    bool result = false;

    struct dirent *dp;
    DIR *dir = opendir(path.c_str());
    if (dir != NULL)
    {
        while ((dp = readdir(dir)) != NULL)
        {
            std::string filepath(path);
            filepath.append("/");
            filepath.append(dp->d_name);

            struct stat buf;
            if (!stat(filepath.c_str(), &buf))
            {
                // Add to the list if this is not a directory
                if (!S_ISDIR(buf.st_mode))
                {
                    files.push_back(dp->d_name);
                }
            }
        }
        closedir(dir);
        result = true;
    }

#ifdef __ANDROID__
    // List the files that are in the android APK at this path
    AAssetDir* assetDir = AAssetManager_openDir(__assetManager, dirPath);
    if (assetDir != NULL)
    {
        AAssetDir_rewind(assetDir);
        const char *file = NULL;
        while ((file = AAssetDir_getNextFileName(assetDir)) != NULL)
        {
            std::string filename(file);
            // Check if this file was already added to the list because it was copied to the SD card.
            if (find(files.begin(), files.end(), filename) == files.end())
            {
                files.push_back(filename);
            }
        }
        AAssetDir_close(assetDir);
        result = true;
    }
#endif

    return result;
#endif
}

bool BaseFileSystem::FileExists(const char *filePath)
{
    assert(filePath);

    //try SingleBaseFileSystem first
    if (__singleFileSystem && !BaseFileSystem::IsAbsolutePath(filePath))
    {
        char path[250];
        if ((filePath[0] == '/') || (filePath[0] == '\\'))
            strcpy(path, filePath);
        else
        {
            strcpy(path, "/");
            strcat(path, filePath);
        }
        if (__singleFileSystem->FileExists(path))
            return true;
    }
    
#ifdef __ANDROID__
    if (androidFileExists(resolvePath(filePath)))
    {
        return true;
    }
#endif

    std::string fullPath;
    getFullPath(filePath, fullPath);

    gp_stat_struct s;
    return stat(fullPath.c_str(), &s) == 0;

}

DataStructures::Stream *BaseFileSystem::Open(const char *path, size_t streamMode, bool cachedInMemory)
{
    if (((streamMode & WRITE) == 0) && __singleFileSystem && !BaseFileSystem::IsAbsolutePath(path))
    {
        if (strlen(path) > 0)
        {
            char p[250];
            if ((path[0] == '/') || (path[0] == '\\'))
                strcpy(p, path);
            else
            {
                strcpy(p, "/");
                strcat(p, path);
            }
            if (__singleFileSystem->FileExists(path))
            {
                DataStructures::SFSFileStream *stream = new DataStructures::SFSFileStream(__singleFileSystem, p, FM_OPEN_READ);
                if (stream && cachedInMemory)
                {
                    MemoryStream *result = new MemoryStream();
                    if (!stream->SaveToStream(result))
                        delete result;
                    else
                    {
                        delete stream;
                        return result;
                    }
                }
                return stream;
            }
        }
    }
        
    char modeStr[] = "rb";
    if ((streamMode & WRITE) != 0)
        modeStr[0] = 'w';
#ifdef __ANDROID__
    if ((streamMode & WRITE) != 0)
    {
        // Open a file on the SD card
        std::string fullPath(__resourcePath);
        fullPath += resolvePath(path);

        size_t index = fullPath.rfind('/');
        if (index != std::string::npos)
        {
            std::string directoryPath = fullPath.substr(0, index);
            struct stat s;
            if (stat(directoryPath.c_str(), &s) != 0)
                makepath(directoryPath, 0777);
        }
        return FileStream::create(fullPath.c_str(), modeStr);
    }
    else
    {
        // Open a file in the read-only asset directory
        return FileStreamAndroid::create(resolvePath(path), modeStr);
    }
#else
    std::string fullPath;
    getFullPath(path, fullPath);
    DataStructures::FileStream *stream = DataStructures::FileStream::Create(fullPath.c_str(), modeStr);
    if (stream && ((streamMode & WRITE) == 0) && cachedInMemory)
    {
        MemoryStream *result = new MemoryStream();
        if (result)
        {
            if (!stream->SaveToStream(result))
                delete result;
            else
            {
                delete stream;
                return result;
            }
        }
    }
    return stream;
#endif
}

DataStructures::Stream *BaseFileSystem::ReOpen(DataStructures::Stream *original, const char *path, size_t streamMode, bool cachedInMemory)
{
    if (original)
        delete original;
    return Open(path, streamMode, cachedInMemory);
}

FILE *BaseFileSystem::OpenFile(const char *filePath, const char *mode)
{
    assert(filePath);
    assert(mode);

    //try export file from single file system
    if ((strstr(mode, "r") == 0) && (strstr(mode, "R") == 0) && __singleFileSystem && !BaseFileSystem::IsAbsolutePath(filePath))
        __singleFileSystem->ExportFiles(filePath, BaseFileSystem::GetResourcePath(), 0xff, false, DataStructures::OM_NEVER);
    
    std::string fullPath;
    getFullPath(filePath, fullPath);

    CreateFileFromAsset(filePath);
    
    FILE *fp = fopen(fullPath.c_str(), mode);
    return fp;
}

char *BaseFileSystem::ReadAll(const char *filePath, int *fileSize)
{
    assert(filePath);

    // Open file for reading.
    std::auto_ptr<DataStructures::Stream> stream(Open(filePath));
    if (stream.get() == NULL)
    {
        //GP_ERROR("Failed to load file: %s", filePath);
        return NULL;
    }
    size_t size = stream->Length();

    // Read entire file contents.
    char *buffer = new char[size + 1];
    size_t read = stream->Read(buffer, size);
    if (read != size)
    {
        //GP_ERROR("Failed to read complete contents of file '%s' (amount read vs. file size: %u < %u).", filePath, read, size);
        if (buffer)
            delete [] buffer;
        buffer = 0;
        return NULL;
    }

    // Force the character buffer to be NULL-terminated.
    buffer[size] = '\0';

    if (fileSize)
    {
        *fileSize = (int)size; 
    }
    return buffer;
}

bool BaseFileSystem::IsAbsolutePath(const char *filePath)
{
    if (filePath == 0 || filePath[0] == '\0')
        return false;
#ifdef WIN32
    if (filePath[1] != '\0')
    {
        char first = filePath[0];
        return (filePath[1] == ':' && ((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z')));
    }
    return false;
#else
    return filePath[0] == '/';
#endif
}

void BaseFileSystem::CreateFileFromAsset(const char *path)
{
#ifdef __ANDROID__
    static std::set<std::string> upToDateAssets;

    GP_ASSERT(path);
    std::string fullPath(__resourcePath);
    std::string resolvedPath = BaseFileSystem::resolvePath(path);
    fullPath += resolvedPath;

    std::string directoryPath = fullPath.substr(0, fullPath.rfind('/'));
    struct stat s;
    if (stat(directoryPath.c_str(), &s) != 0)
        makepath(directoryPath, 0777);

    // To ensure that the files on the file system corresponding to the assets in the APK bundle
    // are always up to date (and in sync), we copy them from the APK to the file system once
    // for each time the process (game) runs.
    if (upToDateAssets.find(fullPath) == upToDateAssets.end())
    {
        AAsset* asset = AAssetManager_open(__assetManager, resolvedPath.c_str(), AASSET_MODE_RANDOM);
        if (asset)
        {
            const void* data = AAsset_getBuffer(asset);
            int length = AAsset_getLength(asset);
            FILE *file = fopen(fullPath.c_str(), "wb");
            if (file != NULL)
            {
                int ret = fwrite(data, sizeof(unsigned char), length, file);
                if (fclose(file) != 0)
                {
                    GP_ERROR("Failed to close file on file system created from APK asset '%s'.", path);
                    return;
                }
                if (ret != length)
                {
                    GP_ERROR("Failed to write all data from APK asset '%s' to file on file system.", path);
                    return;
                }
            }
            else
            {
                GP_ERROR("Failed to create file on file system from APK asset '%s'.", path);
                return;
            }

            upToDateAssets.insert(fullPath);
        }
    }
#endif
}

#include "CommonTypeDefines.h"
std::string BaseFileSystem::GetDirectoryName(const char *path)
{
	//if (path == NULL || strlen(path) == 0)
	//{
	//	return "";
	//}
    //char drive[8];
    //char directory[512];
    //char filename[512];
    //char extension[32];
    //_splitpath(path, drive, directory, filename, extension);
    //strcat(drive, directory);
    //size_t len = strlen(drive);
    //if (len == 0)
    //    return "";
    //if (drive[len - 1] == '\\')
    //    drive[len - 1] = '/';
    //if (drive[len - 1] != '/')
    //    strcat(drive, "/");
    //
    //return drive;
	
	if (path == NULL || strlen(path) == 0)
	{
		return "";
	}
	char drive[520]; // just adjust the allocated-size to be greater than before
	char directory[512];
	char filename[512];
	char extension[32];
	memset(drive, 0, sizeof(drive)); 
	memset(directory, 0, sizeof(directory));// i think it must be initialized to be 0
	_splitpath(path, drive, directory, filename, extension);
	strcat(drive, directory);
	size_t len = strlen(drive);
	if (len == 0)
		return "";
	if (drive[len - 1] == '\\')
		drive[len - 1] = '/';
	if (drive[len - 1] != '/')
		strcat(drive, "/");

	return drive;

   /*
#ifdef WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(path, drive, dir, NULL, NULL);
    std::string dirname;
    size_t driveLength = strlen(drive);
    if (driveLength > 0)
    {
        dirname.reserve(driveLength + strlen(dir));
        dirname.append(drive);
        dirname.append(dir);
    }
    else
    {
        dirname.assign(dir);
    }
    std::replace(dirname.begin(), dirname.end(), '\\', '/');
    return dirname;
#else
    // dirname() modifies the input string so create a temp string
    std::string dirname;
    char *tempPath = new char[strlen(path) + 1];
    strcpy(tempPath, path);
    char *dir = ::dirname(tempPath);
    if (dir && strlen(dir) > 0)
    {
        dirname.assign(dir);
        // dirname() strips off the trailing '/' so add it back to be consistent with Windows
        dirname.append("/");
    }
    if (tempPath)
        delete [] tempPath;
    tempPath = 0;
    return dirname;
#endif
    */
}

std::string BaseFileSystem::GetFileName(const char *path)
{
    if (path == NULL || strlen(path) == 0)
    {
        return "";
    }
    char drive[8];
    char directory[512];
    char filename[512];
    char extension[32];
	memset(filename, 0, sizeof(filename)); // added
    _splitpath(path, drive, directory, filename, extension);
    
    return filename;
}

std::string BaseFileSystem::GetExtension(const char *path)
{
	//if (path == NULL || strlen(path) == 0)
	//{
	//	return "";
	//}
	//char drive[8];
	//char directory[512];
	//char filename[512];
	//char extension[32];
	//_splitpath(path, drive, directory, filename, extension);
	//strcat(drive, directory);
	//size_t len = strlen(drive);
	//if (len == 0)
	//	return "";
	//if (drive[len - 1] == '\\')
	//	drive[len - 1] = '/';
	//if (drive[len - 1] != '/')
	//	strcat(drive, "/");
	//return drive;

	if (path == NULL || strlen(path) == 0)
	{
		return "";
	}
	char drive[8];
	char directory[512];
	char filename[512];
	char extension[32];
	memset(extension, 0, sizeof(extension)); // added
	_splitpath(path, drive, directory, filename, extension);
	return std::string(extension);

    /*
	// just avoid the case: F:\dir.1\file
	// so, should find the last position of '\\' or '/'
	// and then compare the position of '.' with the position of "\\" or "/"

	const char *str = strrchr(path, '.');
	if (str == NULL)
		return "";
	// the comment above should be included which is a part of the following codes.

    std::string ext;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i)
	{
#if defined WIN32 || defined WIN64
		 std::locale loc;
         ext += std::toupper(str[i], loc);
#else
        ext += std::toupper(str[i]);
#endif
	}
    return ext;
    */
}

std::string BaseFileSystem::GetFileNameWithExtension(const char *path)
{
    if (path == NULL || strlen(path) == 0)
    {
        return "";
    }
    char drive[8];
    char directory[512];
    char filename[512];
    char extension[32];
    memset(extension, 0, sizeof(extension)); // added
    _splitpath(path, drive, directory, filename, extension);
    strcat(filename, extension);
    return std::string(filename);
}

long BaseFileSystem::FindFirst(const char *name, _finddata_t *f)
{
    int res = -1;
    if (__singleFileSystem)//&& !BaseFileSystem::IsAbsolutePath(name))
    {
        DataStructures::SearchRec sr;
        memcpy(&sr.findData, f, sizeof(_finddata_t));
        if (__singleFileSystem->FindFirst(name, 0xff, sr) < 0)
            res = -1;
        else
        {
            memcpy(f, &sr.findData, sizeof(_finddata_t));
            res = (int)sr.findHandle;
        }
    }
    if (res == -1)
    {
        std::string fullPath;
        getFullPath(name, fullPath);
        return _findfirst(fullPath.c_str(), f);
    }
    else
        return res;
}

int BaseFileSystem::FindNext(long h, _finddata_t *f)
{
    int res = -1;
    if (__singleFileSystem)
    {
        DataStructures::SearchRec sr;
        memcpy(&sr.findData, f, sizeof(_finddata_t));
        sr.findHandle = h;
        if (__singleFileSystem->FindNext(sr) < 0)
            res = -1;
        else
        {
            memcpy(f, &sr.findData, sizeof(_finddata_t));
            res = (int)sr.findHandle;
        }
    }
    if (res == -1)
        return _findnext(h, f);
    else
        return res;
}

int BaseFileSystem::FindClose(long h)
{
    int res = _findclose(h);
    if ((res != 0) && __singleFileSystem)
    {
        DataStructures::SearchRec sr;
        sr.findHandle = h;
        __singleFileSystem->FindClose(sr);
        return 0;
    }
    return res;
}
