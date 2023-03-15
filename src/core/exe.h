#ifndef AUTOTOOLS_SETUP_EXE_H
#define AUTOTOOLS_SETUP_EXE_H

#include <stdlib.h>
#include <stdbool.h>

#include "../autotools-setup.h"

int exe_search(const char * commandName, char *** listP, size_t * listSize, bool findAll);
int exe_lookup(const char * commandName, char **  pathP, size_t * pathLength);
int exe_lookup2(const char * commandName, char buf[], size_t * writtenSize, size_t maxSize);

#endif
