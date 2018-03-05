#ifndef GCC_FILE_EXISTS_H
#define GCC_FILE_EXISTS_H

#include <sys/types.h>
#include <stdio.h>

bool _fileExists(const char *name);
bool _directoryExists(const char *name);

#endif
