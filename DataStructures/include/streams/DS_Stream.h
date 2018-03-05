#ifndef __STREAM_H
#define __STREAM_H

#include <string>
#include "CommonTypeDefines.h"

namespace DataStructures
{
	class Stream
	{
	public:
        Stream() : mForce32bitCompatable(false) {}

		virtual ~Stream() {}

        virtual bool CanRead() = 0;
        virtual bool CanWrite() = 0;
        virtual bool CanSeek() = 0;
        virtual void Close() = 0;

        virtual size_t Read(void *ptr, size_t size) = 0;
        virtual char *ReadLine(char *str, int num);
        virtual size_t Write(const void *ptr, size_t size) = 0;
        virtual bool Eof();
        virtual __s64 Length();
        virtual bool SetLength(__s64 size) = 0;
        virtual __s64 Position();
        virtual __s64 Seek(__s64 offset, int origin) = 0;
        virtual bool Rewind();
        virtual size_t CopyFrom(Stream *stream, size_t count);

		//helper function for Read/Write
        template<class T> size_t Write(const T &buffer) { return this->Write(&buffer, sizeof(T)); }
		template<class T> bool Read(T &dst) { return this->Read((void *)&dst, sizeof(T)) == sizeof(T); }

		virtual size_t WriteString(const char *buffer, size_t maxsize);
		virtual size_t ReadString(char *dst, size_t maxsize);
        
        inline const char *GetFileName() { return mFileName; }
 
        virtual bool SaveToStream(Stream *stream);
        virtual bool LoadFromStream(Stream *stream);
        virtual bool SaveToFile(const char *fileName);
        virtual bool LoadFromFile(const char *fileName);
        
        std::string GetAsString();
        void Force32bitCompatable(bool compatable) { mForce32bitCompatable = compatable; }
        
        virtual __u32 GetMemoryCost();
    protected:
        char mFileName[512];
        bool mForce32bitCompatable;
	};

}

#endif
