//============================================================
//
//	hexview.h - A Win32 hex editor control
//
//============================================================

#ifndef HEXVIEW_H
#define HEXVIEW_H

#include <windows.h>

extern const TCHAR hexview_class[];

BOOL hexview_registerclass(void);

BOOL hexview_setdata(HWND hexview, const void *data, size_t data_size);
const void *hexview_getdata(void);
size_t hexview_getdatasize(void);

#endif // HEXVIEW_H
