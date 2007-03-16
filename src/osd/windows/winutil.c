//============================================================
//
//  winutil.c - Win32 OSD core utility functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAMEOS headers
#include "winutil.h"
#include "strconv.h"

//============================================================
//  win_message_box_utf8
//============================================================

int win_message_box_utf8(HWND window, const char *text, const char *caption, UINT type)
{
	int result = IDNO;
	TCHAR *t_text = NULL;
	TCHAR *t_caption = NULL;

	if (text)
	{
		t_text = tstring_from_utf8(text);
		if (!t_text)
			goto done;
	}

	if (caption)
	{
		t_caption = tstring_from_utf8(caption);
		if (!t_caption)
			goto done;
	}

	result = MessageBox(window, t_text, t_caption, type);

done:
	if (t_text)
		free(t_text);
	if (t_caption)
		free(t_caption);
	return result;
}



//============================================================
//  win_set_window_text_utf8
//============================================================

BOOL win_set_window_text_utf8(HWND window, const char *text)
{
	BOOL result = FALSE;
	TCHAR *t_text = NULL;

	if (text)
	{
		t_text = tstring_from_utf8(text);
		if (!t_text)
			goto done;
	}

	result = SetWindowText(window, t_text);

done:
	if (t_text)
		free(t_text);
	return result;
}



//============================================================
//  win_get_window_text_utf8
//============================================================

BOOL win_get_window_text_utf8(HWND window, char *buffer, size_t buffer_size)
{
	BOOL result = FALSE;
	char *utf8_buffer = NULL;
	TCHAR t_buffer[256];

	if (!GetWindowText(window, t_buffer, ARRAY_LENGTH(t_buffer)))
		goto done;

	utf8_buffer = utf8_from_tstring(t_buffer);
	if (!utf8_buffer)
		goto done;

	snprintf(buffer, buffer_size, "%s", utf8_buffer);
	result = TRUE;

done:
	if (utf8_buffer)
		free(utf8_buffer);
	return result;
}



//============================================================
//  win_output_debug_string_utf8
//============================================================

void win_output_debug_string_utf8(const char *string)
{
	TCHAR *t_string = tstring_from_utf8(string);
	if (t_string != NULL)
	{
		OutputDebugString(t_string);
		free(t_string);
	}
}



//============================================================
//  win_get_module_file_name_utf8
//============================================================

DWORD win_get_module_file_name_utf8(HMODULE module, char *filename, DWORD size)
{
	TCHAR t_filename[MAX_PATH];
	char *utf8_filename;

	if (GetModuleFileName(module, t_filename, ARRAY_LENGTH(t_filename)))
		return 0;

	utf8_filename = utf8_from_tstring(t_filename);
	if (!utf8_filename)
		return 0;

	size = (DWORD) snprintf(filename, size, "%s", utf8_filename);
	free(utf8_filename);
	return size;
}



//============================================================
//  win_error_to_file_error
//============================================================

file_error win_error_to_file_error(DWORD error)
{
	file_error filerr;

	// convert a Windows error to a file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}



//============================================================
//  win_attributes_to_entry_type
//============================================================

osd_dir_entry_type win_attributes_to_entry_type(DWORD attributes)
{
	if (attributes == 0xFFFFFFFF)
		return ENTTYPE_NONE;
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}



//============================================================
//  win_is_gui_application
//============================================================

BOOL win_is_gui_application(void)
{
	static BOOL is_gui_frontend;
	static BOOL is_first_time = TRUE;
	HMODULE module;
	BYTE *image_ptr;
	IMAGE_DOS_HEADER *dos_header;
	IMAGE_NT_HEADERS *nt_headers;
	IMAGE_OPTIONAL_HEADER *opt_header;

	// is this the first time we've been ran?
	if (is_first_time)
	{
		is_first_time = FALSE;

		// get the current module
		module = GetModuleHandle(NULL);
		if (!module)
			return FALSE;
		image_ptr = (BYTE*) module;

		// access the DOS header
		dos_header = (IMAGE_DOS_HEADER *) image_ptr;
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
			return FALSE;

		// access the NT headers
		nt_headers = (IMAGE_NT_HEADERS *) ((BYTE*)(dos_header) + (DWORD)(dos_header->e_lfanew));
		if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
			return FALSE;

		// access the optional header
		opt_header = &nt_headers->OptionalHeader;
		switch (opt_header->Subsystem)
		{
			case IMAGE_SUBSYSTEM_WINDOWS_GUI:
				is_gui_frontend = TRUE;
				break;

			case IMAGE_SUBSYSTEM_WINDOWS_CUI:
				is_gui_frontend = FALSE;
				break;
		}
	}
	return is_gui_frontend;
}
