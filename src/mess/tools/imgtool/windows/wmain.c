//============================================================
//
//  wmain.h - Win32 GUI Imgtool main code
//
//============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "hexview.h"
#include "../modules.h"
#include "winutf8.h"
#include "strconv.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPWSTR command_line_w, int cmd_show)
{
	MSG msg;
	HWND window;
	BOOL b;
	int pos, rc = -1;
	imgtoolerr_t err;
	HACCEL accel = NULL;
	LPSTR utf8_command_line = utf8_from_wstring(command_line_w);
	// initialize Windows classes
	InitCommonControls();
	if (!wimgtool_registerclass())
		goto done;
	if (!hexview_registerclass())
		goto done;

	// initialize the Imgtool library
	imgtool_init(TRUE, win_output_debug_string_utf8);

	// create the window
	window = CreateWindow(wimgtool_class, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	if (!window)
		goto done;

#ifdef MAME_DEBUG
	// run validity checks and if appropriate, warn the user
	if (imgtool_validitychecks())
	{
		win_message_box_utf8(window,
			"Imgtool has failed its consistency checks; this build has problems",
			wimgtool_producttext, MB_OK);
	}
#endif

	// load image specified at the command line
	if (utf8_command_line && utf8_command_line[0])
	{
		rtrim(utf8_command_line);
		pos = 0;

		// check to see if everything is quoted
		if ((utf8_command_line[pos] == '\"') && (utf8_command_line[strlen(utf8_command_line)-1] == '\"'))
		{
			utf8_command_line[strlen(utf8_command_line)-1] = '\0';
			pos++;
		}

		err = wimgtool_open_image(window, NULL, utf8_command_line + pos, OSD_FOPEN_RW);
		if (err)
			wimgtool_report_error(window, err, utf8_command_line + pos, NULL);
	}

	osd_free(utf8_command_line);
	utf8_command_line = NULL;

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
