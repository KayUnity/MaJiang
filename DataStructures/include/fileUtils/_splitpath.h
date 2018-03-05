#ifndef GCC_SPLITPATH_H
#define GCC_SPLITPATH_H

#if (defined(__GNUC__)  || defined(__GCCXML__)) && !defined(__WIN32)

void _splitpath(const char *path, char *drive, char *directory, char *filename, char *extension);

#else

#include <stdio.h>
#define _splitpath  _splitpath

#endif

#endif
