#include "fileUtils/_fileExists.h"
#include "fileUtils/_FindFirst.h"
#if (defined(__GNUC__)  || defined(__GCCXML__)) && !defined(__WIN32)
#include <strings.h>
#else
#include <string.h>
#endif

bool _fileExists(const char *name)
{
    if (!name || !strlen(name))
        return false;
    FILE *file = fopen(name, "rb");
    if (!file)
        return false;
    fclose(file);
    return true;
}

bool _directoryExists(const char *name)
{
    _finddata_t f;
	char myName[250];
	strcpy(myName, name);
	for (int i=strlen(myName) - 1; i>0; i--)
	{
		if ((myName[i] == '/') || (myName[i] == '\\'))
			myName[i] = 0;
		else
			break;
	}
    long handle = _findfirst(myName, &f);
    if (handle < 0)
        return false;
    bool result = false;
    while (true)
    {
        if ((f.attrib & _A_SUBDIR) != 0)
        {
            result = true;
            break;
        }
        if (_findnext(handle, &f) < 0)
            break;
    }
    _findclose(handle);

    return result;
}
