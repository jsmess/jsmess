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

  mui_util.c

 ***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

// standard C headers
#include <assert.h>
#include <stdio.h>
#include <tchar.h>

#include "emu.h"

// MAME/MAMEUI headers
#include "unzip.h"
#include "sound/samples.h"
#include "winutf8.h"
#include "strconv.h"
#include "winui.h"
#include "mui_util.h"
#include "mui_opts.h"

#include <shlwapi.h>

/***************************************************************************
	function prototypes
 ***************************************************************************/

/***************************************************************************
	External variables
 ***************************************************************************/

/***************************************************************************
	Internal structures
 ***************************************************************************/
static struct DriversInfo
{
	BOOL isClone;
	BOOL isBroken;
	BOOL isHarddisk;
	BOOL hasOptionalBIOS;
	BOOL isStereo;
	int screenCount;
	BOOL isVector;
	BOOL usesRoms;
	BOOL usesSamples;
	BOOL usesTrackball;
	BOOL usesLightGun;
	BOOL usesMouse;
	BOOL supportsSaveState;
	BOOL isVertical;
} *drivers_info = NULL;


/***************************************************************************
	External functions
 ***************************************************************************/

/*
	ErrorMsg
*/
void __cdecl ErrorMsg(const char* fmt, ...)
{
	static FILE*	pFile = NULL;
	DWORD			dwWritten;
	char			buf[5000];
	char			buf2[5000];
	va_list 		va;

	va_start(va, fmt);

	vsprintf(buf, fmt, va);

	win_message_box_utf8(GetActiveWindow(), buf, MAMEUINAME, MB_OK | MB_ICONERROR);

	strcpy(buf2, MAMEUINAME ": ");
	strcat(buf2,buf);
	strcat(buf2, "\n");

	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf2, strlen(buf2), &dwWritten, NULL);

	if (pFile == NULL)
		pFile = fopen("debug.txt", "wt");

	if (pFile != NULL)
	{
		fprintf(pFile, "%s", buf2);
		fflush(pFile);
	}

	va_end(va);
}

void __cdecl dprintf(const char* fmt, ...)
{
	char 	buf[5000];
	va_list va;

	va_start(va, fmt);

	_vsnprintf(buf,sizeof(buf),fmt,va);

	win_output_debug_string_utf8(buf);

	va_end(va);
}

UINT GetDepth(HWND hWnd)
{
	UINT	nBPP;
	HDC 	hDC;

	hDC = GetDC(hWnd);
	
	nBPP = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);

	ReleaseDC(hWnd, hDC);

	return nBPP;
}

/*
 * Return TRUE if comctl32.dll is version 4.71 or greater
 * otherwise return FALSE.
 */
LONG GetCommonControlVersion()
{
	HMODULE hModule = GetModuleHandle(TEXT("comctl32"));

	if (hModule)
	{
		FARPROC lpfnICCE = GetProcAddress(hModule, "InitCommonControlsEx");

		if (NULL != lpfnICCE)
		{
			FARPROC lpfnDLLI = GetProcAddress(hModule, "DllInstall");

			if (NULL != lpfnDLLI) 
			{
				/* comctl 4.71 or greater */

				// see if we can find out exactly
				
				DLLGETVERSIONPROC pDllGetVersion;
				pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hModule, "DllGetVersion");

				/* Because some DLLs might not implement this function, you
				   must test for it explicitly. Depending on the particular 
				   DLL, the lack of a DllGetVersion function can be a useful
				   indicator of the version. */

				if(pDllGetVersion)
				{
					DLLVERSIONINFO dvi;
					HRESULT hr;

					ZeroMemory(&dvi, sizeof(dvi));
					dvi.cbSize = sizeof(dvi);

					hr = (*pDllGetVersion)(&dvi);

					if (SUCCEEDED(hr))
					{
						return PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
					}
				}
				return PACKVERSION(4,71);
			}
			return PACKVERSION(4,7);
		}
		return PACKVERSION(4,0);
	}
	/* DLL not found */
	return PACKVERSION(0,0);
}

void DisplayTextFile(HWND hWnd, const char *cName)
{
	HINSTANCE hErr;
	LPCTSTR	  msg = 0;
	LPTSTR    tName;
	
	tName = tstring_from_utf8(cName);
	if( !tName )
		return;

	hErr = ShellExecute(hWnd, NULL, tName, NULL, NULL, SW_SHOWNORMAL);
	if ((FPTR)hErr > 32) 
	{
		osd_free(tName);
		return;
	}

	switch((FPTR)hErr)
	{
	case 0:
		msg = TEXT("The operating system is out of memory or resources.");
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = TEXT("The specified file was not found."); 
		break;

	case SE_ERR_NOASSOC :
		msg = TEXT("There is no application associated with the given filename extension.");
		break;

	case SE_ERR_OOM :
		msg = TEXT("There was not enough memory to complete the operation.");
		break;

	case SE_ERR_PNF :
		msg = TEXT("The specified path was not found.");
		break;

	case SE_ERR_SHARE :
		msg = TEXT("A sharing violation occurred.");
		break;

	default:
		msg = TEXT("Unknown error.");
	}
 
	MessageBox(NULL, msg, tName, MB_OK);
	
	osd_free(tName);
}

char* MyStrStrI(const char* pFirst, const char* pSrch)
{
	char* cp = (char*)pFirst;
	char* s1;
	char* s2;
	
	while (*cp)
	{
		s1 = cp;
		s2 = (char*)pSrch;
		
		while (*s1 && *s2 && !mame_strnicmp(s1, s2, 1))
			s1++, s2++;
		
		if (!*s2)
			return cp;
		
		cp++;
	}
	return NULL;
}

char * ConvertToWindowsNewlines(const char *source)
{
	static char buf[1024 * 1024];
	char *dest;

	dest = buf;
	while (*source != 0)
	{
		if (*source == '\n')
		{
			*dest++ = '\r';
			*dest++ = '\n';
		}
		else
			*dest++ = *source;
		source++;
	}
	*dest = 0;
	return buf;
}

/* Lop off path and extention from a source file name
 * This assumes their is a pathname passed to the function
 * like src\drivers\blah.c
 */
const char * GetDriverFilename(int nIndex)
{
	static char tmp[40];
	const char *ptmp;

	const char *s = drivers[nIndex]->source_file;

	tmp[0] = '\0';

	ptmp = strrchr(s, '\\');
	if (ptmp == NULL) {
		ptmp = strrchr(s, '/');
	}
	else {
		const char *ptmp2;
		ptmp2 = strrchr(ptmp, '/');
		if (ptmp2 != NULL) {
			ptmp = ptmp2;
		}
	}
	if (ptmp == NULL)
		return s;

	ptmp++;
	strcpy(tmp,ptmp);
	return tmp;
}

BOOL isDriverVector(const machine_config *config)
{
	const screen_device_config *screen  = config->first_screen();

	if (screen != NULL) {
		// parse "vector.ini" for vector games 
		if (SCREEN_TYPE_VECTOR == screen->screen_type())
		{
			return TRUE;
		}
	}
	return FALSE;
}

int numberOfScreens(const machine_config *config)
{
	const screen_device_config *screen  = config->first_screen();
	int i=0;
	for (; screen != NULL; screen = screen->next_screen()) {
		i++;
	}
	return i;	
}



int numberOfSpeakers(const machine_config *config)
{
	return config->m_devicelist.count(SPEAKER);
}

static struct DriversInfo* GetDriversInfo(int driver_index)
{
	if (drivers_info == NULL)
	{
		int ndriver;
		drivers_info = (DriversInfo*)malloc(sizeof(struct DriversInfo) * driver_list_get_count(drivers));
		for (ndriver = 0; ndriver < driver_list_get_count(drivers); ndriver++)
		{
			const game_driver *gamedrv = drivers[ndriver];
			struct DriversInfo *gameinfo = &drivers_info[ndriver];
			const rom_entry *region, *rom;
			windows_options pCurrentOpts;
			load_options(pCurrentOpts, OPTIONS_GLOBAL, driver_index); 
			machine_config config(*gamedrv,pCurrentOpts);
			const rom_source *source;
			int num_speakers;

			gameinfo->isClone = (GetParentRomSetIndex(gamedrv) != -1);
			gameinfo->isBroken = ((gamedrv->flags & GAME_NOT_WORKING) != 0);
			gameinfo->supportsSaveState = ((gamedrv->flags & GAME_SUPPORTS_SAVE) != 0);
			gameinfo->isHarddisk = FALSE;
			gameinfo->isVertical = (gamedrv->flags & ORIENTATION_SWAP_XY) ? TRUE : FALSE;
			for (source = rom_first_source(config); source != NULL; source = rom_next_source(*source))
			{
				for (region = rom_first_region(*source); region; region = rom_next_region(region))
				{
					if (ROMREGION_ISDISKDATA(region))
						gameinfo->isHarddisk = TRUE;
				}
			}
			gameinfo->hasOptionalBIOS = FALSE;
			if (gamedrv->rom != NULL)
			{
				for (rom = gamedrv->rom; !ROMENTRY_ISEND(rom); rom++)
				{
					if (ROMENTRY_ISSYSTEM_BIOS(rom))
					{
						gameinfo->hasOptionalBIOS = TRUE;
						break;
					}
				}
			}

			num_speakers = numberOfSpeakers(&config);

			gameinfo->isStereo = (num_speakers > 1);
			gameinfo->screenCount = numberOfScreens(&config);
			gameinfo->isVector = isDriverVector(&config); // ((drv.video_attributes & VIDEO_TYPE_VECTOR) != 0);
			gameinfo->usesRoms = FALSE;
			for (source = rom_first_source(config); source != NULL; source = rom_next_source(*source))
			{
				for (region = rom_first_region(*source); region; region = rom_next_region(region))
				{
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					{
						gameinfo->usesRoms = TRUE; 
						break; 
					}
				}
			}
			gameinfo->usesSamples = FALSE;
			
			{
				const device_config_sound_interface *sound = NULL;
				const char * const * samplenames = NULL;
				for (bool gotone = config.m_devicelist.first(sound); gotone; gotone = sound->next(sound)) {
					if (sound->devconfig().type() == SAMPLES)
					{
						const samples_interface *intf = (const samples_interface *)sound->devconfig().static_config();
						samplenames = intf->samplenames;

						if (samplenames != 0 && samplenames[0] != 0)
						{
							gameinfo->usesSamples = TRUE;
							break;
						}			
					}				
				}
			}

			gameinfo->usesTrackball = FALSE;
			gameinfo->usesLightGun = FALSE;
			if (gamedrv->ipt != NULL)
			{
				const input_port_config *port;
				ioport_list portlist;
				
				input_port_list_init(portlist, gamedrv->ipt, NULL, 0, FALSE, NULL);
				for (device_config *cfg = config.m_devicelist.first(); cfg != NULL; cfg = cfg->next())
				{
					if (cfg->input_ports()!=NULL) {
						input_port_list_init(portlist, cfg->input_ports(), NULL, 0, FALSE, cfg);
					}
				}

				for (port = portlist.first(); port != NULL; port = port->next())
				{
					const input_field_config *field;
					for (field = port->fieldlist; field != NULL; field = field->next)
 					{
						UINT32 type;
						type = field->type;
						if (type == IPT_END)
							break;
						if (type == IPT_DIAL || type == IPT_PADDLE || 
							type == IPT_TRACKBALL_X || type == IPT_TRACKBALL_Y ||
							type == IPT_AD_STICK_X || type == IPT_AD_STICK_Y)
							gameinfo->usesTrackball = TRUE;
						if (type == IPT_LIGHTGUN_X || type == IPT_LIGHTGUN_Y)
							gameinfo->usesLightGun = TRUE;
						if (type == IPT_MOUSE_X || type == IPT_MOUSE_Y)
							gameinfo->usesMouse = TRUE;
					}
				}
			}
		}
	}
	return &drivers_info[driver_index];
}

BOOL DriverIsClone(int driver_index)
{
	 return GetDriversInfo(driver_index)->isClone;
}

BOOL DriverIsBroken(int driver_index)
{
	return GetDriversInfo(driver_index)->isBroken;
}

BOOL DriverIsHarddisk(int driver_index)
{
	return GetDriversInfo(driver_index)->isHarddisk;
}

BOOL DriverIsBios(int driver_index)
{
	BOOL bBios = FALSE;
	if( !( (drivers[driver_index]->flags & GAME_IS_BIOS_ROOT ) == 0)   )
		bBios = TRUE;
	return bBios;
}

BOOL DriverIsMechanical(int driver_index)
{
	BOOL bMechanical = FALSE;
	if( !( (drivers[driver_index]->flags & GAME_MECHANICAL ) == 0)   )
		bMechanical = TRUE;
	return bMechanical;
}

BOOL DriverHasOptionalBIOS(int driver_index)
{
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

BOOL DriverIsStereo(int driver_index)
{
	return GetDriversInfo(driver_index)->isStereo;
}

int DriverNumScreens(int driver_index)
{
	return GetDriversInfo(driver_index)->screenCount;
}

BOOL DriverIsVector(int driver_index)
{
	return GetDriversInfo(driver_index)->isVector;
}

BOOL DriverUsesRoms(int driver_index)
{
	return GetDriversInfo(driver_index)->usesRoms;
}

BOOL DriverUsesSamples(int driver_index)
{
	return GetDriversInfo(driver_index)->usesSamples;
}

BOOL DriverUsesTrackball(int driver_index)
{
	return GetDriversInfo(driver_index)->usesTrackball;
}

BOOL DriverUsesLightGun(int driver_index)
{
	return GetDriversInfo(driver_index)->usesLightGun;
}

BOOL DriverUsesMouse(int driver_index)
{
	return GetDriversInfo(driver_index)->usesMouse;
}

BOOL DriverSupportsSaveState(int driver_index)
{
	return GetDriversInfo(driver_index)->supportsSaveState;
}

BOOL DriverIsVertical(int driver_index) {
	return GetDriversInfo(driver_index)->isVertical; 
}

void FlushFileCaches(void)
{
	zip_file_cache_clear();
}

BOOL StringIsSuffixedBy(const char *s, const char *suffix)
{
	return (strlen(s) > strlen(suffix)) && (strcmp(s + strlen(s) - strlen(suffix), suffix) == 0);
}

/***************************************************************************
	Win32 wrappers
 ***************************************************************************/

BOOL SafeIsAppThemed(void)
{
	BOOL bResult = FALSE;
	HMODULE hThemes;
	BOOL (WINAPI *pfnIsAppThemed)(void);
	
	hThemes = LoadLibrary(TEXT("uxtheme.dll"));
	if (hThemes != NULL)
	{
		pfnIsAppThemed = (BOOL (WINAPI *)(void)) GetProcAddress(hThemes, "IsAppThemed");
		if (pfnIsAppThemed != NULL)
			bResult = pfnIsAppThemed();
		FreeLibrary(hThemes);
	}
	return bResult;

}


void GetSystemErrorMessage(DWORD dwErrorId, TCHAR **tErrorMessage)
{
	if( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrorId, 0, (LPTSTR)tErrorMessage, 0, NULL) == 0 )
	{
		*tErrorMessage = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
		_tcscpy(*tErrorMessage, TEXT("Unknown Error"));
	}
}


//============================================================
//  win_extract_icon_utf8
//============================================================

HICON win_extract_icon_utf8(HINSTANCE inst, const char* exefilename, UINT iconindex)
{
	HICON icon = 0;
	TCHAR* t_exefilename = tstring_from_utf8(exefilename);
	if( !t_exefilename )
		return icon;
	
	icon = ExtractIcon(inst, t_exefilename, iconindex);
	
	osd_free(t_exefilename);
	
	return icon;
}



//============================================================
//  win_tstring_strdup
//============================================================

TCHAR* win_tstring_strdup(LPCTSTR str)
{
	TCHAR *cpy = NULL;
	if (str != NULL)
	{
		cpy = (TCHAR*)osd_malloc((_tcslen(str) + 1) * sizeof(TCHAR));
		if (cpy != NULL)
			_tcscpy(cpy, str);
	}
	return cpy;
}

//============================================================
//  win_create_file_utf8
//============================================================

HANDLE win_create_file_utf8(const char* filename, DWORD desiredmode, DWORD sharemode, 
					   		LPSECURITY_ATTRIBUTES securityattributes, DWORD creationdisposition,
					   		DWORD flagsandattributes, HANDLE templatehandle)
{
	HANDLE result = 0;
	TCHAR* t_filename = tstring_from_utf8(filename);
	if( !t_filename )
		return result;
	
	result = CreateFile(t_filename, desiredmode, sharemode, securityattributes, creationdisposition,
						flagsandattributes, templatehandle);

	osd_free(t_filename);
						
	return result;
}

//============================================================
//  win_get_current_directory_utf8
//============================================================

DWORD win_get_current_directory_utf8(DWORD bufferlength, char* buffer)
{
	DWORD result = 0;
	TCHAR* t_buffer = NULL;
	char* utf8_buffer = NULL;
	
	if( bufferlength > 0 ) {
		t_buffer = (TCHAR*)malloc((bufferlength * sizeof(TCHAR)) + 1);
		if( !t_buffer )
			return result;
	}
	
	result = GetCurrentDirectory(bufferlength, t_buffer);
	
	if( bufferlength > 0 ) {
		utf8_buffer = utf8_from_tstring(t_buffer);
		if( !utf8_buffer ) {
			osd_free(t_buffer);
			return result;
		}
	}
		
	strncpy(buffer, utf8_buffer, bufferlength);
	
	if( utf8_buffer )
		osd_free(utf8_buffer);
	
	if( t_buffer )
		free(t_buffer);
	
	return result;
}

//============================================================
//  win_find_first_file_utf8
//============================================================

HANDLE win_find_first_file_utf8(const char* filename, LPWIN32_FIND_DATA findfiledata)
{
	HANDLE result = 0;
	TCHAR* t_filename = tstring_from_utf8(filename);
	if( !t_filename )
		return result;
	
	result = FindFirstFile(t_filename, findfiledata);
	
	osd_free(t_filename);
	
	return result;
}

