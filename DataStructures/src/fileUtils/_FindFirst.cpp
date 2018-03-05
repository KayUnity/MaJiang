/**
* Original file by the_viking, fixed by R√¥mulo Fernandes, fixed by Emmanuel Nars
* Should emulate windows finddata structure
*/
#if (defined(__GNUC__)  || defined(__GCCXML__)) && !defined(__WIN32)
#include "fileUtils/_FindFirst.h"
#include "DS_List.h"
// Not supported on a console
// #include <fnmatch.h>
#include <string.h>
#include <sys/stat.h>

static DataStructures::List< _findinfo_t *> fileInfo;
	
bool _match_spec(const char *spec, const char *text)
{
    //If the whole specification string was consumed and the input text is also exhausted: it's a match.
    if (spec[0] == '\0' && text[0] == '\0')
        return true;
    
    // A star matches 0 or more characters.
    if (spec[0] == '*')
    {
        //Skip the star and try to find a match after it by successively incrementing the text pointer.
        do
        {
            if (_match_spec(spec + 1, text))
                return true;
        }
        while (*text++ != '\0');
    }
    
    // An interrogation mark matches any character. Other characters match themself.
    // Also, if the input text is exhausted but the specification isn't, there is no match.
    if (text[0] != '\0' && (spec[0] == '?' || spec[0] == text[0]))
        return _match_spec(spec + 1, text + 1);
    
    return false;
}

bool match_spec(const char *spec, const char *text)
{
    // On Windows, *.* matches everything.
    if (strcmp(spec, "*.*") == 0)
        return true;
    
    return _match_spec(spec, text);
}

long _findfirst(const char *name, _finddata_t *f)
{

	//   char *nameCopy = new char[sizeof(name)];
	//   memset(nameCopy, '\0', sizeof(nameCopy));
	//   
	//   strcpy(nameCopy, name);
	//
	//   char *filter = new char[sizeof(nameCopy)];
	//   memset(filter, '\0', sizeof(filter));

    f->attrib = 0;
    
	int length = (int)strlen(name)+1;
	char *nameCopy = (char*) malloc( length );
	memset(nameCopy, '\0', length);

	strcpy(nameCopy, name);

	char *filter = (char*) malloc( length );
	memset(filter, '\0', length);

	char *lastSep = strrchr(nameCopy, '/');
	if (!lastSep)
	{
		strcpy(filter, nameCopy);
		strcpy(nameCopy, ".");
	}
	else
	{
		strcpy(filter, lastSep+1);
		*lastSep = 0;
	}

	DIR *dir = opendir(nameCopy);

	if (!dir)
	{
        free(nameCopy);
        free(filter);
		return -1;
	}

    free(nameCopy);
    
	_findinfo_t *fi = new _findinfo_t;
	strcpy(fi->filter, filter);
	fi->openedDir = dir;
    free(filter);
    
    bool found = false;
    
	while (true)
	{
		dirent *entry = readdir(dir);
		if (entry == 0)
			break;
		
		// Commented code not supported on a console
        //	if(fnmatch(fi->filter,entry->d_name, 200) == 0)
		//if (strcasecmp(fi->filter,entry->d_name)==0)
		if (match_spec(fi->filter, entry->d_name))
		{
            found = true;
			strcpy(f->name, entry->d_name);
            if (entry->d_type & DT_DIR)
                f->attrib |= _A_SUBDIR;
            else
                f->attrib &= ~_A_SUBDIR;
			if (entry->d_type & DT_REG)
				f->attrib |= _A_NORMAL;
            else
                f->attrib &= ~_A_NORMAL;
			break;
		}
	}

    if (found)
        fileInfo.Insert(fi);
    else
        delete fi;
	return fileInfo.Size()-1;
}

#ifdef _PS3
int _findnext(long h, _finddata_t *f)
{

	_findinfo_t *fi = fileInfo[h];

    f->attrib = 0;
	while (true)
	{
		dirent *entry = readdir(fi->openedDir);
		if(entry == 0)
			return -1;

		// Commented code not supported on a console
		if(match_spec(fi->filter,entry->d_name))
		{
			strcpy(f->name, entry->d_name);
			if (entry->d_type & DT_REG)
				f->attrib |= _A_NORMAL;
			f->size = entry->d_reclen;
			return 0;
		}
		if (entry->d_type & DT_DIR)
		{
			f->attrib |= _A_SUBDIR;
			strcpy(f->name, entry->d_name);
			return 0;
		}
        else
            f->attrib &= ~_A_SUBDIR;
	}

	return -1;
}
#else
int _findnext(long h, _finddata_t *f)
{

	_findinfo_t *fi = fileInfo[(unsigned int)h];

	while (true)
	{
		dirent *entry = readdir(fi->openedDir);
		if(entry == 0)
			return -1;

		struct stat filestat;
		stat(entry->d_name, &filestat);

		// Commented code not supported on a console
		if (match_spec(fi->filter, entry->d_name))
		{
			strcpy(f->name, entry->d_name);
			if (entry->d_type &= DT_REG)            // when readdir, the d_type of folder is still unknow type
			//if (entry &= DT_REG)
            //if (S_ISREG(filestat.st_mode))
                f->attrib = _A_NORMAL;
            //f->attrib |= _A_NORMAL;
            if (entry->d_type == 0 || entry->d_type& DT_DIR)
                //if (S_ISDIR(filestat.st_mode))
            {
                f->attrib = _A_SUBDIR;

            }

			//f->size = entry->d_reclen;
			f->size = filestat.st_size;
			return 0;
		}
//		if (entry->d_type & DT_DIR)
//		//if (S_ISDIR(filestat.st_mode))
//		{
//			f->attrib |= _A_SUBDIR;
//			strcpy(f->name, entry->d_name);
//			return 0;
//		}
	}

	return -1;
}
#endif

/**
* _findclose - equivalent
*/
int _findclose(long h)
{
        if ((int) fileInfo.Size()>h && h>=0)
	{
		_findinfo_t *fi = fileInfo[(unsigned int)h];
        closedir(fi->openedDir);
		fileInfo.RemoveAtIndex((unsigned int)h);
		delete fi;
		return 0;	
	}
	else
	{
		printf("Error _findclose\n");
		return -1;
	}
	
}
#endif
