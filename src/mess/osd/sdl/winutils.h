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

#endif // WINUTILS_H
