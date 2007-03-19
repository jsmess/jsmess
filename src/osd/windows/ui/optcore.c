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

const REG_OPTION *GetOption(const REG_OPTION *option_array, const char *key)
{
	int i;

	for (i = 0; option_array[i].ini_name[0]; i++)
	{
		if (strcmp(option_array[i].ini_name,key) == 0)
			return &option_array[i];
	}
	return NULL;
}



void LoadOption(void *option_struct, const REG_OPTION *option, const char *value_str)
{
	//dprintf("trying to load %s type %i [%s]",option->ini_name,option->m_iType,value_str);
	void *ptr;

	ptr = ((char *) option_struct) + option->m_iDataOffset;

	switch (option->m_iType)
	{
	case RO_DOUBLE :
		*((double *)ptr) = atof(value_str);
		break;

	case RO_COLOR :
	{
		unsigned int r,g,b;
		if (sscanf(value_str,"%u,%u,%u",&r,&g,&b) == 3)
			*((COLORREF *)ptr) = RGB(r,g,b);
		else if (!strchr(value_str, ',') && (atoi(value_str) == -1))
			*((COLORREF *)ptr) = -1;
		break;
	}

	case RO_STRING:
		if (*(char**)ptr != NULL)
			free(*(char**)ptr);
		*(char **)ptr = mame_strdup(value_str);
		break;

	case RO_BOOL:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)ptr) = (value_int != 0);
		break;
	}

	case RO_INT:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)ptr) = value_int;
		break;
	}

	case RO_ENCODE:
		option->decode(value_str, ptr);
		break;

	default:
		break;
	}
}



void LoadDefaultOptions(void *option_struct, const REG_OPTION *option_array)
{
	int i;
	for (i = 0; option_array[i].ini_name[0]; i++)
	{
		if (option_array[i].m_pDefaultValue)
			LoadOption(option_struct, &option_array[i], option_array[i].m_pDefaultValue);
	}
}



BOOL CopyOptionStruct(void *dest, const void *source, const REG_OPTION *option_array, size_t struct_size)
{
	int i;
	BOOL success = TRUE;

	memcpy(dest, source, struct_size);

	// now there's a bunch of strings in dest that need to be reallocated
	// to be a separate copy
	for (i = 0; option_array[i].ini_name[0]; i++)
	{
		if (option_array[i].m_iType == RO_STRING)
		{
			char **string_to_copy = 
				(char **)((char *)dest + option_array[i].m_iDataOffset);
			if (*string_to_copy != NULL)
			{
				*string_to_copy = mame_strdup(*string_to_copy);
				if (!*string_to_copy)
					success = FALSE;
			}
		}
	}
	return success;
}



void FreeOptionStruct(void *option_struct, const REG_OPTION *options)
{
	int i;
	for (i = 0; options[i].ini_name[0]; i++)
	{
		if (options[i].m_iType == RO_STRING)
		{
			FreeIfAllocated((char **) ptr_offset(option_struct, options[i].m_iDataOffset));
		}
	}
}




BOOL IsOptionEqual(const REG_OPTION *opt, const void *o1, const void *o2)
{
	switch(opt->m_iType)
	{
	case RO_DOUBLE:
	{
		double a,b;
		a = *(double *) ptr_offset(o1, opt->m_iDataOffset);
		b = *(double *) ptr_offset(o2, opt->m_iDataOffset);
		return fabs(a-b) < 0.000001;
	}
	case RO_COLOR :
	{
		COLORREF a,b;
		a = *(COLORREF *) ptr_offset(o1, opt->m_iDataOffset);
		b = *(COLORREF *) ptr_offset(o2, opt->m_iDataOffset);
		return a == b;
	}
	case RO_STRING:
	{
		char *a,*b;
		a = *(char **) ptr_offset(o1, opt->m_iDataOffset);
		b = *(char **) ptr_offset(o2, opt->m_iDataOffset);
		if( a != NULL && b != NULL )
			return strcmp(a,b) == 0;
		else 
			return FALSE;
	}
	case RO_BOOL:
	{
		BOOL a,b;
		a = *(BOOL *) ptr_offset(o1, opt->m_iDataOffset);
		b = *(BOOL *) ptr_offset(o2, opt->m_iDataOffset);
		return a == b;
	}
	case RO_INT:
	{
		int a,b;
		a = *(int *) ptr_offset(o1, opt->m_iDataOffset);
		b = *(int *) ptr_offset(o2, opt->m_iDataOffset);
		return a == b;
	}
	case RO_ENCODE:
	{
		char a[1000],b[1000];
		opt->encode(ptr_offset(o1, opt->m_iDataOffset), a);
		opt->encode(ptr_offset(o2, opt->m_iDataOffset), b);
		if( a != NULL && b != NULL )
			return strcmp(a,b) == 0;
		else 
			return FALSE;
	}

	default:
		break;
	}

	return TRUE;
}



BOOL AreOptionsEqual(const REG_OPTION *option_array, const void *o1, const void *o2)
{
	int i;
	for (i = 0; option_array[i].ini_name[0]; i++)
	{
		if (!IsOptionEqual(&option_array[i], o1, o2))
			return FALSE;
	}
	return TRUE;
}



// --------------------------------------------------------------------------

BOOL LoadSettingsFile(DWORD nSettingsFile, void *option_struct, const REG_OPTION *option_array)
{
	struct SettingsHandler handlers[2];

	memset(handlers, 0, sizeof(handlers));
	handlers[0].type = SH_OPTIONSTRUCT;
	handlers[0].u.option_struct.option_struct = option_struct;
	handlers[0].u.option_struct.option_array = option_array;
	handlers[1].type = SH_END;

	return LoadSettingsFileEx(nSettingsFile, handlers);
}



BOOL SaveSettingsFile(DWORD nSettingsFile, const void *option_struct, const void *comparison_struct, const REG_OPTION *option_array)
{
	struct SettingsHandler handlers[2];

	memset(handlers, 0, sizeof(handlers));
	handlers[0].type = SH_OPTIONSTRUCT;
	handlers[0].u.option_struct.option_struct = (void *) option_struct;
	handlers[0].u.option_struct.comparison_struct = comparison_struct;
	handlers[0].u.option_struct.option_array = option_array;
	handlers[1].type = SH_END;

	return SaveSettingsFileEx(nSettingsFile, handlers);
}

