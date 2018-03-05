#ifndef __SINGLE_FILE_SYSTEM_DEFINES_H
#define __SINGLE_FILE_SYSTEM_DEFINES_H

#define _CRTLF "\n\t"
#define SFS_STREAM_SIGNATURE        "SFSS"
#define SIGNATURE_SIZE              4
#define SFS_CURRENT_VERSION         2.11f
#define SFS_FOTMAT_VERSION          1.00f;
#define DEFAULT_PAGE_SIZE           4 * 1024    // size of one page in bytes
#define DEFAULT_EXTENT_PAGE_COUNT   8           // number of Pages per one extent
// page types
#define DIR_PAGE                    1 // directory
#define GAM_PAGE                    2 // global allocation map
#define SGAM_PAGE                   3 // shared global allocation map
#define PFS_PAGE                    4 // page free space
#define UFPM_PAGE                   5 // user file page map
#define UF_PAGE                     6 // user file
// root id
#define ROOT_ID                     -1
// error codes
#define ER_Ok                       0
#define ER_InvalidPath              -2
#define ER_InvalidCurrentDir        -3
#define ER_InvalidSearchRec         -4
#define ER_FileNotFound             -5
#define ER_InvalidHandle            -6
#define ER_ReadPageHeaderError      -7
#define ER_WritePageHeaderError     -8
#define ER_ReadPageError            -9
#define ER_WritePageError           -10
#define ER_CRCError                 -11
#define ER_DecodeError              -12
#define ER_EncodeError              -13
#define ER_NoMemory                 -14
#define ER_RenameFileError          -15
#define ER_FileNotDeleted           -16
#define ER_DiskFull                 -17
// no - no link, no element, etc.
#define NONE                        -1
// Encryption modes
#define ENC_NONE                    0
#define ENC_RIJNDAEL                1

#define SPasswordTitle              "Password for %s"
#define SPasswordPrompt             "Enter password"

#define UNIFORM_MIN_PAGE_COUNT      DEFAULT_EXTENT_PAGE_COUNT

// maximum block size for stream classes, LoadFromStream / SaveToStream
#define DEFAULT_MAX_BLOCK_SIZE      1024 * 1024 // 1.0 Mb for eclNone
#define BLOCK_SIZE_FOR_FASTEST      512 * 1024  // 0.5 Mb for fastest modes
#define BLOCK_SIZE_FOR_NORMAL       1024 * 1024 // 1.0 Mb for normal modes
#define BLOCK_SIZE_FOR_MAX          1536 * 1024 // 1.5 Mb for max modes
#define DEFAULT_COPY_BLOCK_SIZE     100 * 1024  // block size for copy operation
#define UNIFORM_MIN_PAGE_COUNT      DEFAULT_EXTENT_PAGE_COUNT

#define PPM_FASTEST_MO              3
#define PPM_FASTEST_SA              10 // Mb
#define PPM_NORMAL_MO               5
#define PPM_NORMAL_SA               25 // Mb
#define PPM_MAX_MO                  13
#define PPM_MAX_SA                  50 // Mb

#define CRC32_CHECKSUM              1

#define DEFAULT_COMPRESSON_LEVEL    1
#define DEFAULT_CRC_METHOD          CRC32_CHECKSUM

#define SFS_COMPRESS_CURRENT_VERSION   2.0
#define SFS_DIRECTORY_ELEMENT_SIZE  sizeof(DirectoryElement)
#define SFS_PAGE_HEADER_SIZE        sizeof(PageHeader)
#define MIN_PAGE_SIZE               SFS_DIRECTORY_ELEMENT_SIZE + SFS_PAGE_HEADER_SIZE // minimum size of one page in Kbytes

#define SFS_COMPRESSED_HEADER_SIZE  sizeof(SFSHeader) // size in bytes (16)
#define SFS_FILE_STREAM_HEADER_SIZE sizeof(SFSFileStreamHeader) // size in bytes (16)
#define SINGLE_FILE_HEADER_SIZE     sizeof(SingleFileHeader)

// Crc algrorithms
#define CRC_FAST                    0
#define CRC_FULL                    1
#define PASSWORD_MASK               "aFD:S.<@#Q$^562d"
#define MAX_PASSWORD_LENGTH         16

#define FM_CREATE                   1
#define FM_OPEN_READ                2
#define FM_OPEN_WRITE               4
#define FM_OPEN_READ_WRITE          FM_OPEN_READ | FM_OPEN_WRITE

#endif