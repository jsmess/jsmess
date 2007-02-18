/***************************************************************************

  M.A.M.E.32	-  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  M32Util.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <assert.h>
#include <stdio.h>

#include "unzip.h"
#include "screenshot.h"
#include "sound/samples.h"
#include "MAME32.h"
#include "M32Util.h"


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
	BOOL isMultiMon;
	BOOL isVector;
	BOOL usesRoms;
	BOOL usesSamples;
	BOOL usesTrackball;
	BOOL usesLightGun;
	BOOL supportsSaveState;
} *drivers_info = NULL;


/***************************************************************************
	Internal variables
 ***************************************************************************/
#ifndef PATH_SEPARATOR
#ifdef _MSC_VER
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif
#endif


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

	MessageBox(GetActiveWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);

	strcpy(buf2, MAME32NAME ": ");
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
	char	buf[5000];
	va_list va;

	va_start(va, fmt);

	_vsnprintf(buf,sizeof(buf),fmt,va);

	OutputDebugString(buf);

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
	HMODULE hModule = GetModuleHandle("comctl32");

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
	const char	  *msg = 0;

	hErr = ShellExecute(hWnd, NULL, cName, NULL, NULL, SW_SHOWNORMAL);
	if ((int)hErr > 32)
		return;

	switch((int)hErr)
	{
	case 0:
		msg = "The operating system is out of memory or resources.";
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = "The specified file was not found."; 
		break;

	case SE_ERR_NOASSOC :
		msg = "There is no application associated with the given filename extension.";
		break;

	case SE_ERR_OOM :
		msg = "There was not enough memory to complete the operation.";
		break;

	case SE_ERR_PNF :
		msg = "The specified path was not found.";
		break;

	case SE_ERR_SHARE :
		msg = "A sharing violation occurred.";
		break;

	default:
		msg = "Unknown error.";
	}
 
	MessageBox(NULL, msg, cName, MB_OK); 
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
	static char buf[40000];
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
    char *ptmp;

	const char *s = drivers[nIndex]->source_file;

    tmp[0] = '\0';

	ptmp = strrchr(s, '\\');
	if (ptmp == NULL)
		ptmp = strrchr(s, '/');
    if (ptmp == NULL)
		return s;

	ptmp++;
	strcpy(tmp,ptmp);
	return tmp;
}

static struct DriversInfo* GetDriversInfo(int driver_index)
{
	if (drivers_info == NULL)
	{
		int ndriver, i;
		drivers_info = malloc(sizeof(struct DriversInfo) * driver_get_count());
		for (ndriver = 0; ndriver < driver_get_count(); ndriver++)
		{
			const game_driver *gamedrv = drivers[ndriver];
			struct DriversInfo *gameinfo = &drivers_info[ndriver];
			const rom_entry *region, *rom;
			machine_config drv;
			const input_port_entry *input_ports;
			int speakernum, num_speakers;
			gameinfo->isClone = (GetParentRomSetIndex(gamedrv) != -1);
			gameinfo->isBroken = ((gamedrv->flags & GAME_NOT_WORKING) != 0);
			gameinfo->supportsSaveState = ((gamedrv->flags & GAME_SUPPORTS_SAVE) != 0);
			gameinfo->isHarddisk = FALSE;
			for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
				if (ROMREGION_ISDISKDATA(region))
				{
					gameinfo->isHarddisk = TRUE;
					break;
				}
			gameinfo->hasOptionalBIOS = (gamedrv->bios != NULL);
			expand_machine_driver(gamedrv->drv, &drv);

			num_speakers = 0;
			for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
				if (drv.speaker[speakernum].tag != NULL)
					num_speakers++;

			gameinfo->isStereo = (num_speakers > 1);
			//gameinfo->isMultiMon = ((drv.video_attributes & VIDEO_DUAL_MONITOR) != 0);
			// Was removed from Core
			gameinfo->isMultiMon = 0;
			gameinfo->isVector = ((drv.video_attributes & VIDEO_TYPE_VECTOR) != 0);
			gameinfo->usesRoms = FALSE;
			for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					gameinfo->usesRoms = TRUE; 
					break; 
				}
			gameinfo->usesSamples = FALSE;
			
			if (HAS_SAMPLES || HAS_VLM5030)
			{
				for (i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++)
				{
					const char **samplenames = NULL;
					
#if (HAS_SAMPLES || HAS_VLM5030)
					for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
					{
#if (HAS_SAMPLES)
						if( drv.sound[i].sound_type == SOUND_SAMPLES )
							samplenames = ((struct Samplesinterface *)drv.sound[i].config)->samplenames;
#endif
					}
#endif
					if (samplenames != 0 && samplenames[0] != 0)
					{
						gameinfo->usesSamples = TRUE;
						break;
					}
				}
			}

			gameinfo->usesTrackball = FALSE;
			gameinfo->usesLightGun = FALSE;
			if (gamedrv->ipt != NULL)
			{
				begin_resource_tracking();
				input_ports = input_port_allocate(gamedrv->ipt, NULL);
				while (1)
				{
					UINT32 type;
					type = input_ports->type;
					if (type == IPT_END)
						break;
					if (type == IPT_DIAL || type == IPT_PADDLE || 
						type == IPT_TRACKBALL_X || type == IPT_TRACKBALL_Y ||
						type == IPT_AD_STICK_X || type == IPT_AD_STICK_Y)
						gameinfo->usesTrackball = TRUE;
					if (type == IPT_LIGHTGUN_X || type == IPT_LIGHTGUN_Y)
						gameinfo->usesLightGun = TRUE;
					input_ports++;
				}
				end_resource_tracking();
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
	if( !( (drivers[driver_index]->flags & NOT_A_DRIVER ) == 0)   )
		bBios = TRUE;
	return bBios;
}


BOOL DriverHasOptionalBIOS(int driver_index)
{
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

BOOL DriverIsStereo(int driver_index)
{
	return GetDriversInfo(driver_index)->isStereo;
}
BOOL DriverIsMultiMon(int driver_index)
{
	return GetDriversInfo(driver_index)->isMultiMon;
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

BOOL DriverSupportsSaveState(int driver_index)
{
	return GetDriversInfo(driver_index)->supportsSaveState;
}


void FlushFileCaches(void)
{
	zip_file_cache_clear();
}

void FreeIfAllocated(char **s)
{
	if (*s)
		free(*s);
	*s = NULL;
}

BOOL StringIsSuffixedBy(const char *s, const char *suffix)
{
	return (strlen(s) > strlen(suffix)) && (strcmp(s + strlen(s) - strlen(suffix), suffix) == 0);
}

/***************************************************************************
	Internal functions
 ***************************************************************************/

