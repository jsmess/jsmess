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
//  FILE DIALOG WRAPPERS
//============================================================

enum _win_file_dialog_type
{
	WIN_FILE_DIALOG_OPEN = 1,
	WIN_FILE_DIALOG_SAVE
};
typedef enum _win_file_dialog_type win_file_dialog_type;

typedef struct _win_open_file_name win_open_file_name;
struct _win_open_file_name
{
	win_file_dialog_type	type;				// type of file dialog
	HWND					owner;				// owner of the dialog
	HINSTANCE				instance;			// instance
	const char *			filter;				// pipe char ("|") delimited strings
	DWORD					filter_index;		// index into filter
	char					filename[MAX_PATH];	// filename buffer
	const char *			initial_directory;	// initial directory for dialog
	DWORD					flags;				// standard flags
	LPARAM					custom_data;		// custom data for dialog hook
	LPOFNHOOKPROC			hook;				// custom dialog hook
	LPCTSTR					template_name;		// custom dialog template
};

BOOL win_get_file_name_dialog(win_open_file_name *ofn);



//============================================================
//  MISC
//============================================================

void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line);

BOOL win_append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *item);

#endif // WINUTILS_H
