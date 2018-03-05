#ifndef __MEMORY_STREAM_H
#define __MEMORY_STREAM_H

#include "streams/DS_Stream.h"
#include "CommonTypeDefines.h"

#define MEMORYSTREAM_STACK_ALLOCATION_SIZE 256

namespace DataStructures
{
	class MemoryStream : public Stream
	{
	public:
        MemoryStream(const char *filePath);
		inline MemoryStream() : Stream(), m_currentPosition(0), m_length(MEMORYSTREAM_STACK_ALLOCATION_SIZE), m_usedLength(0), m_imported(false) { m_data = (char *)malloc(MEMORYSTREAM_STACK_ALLOCATION_SIZE); }
		inline MemoryStream(char *data, __u32 size) : m_data(data), m_currentPosition(0), m_length(size), m_usedLength(size), m_imported(true) {}
		inline virtual ~MemoryStream() { Clear(); }

		size_t Grow(int growth);
		void Reset();
        
        inline virtual bool CanRead() { return true; }
        inline virtual bool CanWrite() { return true; }
        inline virtual bool CanSeek() { return true; }
        inline virtual void Close() { Clear(); }
        
        virtual size_t Read(void *ptr, size_t size);
        virtual size_t Write(const void *ptr, size_t size);
        virtual bool Eof() { return (m_currentPosition >= (__u32)m_usedLength); }
        virtual __s64 Length() { return m_usedLength; }
        virtual bool SetLength(__s64 size);
        virtual __s64 Position() { return m_currentPosition; }
        virtual __s64 Seek(__s64 offset, int origin);

		inline __u32 GetAllocatedSize() { return (__u32)m_length; }
		inline const char *GetData() { return m_data; }
		inline const char *GetCurrentDataPointer() { return m_data + m_currentPosition; }

		//helper function for Read/Write
        template<class T> size_t Write(const T &buffer) { return this->Write(&buffer, sizeof(T)); }
		template<class T> bool Read(T &dst) { return this->Read((void *)&dst, sizeof(T)) == sizeof(T); }

        virtual __u32 GetMemoryCost();
    protected:
        inline virtual void Clear();
	private:
		char *m_data;
		__u32 m_currentPosition;
		__s32 m_length;
		__s32 m_usedLength;
		const bool m_imported;
	};

	void MemoryStream::Clear()
	{
		if (m_data && !m_imported)
			free(m_data);
		m_data = 0;
		*((bool *)&m_imported) = false;
		m_length = 0;
		m_usedLength = 0;
		m_currentPosition = 0;
	}

}

#endif
