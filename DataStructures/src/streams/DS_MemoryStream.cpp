#include "streams/DS_MemoryStream.h"
#include "streams/DS_FileStream.h"
#include <algorithm>
using std::min;

using namespace DataStructures;

//------------------------------------------------------------------- MemoryStream ------------------------------------------------------------------------
MemoryStream::MemoryStream(const char *filePath)
    : Stream(), m_currentPosition(0), m_length(0), m_usedLength(0), m_imported(false)
{
    strcpy(mFileName, filePath);
    FileStream *stream = new FileStream(filePath, "rb");
    if (!stream->CanSeek())
    {
        delete stream;
        m_length = MEMORYSTREAM_STACK_ALLOCATION_SIZE;
        m_data = (char *)malloc(MEMORYSTREAM_STACK_ALLOCATION_SIZE);
    }
    else
    {
        m_length = (__u32)stream->Length();
        m_data = (char *)malloc(m_length);
        stream->Rewind();
        stream->Read(m_data, m_length);
        m_usedLength = m_length;
        delete stream;
    }
}

size_t MemoryStream::Grow(int growth)
{
	if (!growth)
		return (__u32)m_length;
	m_length = m_length + growth;
	if (m_length<0)
	{
		Reset();
		return 0;
	}
	m_data = (char *)realloc(m_data, m_length);
	if (m_usedLength > m_length)
		m_usedLength = m_length;
	if (m_currentPosition >= (__u32)m_usedLength)
		m_currentPosition = (__u32)m_usedLength;
	return GetAllocatedSize();
}

void MemoryStream::Reset()
{
	m_currentPosition = 0;
	m_usedLength = 0;
}

__s64 MemoryStream::Seek(__s64 position, int option)
{
	__s32 actPosition;
	switch (option)
	{
	case SEEK_SET:
		if (position < 0)
			return false;
		if (position >= m_usedLength)
			return false;
		m_currentPosition = (__u32)position;
		break;
	case SEEK_CUR:
		actPosition = (__s32)(m_currentPosition + position);
		if (actPosition < 0)
			break;
		if (actPosition >= m_usedLength)
			break;
		m_currentPosition = actPosition;
		break;
	case SEEK_END:
		actPosition = (__s32)(m_usedLength - position);
		if (actPosition < 0)
			break;
		if (actPosition >= m_usedLength)
			break;
		m_currentPosition = actPosition;
		break;
	}
	return m_currentPosition;
}

size_t MemoryStream::Write(const void *buffer, size_t size)
{
	__s32 sizeGap = (__s32)(m_currentPosition + size - m_length);
	if (sizeGap>0)
		Grow(sizeGap + MEMORYSTREAM_STACK_ALLOCATION_SIZE);
	memmove(m_data + m_currentPosition, buffer, size);
	m_currentPosition += size;
	if (m_currentPosition > (__u32)m_usedLength)
		m_usedLength = m_currentPosition;
	return size;
}

size_t MemoryStream::Read(void *dst, size_t size)
{
	if (Eof() || !size)
		return 0;
	__u32 actSize = static_cast<__u32>(min((size_t)(m_usedLength - m_currentPosition), size));
	memcpy(dst, m_data + m_currentPosition, actSize);
	m_currentPosition += actSize;
	return actSize;
}

bool MemoryStream::SetLength(__s64 size)
{
    if (!CanSeek())
        return false;
    if (size > m_length)
    {
        m_data = (char *)realloc(m_data, m_length);
        m_length = (__u32)size;
    }
    m_usedLength = (__u32)size;
	if (m_currentPosition >= (__u32)m_usedLength)
		m_currentPosition = (__u32)m_usedLength;
    return true;
}

__u32 MemoryStream::GetMemoryCost()
{
    __u32 cost = Stream::GetMemoryCost();
    
    cost += sizeof(m_data);
    cost += sizeof(__u32) * 3;
    cost += sizeof(bool);
    cost += m_length;
    
    return cost;
}

