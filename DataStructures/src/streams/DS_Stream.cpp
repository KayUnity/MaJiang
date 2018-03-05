#include "CommonTypeDefines.h"
#include "streams/DS_FileStream.h"
#include "fileUtils/_fileExists.h"
#include <algorithm>
using std::min;

namespace DataStructures	
{
	//------------------------------------------------------------------- Stream ------------------------------------------------------------------------
	#if __WORDSIZE == 64
	template <>
	size_t Stream::Write(const unsigned int &buffer)
	{
		if (mForce32bitCompatable)
		{
			__u32 val = buffer;
			return this->Write(&val, sizeof(__u32));
		}
		else
			return this->Write(&buffer, sizeof(unsigned int));
	}

	template <>
	size_t Stream::Write(const int &buffer)
	{
		if (mForce32bitCompatable)
		{
			__s32 val = buffer;
			return this->Write(&val, sizeof(__s32));
		}
		else
			return this->Write(&buffer, sizeof(int));
	}

	template <>
	size_t Stream::Write(const unsigned long &buffer)
	{
		if (mForce32bitCompatable)
		{
			__u32 val = buffer;
			return this->Write(&val, sizeof(__u32));
		}
		else
			return this->Write(&buffer, sizeof(unsigned long));
	}

	template <>
	size_t Stream::Write(const long &buffer)
	{
		if (mForce32bitCompatable)
		{
			__s32 val = buffer;
			return this->Write(&val, sizeof(__s32));
		}
		else
			return this->Write(&buffer, sizeof(long));
	}

	template <>
	bool Stream::Read(unsigned int &dst)
	{
		if (mForce32bitCompatable)
		{
			__u32 t;
			bool res = this->Read(&t, sizeof(__u32)) == sizeof(__u32);
			dst = t;
			return res;
		}
		else
			return this->Read((void *)&dst, sizeof(unsigned int)) == sizeof(unsigned int);
	}

	template <>
	bool Stream::Read(int &dst)
	{
		if (mForce32bitCompatable)
		{
			__s32 t;
			bool res = this->Read(&t, sizeof(__s32)) == sizeof(__s32);
			dst = t;
			return res;
		}
		else
			return this->Read((void *)&dst, sizeof(int)) == sizeof(int);
	}

	template <>
	bool Stream::Read(unsigned long &dst)
	{
		if (mForce32bitCompatable)
		{
			__u32 t;
			bool res = this->Read(&t, sizeof(__u32)) == sizeof(__u32);
			dst = t;
			return res;
		}
		else
			return this->Read((void *)&dst, sizeof(unsigned long)) == sizeof(unsigned long);
	}

	template <>
	bool Stream::Read(long &dst)
	{
		if (mForce32bitCompatable)
		{
			__s32 t;
			bool res = this->Read(&t, sizeof(__s32)) == sizeof(__s32);
			dst = t;
			return res;
		}
		else
			return this->Read((void *)&dst, sizeof(long)) == sizeof(long);
	}
	#endif

	size_t Stream::WriteString(const char *buffer, size_t maxsize)
	{
		if (!CanWrite())
			return 0;
		__u16 size = min(strlen(buffer), maxsize + sizeof(__u16));
		size_t actSize = this->Write(&size, sizeof(size));
		actSize += this->Write(buffer, size);
		return actSize;
	}

	size_t Stream::ReadString(char *dst, size_t maxsize)
	{
		if (Eof() || !dst || !CanRead())
			return 0;
		__u16 size;
		size_t readIn = this->Read(&size, sizeof(size));
		readIn += this->Read(dst, min((size_t)size, maxsize));
		dst[size] = 0;
		return readIn;
	}

	size_t Stream::CopyFrom(Stream *stream, size_t count)
	{
		const size_t _max_buffer_size = 0xf0000;
		size_t size = count;
		if (count == 0)
		{
			stream->Seek(0, SEEK_SET);
			size = stream->Length();
		}
		size_t result = size;
		size_t bufferSize;
		if (size > _max_buffer_size)
			bufferSize = _max_buffer_size;
		else
			bufferSize = size;
		char *buffer = (char *)malloc(bufferSize);
		size_t n;
		while (size != 0)
		{
			if (size > bufferSize)
				n = bufferSize;
			else
				n = size;
			stream->Read(buffer, n);
			Write(buffer, n);
			size -= n;
		}
		free(buffer);
		return result;
	}

	bool Stream::SaveToStream(Stream *stream)
	{
		if (!stream)
			return false;
		__s64 oldPos = Position();
		__s64 oldPos1 = stream->Position();
		bool res = stream->CopyFrom(this, 0) == Length();
		Seek(oldPos, SEEK_SET);
		stream->Seek(oldPos1, SEEK_SET);
		return res;
	}

	bool Stream::LoadFromStream(Stream *stream)
	{
		if (!stream)
			return false;
		__s64 oldPos = Position();
		__s64 oldPos1 = stream->Position();
		size_t size = stream->Length();
		SetLength(size);
		Seek(0, SEEK_SET);
		bool res = CopyFrom(stream, 0) == size;
		Seek(oldPos, SEEK_SET);
		stream->Seek(oldPos1, SEEK_SET);
		return res;
	}

	bool Stream::SaveToFile(const char *fileName)
	{
		FileStream *stream = new FileStream(fileName, "wb+");
		bool result = SaveToStream(stream);
		stream->Close();
		delete stream;
		
		return result;
	}

	bool Stream::LoadFromFile(const char *fileName)
	{
		if (!_fileExists(fileName))
			return false;
		FileStream *stream = new FileStream(fileName, "rb");
		bool result = LoadFromStream(stream);
		stream->Close();
		delete stream;
		
		return result;
	}

	__s64 Stream::Length()
	{
		__s64 oldPos = Seek(0, SEEK_CUR);
		__s64 length = Seek(0, SEEK_END);
		Seek(oldPos, SEEK_SET);
		return length;
	}

	__s64 Stream::Position()
	{
		return Seek(0, SEEK_CUR);
	}

	char *Stream::ReadLine(char *str, int num)
	{
		if (num <= 0)
			return NULL;
		char c = 0;
		size_t maxCharsToRead = num - 1;
		for (size_t i = 0; i < maxCharsToRead; ++i)
		{
			size_t result = Read(&c, 1);
			if (result != 1)
			{
				str[i] = '\0';
				break;
			}
			if (c == '\n')
			{
				str[i] = c;
				str[i + 1] = '\0';
				break;
			}
			else if(c == '\r')
			{
				str[i] = c;
				// next may be '\n'
				size_t pos = Position();
				
				char nextChar = 0;
				if (Read(&nextChar, 1) != 1)
				{
					// no more characters
					str[i + 1] = '\0';
					break;
				}
				if (nextChar == '\n')
				{
					if (i == maxCharsToRead - 1)
					{
						str[i + 1] = '\0';
						break;
					}
					else
					{
						str[i + 1] = nextChar;
						str[i + 2] = '\0';
						break;
					}
				}
				else
				{
					Seek(pos, SEEK_SET);
					str[i + 1] = '\0';
					break;
				}
			}
			str[i] = c;
		}
		return str; // what if first read failed?
	}

	bool Stream::Eof()
	{
		return Position() >= Length();
	}

	bool Stream::Rewind()
	{
		if (!CanSeek())
			return 0;
		else
			return Seek(0, SEEK_SET) == 0;
	}

	std::string Stream::GetAsString()
	{
		char *buffer;
		__s64 fileLen = Length();
		__s64 oldPosition = Position();
		buffer = (char *)malloc(fileLen + 1);
		Seek(0, SEEK_SET);
		Read(buffer, fileLen);
		Seek(oldPosition, SEEK_SET);
		buffer[fileLen] = 0;
		std::string result(buffer);
		free(buffer);
		return result;
	}

	__u32 Stream::GetMemoryCost()
	{
		//this value is not very accurate because of class HexString's cache using
		return static_cast<__u32>(sizeof(*this) + strlen(mFileName));
	}
}