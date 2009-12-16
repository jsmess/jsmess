//============================================================
//
//  winutils.c - Generic Win32 utility code
//
//============================================================

// Needed for better file dialog
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif // _WIN32_WINNT
#define _WIN32_WINNT 0x500

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// MESS headers
#include "winutils.h"
#include "strconv.h"
#include "glob.h"

//============================================================
//  win_get_file_attributes_utf8
//============================================================

DWORD win_get_file_attributes_utf8(const char *filename)
{
	DWORD result = ~0;
	LPTSTR t_filename;

	t_filename = tstring_from_utf8(filename);
	if (t_filename != NULL)
	{
		result = GetFileAttributes(t_filename);
		free(t_filename);
	}
	return result;
}

void win_expand_wildcards(int *argc, char **argv[])
{
	int i;
	glob_t g;

	memset(&g, 0, sizeof(g));

	for (i = 0; i < *argc; i++)
		glob((*argv)[i], (g.gl_pathc > 0) ? GLOB_APPEND|GLOB_NOCHECK : GLOB_NOCHECK, NULL, &g);

	*argc = g.gl_pathc;
	*argv = g.gl_pathv;
}
