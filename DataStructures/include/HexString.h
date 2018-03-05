#ifndef __HEX_STRING_H
#define __HEX_STRING_H 

/// Bug: need to memset 0 for new byte array, when use Trim*()

#include "DS_List.h"
#include <stdio.h>
#include <stdarg.h>
#include "DS_ThreadSafeList.h"

#if defined(__GNUC__)
#define _vsnprintf vsnprintf
#endif
#pragma warning(disable:4996)

namespace DataStructures
{
    /// \brief String class
    /// Has the following improvements over std::string
    /// Reference counting: Suitable to store in lists
    /// Varidic assignment operator
    /// Doesn't cause linker errors
    class HexString
    {
        public:
        /// Constructors
        HexString();
        HexString(char input);
        HexString(unsigned char input);
        HexString(const unsigned char *format, ...)
        {
            char text[8096];
            va_list ap;
            va_start(ap, format);
            _vsnprintf(text, 8096, (const char*) format, ap);
            va_end(ap);
            text[8096-1]=0;
            Assign(text);
        }
        HexString(const char *format, ...){
            char text[8096];
            va_list ap;
            va_start(ap, format);
            _vsnprintf(text, 8096, format, ap);
            va_end(ap);
            text[8096-1]=0;
            Assign(text);
        }
        ~HexString();
        HexString( const HexString & rhs);
        HexString( const char *source, unsigned int maxLen )
        {
            Assign(source);
            SetLength(maxLen);
        }
        
        /// Implicit return of const char*
        operator const char *() const {return sharedString->c_str;}
        
        /// Same as std::string::c_str
        const char *C_String(void) const {return sharedString->c_str;}
        
        /// Assigment operators
        HexString& operator = ( const HexString& rhs );
        HexString& operator = ( const char *str );
        HexString& operator = ( char *str );
        
        /// Concatenation
        HexString& operator +=( const HexString& rhs);
        HexString& operator += ( const char *str );
        HexString& operator += ( char *str );
        
        /// Character index. Do not use to change the string however.
        unsigned char operator[] ( const unsigned int position ) const;
        
        /// Equality
        bool operator==(const HexString &rhs) const;
        bool operator==(const char *str) const;
        bool operator==(char *str) const;
        
        /// Inequality
        bool operator!=(const HexString &rhs) const;
        
        /// erase left and right spaces in string
        void Trim(void);
        
        /// erase left spaces in string
        void TrimLeft(void);
        
        /// erase right spaces in string
        void TrimRight(void);
        
        /// Change all characters to lowercase
        void ToLower(void);
        
        /// Change all characters to uppercase
        void ToUpper(void);
        
        /// Set the value of the string
        void Set(const char *format, ...);
        
        void SetLength(unsigned int len);
        
        /// Returns if the string is empty. Also, C_String() would return ""
        bool IsEmpty(void) const;
        
        /// Returns the length of the string
        size_t GetLength(void) const;
        
        /// Replace character(s) in starting at index, for count, with c
        void Replace(unsigned index, unsigned count, unsigned char c);
        
        /// Replace character(s) in starting at index, for count, with string
        void Replace(unsigned index, unsigned count, const char *string);

        void Replace(char source, char c);

		/// Insert character(s) in starting at index
        void Insert(unsigned index, const HexString &toAdd);

        /// Insert character(s) in starting at index with specific count
        void Insert(unsigned index, const char toAdd, unsigned int count);

        /// Erase characters out of the string at index for count
        void Erase(unsigned index, unsigned count);
        
        /// Compare strings (case sensitive)
        int StrCmp(const HexString &rhs) const;
        
        /// Compare strings (not case sensitive)
        int StrICmp(const HexString &rhs) const;
        
        /// find sub string in given source (case sensitive)
        int IsSubString(const HexString &rhs) const;
        
        /// find sub string in given source (not case sensitive)
        int IsSubIString(const HexString &rhs) const;
        
        /// create a new string started with index
        HexString SubString(unsigned int index, int length = -1) const;

        /// find sub string in this (case sensitive)
        int FindString(const HexString &rhs) const;
        
        /// find sub string in this (not case sensitive)
        int FindIString(const HexString &rhs) const;

        /// find sub string in this from the end (case sensitive)
        int FindStringReverse(const HexString &rhs) const;

        /// find sub string in this (case sensitive)
        int FindString(const HexString &rhs, unsigned int index) const;
        
        /// Clear the string
        void Clear(void);
        
        /// Print the string to the screen
        void Printf(void);
        
        /// Print the string to a file
        void FPrintf(FILE *fp);
        
        /// Does the given IP address match the IP address encoded into this string, accounting for wildcards?
        bool IPAddressMatch(const char *IP);
        
        /// \internal
        static size_t GetSizeToAllocate(size_t bytes)
        {
            const size_t smallStringSize = 128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2;
            if (bytes<=smallStringSize)
			return smallStringSize;
            else
			return bytes*2;
        }
        
        /// \internal
        struct SharedString
        {
            unsigned int refCount;
            size_t bytesUsed;
            char *bigString;
            char *c_str;
            char smallString[128-sizeof(unsigned int)-sizeof(size_t)-sizeof(char*)*2];
        };
        
        /// \internal
        HexString( SharedString *_sharedString );
        
        /// \internal
        SharedString *sharedString;
        
        //	static SimpleMutex poolMutex;
        //	static DataStructures::MemoryPool<SharedString> pool;
        /// \internal
        static SharedString emptyString;
        
        //static SharedString *sharedStringFreeList;
        //static unsigned int sharedStringFreeListAllocationCount;
        /// \internal
        /// List of free objects to reduce memory reallocations
        static DataStructures::ThreadSafeList<SharedString*> freeList;
        
        static int HexStringComp( HexString const &key, HexString const &data );
        
        static void FreeCachedMemory();
        static void EnableCaching() { needCaching = true; }
    protected:
        void Assign(const char *str);
        void Clone(void);
        void Free(void);
        unsigned char ToLower(unsigned char c);
        unsigned char ToUpper(unsigned char c);
        void Realloc(SharedString *sharedString, size_t bytes);
        
        static bool needCaching;
    };
    
}

const DataStructures::HexString operator+(const DataStructures::HexString &lhs, const DataStructures::HexString &rhs);

#endif
