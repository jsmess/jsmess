/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse
 
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef M32UTIL_H
#define M32UTIL_H

extern void __cdecl ErrorMsg(const char* fmt, ...);
extern void __cdecl dprintf(const char* fmt, ...);

extern UINT GetDepth(HWND hWnd);

/* Open a text file */
extern void DisplayTextFile(HWND hWnd, const char *cName);

#define PACKVERSION(major,minor) MAKELONG(minor,major)

/* Check for old version of comctl32.dll */
extern LONG GetCommonControlVersion(void);

extern char * MyStrStrI(const char* pFirst, const char* pSrch);
extern char * ConvertToWindowsNewlines(const char *source);

extern const char * GetDriverFilename(int nIndex);

BOOL DriverIsClone(int driver_index);
BOOL DriverIsBroken(int driver_index);
BOOL DriverIsHarddisk(int driver_index);
BOOL DriverHasOptionalBIOS(int driver_index);
BOOL DriverIsStereo(int driver_index);
BOOL DriverIsMultiMon(int driver_index);
BOOL DriverIsVector(int driver_index);
BOOL DriverIsBios(int driver_index);
BOOL DriverUsesRoms(int driver_index);
BOOL DriverUsesSamples(int driver_index);
BOOL DriverUsesTrackball(int driver_index);
BOOL DriverUsesLightGun(int driver_index);
BOOL DriverSupportsSaveState(int driver_index);

void FlushFileCaches(void);

void FreeIfAllocated(char **s);

BOOL StringIsSuffixedBy(const char *s, const char *suffix);

#endif /* MAME32UTIL_H */
