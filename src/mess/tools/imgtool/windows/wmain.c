//============================================================
//
//	wmain.h - Win32 GUI Imgtool main code
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "hexview.h"
#include "../modules.h"
#include "strconv.h"

int WINAPI _tWinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPTSTR command_line, int cmd_show)
{
	MSG msg;
	HWND window;
	BOOL b;
	int pos, rc = -1;
	imgtoolerr_t err;
	HACCEL accel = NULL;
	char *command_line_utf8;
	
	// initialize Windows classes
	InitCommonControls();
	if (!wimgtool_registerclass())
		goto done;
	if (!hexview_registerclass())
		goto done;

	// initialize the Imgtool library
	imgtool_init(TRUE);

	// create the window
	window = CreateWindow(wimgtool_class, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	if (!window)
		goto done;

#ifdef MAME_DEBUG
	// run validity checks and if appropriate, warn the user
	if (imgtool_validitychecks())
	{
		MessageBox(window,
			TEXT("Imgtool has failed its consistency checks; this build has problems"),
			wimgtool_producttext, MB_OK);
	}
#endif

	// load image specified at the command line
	if (command_line && command_line[0])
	{
		// convert command line to UTF-8
		command_line_utf8 = utf8_from_tstring(command_line);
		rtrim(command_line_utf8);
		pos = 0;

		// check to see if everything is quoted
		if ((command_line_utf8[pos] == '\"') && (command_line_utf8[strlen(command_line_utf8)-1] == '\"'))
		{
			command_line_utf8[strlen(command_line_utf8)-1] = '\0';
			pos++;
		}
		
		err = wimgtool_open_image(window, NULL, command_line_utf8 + pos, OSD_FOPEN_RW);
		if (err)
			wimgtool_report_error(window, err, command_line_utf8 + pos, NULL);

		free(command_line_utf8);
	}

	accel = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_WIMGTOOL_MENU));

	// pump messages until the window is gone
	while(IsWindow(window))
	{
		b = GetMessage(&msg, NULL, 0, 0);
		if (b <= 0)
		{
			window = NULL;
		}
		else if (!TranslateAccelerator(window, accel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	rc = 0;

done:
	imgtool_exit();
	if (accel)
		DestroyAcceleratorTable(accel);
	return rc;
}



int utf8_main(int argc, char *argv[])
{
	/* dummy */
	return 0;
}
