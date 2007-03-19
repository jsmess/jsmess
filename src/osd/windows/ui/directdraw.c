/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

	directdraw.c

	Direct Draw routines.
 
 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifndef _MSC_VER
#ifdef __GNUC__
#define NONAMELESSUNION
#else
#include <multidef.h>
#endif
#else
#ifndef NONAMELESSUNION
#define NONAMELESSUNION 
#endif

#endif
#include <ddraw.h>

#include "M32Util.h"
#include "screenshot.h"
#include "mame32.h"
#include "dxdecode.h"
#include "DirectDraw.h"

/***************************************************************************
	function prototypes
 ***************************************************************************/

static BOOL WINAPI DDEnumInfo(GUID FAR *lpGUID,
							  LPSTR 	lpDriverDescription,
							  LPSTR 	lpDriverName,		 
							  LPVOID	lpContext,
							  HMONITOR	hm);

static BOOL WINAPI DDEnumOldInfo(GUID FAR *lpGUID,
								 LPSTR	   lpDriverDescription,
								 LPSTR	   lpDriverName,		
								 LPVOID    lpContext);

static void CalculateDisplayModes(void);
static HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context);
static HRESULT CALLBACK EnumDisplayModesCallback2(DDSURFACEDESC2* pddsd, LPVOID Context);

/***************************************************************************
	External variables
 ***************************************************************************/

/***************************************************************************
	Internal structures
 ***************************************************************************/

typedef struct
{
   char* name;
   GUID* lpguid;
} display_type;

typedef HRESULT (WINAPI *ddc_proc)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD,
								   IUnknown FAR *pUnkOuter);

/***************************************************************************
	Internal variables
 ***************************************************************************/

#define MAX_DISPLAYS 100
static int					g_nNumDisplays;
static display_type 		g_Displays[MAX_DISPLAYS];

static HANDLE				g_hDLL;
static BOOL 				g_bHWStretch;
static BOOL 				g_bRefresh;
static struct tDisplayModes g_DisplayModes;
static IDirectDraw2*		g_pDirectDraw2;
static IDirectDraw4*		g_pDirectDraw4;

/***************************************************************************
	External functions	
 ***************************************************************************/

/****************************************************************************
 *		DirectDrawInitialize
 *
 *		Initialize the DirectDraw variables.
 *
 *		This entails the following functions:
 *
 *			DirectDrawCreate
 *
 ****************************************************************************/

#if !defined(LPDIRECTDRAWENUMERATE)
#if defined(UNICODE)

typedef HRESULT (WINAPI* LPDIRECTDRAWENUMERATEW)(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext); 

#define LPDIRECTDRAWENUMERATE	LPDIRECTDRAWENUMERATEW

#define SDirectDrawEnumerateEx "DirectDrawEnumerateExW"
#define SDirectDrawEnumerate   "DirectDrawEnumerateW"

#else

typedef HRESULT (WINAPI* LPDIRECTDRAWENUMERATEA)(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext); 

#define LPDIRECTDRAWENUMERATE	LPDIRECTDRAWENUMERATEA

#define SDirectDrawEnumerateEx "DirectDrawEnumerateExA"
#define SDirectDrawEnumerate   "DirectDrawEnumerateA"

#endif
#endif /* LPDIRECTDRAWENUMERATE */

/****************************************************************************/

BOOL DirectDraw_Initialize(void)
{
	HRESULT  hr;
	UINT	 error_mode;
	ddc_proc ddc;
	DDCAPS	 ddCaps;
	DDCAPS	 ddHelCaps;
	IDirectDraw* pDirectDraw1;
	LPDIRECTDRAWENUMERATEEX pDDEnumEx;

	if (g_hDLL != NULL)
		return TRUE;

	g_nNumDisplays = 0;
	g_hDLL		   = NULL;
	g_bHWStretch   = FALSE;
	g_bRefresh	   = FALSE;
	g_pDirectDraw2 = NULL;
	g_pDirectDraw4 = NULL;

	/* Turn off error dialog for this call */
	error_mode = SetErrorMode(0);
	g_hDLL = LoadLibrary("ddraw.dll");
	SetErrorMode(error_mode);

	if (g_hDLL == NULL)
		return FALSE;

	ddc = (ddc_proc)GetProcAddress(g_hDLL, "DirectDrawCreate");
	if (ddc == NULL)
		return FALSE;

	pDirectDraw1   = NULL;
	g_pDirectDraw2 = NULL;
	g_pDirectDraw4 = NULL;
	hr = ddc(NULL, &pDirectDraw1, NULL);
	if (FAILED(hr)) 
	{
		ErrorMsg("DirectDrawCreate failed: %s", DirectXDecodeError(hr));
		return FALSE;
	}

	hr = IDirectDraw_QueryInterface(pDirectDraw1, &IID_IDirectDraw4, (void**)&g_pDirectDraw4);
	if (FAILED(hr))
	{
		g_pDirectDraw4 = NULL;
		hr = IDirectDraw_QueryInterface(pDirectDraw1, &IID_IDirectDraw2, (void**)&g_pDirectDraw2);
		if (FAILED(hr))
		{
			ErrorMsg("Query Interface for DirectDraw 2 failed: %s", DirectXDecodeError(hr));
			g_pDirectDraw2 = NULL;
			IDirectDraw_Release(pDirectDraw1);
			return FALSE;
		}
	}
	
	memset(&ddCaps,    0, sizeof(DDCAPS));
	memset(&ddHelCaps, 0, sizeof(DDCAPS));
	ddCaps.dwSize	 = sizeof(DDCAPS);
	ddHelCaps.dwSize = sizeof(DDCAPS);
	hr = IDirectDraw_GetCaps(pDirectDraw1, &ddCaps, &ddHelCaps); /* 1 2 or 4? */
	if (FAILED(hr))
	{
		ErrorMsg("Error getting DirectDraw capabilities: %s", DirectXDecodeError(hr));
	}
	else
		g_bHWStretch = ((ddCaps.dwCaps & DDCAPS_BLTSTRETCH) != 0) ? TRUE : FALSE;

	IDirectDraw_Release(pDirectDraw1);

	/*
	   Note that you must know which version of the
	   function to retrieve (see the following text).
	   For this example, we use the ANSI version.
	 */
	pDDEnumEx = (LPDIRECTDRAWENUMERATEEX) GetProcAddress(g_hDLL, SDirectDrawEnumerateEx);

	/*
	   If the function is there, call it to enumerate all display devices
	   attached to the desktop, and any non-display DirectDraw devices.
	 */
	if (pDDEnumEx)
	{
		pDDEnumEx(DDEnumInfo, NULL, 
				  DDENUM_ATTACHEDSECONDARYDEVICES | DDENUM_DETACHEDSECONDARYDEVICES);
	}
	else
	{
		LPDIRECTDRAWENUMERATE lpDDEnum;

		lpDDEnum = (LPDIRECTDRAWENUMERATE) GetProcAddress(g_hDLL, SDirectDrawEnumerate);
		/*
		 * We must be running on an old version of ddraw. Therefore, 
		 * by definiton, multimon isn't supported. Fall back on
		 * DirectDrawEnumerate to enumerate standard devices on a 
		 * single monitor system.
		 */
		if (lpDDEnum)
		{
			lpDDEnum(DDEnumOldInfo, NULL);
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

/****************************************************************************
 *
 *		DirectDraw_Close
 *
 *		Terminate our usage of DirectDraw.
 *
 ****************************************************************************/

void DirectDraw_Close(void)
{
	int i;
	
	for (i = 0; i < g_nNumDisplays; i++)
	{
		free(g_Displays[i].name);
		g_Displays[i].name = NULL;
		if (g_Displays[i].lpguid != NULL)
		{
			free(g_Displays[i].lpguid);
			g_Displays[i].lpguid = NULL;
		}
		
	}
	g_nNumDisplays = 0;
	
	/*
		Destroy any lingering IDirectDraw object.
	*/
	if (g_pDirectDraw2) 
	{
		IDirectDraw2_Release(g_pDirectDraw2);
		g_pDirectDraw2 = NULL;
	}

	if (g_pDirectDraw4) 
	{
		IDirectDraw4_Release(g_pDirectDraw4);
		g_pDirectDraw4 = NULL;
	}

	if (g_hDLL)
	{
		FreeLibrary(g_hDLL);
		g_hDLL = NULL;
	}
}

/****************************************************************************/
/*
	Return a list of 16, 24 and 32 bit DirectDraw modes.
*/
struct tDisplayModes* DirectDraw_GetDisplayModes(void)
{
	if (g_DisplayModes.m_nNumModes == 0)
		CalculateDisplayModes();

	return &g_DisplayModes;
}

int DirectDraw_GetNumDisplays(void)
{
	return g_nNumDisplays;
}

BOOL DirectDraw_HasHWStretch(void)
{
	return g_bHWStretch;
}

BOOL DirectDraw_HasRefresh(void)
{
	return g_bRefresh;
}

const char* DirectDraw_GetDisplayName(int num_display)
{
	return g_Displays[num_display].name;
}

/****************************************************************************/
/* internal functions */
/****************************************************************************/

static BOOL WINAPI DDEnumInfo(GUID FAR *lpGUID,
							  LPSTR 	lpDriverDescription,
							  LPSTR 	lpDriverName,		 
							  LPVOID	lpContext,
							  HMONITOR	hm)
{
	g_Displays[g_nNumDisplays].name = malloc(strlen(lpDriverDescription) + 1);
	strcpy(g_Displays[g_nNumDisplays].name, lpDriverDescription);
	
	if (lpGUID == NULL)
		g_Displays[g_nNumDisplays].lpguid = NULL;
	else
	{
		g_Displays[g_nNumDisplays].lpguid = (LPGUID)malloc(sizeof(GUID));
		memcpy(g_Displays[g_nNumDisplays].lpguid, lpGUID, sizeof(GUID));
	}
	
	g_nNumDisplays++;
	if (g_nNumDisplays == MAX_DISPLAYS)
		return DDENUMRET_CANCEL;
	else
		return DDENUMRET_OK;
}

static BOOL WINAPI DDEnumOldInfo(GUID FAR *lpGUID,
								 LPSTR	   lpDriverDescription,
								 LPSTR	   lpDriverName,		
								 LPVOID    lpContext)
{
	return DDEnumInfo(lpGUID, lpDriverDescription, lpDriverName, lpContext, NULL);
}

static HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context)
{
	DWORD dwDepth = pddsd->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;

	struct tDisplayModes* pDisplayModes = (struct tDisplayModes*)Context;
	if (dwDepth == 16
	||	dwDepth == 24
	||	dwDepth == 32)
	{
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwWidth   = pddsd->dwWidth;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwHeight  = pddsd->dwHeight;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwBPP	   = dwDepth;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwRefresh = 0;
		pDisplayModes->m_nNumModes++;
	}

	if (pDisplayModes->m_nNumModes == MAXMODES)
		return DDENUMRET_CANCEL;
	else
		return DDENUMRET_OK;
}

static HRESULT CALLBACK EnumDisplayModesCallback2(DDSURFACEDESC2* pddsd2, LPVOID Context)
{
	struct tDisplayModes* pDisplayModes = (struct tDisplayModes*)Context;

	DWORD dwDepth = pddsd2->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;

	if (dwDepth == 16
	||	dwDepth == 24
	||	dwDepth == 32)
	{
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwWidth   = pddsd2->dwWidth;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwHeight  = pddsd2->dwHeight;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwBPP	   = dwDepth;
		pDisplayModes->m_Modes[pDisplayModes->m_nNumModes].m_dwRefresh = pddsd2->DUMMYUNIONNAMEN(2).dwRefreshRate;
		pDisplayModes->m_nNumModes++;

		if (pddsd2->DUMMYUNIONNAMEN(2).dwRefreshRate != 0)
			g_bRefresh = TRUE;
	}
	
	if (pDisplayModes->m_nNumModes == MAXMODES)
		return DDENUMRET_CANCEL;
	else
		return DDENUMRET_OK;
}

static void CalculateDisplayModes(void)
{
	g_DisplayModes.m_nNumModes = 0;

	if (g_pDirectDraw4)
		IDirectDraw4_EnumDisplayModes(g_pDirectDraw4, DDEDM_REFRESHRATES, NULL, &g_DisplayModes, EnumDisplayModesCallback2);
	else
	if (g_pDirectDraw2)
		IDirectDraw2_EnumDisplayModes(g_pDirectDraw2, 0, NULL, &g_DisplayModes, EnumDisplayModesCallback);
}


