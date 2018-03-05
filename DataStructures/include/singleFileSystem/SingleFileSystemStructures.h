#ifndef __SINGLE_FILE_SYSTEM_STRUCTURES_H
#define __SINGLE_FILE_SYSTEM_STRUCTURES_H

#include "SingleFileSystemDefines.h"
#include "CommonTypeDefines.h"
#include <time.h>
#include "fileUtils/_FindFirst.h"
#include <string.h>

namespace DataStructures
{
    
    struct SearchRec
    {
        __u32 findHandle;
        __u32 exclusiveAttr;
        _finddata_t findData;
        
        inline SearchRec() : findHandle(0), exclusiveAttr(0) {}
    };
    
#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct PasswordHeader
    {
        char key[MAX_PASSWORD_LENGTH + 1]; //
        char pass[MAX_PASSWORD_LENGTH + 1]; //
        __u32 keyCRC; // checksum for key
        __u32 passCRC; // checksum for pass
        PasswordHeader& operator = (const PasswordHeader &source)
        {
            strcpy(key, source.key);
            strcpy(pass, source.pass);
            keyCRC = source.keyCRC;
            passCRC = source.passCRC;
            return *this;
        }
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct SingleFileHeader
    {
        char signature[4];
        __u32 crc; // check sum for other fields in header
        float version;  // format version
        __s32 pageSize; // size in bytes
        __s32 extentPageCount; // extent size in pages
        __s32 hdrPageCount; // number of pages occupied by HEADER, usually 1
        __s32 gamPageCount; // number of pages occupied by GAM
        __s32 pfsPageCount; // number of pages occupied by PFS
        __s32 dirPageCount; // number of pages occupied by DIRECTORY
        __s32 dirFirstPageNo; // No of first page occupied by DIRECTORY
        __s32 dirElementsCount;// DIR elements quantity = files and folders qty
        __s32 totalPageCount;// number of pages in file
        PasswordHeader passwordHeader; // password header
        unsigned char encMethod; // 0 - none, 1 - Rijndael
        unsigned char compressionLevel; // default compression level
        char reserved[30];
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct PageHeader
    {
        __s32 nextPageNo;  // used for building page chains
        __u32 crc; // check sum for page
        unsigned char pageType; // header, dir, ... page
        unsigned char encType; // 0 - None, 1-Rijndael, ...
        unsigned char crcType; // 0 - Fast, 1- Full
        unsigned char reserved1;  // for future extension
        unsigned char reserved2[20];  // for future extending
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct FFPage
    {
        PageHeader pageHeader; // header of page
        void *pData; // pointer to data stored in page
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

     // this is Single-file directory element
     // user-defined folders not supported yet
     // compression, CRC protection and encryption not supported yet
     // last access and last modified times will supported later
     // if IsCrcProtected not specified,
     //  if IsEncrypted specified - in FileCRC will be stored
     //   only first page check sum.
#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct DirectoryElement
    {
        inline DirectoryElement() { memset(this, 0, sizeof(DirectoryElement)); }
        
        __s32 firstMapPageNo;   // number of first file map page no
        __s64 fileSize;  // file size in bytes
		__u64 creationTime; // creation time
		__u64 lastModifiedTime; // last modification time
		__u64 lastAccessTime; // last access time
        __u32 attributes;   // file id (unique number)
        __s32 parentId;   // id of parent folder
        // -1 - root
        __u32 fileCRC;  // file check sum (not used)
        PasswordHeader passwordHeader; // password
        unsigned char isFolder;      // 0 - no, 1 - yes
        unsigned char isDeleted;      // 0 - no, 1 - yes
        unsigned char encMethod;      // 0 - none, 1 - Rijndael
        unsigned char reserved1;      //
        
        char fileName[250];  // file name (0-terminated)
        __s32 reserved2[7]; //

		DirectoryElement & operator = (const DirectoryElement &source) { memcpy(this, &source, sizeof(DirectoryElement)); passwordHeader = source.passwordHeader; return *this; }
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct EmptyPageElement
    {
       __s32 pageNo; // start block of empty area
       __s32 pageCount; // number of pages in this area
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct UserFileHandle
    {
        __s32 fileId; // ID of Directory Element for this file
        __s64 position;  // currentPosition
        __u32 mode; // open mode
        char key[MAX_PASSWORD_LENGTH * 2 + 8];
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct SFSFileStreamHeader
    {
        inline SFSFileStreamHeader() { memset(this, 0, sizeof(SFSFileStreamHeader)); }

        char signature[4]; // signature
        __s32 blockSize; // block size
        __s32 crc32;  // crc of control block
        __s32 numBlocks;  // number of blocks
        float version;   // version
        unsigned char compressionLevel; // compression level
        unsigned char crcMode; // check sum method
        unsigned char EncMethod; // encryption algorithm
        __s32 customHeaderSize;	// size of custom heade
        char reserved[1]; // reserved
        char controlBlock[100]; // control for encryption
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
    }__attribute__ ((packed));
#endif

#if defined _WIN32 || defined _WIN64
# pragma pack (1)
#endif
    struct SFSHeader
    {
        __u32 packedSize; // packed block size
        __u32 trueSize; // unpacked block size
        __u32 crc32; // check sum for this block page
        __u32 nextHeaderNo; // offset from beginning of compressed file to next block header
#if defined _WIN32 || defined _WIN64
	};
# pragma pack ()
#else
	}__attribute__ ((packed));
#endif

}

#endif