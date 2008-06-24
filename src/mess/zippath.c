/***************************************************************************

    zippath.c

    File/directory/path operations that work with ZIP files

***************************************************************************/

#include <ctype.h>

#include "zippath.h"
#include "unzip.h"
#include "osdmess.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _zippath_returned_directory zippath_returned_directory;
struct _zippath_returned_directory
{
	zippath_returned_directory *next;
	char name[1];
};



struct _zippath_directory
{
	/* common */
	unsigned int returned_parent : 1;
	osd_directory_entry returned_entry;

	/* specific to normal directories */
	osd_directory *directory;

	/* specific to ZIP directories */
	unsigned int called_zip_first : 1;
	zip_file *zipfile;
	astring *zipprefix;
	zippath_returned_directory *returned_dirlist;
};



/***************************************************************************
    PATH OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    parse_parent_path - parses out the parent path
-------------------------------------------------*/

static void parse_parent_path(const char *path, int *beginpos, int *endpos)
{
	int length = strlen(path);
	int pos;

	/* skip over trailing path separators */
	pos = length - 1;
	while((pos > 0) && osd_is_path_separator(path[pos]))
		pos--;

	/* return endpos */
	if (endpos != NULL)
		*endpos = pos;

	/* now skip until we find a path separator */
	while((pos > 0) && !osd_is_path_separator(path[pos]))
		pos--;

	/* return beginpos */
	if (beginpos != NULL)
		*beginpos = pos;
}



/*-------------------------------------------------
    zippath_parent - retrieves the parent directory
-------------------------------------------------*/

astring *zippath_parent(astring *dst, const char *path)
{
	int pos;
	parse_parent_path(path, &pos, NULL);

	/* return the result */
	return (pos > 0) ? astring_cpych(dst, path, pos + 1) : NULL;
}



/*-------------------------------------------------
    zippath_parent_basename - retrieves the parent
	directory basename
-------------------------------------------------*/

astring *zippath_parent_basename(astring *dst, const char *path)
{
	int beginpos, endpos;
	parse_parent_path(path, &beginpos, &endpos);

	return astring_cpych(dst, path + beginpos + 1, endpos - beginpos);
}



/*-------------------------------------------------
    zippath_combine - combines two paths
-------------------------------------------------*/

astring *zippath_combine(astring *dst, const char *path1, const char *path2)
{
	astring *result;

	if (!strcmp(path2, "."))
	{
		result = astring_cpyc(dst, path1);
	}
	else if (!strcmp(path2, ".."))
	{
		result = zippath_parent(dst, path1);
	}
	else if (osd_is_absolute_path(path2))
	{
		result = astring_cpyc(dst, path2);
	}
	else if ((path1[0] != '\0') && !osd_is_path_separator(path1[strlen(path1) - 1]))
	{
		result = astring_assemble_3(dst, path1, PATH_SEPARATOR, path2);		
	}
	else
	{
		result = astring_assemble_2(dst, path1, path2);		
	}
	return result;
}



/***************************************************************************
    DIRECTORY OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    is_root - tests to see if this path is the root
-------------------------------------------------*/

static int is_root(const char *path)
{
	int i = 0;

	/* skip drive letter */
	if (isalpha(path[i]) && (path[i + 1] == ':'))
		i += 2;

	/* skip path separators */
	while (osd_is_path_separator(path[i]))
		i++;

	return path[i] == '\0';
}



/*-------------------------------------------------
    is_zip_file - tests to see if this file is a
	ZIP file
-------------------------------------------------*/

static int is_zip_file(const char *path)
{
	const char *s = strrchr(path, '.');
	return (s != NULL) && !mame_stricmp(s, ".zip");
}



/*-------------------------------------------------
    zippath_resolve - separates a ZIP path out into
	true path and ZIP entry components
-------------------------------------------------*/

static file_error zippath_resolve(const char *path, osd_dir_entry_type *entry_type,
	zip_file **zipfile, astring *newpath)
{
	file_error err;
	osd_directory_entry *current_entry = NULL;
	osd_dir_entry_type current_entry_type;
	astring *apath = astring_cpyc(astring_alloc(), path);
	astring *apath_trimmed = astring_alloc();
	astring *parent = NULL;
	int went_up = FALSE;
	int i;

	/* be conservative */
	*entry_type = ENTTYPE_NONE;
	*zipfile = NULL;

	do
	{
		/* trim the path of trailing path separators */
		i = astring_len(apath);
		while((i > 0) && osd_is_path_separator(astring_c(apath)[i - 1]))
			i--;
		apath_trimmed = astring_cpysubstr(apath_trimmed, apath, 0, i);

		/* stat the path */
		current_entry = osd_stat(astring_c(apath_trimmed));
		if (current_entry == NULL)
		{
			err = FILERR_FAILURE;
			goto done;
		}

		/* get the entry type and free the stat entry */
		current_entry_type = current_entry->type;
		free(current_entry);
		current_entry = NULL;

		/* if we have not found the file or directory, go up */
		if (current_entry_type == ENTTYPE_NONE)
		{
			went_up = TRUE;
			parent = zippath_parent(astring_alloc(), astring_c(apath));
			astring_free(apath);
			apath = parent;
		}
	}
	while((current_entry_type == ENTTYPE_NONE) && (apath != NULL));

	/* if we did not find anything, then error out */
	if (current_entry_type == ENTTYPE_NONE)
	{
		err = FILERR_NOT_FOUND;
		goto done;
	}

	/* is this file a ZIP file? */
	if ((current_entry_type == ENTTYPE_FILE) && is_zip_file(astring_c(apath_trimmed))
		&& (zip_file_open(astring_c(apath_trimmed), zipfile) == ZIPERR_NONE))
	{
		/* this was a true ZIP path - copy out the sub path */
		astring_cpyc(newpath, path + astring_len(apath));
		*entry_type = (astring_len(newpath) == 0) || (astring_c(newpath)[astring_len(newpath)-1] == '/')
			? ENTTYPE_DIR : ENTTYPE_FILE;
	}
	else
	{
		/* this was a normal path */
		if (went_up)
		{
			err = FILERR_NOT_FOUND;
			goto done;
		}
		astring_cpyc(newpath, path);
		*entry_type = current_entry_type;
	}

	/* success! */
	err = FILERR_NONE;

done:
	if (apath != NULL)
		astring_free(apath);
	if (apath_trimmed != NULL)
		astring_free(apath_trimmed);
	return err;
}


/*-------------------------------------------------
    zippath_opendir - opens a directory
-------------------------------------------------*/

file_error zippath_opendir(const char *path, zippath_directory **directory)
{
	file_error err;
	osd_dir_entry_type entry_type;
	astring *newpath = astring_alloc();
	zippath_directory *result;

	/* allocate a directory */
	result = (zippath_directory *) malloc(sizeof(*result));
	if (result == NULL)
	{
		err = FILERR_OUT_OF_MEMORY;
		goto done;
	}
	memset(result, 0, sizeof(*result));

	/* resolve the path */
	err = zippath_resolve(path, &entry_type, &result->zipfile, newpath);
	if (err != FILERR_NONE)
		goto done;

	/* we have to be a directory */
	if (entry_type != ENTTYPE_DIR)
	{
		err = FILERR_NOT_FOUND;
		goto done;
	}

	/* was the result a ZIP? */
	if (result->zipfile != NULL)
	{
		result->zipprefix = newpath;
		newpath = NULL;
	}
	else
	{
		/* a conventional directory */
		result->directory = osd_opendir(path);
		if (result->directory == NULL)
		{
			err = FILERR_FAILURE;
			goto done;
		}

		/* is this path the root? if so, pretend we've already returned the parent */
		if (is_root(path))
			result->returned_parent = TRUE;
	}

done:
	if (((directory == NULL) || (err != FILERR_NONE)) && (result != NULL))
	{
		zippath_closedir(result);
		result = NULL;
	}
	if (newpath != NULL)
	{
		astring_free(newpath);
		newpath = NULL;
	}
	if (directory != NULL)
		*directory = result;
	return err;
}


/*-------------------------------------------------
    zippath_closedir - closes a directory
-------------------------------------------------*/

void zippath_closedir(zippath_directory *directory)
{
	zippath_returned_directory *dirlist;

	if (directory->directory != NULL)
		osd_closedir(directory->directory);

	if (directory->zipfile != NULL)
		zip_file_close(directory->zipfile);

	if (directory->zipprefix != NULL)
		astring_free(directory->zipprefix);

	while(directory->returned_dirlist != NULL)
	{
		dirlist = directory->returned_dirlist;
		directory->returned_dirlist = directory->returned_dirlist->next;
		free(dirlist);
	}

	free(directory);
}


/*-------------------------------------------------
    get_relative_path - checks to see if a specified
	header is in the zippath_directory, and if so
	returns the relative path
-------------------------------------------------*/

static const char *get_relative_path(zippath_directory *directory, const zip_file_header *header)
{
	const char *result = NULL;
	int len = astring_len(directory->zipprefix);

	if ((len <= strlen(header->filename))
		&& !strncmp(astring_c(directory->zipprefix), header->filename, len))
	{
		result = &header->filename[len];
	}

	return result;
}


/*-------------------------------------------------
    zippath_readdir - reads a directory
-------------------------------------------------*/

const osd_directory_entry *zippath_readdir(zippath_directory *directory)
{
	const osd_directory_entry *result = NULL;
	const zip_file_header *header;
	const char *relpath;
	const char *separator;
	zippath_returned_directory *rdent;

	if (!directory->returned_parent)
	{
		/* first thing's first - return parent directory */
		directory->returned_parent = TRUE;
		memset(&directory->returned_entry, 0, sizeof(directory->returned_entry));
		directory->returned_entry.name = "..";
		directory->returned_entry.type = ENTTYPE_DIR;
		result = &directory->returned_entry;						
	}
	else if (directory->directory != NULL)
	{
		/* a normal directory read */
		do
		{
			result = osd_readdir(directory->directory);
		}
		while((result != NULL) && (!strcmp(result->name, ".") || !strcmp(result->name, "..")));

		/* special case - is this entry a ZIP file?  if so we need to return it as a "directory" */
		if ((result != NULL) && is_zip_file(result->name))
		{
			/* copy; but change the entry type */
			directory->returned_entry = *result;
			directory->returned_entry.type = ENTTYPE_DIR;
			result = &directory->returned_entry;
		}
	}
	else if (directory->zipfile != NULL)
	{
		do
		{
			/* a zip file read */
			do
			{
				if (!directory->called_zip_first)
					header = zip_file_first_file(directory->zipfile);
				else
					header = zip_file_next_file(directory->zipfile);
				directory->called_zip_first = TRUE;
				relpath = NULL;
			}
			while((header != NULL) && ((relpath = get_relative_path(directory, header)) == NULL));

			if (relpath != NULL)
			{
				/* we've found a ZIP entry; but this may be an entry deep within the target directory */
				separator = strchr(relpath, '/');
				if (separator != NULL)
				{
					/* a nested entry; loop through returned_dirlist to see if we've returned the parent directory */
					for (rdent = directory->returned_dirlist; rdent != NULL; rdent = rdent->next)
					{
						if (!core_strnicmp(rdent->name, relpath, separator - relpath))
							break;
					}

					if (rdent == NULL)
					{
						/* we've found a new directory; add this to returned_dirlist */
						rdent = malloc(sizeof(*rdent) + (separator - relpath));
						rdent->next = directory->returned_dirlist;
						memcpy(rdent->name, relpath, (separator - relpath) * sizeof(rdent->name[0]));
						rdent->name[separator - relpath] = '\0';
						directory->returned_dirlist = rdent;						

						/* ...and return it */
						memset(&directory->returned_entry, 0, sizeof(directory->returned_entry));
						directory->returned_entry.name = rdent->name;
						directory->returned_entry.type = ENTTYPE_DIR;
						result = &directory->returned_entry;						
					}
				}
				else
				{
					/* a real file */
					memset(&directory->returned_entry, 0, sizeof(directory->returned_entry));
					directory->returned_entry.name = relpath;
					directory->returned_entry.type = ENTTYPE_FILE;
					directory->returned_entry.size = header->uncompressed_length;
					result = &directory->returned_entry;						
				}
			}
		}
		while((relpath != NULL) && (result == NULL));
	}
	return result;
}



/*-------------------------------------------------
    zippath_is_zip - returns TRUE if this path is
	a ZIP path or FALSE if not
-------------------------------------------------*/

int zippath_is_zip(zippath_directory *directory)
{
	return directory->zipfile != NULL;
}
