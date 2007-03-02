//============================================================
//
//  sdlwork.c - SDL core directory access functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#define _LARGEFILE64_SOURCE
#ifndef SDLMAME_FREEBSD
#define _XOPEN_SOURCE 500
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __USE_BSD	// to get DT_xxx on Linux
#undef _POSIX_C_SOURCE  // to get DT_xxx on OS X
#include <dirent.h>

#include "osdcore.h"

struct _osd_directory
{
	osd_directory_entry ent;
	struct dirent *data;
	DIR *fd;
};


#ifndef SDLMAME_WIN32
static osd_dir_entry_type get_attributes_enttype(int attributes)
{
	if (attributes == DT_DIR)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}
#else
static osd_dir_entry_type get_attributes_w32(const char *file)
{
	struct stat st;
	if(stat(file, &st))
		return 0;

	if (S_ISBLK(st.st_mode)) return ENTTYPE_DIR;

	return ENTTYPE_FILE;
}
#endif

static UINT64 osd_get_file_size(const char *file)
{
#if defined(SDLMAME_MACOSX) || defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_FREEBSD)
	struct stat st;
	if(stat(file, &st))
		return 0;
#else
	struct stat64 st;
	if(stat64(file, &st))
		return 0;
#endif
	return st.st_size;
}

//============================================================
//  osd_opendir
//============================================================

osd_directory *osd_opendir(const char *dirname)
{
	osd_directory *dir = NULL;

	dir = malloc(sizeof(*dir));
	if (dir)
	{
		memset(dir, 0, sizeof(*dir));
		dir->fd = NULL;
	}

	dir->fd = opendir(dirname);

	if (dir && (dir->fd == NULL))
	{
		free(dir);
		dir = NULL;
	}

	return dir;
}


//============================================================
//  osd_readdir
//============================================================

const osd_directory_entry *osd_readdir(osd_directory *dir)
{
	dir->data = readdir(dir->fd);

	if (dir->data == NULL)
		return NULL;

	dir->ent.name = dir->data->d_name;
	#ifdef SDLMAME_WIN32
	dir->ent.type = get_attributes_w32(dir->data->d_name);
	#else
	dir->ent.type = get_attributes_enttype(dir->data->d_type);
	#endif
	dir->ent.size = osd_get_file_size(dir->data->d_name);
	return &dir->ent;
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir)
{
	if (dir->fd != NULL)
		closedir(dir->fd);
	free(dir);
}

