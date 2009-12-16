//============================================================
//
//  winutils.h - Generic Win32 utility code in MESS
//
//============================================================

#ifndef WINUTILS_H
#define WINUTILS_H

#include <windows.h>
#include "osdcore.h"

//============================================================
//  MISC
//============================================================

DWORD win_get_file_attributes_utf8(const char *filename);

/* expand wildcards so '*' can be used */
void win_expand_wildcards(int *argc, char **argv[]);

#endif // WINUTILS_H
