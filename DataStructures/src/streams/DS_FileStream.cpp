#include "streams/DS_FileStream.h"
#include "fileUtils/_FindFirst.h"

#ifdef __ANDROID__
#include <android/asset_manager.h>
extern AAssetManager* __assetManager;
#endif

#include "fileUtils/_splitpath.h"

namespace DataStructures
{
	//------------------------------------------------------------------- FileStream ------------------------------------------------------------------------
	FileStream::FileStream(const char *filePath, const char *mode)
		: Stream(), mFile(0), mCanRead(false), mCanWrite(false)
	{
		strcpy(mFileName, filePath);
		mFile = fopen(filePath, mode);
		
		if (mFile)
		{
			const char *s = mode;
			while (s != NULL && *s != '\0')
			{
				if (*s == 'r')
					mCanRead = true;
				else
					if (*s == 'w')
						mCanWrite = true;
				++s;
			}
		}
	}

	FileStream* FileStream::Create(const char *filePath, const char *mode)
	{
		FileStream *stream = new FileStream(filePath, mode);
		if (!stream->CanSeek())
		{
			delete stream;
			return 0;
		}
		else
			return stream;
	}

	void FileStream::Close()
	{
		if (mFile)
			fclose(mFile);
		mFile = 0;
	}

	size_t FileStream::Read(void *ptr, size_t size)
	{
		if (!mFile)
			return 0;
		size_t res = fread(ptr, 1, size, mFile);
		return res;
	}

	char *FileStream::ReadLine(char *str, int num)
	{
		if (!mFile)
			return 0;
		return fgets(str, num, mFile);
	}

	size_t FileStream::Write(const void *ptr, size_t size)
	{
		if (!mFile)
			return 0;
		size_t res = fwrite(ptr, size, 1, mFile);
		return res * size;
	}

	bool FileStream::Eof()
	{
		if (!mFile || feof(mFile))
			return true;
		return ((size_t)Position()) >= Length();
	}

	bool FileStream::SetLength(__s64 size)
	{
		if (!mFile)
			return false;
#ifdef WIN32
		int fd = _fileno(mFile);
#else
		int fd = fileno(mFile);
#endif
		return __chsize(fd, (long int)size) == 0;
	}

	__s64 FileStream::Seek(__s64 offset, int origin)
	{
		if (!mFile)
			return -1;
		if (fseek(mFile, (long int)offset, origin) == 0)
			return ftell(mFile);
		else
			return -1;
	}

	bool FileStream::Rewind()
	{
		if (CanSeek())
		{
			::rewind(mFile);
			return true;
		}
		return false;
	}

	__u32 FileStream::GetMemoryCost()
	{
		return Stream::GetMemoryCost() + sizeof(mCanRead) + sizeof(mCanWrite) + sizeof(mFile) + sizeof(*mFile);
	}

	#ifdef __ANDROID__

	FileStreamAndroid::FileStreamAndroid(AAsset* asset)
	: mAsset(asset)
	{
	}

	FileStreamAndroid::~FileStreamAndroid()
	{
		if (mAsset)
			close();
	}

	FileStreamAndroid* FileStreamAndroid::Create(const char *filePath, const char *mode)
	{
		AAsset* asset = AAssetManager_open(__assetManager, filePath, AASSET_MODE_RANDOM);
		if (asset)
		{
			FileStreamAndroid* stream = new FileStreamAndroid(asset);
			return stream;
		}
		return NULL;
	}

	bool FileStreamAndroid::CanRead()
	{
		return true;
	}

	bool FileStreamAndroid::CanWrite()
	{
		return false;
	}

	bool FileStreamAndroid::CanSeek()
	{
		return true;
	}

	void FileStreamAndroid::Close()
	{
		if (mAsset)
			AAsset_close(mAsset);
		mAsset = NULL;
	}

	size_t FileStreamAndroid::Read(void* ptr, size_t size, size_t count)
	{
		int result = AAsset_read(mAsset, ptr, size * count);
		return result > 0 ? ((size_t)result) / size : 0;
	}

	char *FileStreamAndroid::ReadLine(char *str, int num)
	{
		if (num <= 0)
			return NULL;
		char c = 0;
		size_t maxCharsToRead = num - 1;
		for (size_t i = 0; i < maxCharsToRead; ++i)
		{
			size_t result = read(&c, 1, 1);
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
				size_t pos = position();
				
				char nextChar = 0;
				if (read(&nextChar, 1, 1) != 1)
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
					seek(pos, SEEK_SET);
					str[i + 1] = '\0';
					break;
				}
			}
			str[i] = c;
		}
		return str; // what if first read failed?
	}

	size_t FileStreamAndroid::Write(const void* ptr, size_t size, size_t count)
	{
		return 0;
	}

	bool FileStreamAndroid::Eof()
	{
		return Position() >= Length();
	}

	size_t FileStreamAndroid::Length()
	{
		return (size_t)AAsset_getLength(mAsset);
	}

	long int FileStreamAndroid::Position()
	{
		return AAsset_getLength(mAsset) - AAsset_getRemainingLength(mAsset);
	}

	bool FileStreamAndroid::Seek(long int offset, int origin)
	{
		return AAsset_seek(mAsset, offset, origin) != -1;
	}

	bool FileStreamAndroid::Rewind()
	{
		if (CanSeek())
		{
			return AAsset_seek(mAsset, 0, SEEK_SET) != -1;
		}
		return false;
	}

	__u32 FileStream::GetMemoryCost()
	{
		return FileStreamAndroid::GetMemoryCost() + sizeof(mAsset) + sizeof(*mAsset);
	}
	#endif
}



