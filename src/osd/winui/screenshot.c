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

  Screenshot.c
  
    Win32 DIB handling.
    
      Created 7/1/98 by Mike Haaland (mhaaland@hypertech.com)
      
***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

// MAME/MAMEUI headers
#include "emu.h"
#include "png.h"
#include "osdepend.h"
#include "unzip.h"

#include "mui_opts.h"
#include "mui_util.h"
#include "winui.h"


/***************************************************************************
    function prototypes
***************************************************************************/

static BOOL     AllocatePNG(png_info *p, HGLOBAL *phDIB, HPALETTE* pPal);

static int png_read_bitmap_gui(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *pPAL);
/***************************************************************************
    Static global variables
***************************************************************************/

/* these refer to the single image currently loaded by the ScreenShot functions */
static HGLOBAL   m_hDIB = NULL;
static HPALETTE  m_hPal = NULL;
static HANDLE m_hDDB = NULL;
static int current_image_game = -1;
static int current_image_type = -1;

#define WIDTHBYTES(width) ((width) / 8)

/* PNG variables */

static int   copy_size = 0;
static char* pixel_ptr = 0;
static int   row = 0;
static int   effWidth;

/***************************************************************************
    Functions
***************************************************************************/

BOOL ScreenShotLoaded(void)
{
	return m_hDDB != NULL;
}

#ifdef MESS
static BOOL LoadSoftwareScreenShot(const game_driver *drv, LPCSTR lpSoftwareName, int nType)
{
	BOOL result;
	char *s = (char*)alloca(strlen(drv->name) + 1 + strlen(lpSoftwareName) + 5);
	sprintf(s, "%s/%s.png", drv->name, lpSoftwareName);
	result = LoadDIB(s, &m_hDIB, &m_hPal, nType);
	return result;
}
#endif /* MESS */

/* Allow us to pre-load the DIB once for future draws */
#ifdef MESS
BOOL LoadScreenShotEx(int nGame, LPCSTR lpSoftwareName, int nType)
#else /* !MESS */
BOOL LoadScreenShot(int nGame, int nType)
#endif /* MESS */
{
	BOOL loaded = FALSE;
	/* No need to reload the same one again */
#ifndef MESS
	if (nGame == current_image_game && nType == current_image_type)
		return TRUE;
#endif

	/* Delete the last ones */
	FreeScreenShot();

	/* Load the DIB */
#ifdef MESS
	if (lpSoftwareName)
	{
		int nParentIndex = -1;
		loaded = LoadSoftwareScreenShot(drivers[nGame], lpSoftwareName, nType);
		if (!loaded && DriverIsClone(nGame) == TRUE)
		{
			nParentIndex = GetParentIndex(drivers[nGame]);
			loaded = LoadSoftwareScreenShot(drivers[nParentIndex], lpSoftwareName, nType);
		}
	}
	if (!loaded)
#endif /* MESS */
	{
		loaded = LoadDIB(drivers[nGame]->name, &m_hDIB, &m_hPal, nType);
	}

	/* If not loaded, see if there is a clone and try that */
	if (!loaded)
	{
		int nParentIndex = GetParentIndex(drivers[nGame]);
		if( nParentIndex >= 0)
		{
			loaded = LoadDIB(drivers[nParentIndex]->name, &m_hDIB, &m_hPal, nType);
			nParentIndex = GetParentIndex(drivers[nParentIndex]);
			if (!loaded && nParentIndex >= 0)
				loaded = LoadDIB(drivers[nParentIndex]->name, &m_hDIB, &m_hPal, nType);
		}
	}

#ifndef MESS
	/* MSH 20071029 - If driver is broken and no images exists, look for nonworking.png */
	if (!loaded && DriverIsBroken(nGame))
	{
		loaded = LoadDIB("nonworking", &m_hDIB, &m_hPal, nType);
	}
#endif

	if (loaded)
	{
		HDC hdc = GetDC(GetMainWindow());
		m_hDDB = DIBToDDB(hdc, m_hDIB, NULL);
		ReleaseDC(GetMainWindow(),hdc);
		
		current_image_game = nGame;
		current_image_type = nType;

	}

	return (loaded) ? TRUE : FALSE;
}

HANDLE GetScreenShotHandle()
{
	return m_hDDB;
}

int GetScreenShotWidth(void)
{
	return ((LPBITMAPINFO)m_hDIB)->bmiHeader.biWidth;
}

int GetScreenShotHeight(void)
{
	return ((LPBITMAPINFO)m_hDIB)->bmiHeader.biHeight;
}

/* Delete the HPALETTE and Free the HDIB memory */
void FreeScreenShot(void)
{
	if (m_hDIB != NULL)
		GlobalFree(m_hDIB);
	m_hDIB = NULL;

	if (m_hPal != NULL)
		DeletePalette(m_hPal);
	m_hPal = NULL;

	if (m_hDDB != NULL)
		DeleteObject(m_hDDB);
	m_hDDB = NULL;

	current_image_game = -1;
	current_image_type = -1;
}

static const zip_file_header *zip_file_seek_file(zip_file *zip, const char *filename)
{
	const zip_file_header *header;
	char *new_filename;
	int i;

	// we need to change filename; allocate a copy
	new_filename = (char*)malloc(strlen(filename) + 1);
	if (!new_filename)
		return NULL;
	
	// change all backslashes to forward slashes
	for (i = 0; filename[i]; i++)
		new_filename[i] = (filename[i] != '\\') ? filename[i] : '/';
	new_filename[i] = '\0';

	// find the entry
	header = zip_file_first_file(zip);
	while(header && mame_stricmp(header->filename, new_filename))
	{
		header = zip_file_next_file(zip);
	}

	free(new_filename);
	return header;
}

static file_error OpenDIBFile(const char *dir_name, const char *zip_name, const char *filename,
	core_file **file, void **buffer)
{
	file_error filerr;
	zip_error ziperr;
	zip_file *zip;
	const zip_file_header *zip_header;
	astring *fname;

	// clear out result
	*file = NULL;

	// look for the raw file
	fname = astring_assemble_3(astring_alloc(), dir_name, PATH_SEPARATOR, filename);
	filerr = core_fopen(astring_c(fname), OPEN_FLAG_READ, file);
	astring_free(fname);

	// did the raw file not exist?
	if (filerr != FILERR_NONE)
	{
		// look into zip file
		fname = astring_assemble_4(astring_alloc(), dir_name, PATH_SEPARATOR, zip_name, ".zip");
		ziperr = zip_file_open(astring_c(fname), &zip);
		astring_free(fname);
		if (ziperr == ZIPERR_NONE)
		{
			zip_header = zip_file_seek_file(zip, filename);
			if (zip_header != NULL)
			{
				*buffer = malloc(zip_header->uncompressed_length);
				ziperr = zip_file_decompress(zip, *buffer, zip_header->uncompressed_length);
				if (ziperr == ZIPERR_NONE)
				{
					filerr = core_fopen_ram(*buffer, zip_header->uncompressed_length, OPEN_FLAG_READ, file);
				}
			}
			zip_file_close(zip);
		}
	}
	return filerr;
}

BOOL LoadDIB(const char *filename, HGLOBAL *phDIB, HPALETTE *pPal, int pic_type)
{
	file_error filerr;
	core_file *file = NULL;
	BOOL success = FALSE;
	const char *dir_name;
	const char *zip_name;
	astring *fname;
	void *buffer = NULL;
	if (pPal != NULL ) {
		DeletePalette(pPal);
	}

	switch (pic_type)
	{
		case TAB_SCREENSHOT:
			dir_name = GetImgDir();
			zip_name = "snap";
			break;
		case TAB_FLYER:
			dir_name = GetFlyerDir();
			zip_name = "flyers";
			break;
		case TAB_CABINET:
			dir_name = GetCabinetDir();
			zip_name = "cabinets";
			break;
		case TAB_MARQUEE:
			dir_name = GetMarqueeDir();
			zip_name = "marquees";
			break;
		case TAB_TITLE:
			dir_name = GetTitlesDir();
			zip_name = "titles";
			break;
		case TAB_CONTROL_PANEL:
			dir_name = GetControlPanelDir();
			zip_name = "cpanel";
			break;
        case TAB_PCB :
			dir_name = GetPcbDir();
		    zip_name = "pcb";
			break;
		case BACKGROUND:
			dir_name = GetBgDir();
			zip_name = "bkground";
			break;
		default :
			// in case a non-image tab gets here, which can happen
			return FALSE;
	}
	//Add handling for the displaying of all the different supported snapshot patterntypes
	//%g
	fname = astring_assemble_2(astring_alloc(), filename, ".png");
	filerr = OpenDIBFile(dir_name, zip_name, astring_c(fname), &file, &buffer);
	astring_free(fname);
	if (filerr != FILERR_NONE) {
		//%g/%i
		fname = astring_assemble_3(astring_alloc(), filename, PATH_SEPARATOR, "0000.png");
		filerr = OpenDIBFile(dir_name, zip_name, astring_c(fname), &file, &buffer);
		astring_free(fname);
	}
	if (filerr != FILERR_NONE) {
		//%g%i
		fname = astring_assemble_2(astring_alloc(), filename, "0000.png");
		filerr = OpenDIBFile(dir_name, zip_name, astring_c(fname), &file, &buffer);
		astring_free(fname);
	}
	if (filerr != FILERR_NONE) {
		//%g/%g
		fname = astring_assemble_4(astring_alloc(), filename, PATH_SEPARATOR, filename, ".png");
		filerr = OpenDIBFile(dir_name, zip_name, astring_c(fname), &file, &buffer);
		astring_free(fname);
	}
	if (filerr != FILERR_NONE) {
		//%g/%g%i
		fname = astring_assemble_4(astring_alloc(), filename, PATH_SEPARATOR, filename, "0000.png");
		filerr = OpenDIBFile(dir_name, zip_name, astring_c(fname), &file, &buffer);
		astring_free(fname);
	}

	if (filerr == FILERR_NONE) {
		success = png_read_bitmap_gui(file, phDIB, pPal);
		core_fclose(file);
	}

	// free the buffer if we have to
	if (buffer != NULL) {
		free(buffer);
	}
	return success;
}

HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc)
{
	LPBITMAPINFOHEADER	lpbi;
	HBITMAP 			hBM;
	int 				nColors;
	BITMAPINFO *		bmInfo = (LPBITMAPINFO)hDIB;
	LPVOID				lpDIBBits;

	if (hDIB == NULL)
		return NULL;

	lpbi = (LPBITMAPINFOHEADER)hDIB;
	nColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1 << lpbi->biBitCount;

	if (bmInfo->bmiHeader.biBitCount > 8)
		lpDIBBits = (LPVOID)((LPDWORD)(bmInfo->bmiColors + 
			bmInfo->bmiHeader.biClrUsed) + 
			((bmInfo->bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
	else
		lpDIBBits = (LPVOID)(bmInfo->bmiColors + nColors);

	if (desc != 0)
	{
		/* Store for easy retrieval later */
		desc->bmWidth  = bmInfo->bmiHeader.biWidth;
		desc->bmHeight = bmInfo->bmiHeader.biHeight;
		desc->bmColors = (nColors <= 256) ? nColors : 0;
	}

	hBM = CreateDIBitmap(hDC,					  /* handle to device context */
						(LPBITMAPINFOHEADER)lpbi, /* pointer to bitmap info header	*/
						(LONG)CBM_INIT, 		  /* initialization flag */
						lpDIBBits,				  /* pointer to initialization data  */
						(LPBITMAPINFO)lpbi, 	  /* pointer to bitmap info */
						DIB_RGB_COLORS);		  /* color-data usage  */

	return hBM;
}


/***************************************************************************
    PNG graphics handling functions
***************************************************************************/

static void store_pixels(UINT8 *buf, int len)
{
	if (pixel_ptr && copy_size)
	{
		memcpy(&pixel_ptr[row * effWidth], buf, len);
		row--;
		copy_size -= len;
	}
}

BOOL AllocatePNG(png_info *p, HGLOBAL *phDIB, HPALETTE *pPal)
{
	int 				dibSize;
	HGLOBAL 			hDIB;
	BITMAPINFOHEADER	bi;
	LPBITMAPINFOHEADER	lpbi;
	LPBITMAPINFO		bmInfo;
	LPVOID				lpDIBBits = 0;
	int 				lineWidth = 0;
	int 				nColors = 0;
	RGBQUAD*			pRgb;
	copy_size = 0;
	pixel_ptr = 0;
	row 	  = p->height - 1;
	lineWidth = p->width;
	
	if (p->color_type != 2 && p->num_palette <= 256)
		nColors =  p->num_palette;

	bi.biSize			= sizeof(BITMAPINFOHEADER); 
	bi.biWidth			= p->width;
	bi.biHeight 		= p->height;
	bi.biPlanes 		= 1;
	bi.biBitCount		= (p->color_type == 3) ? 8 : 24; /* bit_depth; */
	
	bi.biCompression	= BI_RGB; 
	bi.biSizeImage		= 0; 
	bi.biXPelsPerMeter	= 0; 
	bi.biYPelsPerMeter	= 0; 
	bi.biClrUsed		= nColors;
	bi.biClrImportant	= nColors;
	
	effWidth = (long)(((long)lineWidth*bi.biBitCount + 31) / 32) * 4;
	
	dibSize = (effWidth * bi.biHeight);
	hDIB = GlobalAlloc(GMEM_FIXED, bi.biSize + (nColors * sizeof(RGBQUAD)) + dibSize);
	
	if (!hDIB)
	{
		return FALSE;
	}

	lpbi = (LPBITMAPINFOHEADER)hDIB;
	memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
	pRgb = (RGBQUAD*)((LPSTR)lpbi + bi.biSize);
	lpDIBBits = (LPVOID)((LPSTR)lpbi + bi.biSize + (nColors * sizeof(RGBQUAD)));
	if (nColors)
	{
		int i;
		/*
		  Convert a PNG palette (3 byte RGBTRIPLEs) to a new
		  color table (4 byte RGBQUADs)
		*/
		for (i = 0; i < nColors; i++)
		{
			RGBQUAD rgb;
			
			rgb.rgbRed		= p->palette[i * 3 + 0];
			rgb.rgbGreen	= p->palette[i * 3 + 1];
			rgb.rgbBlue 	= p->palette[i * 3 + 2];
			rgb.rgbReserved = (BYTE)0;
			
			pRgb[i] = rgb; 
		} 
	} 
	
	bmInfo = (LPBITMAPINFO)hDIB;
	
	/* Create a halftone palette if colors > 256. */
	if (0 == nColors || nColors > 256)
	{
		HDC hDC = CreateCompatibleDC(0); /* Desktop DC */
		*pPal = CreateHalftonePalette(hDC);
		DeleteDC(hDC);
	}
	else
	{
		UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);
		LOGPALETTE *pLP = (LOGPALETTE *)malloc(nSize);
		int  i;
		
		pLP->palVersion 	= 0x300;
		pLP->palNumEntries	= nColors;
		
		for (i = 0; i < nColors; i++)
		{
			pLP->palPalEntry[i].peRed	= bmInfo->bmiColors[i].rgbRed;
			pLP->palPalEntry[i].peGreen = bmInfo->bmiColors[i].rgbGreen; 
			pLP->palPalEntry[i].peBlue	= bmInfo->bmiColors[i].rgbBlue;
			pLP->palPalEntry[i].peFlags = 0;
		}
		
		*pPal = CreatePalette(pLP);
		
		free (pLP);
	}
	
	copy_size = dibSize;
	pixel_ptr = (char*)lpDIBBits;
	*phDIB = hDIB;
	return TRUE;
}

/* Copied and modified from png.c */
static int png_read_bitmap_gui(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *pPAL)
{
	png_info p;
	UINT32 i;
	int bytespp;
	
	if (png_read_file((core_file*)mfile, &p) != PNGERR_NONE)
		return 0;

	if (p.color_type != 3 && p.color_type != 2)
	{
		logerror("Unsupported color type %i (has to be 3)\n", p.color_type);
		png_free(&p);
		return 0;
	}
	if (p.interlace_method != 0)
	{
		logerror("Interlace unsupported\n");
		png_free(&p);
		return 0;
	}
 
	/* Convert < 8 bit to 8 bit */
	png_expand_buffer_8bit(&p);
	
	if (!AllocatePNG(&p, phDIB, pPAL))
	{
		logerror("Unable to allocate memory for artwork\n");
		png_free(&p);
		return 0;
	}

	bytespp = (p.color_type == 2) ? 3 : 1;

	for (i = 0; i < p.height; i++)
	{
		UINT8 *ptr = p.image + i * (p.width * bytespp);

		if (p.color_type == 2) /*(p->bit_depth > 8) */
		{
			int j;
			UINT8 bTmp;

			for (j = 0; j < p.width; j++)
			{
				bTmp = ptr[0];
				ptr[0] = ptr[2];
				ptr[2] = bTmp;
				ptr += 3;
			}
		}	
		store_pixels(p.image + i * (p.width * bytespp), p.width * bytespp);
	}

	png_free(&p);

	return 1;
}

#ifdef MESS
BOOL LoadScreenShot(int nGame, int nType)
{
	return LoadScreenShotEx(nGame, NULL, nType);
}
#endif

/* End of source */
