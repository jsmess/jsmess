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
#include "strconv.h"
#endif

#ifdef SDLMAME_MACOSX
#include <Carbon/Carbon.h>
#endif

//============================================================
//   osd_getcurdir
//============================================================

file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	file_error filerr = FILERR_NONE;

	if (getcwd(buffer, buffer_len) == NULL)
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
//	osd_get_full_path
//============================================================

file_error osd_get_full_path(char **dst, const char *path)
{
	file_error err;
	char path_buffer[512];

	err = FILERR_NONE;

	if (getcwd(path_buffer, 511) == NULL)
	{
		printf("osd_get_full_path: failed!\n");
		err = FILERR_FAILURE;
	}
	else
	{
		*dst = (char *)malloc(strlen(path_buffer)+strlen(path)+3);

		// if it's already a full path, just pass it through
		if (path[0] == '/')
		{
			strcpy(*dst, path);
		}
		else
		{
			sprintf(*dst, "%s%s%s", path_buffer, PATH_SEPARATOR, path);
		}
	}

	return err;
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
	char *result = NULL; /* core expects a malloced C string of uft8 data */

	PasteboardRef pasteboard_ref;
	OSStatus err;
	PasteboardSyncFlags sync_flags;
	PasteboardItemID item_id;
	CFIndex flavor_count;
	CFArrayRef flavor_type_array;
	CFIndex flavor_index;
	ItemCount item_count;
	UInt32 item_index;
	Boolean	success = false;
	
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
						CFStringEncoding encoding;
						CFStringRef string_ref;
						CFDataRef data_ref;
						CFIndex length;
						CFRange range;
						
						flavor_type = (CFStringRef)CFArrayGetValueAtIndex(flavor_type_array, flavor_index);
						
						if (UTTypeConformsTo (flavor_type, kUTTypeUTF16PlainText))
							encoding = kCFStringEncodingUTF16;
						else if (UTTypeConformsTo (flavor_type, kUTTypeUTF8PlainText))
							encoding = kCFStringEncodingUTF8;
						else if (UTTypeConformsTo (flavor_type, kUTTypePlainText))
							encoding = kCFStringEncodingMacRoman;
						else
							continue;

						err = PasteboardCopyItemFlavorData(pasteboard_ref, item_id, flavor_type, &flavor_data);
						
						if( !err )
						{
							string_ref = CFStringCreateFromExternalRepresentation (kCFAllocatorDefault, flavor_data, encoding);
							data_ref = CFStringCreateExternalRepresentation (kCFAllocatorDefault, string_ref, kCFStringEncodingUTF8, '?');
							
							length = CFDataGetLength (data_ref);
							range = CFRangeMake (0,length);
							
							result = malloc (length+1);
							if (result != NULL)
							{
								CFDataGetBytes (data_ref, range, (unsigned char *)result);
								result[length] = 0;
								success = true;
								break;
							}

							CFRelease(data_ref);
							CFRelease(string_ref);
							CFRelease(flavor_data);
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

#if defined(SDL_VIDEO_DRIVER_X11)
//============================================================
//	osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	SDL_SysWMinfo info;
	Display* display;
	Window our_win;
	Window selection_win;
	Atom data_type;
	int data_format;
	unsigned long nitems;
	unsigned long bytes_remaining;
	unsigned char* prop;
	char* result;
	XEvent event;
	Uint32 t0, t1;
	Atom types[2];
	int i;

	/* get & validate SDL sys-wm info */
	SDL_VERSION(&info.version);
	if ( ! SDL_GetWMInfo( &info ) ) 
		return NULL;
	if ( info.subsystem != SDL_SYSWM_X11 ) 
		return NULL;
	if ( (display = info.info.x11.display) == NULL ) 
		return NULL;
	if ( (our_win = info.info.x11.window) == None ) 
		return NULL;

	/* request data to owner */
	selection_win = XGetSelectionOwner( display, XA_PRIMARY ); 
	if ( selection_win == None ) 
		return NULL;

	/* first, try UTF-8, then latin-1 */
	types[0] = XInternAtom( display, "UTF8_STRING", False );
	types[1] = XA_STRING; /* latin-1 */

	for ( i = 0; i < ARRAY_LENGTH(types); i++ )
	{

		XConvertSelection( display, XA_PRIMARY, types[i], types[i], our_win, CurrentTime );

		/* wait for SelectionNotify, but no more than 100 ms */
		t0 = t1 = SDL_GetTicks();
		while ( 1 )
		{
			if (  XCheckTypedWindowEvent( display, our_win,  SelectionNotify, &event ) ) break;
			SDL_Delay( 1 );
			t1 = SDL_GetTicks();
			if ( t1 - t0 > 100 )
				return NULL;
		}
		if ( event.xselection.property == None )
			continue;

		/* get property & check its type */
		if ( XGetWindowProperty( display, our_win, types[i], 0, 65536, False, types[i],
					 &data_type, &data_format, &nitems, &bytes_remaining, &prop ) 
		     != Success )
			continue;
		if ( ! prop ) 
			continue;
		if ( (data_format != 8) || (data_type != types[i]) )
		{
			XFree( prop );
			continue;
		}
		
		/* return a copy & free original */
		result = core_strdup( (char*) prop );
		XFree( prop );
		return result;
	}

	return NULL;
}

#endif

#if defined(SDLMAME_UNIX) && !defined(SDLMAME_MACOSX) && !defined(SDL_VIDEO_DRIVER_X11)
//============================================================
//	osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	char *result = NULL;

	return result;
}
#endif
