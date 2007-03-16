#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <tchar.h>

#include "osdmess.h"
#include "utils.h"
#include "winutil.h"
#include "strconv.h"

//============================================================
//  win_error_to_mame_file_error
//============================================================

static file_error win_error_to_mame_file_error(DWORD error)
{
	file_error filerr;

	// convert a Windows error to a file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}


//============================================================
//	osd_get_temp_filename
//============================================================

file_error osd_get_temp_filename(char *buffer, size_t buffer_len, const char *basename)
{
	TCHAR tempbuf[MAX_PATH];
	TCHAR *t_filename;
	char *u_tempbuf;

	if (!basename)
		basename = "tempfile.tmp";

	GetTempPath(sizeof(tempbuf) / sizeof(tempbuf[0]), tempbuf);
	t_filename = tstring_from_utf8(basename);
	_tcscat(tempbuf, t_filename);
	free(t_filename);
	DeleteFile(tempbuf);

	u_tempbuf = utf8_from_tstring(tempbuf);
	snprintf(buffer, buffer_len, "%s", u_tempbuf);
	free(u_tempbuf);

	return FILERR_NONE;
}


//============================================================
//	osd_copyfile
//============================================================

file_error osd_copyfile(const char *destfile, const char *srcfile)
{
	file_error filerr = FILERR_NONE;
	TCHAR *t_destfile = tstring_from_utf8(destfile);
	TCHAR *t_srcfile = tstring_from_utf8(srcfile);

	if (!t_destfile || !t_srcfile)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!CopyFile(t_srcfile, t_destfile, TRUE))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (t_destfile)
		free(t_destfile);
	if (t_srcfile)
		free(t_srcfile);
	return filerr;
}


//============================================================
//  osd_stat
//============================================================

osd_directory_entry *osd_stat(const char *path)
{
	osd_directory_entry *result = NULL;
	TCHAR *t_path;
	HANDLE find = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA find_data;

	// convert the path to TCHARs
	t_path = tstring_from_utf8(path);
	if (t_path == NULL)
		goto done;

	// attempt to find the first file
	find = FindFirstFile(t_path, &find_data);
	if (find == INVALID_HANDLE_VALUE)
		goto done;

	// create an osd_directory_entry; be sure to make sure that the caller can
	// free all resources by just freeing the resulting osd_directory_entry
	result = (osd_directory_entry *) malloc(sizeof(*result) + strlen(path) + 1);
	if (!result)
		goto done;
	strcpy(((char *) result) + sizeof(*result), path);
	result->name = ((char *) result) + sizeof(*result);
	result->type = win_attributes_to_entry_type(find_data.dwFileAttributes);
	result->size = find_data.nFileSizeLow | ((UINT64) find_data.nFileSizeHigh << 32);

done:
	if (t_path)
		free(t_path);
	return result;
}



//============================================================
//  osd_is_path_separator
//============================================================

int osd_is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
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
//	osd_mkdir
//============================================================

file_error osd_mkdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!CreateDirectory(tempstr, NULL))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_rmdir
//============================================================

file_error osd_rmdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!RemoveDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_getcurdir
//============================================================

file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	file_error filerr = FILERR_NONE;
	char *tempstr = NULL;
	TCHAR path[_MAX_PATH];

	if (!GetCurrentDirectory(ARRAY_LENGTH(path), path))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

	tempstr = utf8_from_tstring(path);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}
	snprintf(buffer, buffer_len, "%s", tempstr);

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_setcurdir
//============================================================

file_error osd_setcurdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!SetCurrentDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}



/* ************************************************************************ */
/* Directories                                                              */
/* ************************************************************************ */

typedef struct {
	HANDLE hDir;
	WIN32_FIND_DATA finddata;
	BOOL bDone;
} OSD_WIN32_FIND_DATA;

static char szCurrentDirectory[MAX_PATH];

static void checkinit_curdir(void)
{
	if (szCurrentDirectory[0] == '\0') {
#ifdef UNDER_CE
		WCHAR szBuffer[MAX_PATH];
		GetModuleFileName(NULL, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
		wcsrchr(szBuffer, '\\')[1] = '\0';
		_snprintf(szCurrentDirectory, sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]), "%S", szBuffer);
#else
		const char *p;
		GetCurrentDirectoryA(sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]), szCurrentDirectory);
		/* Make sure there is a trailing backslash */
		p = strrchr(szCurrentDirectory, '\\');
		if (!p || p[1])
			strncatz(szCurrentDirectory, "\\", sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]));
#endif
	}
}

void resetdir(void)
{
	szCurrentDirectory[0] = '\0';
}

static int preserve_current_drive_letter(const char *path)
{
#ifdef UNDER_CE
	return 0;
#else
	return (path[1] != ':') && (path[1] != '\\');
#endif
}

/* This function takes in a pathname, and resolves it according to our
 * current directory
 */
const char *resolve_path(const char *path, char *buf, size_t buflen)
{
	char *s;

	checkinit_curdir();

	/* A bogus resolve? */
	if (!path || !path[0] || ((path[0] == '.') && (!path[1])))
		return szCurrentDirectory;

	/* Absolute pathname? */
	if ((path[0] == '\\') || (path[1] == ':')) {
		if (preserve_current_drive_letter(path)) {
			/* Absolute pathname without drive letter */
			buf[0] = szCurrentDirectory[0];
			buf[1] = szCurrentDirectory[1];
			buf[2] = '\0';
		}
		else {
			/* Absolute pathname with drive letter or UNC pathname */
			buf[0] = '\0';
		}
		strncatz(buf, path, buflen);
		return buf;
	}

	strncpyz(buf, szCurrentDirectory, buflen);

	while(path && (path[0] == '.')) {
		/* Assume that if it is not a ., then it is a .. */
		if (path[1] != '\\') {
			/* Up one directory */
			s = strrchr(buf, '\\');
			if (s && !s[1]) {
				/* A backslash at the very end doesn't count */
				*s = '\0';
				s = strrchr(buf, '\\');
			}

			if (s)
				*s = '\0';
			else
				strncatz(buf, "\\..", buflen);
		}
		path = strchr(path, '\\');
		if (path)
			path++;
	}

	if (path && *path) {
		strncatz(buf, path, buflen);
	}
	return buf;
}

#ifdef UNDER_CE
int osd_num_devices(void)
{
	return 1;
}

const char *osd_get_device_name(int idx)
{
	return "\\";
}
#else
int osd_num_devices(void)
{
	char szBuffer[128];
	char *p;
	int i;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	i = 0;
	for (p = szBuffer; *p; p += (strlen(p) + 1))
		i++;
	return i;
}

const char *osd_get_device_name(int idx)
{
	static char szBuffer[128];
	const char *p;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	p = szBuffer;
	while(idx--)
		p += strlen(p) + 1;

	return p;
}
#endif

static BOOL dir_exists(const char *s)
{
	osd_directory *dp;
	int rc = 0;

	dp = osd_opendir(s);
	if (dp)
	{
		rc = (osd_readdir(dp) != NULL);
		osd_closedir(dp);
	}
	return rc > 0;
}

void osd_change_directory(const char *dir)
{
	char buf[MAX_PATH];
	const char *s;

	s = resolve_path(dir, buf, sizeof(buf) / sizeof(buf[0]));
	if (s && (s != szCurrentDirectory))
	{
		/* If there is no trailing backslash, add one */
		if ((s == buf) && (buf[strlen(buf)-1] != '\\') && (buf[strlen(buf)-1] != '/'))
			strncatz(buf, "\\", sizeof(buf) / sizeof(buf[0]));

		if (dir_exists(s))
			strncpyz(szCurrentDirectory, s, sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]));
	}
}

void osd_change_device(const char *device)
{
	char szBuffer[3];
	szBuffer[0] = device[0];
	szBuffer[1] = ':';
	szBuffer[2] = '\0';
	osd_change_directory(szBuffer);
}
