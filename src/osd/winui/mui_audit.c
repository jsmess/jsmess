/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  mui_audit.c

  Audit dialog

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>

// standard C headers
#include <stdio.h>
#include <tchar.h>

// MAME/MAMEUI headers
#include "winui.h"
#include "winutf8.h"
#include "strconv.h"
#include "audit.h"
#include "resource.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"


#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg);
static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void ProcessNextRom(void);
static void ProcessNextSample(void);
static void CLIB_DECL DetailsPrintf(const char *fmt, ...) ATTR_PRINTF(1,2);
static const char * StatusString(int iStatus);

/***************************************************************************
    Internal variables
 ***************************************************************************/

#define SAMPLES_NOT_USED    3
#define MAX_AUDITBOX_TEXT	0x7FFFFFFE

static volatile HWND hAudit;
static volatile int rom_index;
static volatile int roms_correct;
static volatile int roms_incorrect;
static volatile int sample_index;
static volatile int samples_correct;
static volatile int samples_incorrect;

static volatile BOOL bPaused = FALSE;
static volatile BOOL bCancel = FALSE;

/***************************************************************************
    External functions  
 ***************************************************************************/

void AuditDialog(HWND hParent)
{
	HMODULE hModule = NULL;
	rom_index         = 0;
	roms_correct      = 0;
	roms_incorrect    = 0;
	sample_index      = 0;
	samples_correct   = 0;
	samples_incorrect = 0;

	//RS use Riched32.dll
	hModule = LoadLibrary(TEXT("Riched32.dll"));
	if( hModule )
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_AUDIT),hParent,AuditWindowProc);
		FreeLibrary( hModule );
		hModule = NULL;
	}
	else
	{
	    MessageBox(GetMainWindow(),TEXT("Unable to Load Riched32.dll"),TEXT("Error"),
				   MB_OK | MB_ICONERROR);
	}
}

void InitGameAudit(int gameIndex)
{
	rom_index = gameIndex;
}

const char * GetAuditString(int audit_result)
{
	switch (audit_result)
	{
	case CORRECT :
	case BEST_AVAILABLE :
		return "Yes";

	case NOTFOUND :
	case INCORRECT :
		return "No";

	case UNKNOWN :
		return "?";

	default:
		dprintf("unknown audit value %i\n",audit_result);
	}

	return "?";
}

BOOL IsAuditResultKnown(int audit_result)
{
	return audit_result != UNKNOWN;
}

BOOL IsAuditResultYes(int audit_result)
{
	return audit_result == CORRECT || audit_result == BEST_AVAILABLE;
}

BOOL IsAuditResultNo(int audit_result)
{
	return audit_result == NOTFOUND || audit_result == INCORRECT;
}


/***************************************************************************
    Internal functions
 ***************************************************************************/

static void MameUIOutput(void *param, const char *format, va_list argptr)
{
	char buffer[512];
	vsnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, argptr);
	DetailsPrintf("%s", buffer);
}

static int ProcessAuditResults(int game, audit_record *audit, int audit_records)
{
	output_callback_func prevcb;
	void *prevparam;
	int res;

	mame_set_output_channel(OUTPUT_CHANNEL_INFO, MameUIOutput, NULL, &prevcb, &prevparam);
	res = audit_summary(drivers[game], audit_records, audit, TRUE);
	mame_set_output_channel(OUTPUT_CHANNEL_INFO, prevcb ? prevcb : mame_null_output_callback, prevparam, NULL, NULL);

	return res;
}

// Verifies the ROM set while calling SetRomAuditResults	
int MameUIVerifyRomSet(int game)
{
	audit_record *audit;
	int audit_records;
	int res;

	// perform the audit
	audit_records = audit_images(MameUIGlobal(), drivers[game], AUDIT_VALIDATE_FAST, &audit);
	res = ProcessAuditResults(game, audit, audit_records);
	if (audit_records > 0)
		global_free(audit);

	SetRomAuditResults(game, res);
	return res;
}

// Verifies the Sample set while calling SetSampleAuditResults	
int MameUIVerifySampleSet(int game)
{
	audit_record *audit;
	int audit_records;
	int res;

	// perform the audit
	audit_records = audit_samples(MameUIGlobal(), drivers[game], &audit);
	res = ProcessAuditResults(game, audit, audit_records);
	if (audit_records > 0)
		global_free(audit);

	SetSampleAuditResults(game, res);
	return res;
}

static DWORD WINAPI AuditThreadProc(LPVOID hDlg)
{
	char buffer[200];

	while (!bCancel)
	{
		if (!bPaused)
		{
			if (rom_index != -1)
			{
				sprintf(buffer, "Checking Game %s - %s",
					drivers[rom_index]->name, drivers[rom_index]->description);
				win_set_window_text_utf8((HWND)hDlg, buffer);
				ProcessNextRom();
			}
			else
			{
				if (sample_index != -1)
				{
					sprintf(buffer, "Checking Game %s - %s",
						drivers[sample_index]->name, drivers[sample_index]->description);
					win_set_window_text_utf8((HWND)hDlg, buffer);
					ProcessNextSample();
				}
				else
				{
					win_set_window_text_utf8((HWND)hDlg, "File Audit");
					EnableWindow(GetDlgItem((HWND)hDlg, IDPAUSE), FALSE);
					ExitThread(1);
				}
			}
		}
	}
	return 0;
}

static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread;
	static DWORD dwThreadID;
	DWORD dwExitCode;
	HWND hEdit;

	switch (Msg)
	{
	case WM_INITDIALOG:
		hAudit = hDlg;
		//RS 20030613 Set Bkg of RichEdit Ctrl
		hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
		if (hEdit != NULL)
		{
			SendMessage( hEdit, EM_SETBKGNDCOLOR, FALSE, GetSysColor(COLOR_BTNFACE) );
			// MSH - Set to max
			SendMessage( hEdit, EM_SETLIMITTEXT, MAX_AUDITBOX_TEXT, 0 );

		}
		SendDlgItemMessage(hDlg, IDC_ROMS_PROGRESS,    PBM_SETRANGE, 0, MAKELPARAM(0, driver_list_get_count(drivers)));
		SendDlgItemMessage(hDlg, IDC_SAMPLES_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, driver_list_get_count(drivers)));
		bPaused = FALSE;
		bCancel = FALSE;
		rom_index = 0;
		hThread = CreateThread(NULL, 0, AuditThreadProc, hDlg, 0, &dwThreadID);
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
            bPaused = FALSE;
			if (hThread)
			{
				bCancel = TRUE;
				if (GetExitCodeThread(hThread, &dwExitCode) && (dwExitCode == STILL_ACTIVE))
				{					
					PostMessage(hDlg, WM_COMMAND, wParam, lParam);
					return 1;
				}
				CloseHandle(hThread);
			}
			EndDialog(hDlg,0);
			break;

		case IDPAUSE:
			if (bPaused)
			{
				SendDlgItemMessage(hAudit, IDPAUSE, WM_SETTEXT, 0, (LPARAM)TEXT("Pause"));
				bPaused = FALSE;
			}
			else
			{
				SendDlgItemMessage(hAudit, IDPAUSE, WM_SETTEXT, 0, (LPARAM)TEXT("Continue"));
				bPaused = TRUE;
			}
			break;
		}
		return 1;
	}
	return 0;	
}

/* Callback for the Audit property sheet */
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		FlushFileCaches();
		hAudit = hDlg;
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(OPTIONS_GAME, rom_index));
		SetTimer(hDlg, 0, 1, NULL);
		return 1;

	case WM_TIMER:
		KillTimer(hDlg, 0);
		{
			int iStatus;
			LPCSTR lpStatus;

			iStatus = MameUIVerifyRomSet(rom_index);
			lpStatus = DriverUsesRoms(rom_index) ? StatusString(iStatus) : "None required";
			win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_ROMS), lpStatus);

			iStatus = MameUIVerifySampleSet(rom_index);
			lpStatus = DriverUsesSamples(rom_index) ? StatusString(iStatus) : "None required";
			win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_SAMPLES), lpStatus);
		}
		ShowWindow(hDlg, SW_SHOW);
		break;
	}
	return 0;
}

static void ProcessNextRom()
{
	int retval;
	TCHAR buffer[200];

	retval = MameUIVerifyRomSet(rom_index);
	switch (retval)
	{
	case BEST_AVAILABLE: /* correct, incorrect or separate count? */
	case CORRECT:
		roms_correct++;
		_stprintf(buffer, TEXT("%i"), roms_correct);
		SendDlgItemMessage(hAudit, IDC_ROMS_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;

	case NOTFOUND:
		break;

	case INCORRECT:
		roms_incorrect++;
		_stprintf(buffer, TEXT("%i"), roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;
	}

	rom_index++;
	SendDlgItemMessage(hAudit, IDC_ROMS_PROGRESS, PBM_SETPOS, rom_index, 0);

	if (rom_index == driver_list_get_count(drivers))
	{
		sample_index = 0;
		rom_index = -1;
	}
}

static void ProcessNextSample()
{
	int  retval;
	TCHAR buffer[200];
	
	retval = MameUIVerifySampleSet(sample_index);
	
	switch (retval)
	{
	case CORRECT:
		if (DriverUsesSamples(sample_index))
		{
			samples_correct++;
			_stprintf(buffer, TEXT("%i"), samples_correct);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
			_stprintf(buffer, TEXT("%i"), samples_correct + samples_incorrect);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
			break;
		}

	case NOTFOUND:
		break;
			
	case INCORRECT:
		samples_incorrect++;
		_stprintf(buffer, TEXT("%i"), samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), samples_correct + samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		
		break;
	}

	sample_index++;
	SendDlgItemMessage(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETPOS, sample_index, 0);
	
	if (sample_index == driver_list_get_count(drivers))
	{
		DetailsPrintf("Audit complete.\n");
		SendDlgItemMessage(hAudit, IDCANCEL, WM_SETTEXT, 0, (LPARAM)TEXT("Close"));
		sample_index = -1;
	}
}

static void CLIB_DECL DetailsPrintf(const char *fmt, ...)
{
	HWND	hEdit;
	va_list marker;
	char	buffer[2000];
	TCHAR*  t_s;
	int		textLength;

	//RS 20030613 Different Ids for Property Page and Dialog
	// so see which one's currently instantiated
	hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
	if (hEdit ==  NULL)
		hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS_PROP);
	
	if (hEdit == NULL)
	{
		dprintf("audit detailsprintf() can't find any audit control\n");
		return;
	}

	va_start(marker, fmt);
	
	vsprintf(buffer, fmt, marker);
	
	va_end(marker);

	t_s = tstring_from_utf8(ConvertToWindowsNewlines(buffer));
	if( !t_s || _tcscmp(TEXT(""), t_s) == 0)
		return;

	textLength = Edit_GetTextLength(hEdit);
	Edit_SetSel(hEdit, textLength, textLength);
	SendMessage( hEdit, EM_REPLACESEL, FALSE, (WPARAM)(LPCTSTR)win_tstring_strdup(t_s) );
	
	osd_free(t_s);
}

static const char * StatusString(int iStatus)
{
	static const char *ptr;

	ptr = "Unknown";

	switch (iStatus)
	{
	case CORRECT:
		ptr = "Passed";
		break;
		
	case BEST_AVAILABLE:
		ptr = "Best available";
		break;
		
	case NOTFOUND:
		ptr = "Not found";
		break;
		
	case INCORRECT:
		ptr = "Failed";
		break;
	}

	return ptr;
}
