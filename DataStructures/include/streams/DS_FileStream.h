#ifndef __FILE_STREAM_H
#define __FILE_STREAM_H

#include "streams/DS_Stream.h"
#include "CommonTypeDefines.h"

#define MEMORYSTREAM_STACK_ALLOCATION_SIZE 256

#define WILD_CARD_MULTIPLE_CHAR     "*"
#define WILD_CARD_SINGLE_CHAR       "?"
#define WILD_CARD_ANYFILE           "*.*"

namespace DataStructures
{
	class FileStream : public Stream
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
        
        FileStream(const char *filePath, const char *mode);
		inline virtual ~FileStream() { Close(); }
        
        inline virtual bool CanRead() { return mFile && mCanRead; }
        inline virtual bool CanWrite() { return mFile && mCanWrite; }
        inline virtual bool CanSeek() { return mFile != NULL; }
        virtual void Close();
        
        virtual size_t Read(void *ptr, size_t size);
        virtual char *ReadLine(char *str, int num);
        virtual size_t Write(const void *ptr, size_t size);
        
        virtual bool Eof();
        virtual bool SetLength(__s64 size);
        
        virtual __s64 Seek(__s64 offset, int origin);
        virtual bool Rewind();
        
        static FileStream* Create(const char *filePath, const char *mode);

        //helper function for Read/Write
        template<class T> size_t Write(const T &buffer) { return this->Write(&buffer, sizeof(T)); }
		template<class T> bool Read(T &dst) { return this->Read((void *)&dst, sizeof(T)) == sizeof(T); }

        virtual __u32 GetMemoryCost();
    private:
        FILE *mFile;
        bool mCanRead;
        bool mCanWrite;
    };
    
#ifdef __ANDROID__
    
    class FileStreamAndroid : public Stream
    {
    public:
        FileStreamAndroid(AAsset* asset);
        
        ~FileStreamAndroid();
        virtual bool CanRead();
        virtual bool CanWrite();
        virtual bool CanSeek();
        virtual void Close();
        virtual size_t Read(void *ptr, size_t size);
        virtual char *ReadLine(char *str, int num);
        virtual size_t Write(const void *ptr, size_t size);
        virtual bool Eof();
        virtual size_t Length();
        virtual long int Position();
        virtual __s64 Seek(long int offset, int origin);
        virtual bool Rewind();
        
        static FileStreamAndroid* Create(const char *filePath, const char *mode);
        
		//helper function for Read/Write
        template<class T> size_t Write(const T &buffer) { return this->Write(&buffer, sizeof(T)); }
		template<class T> bool Read(T &dst) { return this->Read((void *)&dst, sizeof(T)) == sizeof(T); }

        virtual __u32 GetMemoryCost();
    private:
        AAsset* mAsset;
    };
    
#endif
    
}

#endif
