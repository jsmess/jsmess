//============================================================
//
//	winutils.h - Generic Win32 utility code
//
//============================================================

#ifndef WINUTILS_H
#define WINUTILS_H

#include <windows.h>
#include "osdcore.h"

// Misc
void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line);

#endif // WINUTILS_H
