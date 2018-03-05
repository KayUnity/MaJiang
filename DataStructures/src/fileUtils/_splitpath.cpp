#if !defined _WIN32 && !defined _WIN64

#include <string.h>

//Wraps the system implementation of splitpath(), and guarantees that the
//	resulting directory path contains only '/' instead of '\\'.
void _splitpath(const char *path, char *drive, char *directory, char *filename, char *extension)
{
	// The drive name is never valid on a Mac.
	drive[0] = '\0';
    
	// Copy the full filename to the directory buffer.  That's where we'll
	// start screwing with it.
	strcpy(directory, path);
    
	// Fix any backslashes that occur in the path.
    for (unsigned int i=0; i<strlen(path); i++)
        if (directory[i] == '\\')
            directory[i] = '/';
    
	// Try to find the last '/' in the filename.
    char *pFilename = strchr(directory, '/');
    int lastPos;
    int len = strlen(directory);
    while (pFilename)
    {
        lastPos = pFilename - directory;
        if (lastPos == len - 1)
            break;
        char *lastFound = pFilename;
        pFilename = strchr(pFilename + 1, '/');
        if (!pFilename)
        {
            pFilename = lastFound;
            break;
        }
    }
	// If there is one, then the file name has to come after the '/'.
	if (pFilename != 0)
    {
        strncpy(directory, path, lastPos + 1);
        strcpy(filename, pFilename + 1);
        directory[lastPos + 1] = 0;
    }
    else
    {
        directory[0] = 0;
        strcpy(filename, path);
    }

    char *pExtension = strchr(filename, '.');
    if (!pExtension)
        extension[0] = 0;
    else
    {
        lastPos = pExtension - filename;
        len = strlen(filename);
        while (pExtension)
        {
            lastPos = pExtension - filename;
            if (lastPos == len - 1)
                break;
            char *lastFound = pExtension;
            pExtension = strchr(pExtension + 1, '.');
            if (!pExtension)
            {
                pExtension = lastFound;
                break;
            }
        }
        strcpy(extension, pExtension);
        filename[lastPos] = 0;
    }
}
#endif
