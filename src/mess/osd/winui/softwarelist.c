//============================================================
//
//  softwarelist.c - MESS's software picker
//
//============================================================

#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>
#include <winuser.h>
#include <tchar.h>
#include <ctype.h>

#include "emu.h"
#include "unzip.h"
#include "strconv.h"
#include "hashfile.h"
#include "picker.h"
#include "screenshot.h"
#include "bitmask.h"
#include "winui.h"
#include "resourcems.h"
#include "mui_opts.h"
#include "softwarelist.h"
#include "mui_util.h"



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _file_info file_info;
struct _file_info
{
	const device_config *device;

	// hash information
	char hash_string[HASH_BUF_SIZE];
	BOOL hash_realized;
	const hash_info *hashinfo;

	const char *zip_entry_name;
	char list_name[256];
	char description[256];
	char file_name[256];
	char publisher[256];
	char year[10];
};

typedef struct _directory_search_info directory_search_info;
struct _directory_search_info
{
	directory_search_info *next;
	HANDLE find_handle;
	WIN32_FIND_DATA fd;
	char directory_name[1];
};

typedef struct _software_list_info software_list_info;
struct _software_list_info
{
	WNDPROC old_window_proc;
	file_info **file_index;
	int file_index_length;
	int hashes_realized;
	int current_position;
	directory_search_info *first_search_info;
	directory_search_info *last_search_info;
	const software_config *config;
};



//============================================================
//  CONSTANTS
//============================================================

static const TCHAR software_list_property_name[] = TEXT("SWLIST");




static software_list_info *GetSoftwareListInfo(HWND hwndPicker)
{
	HANDLE h;
	h = GetProp(hwndPicker, software_list_property_name);
	assert(h);
	return (software_list_info *) h;
}



LPCSTR SoftwareList_LookupFilename(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->file_name;
}



const device_config *SoftwareList_LookupDevice(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
	{
		return NULL;
	}
	return pPickerInfo->file_index[nIndex]->device;
}



int SoftwareList_LookupIndex(HWND hwndPicker, LPCSTR pszFilename)
{
	software_list_info *pPickerInfo;
	int i;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	for (i = 0; i < pPickerInfo->file_index_length; i++)
	{
		if (!mame_stricmp(pPickerInfo->file_index[i]->file_name, pszFilename))
			return i;
	}
	return -1;
}



iodevice_t SoftwareList_GetImageType(HWND hwndPicker, int nIndex)
{
	iodevice_t type;
	const device_config *device = SoftwareList_LookupDevice(hwndPicker, nIndex);

	if (device != NULL)
	{
		type = image_device_getinfo(GetSoftwareListInfo(hwndPicker)->config->mconfig, device).type;
	}
	else
	{
		type = IO_UNKNOWN;
	}
	return type;
}



void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config)
{
	int i;
	software_list_info *pPickerInfo;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	pPickerInfo->config = config;

	// invalidate the hash "realization"
	for (i = 0; i < pPickerInfo->file_index_length; i++)
	{
		pPickerInfo->file_index[i]->hashinfo = NULL;
		pPickerInfo->file_index[i]->hash_realized = FALSE;
	}
	pPickerInfo->hashes_realized = 0;
}



static void ComputeFileHash(software_list_info *pPickerInfo,
	file_info *pFileInfo, const unsigned char *pBuffer, unsigned int nLength)
{
	image_device_info info;
	unsigned int functions;

	// get the device info
	info = image_device_getinfo(pPickerInfo->config->mconfig, pFileInfo->device);

	// determine which functions to use
	functions = hashfile_functions_used(pPickerInfo->config->hashfile, info.type);

	// compute the hash
	image_device_compute_hash(pFileInfo->hash_string, pFileInfo->device, pBuffer, nLength, functions);
}



static BOOL SoftwareList_CalculateHash(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	file_info *pFileInfo;
	LPSTR pszZipName;
	BOOL rc = FALSE;
	unsigned char *pBuffer;
	unsigned int nLength;
	HANDLE hFile, hFileMapping;
	LVFINDINFO lvfi;
	BOOL res;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	assert((nIndex >= 0) && (nIndex < pPickerInfo->file_index_length));
	pFileInfo = pPickerInfo->file_index[nIndex];

	if (pFileInfo->zip_entry_name)
	{
		// this is in a ZIP file
		zip_file *zip;
		zip_error ziperr;
		const zip_file_header *zipent;

		// open the ZIP file
		nLength = pFileInfo->zip_entry_name - pFileInfo->file_name;
		pszZipName = (LPSTR) alloca(nLength);
		memcpy(pszZipName, pFileInfo->file_name, nLength);
		pszZipName[nLength - 1] = '\0';

		// get the entry name
		ziperr = zip_file_open(pszZipName, &zip);
		if (ziperr == ZIPERR_NONE)
		{
			zipent = zip_file_first_file(zip);
			while(!rc && zipent)
			{
				if (!mame_stricmp(zipent->filename, pFileInfo->zip_entry_name))
				{
					pBuffer = (unsigned char *)malloc(zipent->uncompressed_length);
					if (pBuffer)
					{
						ziperr = zip_file_decompress(zip, pBuffer, zipent->uncompressed_length);
						if (ziperr == ZIPERR_NONE)
						{
							ComputeFileHash(pPickerInfo, pFileInfo, pBuffer, zipent->uncompressed_length);
							rc = TRUE;
						}
						free(pBuffer);
					}
				}
				zipent = zip_file_next_file(zip);
			}
			zip_file_close(zip);
		}
	}
	else
	{
		// plain open file; map it into memory and calculate the hash
		hFile = win_create_file_utf8(pFileInfo->file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			nLength = GetFileSize(hFile, NULL);
			hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (hFileMapping)
			{
				pBuffer = (unsigned char *)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
				if (pBuffer)
				{
					ComputeFileHash(pPickerInfo, pFileInfo, pBuffer, nLength);
					UnmapViewOfFile(pBuffer);
					rc = TRUE;
				}
				CloseHandle(hFileMapping);
			}
			CloseHandle(hFile);
		}
	}

	if (rc)
	{
		memset(&lvfi, 0, sizeof(lvfi));
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = nIndex;
		nIndex = ListView_FindItem(hwndPicker, -1, &lvfi);
		if (nIndex > 0)
			res = ListView_RedrawItems(hwndPicker, nIndex, nIndex);
	}

	return rc;
}



static void SoftwareList_RealizeHash(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	file_info *pFileInfo;
	unsigned int nHashFunctionsUsed = 0;
	unsigned int nCalculatedHashes = 0;
	iodevice_t type;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	assert((nIndex >= 0) && (nIndex < pPickerInfo->file_index_length));
	pFileInfo = pPickerInfo->file_index[nIndex];

	// Determine which hash functions we need to use for this file, and which hashes
	// have already been calculated
	if ((pPickerInfo->config->hashfile != NULL) && (pFileInfo->device != NULL))
	{
		image_device_info info = image_device_getinfo(pPickerInfo->config->mconfig, pFileInfo->device);
		type = info.type;
		if (type < IO_COUNT)
	        nHashFunctionsUsed = hashfile_functions_used(pPickerInfo->config->hashfile, type);
		nCalculatedHashes = hash_data_used_functions(pFileInfo->hash_string);
	}

	// Did we fully compute all hashes?
	if ((nHashFunctionsUsed & ~nCalculatedHashes) == 0)
	{
		// We have calculated all hashs for this file; mark it as realized
		pPickerInfo->file_index[nIndex]->hash_realized = TRUE;
		pPickerInfo->hashes_realized++;

		if (pPickerInfo->config->hashfile)
		{
			pPickerInfo->file_index[nIndex]->hashinfo = hashfile_lookup(pPickerInfo->config->hashfile,
				pPickerInfo->file_index[nIndex]->hash_string);
		}
	}
}


BOOL SoftwareList_AddFile(HWND hwndPicker,LPCSTR pszName, LPCSTR pszListname, LPCSTR pszDescription, LPCSTR pszPublisher, LPCSTR pszYear)
{
	Picker_ResetIdle(hwndPicker);
	
	software_list_info *pPickerInfo;
	file_info **ppNewIndex;
	file_info *pInfo;
	int nIndex, nSize;
//	LPCSTR pszExtension = NULL;
	const device_config *device = NULL;

	// first check to see if it is already here
	if (SoftwareList_LookupIndex(hwndPicker, pszName) >= 0)
		return TRUE;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);

	// create the FileInfo structure
	nSize = sizeof(file_info);
	pInfo = (file_info *) malloc(nSize);
	if (!pInfo)
		goto error;
	memset(pInfo, 0, nSize);

	// copy the filename
	strcpy(pInfo->file_name, pszName);
	strcpy(pInfo->list_name, pszListname);
	strcpy(pInfo->description, pszDescription);
	strcpy(pInfo->publisher, pszPublisher);
	strcpy(pInfo->year, pszYear);

	// set up device and CRC, if specified
	pInfo->device = device;
	
	ppNewIndex = (file_info**)malloc((pPickerInfo->file_index_length + 1) * sizeof(*pPickerInfo->file_index));
	memcpy(ppNewIndex,pPickerInfo->file_index,pPickerInfo->file_index_length * sizeof(*pPickerInfo->file_index));
	if (pPickerInfo->file_index) free(pPickerInfo->file_index);
	if (!ppNewIndex)
		goto error;

	nIndex = pPickerInfo->file_index_length++;
	pPickerInfo->file_index = ppNewIndex;
	pPickerInfo->file_index[nIndex] = pInfo;

	// Realize the hash
	SoftwareList_RealizeHash(hwndPicker, nIndex);

	// Actually insert the item into the picker
	Picker_InsertItemSorted(hwndPicker, nIndex);
	return TRUE;

error:
	if (pInfo)
		free(pInfo);
	return FALSE;	
}


static void SoftwareList_FreeSearchInfo(directory_search_info *pSearchInfo)
{
	if (pSearchInfo->find_handle != INVALID_HANDLE_VALUE)
		FindClose(pSearchInfo->find_handle);
	free(pSearchInfo);
}



static void SoftwareList_InternalClear(software_list_info *pPickerInfo)
{
	directory_search_info *p;
	int i;

	for (i = 0; i < pPickerInfo->file_index_length; i++)
		free(pPickerInfo->file_index[i]);

	while(pPickerInfo->first_search_info)
	{
		p = pPickerInfo->first_search_info->next;
		SoftwareList_FreeSearchInfo(pPickerInfo->first_search_info);
		pPickerInfo->first_search_info = p;
	}

	pPickerInfo->file_index = NULL;
	pPickerInfo->file_index_length = 0;
	pPickerInfo->current_position = 0;
	pPickerInfo->last_search_info = NULL;
}



void SoftwareList_Clear(HWND hwndPicker)
{
	software_list_info *pPickerInfo;
	BOOL res;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	SoftwareList_InternalClear(pPickerInfo);
	res = ListView_DeleteAllItems(hwndPicker);
}


BOOL SoftwareList_Idle(HWND hwndPicker)
{
	software_list_info *pPickerInfo;
	file_info *pFileInfo;
	static const char szWildcards[] = "\\*.*";
	directory_search_info *pSearchInfo;
	LPSTR pszFilter;
	BOOL bSuccess;
	int nCount;
	BOOL bDone = FALSE;
	image_device_info info;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);

	pSearchInfo = pPickerInfo->first_search_info;

	if (pSearchInfo)
	{
		// searching through directories
		if (pSearchInfo->find_handle != INVALID_HANDLE_VALUE)
		{
			bSuccess = FindNextFile(pSearchInfo->find_handle, &pSearchInfo->fd);
		}
		else
		{
			pszFilter = (LPSTR)alloca(strlen(pSearchInfo->directory_name) + strlen(szWildcards) + 1);
			strcpy(pszFilter, pSearchInfo->directory_name);
			strcat(pszFilter, szWildcards);
			pSearchInfo->find_handle = win_find_first_file_utf8(pszFilter, &pSearchInfo->fd);
			bSuccess = pSearchInfo->find_handle != INVALID_HANDLE_VALUE;
		}

		if (bSuccess)
		{
			//SoftwareList_AddEntry(hwndPicker, pSearchInfo);
		}
		else
		{
			pPickerInfo->first_search_info = pSearchInfo->next;
			if (!pPickerInfo->first_search_info)
				pPickerInfo->last_search_info = NULL;
			SoftwareList_FreeSearchInfo(pSearchInfo);
		}
	}
	else if (pPickerInfo->config!=NULL  && pPickerInfo->config->hashfile && (pPickerInfo->hashes_realized
		< pPickerInfo->file_index_length))
	{
		// time to realize some hashes
		nCount = 100;

		while((nCount > 0) && pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized)
		{
			pPickerInfo->current_position++;
			pPickerInfo->current_position %= pPickerInfo->file_index_length;
			nCount--;
		}

		pFileInfo = pPickerInfo->file_index[pPickerInfo->current_position];
		if (!pFileInfo->hash_realized)
		{
			info = image_device_getinfo(pPickerInfo->config->mconfig, pFileInfo->device);
			if (hashfile_functions_used(pPickerInfo->config->hashfile, info.type))
			{
				// only calculate the hash if it is appropriate for this device
				if (!SoftwareList_CalculateHash(hwndPicker, pPickerInfo->current_position))
					return FALSE;
			}
			SoftwareList_RealizeHash(hwndPicker, pPickerInfo->current_position);

			// under normal circumstances this will be redundant, but in the unlikely
			// event of a failure, we do not want to keep running into a brick wall
			// by calculating this hash over and over
			if (!pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized)
			{
				pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized = TRUE;
				pPickerInfo->hashes_realized++;
			}
		}
	}
	else
	{
		// we are done!
		bDone = TRUE;
	}

	return !bDone;
}



LPCTSTR SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength)
{
	software_list_info *pPickerInfo;
	const file_info *pFileInfo;
	LPCTSTR s = NULL;
	TCHAR* t_buf;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nRow < 0) || (nRow >= pPickerInfo->file_index_length))
		return NULL;

	pFileInfo = pPickerInfo->file_index[nRow];

	switch(nColumn)
	{
		case MESS_COLUMN_IMAGES:
			t_buf = tstring_from_utf8(pFileInfo->file_name);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 1:
			t_buf = tstring_from_utf8(pFileInfo->list_name);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 2:
			t_buf = tstring_from_utf8(pFileInfo->description);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 3:
			t_buf = tstring_from_utf8(pFileInfo->year);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 4:
			t_buf = tstring_from_utf8(pFileInfo->publisher);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		
	}
	return s;
}



static LRESULT CALLBACK SoftwareList_WndProc(HWND hwndPicker, UINT nMessage,
	WPARAM wParam, LPARAM lParam)
{
	software_list_info *pPickerInfo;
	LRESULT rc;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	rc = CallWindowProc(pPickerInfo->old_window_proc, hwndPicker, nMessage, wParam, lParam);

	if (nMessage == WM_DESTROY)
	{
		SoftwareList_InternalClear(pPickerInfo);
		SoftwareList_SetDriver(hwndPicker, NULL);
		free(pPickerInfo);
	}

	return rc;
}



BOOL SetupSoftwareList(HWND hwndPicker, const struct PickerOptions *pOptions)
{
	software_list_info *pPickerInfo = NULL;
	LONG_PTR l;

	if (!SetupPicker(hwndPicker, pOptions))
		goto error;

	pPickerInfo = (software_list_info *)malloc(sizeof(*pPickerInfo));
	if (!pPickerInfo)
		goto error;

	memset(pPickerInfo, 0, sizeof(*pPickerInfo));
	if (!SetProp(hwndPicker, software_list_property_name, (HANDLE) pPickerInfo))
		goto error;

	l = (LONG_PTR) SoftwareList_WndProc;
	l = SetWindowLongPtr(hwndPicker, GWLP_WNDPROC, l);
	pPickerInfo->old_window_proc = (WNDPROC) l;
	return TRUE;

error:
	if (pPickerInfo)
		free(pPickerInfo);
	return FALSE;
}
