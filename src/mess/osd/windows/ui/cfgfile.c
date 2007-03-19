
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <math.h>
#include <direct.h>
#include <driver.h>

#include "screenshot.h"
#include "bitmask.h"
#include "mame32.h"
#include "m32util.h"
#include "resource.h"
#include "treeview.h"
#include "file.h"
#include "splitters.h"
#include "dijoystick.h"
#include "audit.h"
#include "optcore.h"
#include "options.h"
#include "picker.h"
#include "windows/config.h"



static BOOL SettingsFileName(DWORD nSettingsFile, char *buffer, size_t bufsize)
{
	extern FOLDERDATA g_folderData[];
	extern LPEXFOLDERDATA ExtraFolderData[];
	char *ptr;
	int i;
	BOOL success = TRUE;
	char title[32];
	DWORD arg = nSettingsFile & 0x0FFFFFFF;
	const char *pParent;

	switch(nSettingsFile & 0xF0000000)
	{
		case SETTINGS_FILE_UI:
			_snprintf(buffer, bufsize, "%s", MAME32NAME "ui.ini");
			break;

		case SETTINGS_FILE_GLOBAL:
			_snprintf(buffer, bufsize, "%s", "default.cfg");
			break;

		case SETTINGS_FILE_GAME:
			snprintf(buffer, bufsize, "%s", drivers[arg]->name);
			break;

		case SETTINGS_FILE_FOLDER:
			snprintf(buffer, bufsize, "%s", g_folderData[arg].m_lpTitle);
			break;

		case SETTINGS_FILE_EXFOLDER:
			snprintf(buffer, bufsize, "%s", ExtraFolderData[arg]->m_szTitle );
			break;

		case SETTINGS_FILE_SOURCEFILE:
			//we have a source ini to create, so remove the ".c" at the end of the title
			assert(arg >= 0);
			assert(arg < GetNumGames());
			strncpy(title, drivers[arg]->source_file, strlen(drivers[arg]->source_file)-2 );
			title[strlen(drivers[arg]->source_file)-2] = '\0';

			//Core expects it there
			snprintf(buffer, bufsize, "drivers\\%s", title );
			break;

		case SETTINGS_FILE_EXFOLDERPARENT:
			pParent = GetFolderNameByID(ExtraFolderData[arg]->m_nParent);
			snprintf(buffer, bufsize, "%s\\%s", pParent, ExtraFolderData[arg]->m_szTitle );
			break;

		default:
			success = FALSE;
			break;
	}
	return success;
}



BOOL LoadSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers)
{
	char buffer[256];
	mame_file *file = NULL;
	struct xml_data_node *node = NULL;
	BOOL success = FALSE;

	if (!SettingsFileName(nSettingsFile, buffer, sizeof(buffer) / sizeof(buffer[0])))
		goto done;

	file = mame_fopen(buffer, 0, FILETYPE_CONFIG, 0);
	if (!file)
		goto done;

	node = xml_file_read(file);
	if (!node)
		goto done;

done:
	if (node)
		xml_file_free(node);
	if (file)
		mame_fclose(file);
	return success;
}



BOOL SaveSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers)
{
	char buffer[256];
	mame_file *file = NULL;
	struct xml_data_node *node = NULL;
	BOOL success = FALSE;

	if (!SettingsFileName(nSettingsFile, buffer, sizeof(buffer) / sizeof(buffer[0])))
		goto done;

	file = mame_fopen(buffer, 0, FILETYPE_CONFIG, 0);
	if (!file)
		goto done;

	node = xml_file_read(file);
	if (!node)
		goto done;

	mame_fclose(file);
	file = mame_fopen(buffer, 0, FILETYPE_CONFIG, 1);
	if (!file)
		goto done;

	xml_file_write(node, file);

done:
	if (node)
		xml_file_free(node);
	if (file)
		mame_fclose(file);
	return success;
}
