#include "HexString.h"
#include "SimpleMutex.h"
#include <string.h>
#include "LinuxStrings.h"
/// Bug: need to memset 0 for new byte array, when use Trim*()
///		 I have memset 0 to all which are needed   

#if defined(__GNUC__)
#define _vsnprintf vsnprintf
#endif

using namespace DataStructures;

HexString::SharedString HexString::emptyString = {0, 0, 0, (char *)"", ""};
DataStructures::ThreadSafeList<HexString::SharedString*> HexString::freeList;
bool HexString::needCaching = true;

void HexString::FreeCachedMemory()
{
	needCaching = false;
    DataStructures::List<HexString::SharedString*> *list = HexString::freeList.LockList();
	for (unsigned int i=0; i<list->Size(); i++)
	{
		DataStructures::HexString::SharedString *sharedString = (*list)[i];
		free(sharedString);
	}
	list->Clear();
	HexString::freeList.ReleaseList();
}

void HexString::Trim(void)
{
	TrimRight();
	TrimLeft();
}

void HexString::TrimLeft(void)
{
	if (IsEmpty())
		return;
	int count = 0;
	int size = (int)strlen(C_String());
	for (int i=0; i<size; i++)
		if (C_String()[i]==' ')
			count ++;
		else
			break;
	if (count==0)
		return;
	char buffer[1024];
	memset(buffer, 0, 1024);
	strncpy(buffer, C_String() + count, size - count);
	Free();
	Assign(buffer);
}

//very slow function, optimize later
void HexString::TrimRight(void)
{
	char buffer[1024];
	memset(buffer, 0, 1024);
	strcpy(buffer, C_String());
	for (int i=(int)strlen(buffer)-1; i>=0; i--)
		if (buffer[i]==' ')
			buffer[i] = 0;
		else
			break;
	Free();
	Assign(buffer);
}

int HexString::HexStringComp( HexString const &key, HexString const &data )
{
	return key.StrCmp(data);
}

HexString::HexString()
{
	sharedString = &emptyString;
}

HexString::HexString( HexString::SharedString *_sharedString )
{
	sharedString = _sharedString;
}

HexString::HexString(char input)
{
	char str[2];
	str[0] = input;
	str[1] = 0;
	Assign(str);
}

HexString::HexString(unsigned char input)
{
	char str[2];
	str[0] = (char) input;
	str[1] = 0;
	Assign(str);
}

HexString::HexString( const HexString & rhs)
{
	sharedString = rhs.sharedString;
	rhs.sharedString->refCount ++;
}

HexString::~HexString()
{
	Free();
}

HexString& HexString::operator = ( const HexString& rhs )
{
	Free();
	sharedString = rhs.sharedString;
	sharedString->refCount ++;	
	return *this;
}

HexString& HexString::operator = ( const char *str )
{
	Free();
	Assign(str);
	return *this;
}

HexString& HexString::operator = ( char *str )
{
	return operator = ((const char*)str);
}

void HexString::Realloc(SharedString *sharedString, size_t bytes)
{
	if (bytes <= sharedString->bytesUsed)
		return;
	assert(bytes>0);
	size_t oldBytes = sharedString->bytesUsed;
	size_t newBytes;
	const size_t smallStringSize = 128 - sizeof(unsigned int) - sizeof(size_t) - sizeof(char*) * 2;
	newBytes = GetSizeToAllocate(bytes);
	if (oldBytes <=(size_t)smallStringSize && newBytes > (size_t)smallStringSize)
	{
		sharedString->bigString = (char*)malloc(newBytes);
		strcpy(sharedString->bigString, sharedString->smallString);
		sharedString->c_str = sharedString->bigString;
	}
	else 
		if (oldBytes > smallStringSize)
		{
			sharedString->bigString = (char*)realloc(sharedString->bigString, newBytes);
			sharedString->c_str = sharedString->bigString;
		}
	sharedString->bytesUsed = newBytes;
}

HexString& HexString::operator +=( const HexString& rhs)
{
	if (rhs.IsEmpty())
		return *this;

	if (IsEmpty())
	{
		return operator = (rhs);
	}
	else
	{
		Clone();
		size_t strLen = rhs.GetLength() + GetLength() + 1;
		Realloc(sharedString, strLen + GetLength());
		strcat(sharedString->c_str, rhs.C_String());
	}
	return *this;
}

HexString& HexString::operator +=( const char *str )
{
	if (str==0 || str[0]==0)
		return *this;

	if (IsEmpty())
	{
		Assign(str);
	}
	else
	{
		Clone();
		size_t strLen = strlen(str) + GetLength() + 1;
		Realloc(sharedString, strLen);
		strcat(sharedString->c_str, str);
	}
	return *this;
}

HexString& HexString::operator +=( char *str )
{
	return operator += ((const char*)str);
}

unsigned char HexString::operator[] ( const unsigned int position ) const
{
	assert(position < GetLength());
	return sharedString->c_str[position];
}

bool HexString::operator==(const HexString &rhs) const
{
	return strcmp(sharedString->c_str, rhs.sharedString->c_str) == 0;
}

bool HexString::operator==(const char *str) const
{
	return strcmp(sharedString->c_str, str) == 0;
}

bool HexString::operator==(char *str) const
{
	return strcmp(sharedString->c_str, str) == 0;
}

bool HexString::operator!=(const HexString &rhs) const
{
	return strcmp(sharedString->c_str, rhs.sharedString->c_str) != 0;
}

const DataStructures::HexString operator + (const DataStructures::HexString &lhs, const DataStructures::HexString &rhs)
{
	if (lhs.IsEmpty() && rhs.IsEmpty())
		return HexString(&HexString::emptyString);
	if (lhs.IsEmpty())
	{
		rhs.sharedString->refCount ++;
		return HexString(rhs.sharedString);
	}
	if (rhs.IsEmpty())
	{
		lhs.sharedString->refCount ++;
		return HexString(lhs.sharedString);
	}

	size_t len1 = lhs.GetLength();
	size_t len2 = rhs.GetLength();
	size_t allocatedBytes = len1 + len2 + 1;
	allocatedBytes = HexString::GetSizeToAllocate(allocatedBytes);
	HexString::SharedString *_sharedString;

    DataStructures::List<HexString::SharedString*> *list = HexString::freeList.LockList();
	if (list->Size()==0)
	{
		unsigned i;
		for (i=0; i < 1024; i++)
			list->Insert((HexString::SharedString*)malloc(sizeof(HexString::SharedString)));
	}
	_sharedString = (*list)[list->Size()-1];
	list->RemoveAtIndex(list->Size()-1);
	HexString::freeList.ReleaseList();

	const int smallStringSize = 128 - sizeof(unsigned int) - sizeof(size_t) - sizeof(char*) * 2;
	_sharedString->bytesUsed = allocatedBytes;
	_sharedString->refCount = 1;
	if (allocatedBytes <= (size_t) smallStringSize)
	{
		_sharedString->c_str = _sharedString->smallString;
	}
	else
	{
		_sharedString->bigString = (char*)malloc(_sharedString->bytesUsed);
		_sharedString->c_str = _sharedString->bigString;
	}

	strcpy(_sharedString->c_str, lhs.C_String());
	strcat(_sharedString->c_str, rhs.C_String());

	return HexString(_sharedString);
}

void HexString::ToLower(void)
{
	Clone();

	size_t strLen = strlen(sharedString->c_str);
	unsigned i;
	for (i=0; i < strLen; i++)
		sharedString->c_str[i]=ToLower(sharedString->c_str[i]);
}

void HexString::ToUpper(void)
{
	Clone();

	size_t strLen = strlen(sharedString->c_str);
	unsigned i;
	for (i=0; i < strLen; i++)
		sharedString->c_str[i] = ToUpper(sharedString->c_str[i]);
}

void HexString::Set(const char *format, ...)
{
	char text[8096];
	memset(text, 0, 8096);
	va_list ap;
	va_start(ap, format);
	_vsnprintf(text, 8096, format, ap);
	va_end(ap);
	text[8096-1] = 0;
	Clear();
	Assign(text);
}

void HexString::SetLength(unsigned int len)
{
    if (GetLength() <= len)
        return;
    else
        sharedString->c_str[len] = 0;
}

bool HexString::IsEmpty(void) const
{
	return sharedString == &emptyString;
}

size_t HexString::GetLength(void) const
{
	return strlen(sharedString->c_str);
}

void HexString::Replace(unsigned index, unsigned count, unsigned char c)
{
	assert(index+count <= GetLength());
	Clone();
	unsigned countIndex = 0;
	while (countIndex < count)
	{
		sharedString->c_str[index] = c;
		index ++;
		countIndex ++;
	}
}

void HexString::Replace(unsigned index, unsigned count, const char *string)
{
	size_t length = GetLength();
	assert(index+count <= length);
	Clone();
    char buffer[8192];
	memset(buffer, 0, 8192);
	if (index + count == length)
		buffer[0] = 0;
	else
	    strcpy(buffer, sharedString->c_str + index + count);
	sharedString->c_str[index] = 0;
    strcat(sharedString->c_str, string);
    strcat(sharedString->c_str, buffer);
}

void HexString::Replace(char source, char c)
{
	Clone();
	for (unsigned int i=0; i<GetLength(); i++)
	{
		if (sharedString->c_str[i] == source)
			sharedString->c_str[i] = c;
	}
}

void HexString::Insert(unsigned index, const HexString &toAdd)
{
    size_t len = GetLength();
	assert(index <= len);
	Clone();
    if (index == len)
        strcpy(sharedString->c_str, toAdd.C_String());
    else
    {
        char buffer[8192];
		memset(buffer, 0, 8192);
        strcpy(buffer, (sharedString->c_str + index));
        sharedString->c_str[index] = 0;
        strcat(sharedString->c_str, toAdd.C_String());
        strcat(sharedString->c_str, buffer);
    }
}

void HexString::Insert(unsigned index, const char toAdd, unsigned int count)
{
    size_t len = GetLength();
	assert(index <= len);
    char buffer[8192];
    memset(buffer, toAdd, count);
    buffer[count] = 0;
    HexString tmp(buffer);
    Insert(index, tmp);
}

void HexString::Erase(unsigned index, unsigned count)
{
	assert(index+count < GetLength());
	Clone();
	unsigned i;
	for (i=index; i < count; i++)
	{
		sharedString->c_str[i] = sharedString->c_str[i + index];
	}
	sharedString->c_str[i] = 0;
}

int HexString::StrCmp(const HexString &rhs) const
{
	return strcmp(sharedString->c_str, rhs.sharedString->c_str);
}

int HexString::StrICmp(const HexString &rhs) const
{
	return _stricmp(sharedString->c_str, rhs.sharedString->c_str);
}

int HexString::IsSubString(const HexString &rhs) const
{
	char *str = strstr(rhs.sharedString->c_str, sharedString->c_str);
	if (str == 0)
		return -1;
	else
		return (int)(str - rhs.sharedString->c_str);
}

int HexString::IsSubIString(const HexString &rhs) const
{
	char source[8096];
	char sub[1024];
	memset(source, 0, 8096);
	memset(sub, 0, 1024);
	strcpy(sub, sharedString->c_str);
	strcpy(source, rhs.sharedString->c_str);
	char *str = strstr(source, sub);
	if (str == 0)
		return -1;
	else
		return (int)(str - source);
}

HexString HexString::SubString(unsigned int index, int length) const
{
    size_t len = GetLength();
    assert(index < GetLength());
    if (length == 0)
        return HexString();
    else
    {
        if (length < 0)
            length = (int)len;
        length = length > (int)len - index ? (int)len - index : length;
        char buffer[8192];
		memset(buffer, 0, 8192);
        strncpy(buffer, sharedString->c_str + index, length);
        return HexString(buffer);
    }
}

int HexString::FindString(const HexString &rhs) const
{
	char *str = strstr(sharedString->c_str, rhs.sharedString->c_str);
	if (str == 0)
        return -1;
	else
        return (int)(str - sharedString->c_str);
}

int HexString::FindStringReverse(const HexString &rhs) const
{
    int lastFound = -1;
	int length = GetLength();
    while (true)
    {
        char *str = sharedString->c_str;
        if ((lastFound > 0) && (lastFound < length - 1))
            str = sharedString->c_str + lastFound + 1;
        char *found = strstr(str, rhs.C_String());
        if (!found)
            break;
        else
            lastFound = (int)(found - sharedString->c_str);
    }
    return lastFound;
}

int HexString::FindString(const HexString &rhs, unsigned int index) const
{
    if (index >= GetLength())
        return -1;
    char *buffer = strstr(sharedString->c_str + index, rhs.C_String());
    if (!buffer)
        return -1;
    else
        return (int)(buffer - sharedString->c_str);
}

int HexString::FindIString(const HexString &rhs) const
{
	char source[8096];
	char sub[1024];
	memset(source, 0, 8096);
	memset(sub, 0, 1024);
	strcpy(source, sharedString->c_str);
	strcpy(sub, rhs.sharedString->c_str);
	char *str = strstr(source, sub);
	if (str == 0)
        return -1;
	else
        return (int)(str - source);
}


void HexString::Printf(void)
{
	printf("%s", sharedString->c_str);
}

void HexString::FPrintf(FILE *fp)
{
	fprintf(fp, "%s", sharedString->c_str);
}

bool HexString::IPAddressMatch(const char *IP)
{
	unsigned characterIndex;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return false;

	characterIndex = 0;

#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
	while ( true )
	{
		if (sharedString->c_str[ characterIndex ] == IP[ characterIndex ] )
		{
			// Equal characters
			if ( IP[ characterIndex ] == 0 )
			{
				// End of the string and the strings match

				return true;
			}

			characterIndex++;
		}

		else
		{
			if ( sharedString->c_str[ characterIndex ] == 0 || IP[ characterIndex ] == 0 )
			{
				// End of one of the strings
				break;
			}

			// Characters do not match
			if ( sharedString->c_str[ characterIndex ] == '*' )
			{
				// Domain is banned.
				return true;
			}

			// Characters do not match and it is not a *
			break;
		}
	}


	// No match found.
	return false;
}

void HexString::Clear(void)
{
	Free();
}

void HexString::Assign(const char *str)
{
	if (str==0 || str[0]==0)
	{
		sharedString = &emptyString;
		return;
	}

	size_t len = strlen(str) + 1;
    DataStructures::List<HexString::SharedString*> *list = HexString::freeList.LockList();
	if (list->Size()==0)
	{
		unsigned i;
		for (i=0; i < 1024; i++)
		{
			list->Insert((HexString::SharedString*)malloc(sizeof(HexString::SharedString)));
		}
	}
	sharedString = (*list)[list->Size() - 1];
	list->RemoveAtIndex(list->Size() - 1);
    HexString::freeList.ReleaseList();

	const size_t smallStringSize = 128 - sizeof(unsigned int) - sizeof(size_t) - sizeof(char*) * 2;
	sharedString->refCount = 1;
	if (len <= smallStringSize)
	{
		sharedString->bytesUsed = smallStringSize;
		sharedString->c_str = sharedString->smallString;
	}
	else
	{
		sharedString->bytesUsed = len << 1;
		sharedString->bigString = (char*)malloc(sharedString->bytesUsed);
		sharedString->c_str = sharedString->bigString;
	}
	memcpy(sharedString->c_str, str, len);
}

void HexString::Clone(void)
{
	// Empty or solo then no point to cloning
	if (sharedString==&emptyString || sharedString->refCount==1)
		return;

	sharedString->refCount --;
	Assign(sharedString->c_str);
}

void HexString::Free(void)
{
	if (sharedString == &emptyString)
		return;
	if (sharedString->refCount == 0)
		return;
	sharedString->refCount --;
	if (sharedString->refCount == 0)
	{
		const size_t smallStringSize = 128 - sizeof(unsigned int) - sizeof(size_t) - sizeof(char*) * 2;
		if (sharedString->bytesUsed > smallStringSize)
			free(sharedString->bigString);

		if (needCaching)
		{
			HexString::freeList.Insert(sharedString);
		}
		else
			free(sharedString);
	}
	sharedString = &emptyString;
}

unsigned char HexString::ToLower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return c-'A'+'a';
	return c;
}

unsigned char HexString::ToUpper(unsigned char c)
{
	if (c >= 'a' && c <= 'z')
		return c-'a'+'A';
	return c;
}


/*
int main(void)
{
	HexString s3("Hello world");
	HexString s5=s3;

	HexString s1;
	HexString s2('a');

	HexString s4("%i %f", 5, 6.0);
	MyFunc(s4);

	HexString s6=s3;
	HexString s7=s6;
	HexString s8=s6;
	HexString s9;
	s9=s9;
	HexString s10(s3);
	HexString s11=s10 + s4 + s9 + s2;
	s11+=HexString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	HexString s12("Test");
	s12+=s11;
	bool b1 = s12==s12;
	s11=s5;
	s12.ToUpper();
	s12.ToLower();
	HexString s13;
	bool b3 = s13.IsEmpty();
	s13.Set("blah %s", s12.C_String());
	bool b4 = s13.IsEmpty();
	size_t i1=s13.GetLength();
	s3.Clear();
	s4.Clear();
	s5.Clear();
	s5.Clear();
	MyFunc(s5);
	MyFunc(s6);
	s6.Printf();
	s7.Printf();
	printf("\n");

	static const int repeatCount=7500;
	DataStructures::List<HexString> hexStringList;
	DataStructures::List<std::string> stdStringList;
	DataStructures::List<char*> referenceStringList;
	char *c;
	unsigned i;
	HexTime beforeReferenceList, beforeHexString, beforeStdString, afterStdString;

	unsigned loop;
	for (loop=0; loop<2; loop++)
	{
		beforeReferenceList=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			c = new char [56];
			strcpy(c, "Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
			referenceStringList.Insert(c);
		}
		beforeHexString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			hexStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		beforeStdString=DataStructures::GetTime();

		for (i=0; i < repeatCount; i++)
			stdStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		afterStdString=DataStructures::GetTime();
		printf("Insertion 1 Ref=%i Hex=%i, Std=%i\n", beforeHexString-beforeReferenceList, beforeStdString-beforeHexString, afterStdString-beforeStdString);

		beforeReferenceList=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			delete referenceStringList[0];
			referenceStringList.RemoveAtIndex(0);
		}
		beforeHexString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			hexStringList.RemoveAtIndex(0);
		beforeStdString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.RemoveAtIndex(0);
		afterStdString=DataStructures::GetTime();
		printf("RemoveHead Ref=%i Hex=%i, Std=%i\n", beforeHexString-beforeReferenceList, beforeStdString-beforeHexString, afterStdString-beforeStdString);

		beforeReferenceList=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			c = new char [56];
			strcpy(c, "Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
			referenceStringList.Insert(0);
		}
		beforeHexString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			hexStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		beforeStdString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.Insert("Aalsdkj alsdjf laksdjf ;lasdfj ;lasjfd");
		afterStdString=DataStructures::GetTime();
		printf("Insertion 2 Ref=%i Hex=%i, Std=%i\n", beforeHexString-beforeReferenceList, beforeStdString-beforeHexString, afterStdString-beforeStdString);

		beforeReferenceList=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
		{
			delete [] referenceStringList[referenceStringList.Size()-1];
			referenceStringList.RemoveAtIndex(referenceStringList.Size()-1);
		}
		beforeHexString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			hexStringList.RemoveAtIndex(hexStringList.Size()-1);
		beforeStdString=DataStructures::GetTime();
		for (i=0; i < repeatCount; i++)
			stdStringList.RemoveAtIndex(stdStringList.Size()-1);
		afterStdString=DataStructures::GetTime();
		printf("RemoveTail Ref=%i Hex=%i, Std=%i\n", beforeHexString-beforeReferenceList, beforeStdString-beforeHexString, afterStdString-beforeStdString);

	}

	char str[128];
	gets(str);
	return 1;
	*/
