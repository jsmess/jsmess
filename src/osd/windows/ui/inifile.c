/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <direct.h>
#include <driver.h>
#include <ctype.h>

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

/***************************************************************************
    Internal defines
 ***************************************************************************/

#define UI_INI_FILENAME MAME32NAME "ui.ini"

/***************************************************************************/

// out of a string, parse two substrings (non-blanks, or quoted).
// we modify buffer, and return key and value to point to the substrings, or NULL
static void ParseKeyValueStrings(char *buffer,char **key,char **value)
{
	char *ptr;
	BOOL quoted;

	*key = NULL;
	*value = NULL;

	ptr = buffer;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	*key = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of key
			if (*ptr == 0)
				return;

			*ptr = '\0';
			ptr++;
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	*value = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of value;
			*ptr = '\0';
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	if (**key == '\"')
		(*key)++;

	if (**value == '\"')
		(*value)++;

	if (**key == '\0')
	{
		*key = NULL;
		*value = NULL;
	}
}



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
			_snprintf(buffer, bufsize, "%s", UI_INI_FILENAME);
			break;

		case SETTINGS_FILE_GLOBAL:
			i = snprintf(buffer, bufsize, "%s\\", GetIniDir());
			buffer += i;
			bufsize -= 1;

			GetModuleFileName(GetModuleHandle(NULL), buffer, bufsize);

			// take out the path if there is one
			ptr = strrchr(buffer, '\\');
			if (ptr != NULL)
			{
				// move length of string, minus the backslash but plus the terminating 0
				memmove(buffer, ptr+1, strlen(ptr));
			}

			// take out the extension
			ptr = strrchr(buffer, '.');
			if (ptr)
				*ptr = '\0';

			// add .ini
			strcat(buffer, ".ini");
			break;

		case SETTINGS_FILE_GAME:
			snprintf(buffer, bufsize, "%s\\%s.ini", GetIniDir(), drivers[arg]->name);
			break;

		case SETTINGS_FILE_FOLDER:
			// use lower case
			snprintf(title, sizeof(title), "%s", g_folderData[arg].m_lpTitle);
			for (i = 0; title[i]; i++)
				title[i] = tolower(title[i]);

			snprintf(buffer, bufsize, "%s\\%s.ini", GetIniDir(), title);
			break;

		case SETTINGS_FILE_EXFOLDER:
			snprintf(buffer, bufsize, "%s\\%s.ini", GetIniDir(), ExtraFolderData[arg]->m_szTitle );
			break;

		case SETTINGS_FILE_SOURCEFILE:
			assert(arg >= 0);
			assert(arg < driver_get_count());

			strcpy(title, GetDriverFilename(arg));

			// we have a source ini to create, so remove the ".c" at the end of the title
			title[strlen(title) - 2] = '\0';

			//Core expects it there
			snprintf(buffer, bufsize, "%s\\drivers\\%s.ini", GetIniDir(), title );
			break;

		case SETTINGS_FILE_EXFOLDERPARENT:
			pParent = GetFolderNameByID(ExtraFolderData[arg]->m_nParent);
			snprintf(buffer, bufsize, "%s\\%s\\%s.ini", GetIniDir(), pParent, ExtraFolderData[arg]->m_szTitle );
			break;

		default:
			success = FALSE;
			break;
	}
	return success;
}



BOOL LoadSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers)
{
	FILE *fptr;
	char buffer[512];
	char *key,*value_str;
	int i;
	BOOL success;
	const REG_OPTION *option;

	// open the settings INI file
	if (!SettingsFileName(nSettingsFile, buffer, sizeof(buffer) / sizeof(buffer[0])))
		return FALSE;
	fptr = fopen(buffer, "rt");
	if (fptr == NULL)
		return FALSE;

	while (fgets(buffer, sizeof(buffer), fptr) != NULL)
	{
		// strip trailing spaces and endlines
		i = strlen(buffer);
		while((i > 0) && isspace(buffer[i-1]))
			buffer[--i] = '\0';

		// continue if it was an empty line
		if (i == 0)
			continue;
		
		// # starts a comment, but #* is a special MAME32 code
		// saying it's an option for us, but NOT for the main
		// MAME
		if (buffer[0] == '#')
		{
			if (buffer[1] != '*')
				continue;
		}

		ParseKeyValueStrings(buffer,&key,&value_str);
		if (key == NULL || value_str == NULL)
		{
			dprintf("invalid line [%s]",buffer);
			continue;
		}

		success = FALSE;
		for (i = 0; !success && handlers[i].type; i++)
		{
			switch(handlers[i].type)
			{
				case SH_OPTIONSTRUCT:
					option = GetOption(handlers[i].u.option_struct.option_array, key);
					if (option)
					{
						LoadOption(handlers[i].u.option_struct.option_struct, option, value_str);
						success = TRUE;
					}
					break;

				case SH_MANUAL:
					success = handlers[i].u.manual.parse(nSettingsFile, key, value_str);
					break;

				default:
					assert(0);
					break;
			}
		}
		
		if (!success)
		{
			dprintf("load game options found unknown option %s\n", key);
		}
	}

	fclose(fptr);
	return TRUE;
}



/* ----------------------------------------------------------------------- */

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value)
{
	if (value[0] && !strchr(value,' '))
		fprintf(fptr,"%-21s   %s\n",key,value);
	else
		fprintf(fptr,"%-21s   \"%s\"\n",key,value);
}

static void WriteIntOptionToFile(FILE *fptr,const char *key,int value)
{
	fprintf(fptr,"%-21s   %i\n",key,value);
}

static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value)
{
	fprintf(fptr,"%-21s   %i\n",key,value ? 1 : 0);
}

static void WriteColorOptionToFile(FILE *fptr,const char *key,COLORREF value)
{
	fprintf(fptr,"%-21s   %i,%i,%i\n",key,(int)(value & 0xff),(int)((value >> 8) & 0xff),
			(int)((value >> 16) & 0xff));
}

static void WriteOptionToFile(FILE *fptr, const void *option_struct, const REG_OPTION *regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	const char *key = regOpt->ini_name;
	char	cTemp[1000];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*) ptr_offset(option_struct, regOpt->m_iDataOffset);
        _gcvt( *pDouble, 10, cTemp );
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	case RO_COLOR :
	{
		COLORREF color = *(COLORREF *) ptr_offset(option_struct, regOpt->m_iDataOffset);
		if (color != (COLORREF)-1)
			WriteColorOptionToFile(fptr,key,color);
		break;
	}

	case RO_STRING:
		pString = *((char **) ptr_offset(option_struct, regOpt->m_iDataOffset));
		if (pString != NULL && pString[0] != 0)
		    WriteStringOptionToFile(fptr, key, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*) ptr_offset(option_struct, regOpt->m_iDataOffset);
		WriteBoolOptionToFile(fptr, key, *pBool);
		break;

	case RO_INT:
		pInt = (int*) ptr_offset(option_struct, regOpt->m_iDataOffset);
		WriteIntOptionToFile(fptr, key, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(ptr_offset(option_struct, regOpt->m_iDataOffset), cTemp);
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	default:
		break;
	}

}



static void EmitCallback(void *param, const char *key, const char *value_str, const char *comment)
{
	FILE *fptr = (FILE *) param;

	if (comment && comment[0])
		fprintf(fptr, "# %s\n", comment);
	WriteStringOptionToFile(fptr, key, value_str);
}



BOOL SaveSettingsFileEx(DWORD nSettingsFile, const struct SettingsHandler *handlers)
{
	char buffer[512];
	FILE *fptr;
	int qualifier_param;
	const void *option_struct;
	const void *comparison_struct;
	const REG_OPTION *option_array;
	char *s;
	const char *base_filename;
	int i, j;

	// open the settings INI file
	if (!SettingsFileName(nSettingsFile, buffer, sizeof(buffer) / sizeof(buffer[0])))
		return FALSE;

	// special case
	if ((handlers[0].type == SH_OPTIONSTRUCT)
		&& (handlers[1].type == SH_END)
		&& (handlers[0].u.option_struct.comparison_struct)
		&& AreOptionsEqual(
			handlers[0].u.option_struct.option_array,
			handlers[0].u.option_struct.option_struct,
			handlers[0].u.option_struct.comparison_struct))
	{
		// same as comparison; delete
		if (GetFileAttributes(buffer) != 0xFFFFFFFF)
		{
			// We successfully obtained the Attributes, so File exists, and we can delete it
			if (DeleteFile(buffer) == 0)
				dprintf("error deleting %s; error %d\n",buffer, GetLastError());

			// Try to delete extra directories
			s = strrchr(buffer, '\\');
			if (*s)
				*s = '\0';
			if (strchr(buffer, '\\'))
				RemoveDirectory(buffer);
		}
		return TRUE;
	}

	// create any requisite directories
	s = buffer;
	while((s = strchr(s, '\\')) != NULL)
	{
		*s = '\0';
		if (GetFileAttributes(buffer) == 0xFFFFFFFF)
			CreateDirectory(buffer, NULL);
		*s = '\\';
		s++;
	}

	// open the file
	fptr = fopen(buffer, "wt");
	if (fptr == NULL)
		return FALSE;

	base_filename = strrchr(buffer, '\\');
	base_filename = base_filename ? base_filename + 1 : buffer;
	fprintf(fptr, "### %s ###\n\n", base_filename);

	for (i = 0; handlers[i].type; i++)
	{
		if (i > 0)
			fprintf(fptr, "\n");
		if (handlers[i].comment)
			fprintf(fptr, "### %s ###\n\n", handlers[i].comment);
		else if (handlers[1].type != SH_END)
			fprintf(fptr, "### section #%d ###\n\n", i + 1);

		switch(handlers[i].type)
		{
			case SH_OPTIONSTRUCT:
				option_struct = handlers[i].u.option_struct.option_struct;
				option_array = handlers[i].u.option_struct.option_array;
				comparison_struct = handlers[i].u.option_struct.comparison_struct;
				qualifier_param = handlers[i].u.option_struct.qualifier_param;

				for (j = 0; option_array[j].ini_name[0]; j++)
				{
					if (!comparison_struct || !IsOptionEqual(&option_array[j], option_struct, comparison_struct))
					{
						if (!option_array[j].m_pfnQualifier || option_array[j].m_pfnQualifier(qualifier_param))
						{
							WriteOptionToFile(fptr, option_struct, &option_array[j]);
						}
					}
				}
				break;

			case SH_MANUAL:
				handlers[i].u.manual.emit(nSettingsFile, EmitCallback, (void *) fptr);
				break;

			default:
				assert(0);
				break;
		}
	}
	fclose(fptr);
	return TRUE;
}



