#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#include <shlwapi.h>

#include "strconv.h"
#include "assoc.h"


static LONG reg_query_string(HKEY key, TCHAR *buf, DWORD buflen)
{
	LONG rc;
	DWORD type;

	buflen *= sizeof(*buf);
	rc = RegQueryValueEx(key, NULL, NULL, &type, (LPBYTE) buf, &buflen);
	if (rc != ERROR_SUCCESS)
		return rc;
	if (type != REG_SZ)
		return -1;
	return 0;
}



static void get_open_command(const struct win_association_info *assoc,
	TCHAR *buf, size_t buflen)
{
	int i;

	GetModuleFileName(GetModuleHandle(NULL), buf, buflen);
	
	for (i = 0; buf[i]; i++)
		buf[i] = toupper(buf[i]);
	buf[i++] = ' ';
	_tcscpy(&buf[i], assoc->open_args);
}



BOOL win_association_exists(const struct win_association_info *assoc)
{
	BOOL rc = FALSE;
	TCHAR buf[1024];
	TCHAR expected[1024];
	HKEY key1 = NULL;
	HKEY key2 = NULL;
	TCHAR *assoc_file_class = tstring_from_utf8(assoc->file_class);

	// first check to see if the extension is there at all
	if (RegOpenKey(HKEY_CLASSES_ROOT, assoc_file_class, &key1))
		goto done;

	if (RegOpenKey(key1, TEXT("shell\\open\\command"), &key2))
		goto done;

	if (reg_query_string(key2, buf, sizeof(buf) / sizeof(buf[0])))
		goto done;

	get_open_command(assoc, expected, sizeof(expected) / sizeof(expected[0]));
	rc = !_tcscmp(expected, buf);

done:
	if (key2)
		RegCloseKey(key2);
	if (key1)
		RegCloseKey(key1);
	free(assoc_file_class);
	return rc;
}



BOOL win_is_extension_associated(const struct win_association_info *assoc,
	const char *extension)
{
	HKEY key = NULL;
	TCHAR buf[256];
	BOOL rc = FALSE;
	TCHAR *t_extension = tstring_from_utf8(extension);

	// first check to see if the extension is there at all
	if (!win_association_exists(assoc))
		goto done;

	if (RegOpenKey(HKEY_CLASSES_ROOT, t_extension, &key))
		goto done;

	if (reg_query_string(key, buf, sizeof(buf) / sizeof(buf[0])))
		goto done;

	rc = !_tcscmp(buf, assoc->file_class);

done:
	if (key)
		RegCloseKey(key);
	if (t_extension)
		free(t_extension);
	return rc;
}



BOOL win_associate_extension(const struct win_association_info *assoc,
	const char *extension, BOOL is_set)
{
	HKEY key1 = NULL;
	HKEY key2 = NULL;
	HKEY key3 = NULL;
	HKEY key4 = NULL;
	HKEY key5 = NULL;
	DWORD disposition;
	TCHAR buf[1024];
	BOOL rc = FALSE;
	TCHAR *t_extension = tstring_from_utf8(extension);

	if (!is_set)
	{
		if (win_is_extension_associated(assoc, extension))
		{
			SHDeleteKey(HKEY_CLASSES_ROOT, t_extension);
		}
	}
	else
	{
		if (!win_association_exists(assoc))
		{
			if (RegCreateKeyEx(HKEY_CLASSES_ROOT, assoc->file_class, 0, NULL, 0,
					KEY_ALL_ACCESS, NULL, &key1, &disposition))
				goto done;
			if (RegCreateKeyEx(key1, TEXT("shell"), 0, NULL, 0,
					KEY_ALL_ACCESS, NULL, &key2, &disposition))
				goto done;
			if (RegCreateKeyEx(key2, TEXT("open"), 0, NULL, 0,
					KEY_ALL_ACCESS, NULL, &key3, &disposition))
				goto done;
			if (RegCreateKeyEx(key3, TEXT("command"), 0, NULL, 0,
					KEY_ALL_ACCESS, NULL, &key4, &disposition))
				goto done;

			get_open_command(assoc, buf, sizeof(buf) / sizeof(buf[0]));
			if (RegSetValue(key4, NULL, REG_SZ, buf, sizeof(buf)))
				goto done;
		}

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, t_extension, 0, NULL, 0,
				KEY_ALL_ACCESS, NULL, &key5, &disposition))
			goto done;
		if (RegSetValue(key5, NULL, REG_SZ, assoc->file_class,
				(_tcslen(assoc->file_class) + 1) * sizeof(TCHAR)))
			goto done;
	}

	rc = TRUE;

done:
	if (key5)
		RegCloseKey(key5);
	if (key4)
		RegCloseKey(key4);
	if (key3)
		RegCloseKey(key3);
	if (key2)
		RegCloseKey(key2);
	if (key1)
		RegCloseKey(key1);
	if (t_extension)
		free(t_extension);
	return rc;
}
