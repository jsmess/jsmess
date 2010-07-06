//============================================================
//
//  wimgtool.c - Win32 Imgtool File Association Dialog
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// standard C headers
#include <stdio.h>

// MESS headers
#include "assoc.h"
#include "wimgres.h"
#include "strconv.h"
#include "../imgtool.h"
#include "imageutl.h"

//============================================================
//  PARAMETERS
//============================================================

#define CONTROL_START 10000



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _assocdlg_info assocdlg_info;
struct _assocdlg_info
{
	int extension_count;
	const char *extensions[128];
	char buffer[1024];
};



//============================================================
//  LOCAL VARIABLES
//============================================================

static const struct win_association_info assoc_info =
{
	TEXT("ImgtoolImage"),
	0,
	TEXT("%1")
};



//============================================================
//  win_association_dialog_proc
//============================================================

static INT_PTR CALLBACK win_association_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	assocdlg_info *dlginfo;
	HWND ok_button, cancel_button, control;
	LRESULT font;
	LONG_PTR l;
	RECT r1, r2;
	int xmargin, y, width, i;
	int height = 20;
	int id;
	DWORD style;
	TCHAR buf[32];
	BOOL is_set;
	BOOL currently_set;
	TCHAR *t_extension;

	switch(message)
	{
		case WM_INITDIALOG:
			dlginfo = (assocdlg_info *) lparam;
			ok_button = GetDlgItem(dialog, IDOK);
			cancel_button = GetDlgItem(dialog, IDCANCEL);
			font = SendMessage(ok_button, WM_GETFONT, 0, 0);
			l = (LONG_PTR) dlginfo;
			SetWindowLongPtr(dialog, GWLP_USERDATA, l);

			GetWindowRect(cancel_button, &r1);
			GetWindowRect(dialog, &r2);
			xmargin = r1.left - r2.left;
			y = r1.top - r2.top - 20;
			width = r2.right - r2.left - xmargin * 2;

			for (i = 0; i < dlginfo->extension_count; i++)
			{
				style = WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX;
				t_extension = tstring_from_utf8(dlginfo->extensions[i]);
				_sntprintf(buf, ARRAY_LENGTH(buf), TEXT(".%s"), t_extension);
				osd_free(t_extension);

				control = CreateWindow(TEXT("BUTTON"), buf, style,
					 xmargin, y, width, height, dialog, NULL, NULL, NULL);
				if (!control)
					return -1;

				if (win_is_extension_associated(&assoc_info, buf))
					SendMessage(control, BM_SETCHECK, TRUE, 0);
				SendMessage(control, WM_SETFONT, font, 0);
				SetWindowLong(control, GWL_ID, CONTROL_START + i);

				y += height;
			}

			r1.top += y;
			r1.bottom += y;
			SetWindowPos(cancel_button, NULL, r1.left - r2.left, r1.top - r2.top,
				0, 0, SWP_NOZORDER | SWP_NOSIZE);
			GetWindowRect(ok_button, &r1);
			r1.top += y;
			r1.bottom += y;
			SetWindowPos(ok_button, NULL, r1.left - r2.left, r1.top - r2.top,
				0, 0, SWP_NOZORDER | SWP_NOSIZE);
			r2.bottom += y + r1.bottom - r1.top;
			SetWindowPos(dialog, NULL, 0, 0, r2.right - r2.left, r2.bottom - r2.top, SWP_NOZORDER | SWP_NOMOVE);
			break;

		case WM_COMMAND:
			id = LOWORD(wparam);

			if ((id == IDOK) || (id == IDCANCEL))
			{
				if (id == IDOK)
				{
					l = GetWindowLongPtr(dialog, GWLP_USERDATA);
					dlginfo = (assocdlg_info *)l;

					for (i = 0; i < dlginfo->extension_count; i++)
					{
						is_set = SendMessage(GetDlgItem(dialog, CONTROL_START + i), BM_GETCHECK, 0, 0);

						t_extension = tstring_from_utf8(dlginfo->extensions[i]);
						_sntprintf(buf, ARRAY_LENGTH(buf), TEXT(".%s"), t_extension);
						osd_free(t_extension);
						currently_set = win_is_extension_associated(&assoc_info, buf);

						if (is_set && !currently_set)
							win_associate_extension(&assoc_info, buf, TRUE);
						else if (!is_set && currently_set)
							win_associate_extension(&assoc_info, buf, FALSE);
					}
				}

				EndDialog(dialog, id);
			}
			break;
	}
	return rc;
}



//============================================================
//  extension_compare
//============================================================

static int CLIB_DECL extension_compare(const void *p1, const void *p2)
{
	const char *e1 = *((const char **) p1);
	const char *e2 = *((const char **) p2);
	return strcmp(e1, e2);
}



//============================================================
//  setup_extensions
//============================================================

static void setup_extensions(assocdlg_info *dlginfo)
{
	const imgtool_module *module = NULL;
	char *s;
	int buffer_pos;

	// merge all file extensions onto one comma-delimited list
	for (module = imgtool_find_module(NULL); module; module = module->next)
		image_specify_extension(dlginfo->buffer, ARRAY_LENGTH(dlginfo->buffer) - 1, module->extensions);

	// split the comma delimited list, and convert it to a string array
	dlginfo->extension_count = 0;
	buffer_pos = 0;
	while(dlginfo->buffer[buffer_pos] != '\0')
	{
		// an assertion to check to see if we are out of extensions
		assert(dlginfo->extension_count < ARRAY_LENGTH(dlginfo->extensions));

		// search for a comma, and if found null it out
		s = strchr(&dlginfo->buffer[buffer_pos], ',');
		if (s)
			*s = '\0';

		// append this to the extensions list
		dlginfo->extensions[dlginfo->extension_count++] = &dlginfo->buffer[buffer_pos];

		// move to next extension
		if (s)
			buffer_pos += s - &dlginfo->buffer[buffer_pos] + 1;
		else
			buffer_pos += strlen(&dlginfo->buffer[buffer_pos]);
	}

	// sort the extensions list
	qsort((void *) dlginfo->extensions, dlginfo->extension_count,
		sizeof(dlginfo->extensions[0]), extension_compare);
}



//============================================================
//  win_association_dialog
//============================================================

void win_association_dialog(HWND parent)
{
	assocdlg_info dlginfo;

	// initialize data structure
	memset(&dlginfo, 0, sizeof(dlginfo));
	setup_extensions(&dlginfo);

	// display dialog
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ASSOCIATIONS), parent,
		win_association_dialog_proc, (LPARAM) &dlginfo);
}
