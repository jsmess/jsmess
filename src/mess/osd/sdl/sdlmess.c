#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

#include "osdmess.h"
#include "utils.h"

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#endif

#ifdef SDLMAME_MACOSX
#include <Carbon/Carbon.h>
#endif

#if defined(SDLMAME_UNIX) && !defined(SDLMAME_MACOSX)
#include <X11/Xatom.h>
#endif

//============================================================
//   osd_getcurdir
//============================================================

file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	file_error filerr = FILERR_NONE;

	if (getcwd(buffer, buffer_len) == 0)
	{
		filerr = FILERR_FAILURE;
	}

	if( filerr != FILERR_FAILURE )
	{
		if( strcmp( &buffer[strlen(buffer)-1], PATH_SEPARATOR ) != 0 )
		{
		        strncat( buffer, PATH_SEPARATOR, buffer_len );
		}
	}

	return filerr;
}
//============================================================
//	osd_setcurdir
//============================================================

file_error osd_setcurdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	if (chdir(dir) != 0)
	{
		filerr = FILERR_FAILURE;
	}

	return filerr;
}

//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}

//============================================================
//	osd_dirname
//============================================================

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
		return NULL;

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}

// no idea what these are for, just cheese them
int osd_num_devices(void)
{
	return 1;
}

const char *osd_get_device_name(int idx)
{
	return "/";
}

//============================================================
//	osd_get_temp_filename
//============================================================

file_error osd_get_temp_filename(char *buffer, size_t buffer_len, const char *basename)
{
	char tempbuf[512];

	if (!basename)
		basename = "tempfile";

	sprintf(tempbuf, "/tmp/%s", basename);
	unlink(tempbuf);

	strncpy(buffer, tempbuf, buffer_len);

	return FILERR_NONE;
}

//============================================================
//	osd_mkdir
//============================================================

file_error osd_mkdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	#ifdef SDLMAME_WIN32
	if (mkdir(dir) != 0)
	#else
	if (mkdir(dir, 0700) != 0)
	#endif
	{
		filerr = FILERR_FAILURE;
	}

	return filerr;
}

//============================================================
//  osd_is_path_separator
//============================================================

int osd_is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
}


//============================================================
//	osd_rmdir
//============================================================

file_error osd_rmdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	if (rmdir(dir) != 0)
	{
		filerr = FILERR_FAILURE;
	}

	return filerr;
}

#ifdef SDLMAME_MACOSX
//============================================================
//	osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	char *result = NULL; /* core expects a malloced buffer of uft8 data */

	PasteboardRef pasteboard_ref;
	OSStatus err;
	PasteboardSyncFlags sync_flags;
	PasteboardItemID item_id;
	CFIndex flavor_count;
	CFArrayRef flavor_type_array;
	CFIndex flavor_index;
	ItemCount item_count;
	UInt32 item_index;
	Boolean	success = 0;
	
	err = PasteboardCreate(kPasteboardClipboard, &pasteboard_ref);

	if (!err)
	{
		sync_flags = PasteboardSynchronize( pasteboard_ref );
		
		err = PasteboardGetItemCount(pasteboard_ref, &item_count );
		
		for (item_index=1; item_index<=item_count; item_index++)
		{
			err = PasteboardGetItemIdentifier(pasteboard_ref, item_index, &item_id);
			
			if (!err)
			{
				err = PasteboardCopyItemFlavors(pasteboard_ref, item_id, &flavor_type_array);
				
				if (!err)
				{
					flavor_count = CFArrayGetCount(flavor_type_array);
				
					for (flavor_index = 0; flavor_index < flavor_count; flavor_index++)
					{
						CFStringRef flavor_type;
						CFDataRef flavor_data;
						
						flavor_type = (CFStringRef)CFArrayGetValueAtIndex(flavor_type_array, flavor_index);
						
						if (UTTypeConformsTo(flavor_type, CFSTR("public.utf16-plain-text")))
						{
							CFStringRef string_ref;
							CFIndex length;
							
							err = PasteboardCopyItemFlavorData(pasteboard_ref, item_id, flavor_type, &flavor_data);
							string_ref = CFStringCreateWithBytes(NULL, CFDataGetBytePtr(flavor_data), CFDataGetLength(flavor_data), kCFStringEncodingUTF16, false );
							length = CFStringGetLength(string_ref) * 2;
							result = malloc(length);
							success = CFStringGetCString(string_ref, result, length, kCFStringEncodingUTF8);
							
							if (!success)
							{
								free(result);
								result = NULL;
							}
				
							CFRelease(string_ref);
							CFRelease(flavor_data);
							break;
						}
					}
				
					CFRelease(flavor_type_array);
				}
			}
			
			if (success)
				break;
		}

		CFRelease(pasteboard_ref);
	}
	
	return result;
}
#endif

#ifdef SDLMAME_WIN32
//============================================================
//	get_clipboard_text_by_format
//============================================================

static char *get_clipboard_text_by_format(UINT format, char *(*convert)(LPCVOID data))
{
	char *result = NULL;
	HANDLE data_handle;
	LPVOID data;

	// check to see if this format is available
	if (IsClipboardFormatAvailable(format))
	{
		// open the clipboard
		if (OpenClipboard(NULL))
		{
			// try to access clipboard data
			data_handle = GetClipboardData(format);
			if (data_handle != NULL)
			{
				// lock the data
				data = GlobalLock(data_handle);
				if (data != NULL)
				{
					// invoke the convert
					result = (*convert)(data);

					// unlock the data
					GlobalUnlock(data_handle);
				}
			}

			// close out the clipboard
			CloseClipboard();
		}
	}
	return result;
}



//============================================================
//	convert_wide
//============================================================

static char *convert_wide(LPCVOID data)
{
	return utf8_from_wstring((LPCWSTR) data);
}



//============================================================
//	convert_ansi
//============================================================

static char *convert_ansi(LPCVOID data)
{
	return utf8_from_astring((LPCSTR) data);
}



//============================================================
//	osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	char *result = NULL;

	// try to access unicode text
	if (result == NULL)
		result = get_clipboard_text_by_format(CF_UNICODETEXT, convert_wide);

	// try to access ANSI text
	if (result == NULL)
		result = get_clipboard_text_by_format(CF_TEXT, convert_ansi);

	return result;
}
#endif

#if defined(SDLMAME_UNIX) && !defined(SDLMAME_MACOSX)
//============================================================
//	osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	char *result = NULL;

	return result;
}
#endif
