//============================================================
//
//  winutil.h - Win32 OSD core utility functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WINUTIL__
#define __WINUTIL__

#include "osdcore.h"

// UTF-8 wrappers for Win32 APIs
int win_message_box_utf8(HWND window, const char *text, const char *caption, UINT type);
BOOL win_set_window_text_utf8(HWND window, const char *text);
BOOL win_get_window_text_utf8(HWND window, char *buffer, size_t buffer_size);
void win_output_debug_string_utf8(const char *string);
DWORD win_get_module_file_name_utf8(HMODULE module, char *filename, DWORD size);

// Shared code
file_error win_error_to_file_error(DWORD error);
osd_dir_entry_type win_attributes_to_entry_type(DWORD attributes);
BOOL win_is_gui_application(void);

#endif // __WINUTIL__
