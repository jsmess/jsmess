/***************************************************************************

    fileio.c

    File access functions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "corefile.h"
#include "driver.h"
#include "chd.h"
#include "hash.h"
#include "unzip.h"
#include "options.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PLAIN_FILE				0
#define ZIPPED_FILE				1

#define OPEN_FLAG_HAS_CRC		0x10000

#ifdef MAME_DEBUG
#define DEBUG_COOKIE			0xbaadf00d
#endif



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* typedef struct _mame_file mame_file -- declared in fileio.h */
struct _mame_file
{
#ifdef DEBUG_COOKIE
	UINT32			debug_cookie;					/* sanity checking for debugging */
#endif
	core_file *		file;							/* core file pointer */
	UINT32			openflags;						/* flags we used for the open */
	char			hash[HASH_BUF_SIZE];			/* hash data for the file */
	zip_file *		zipfile;						/* ZIP file pointer */
	UINT8 *			zipdata;						/* ZIP file data */
	UINT64			ziplength;						/* ZIP file length */
};


typedef struct _path_iterator path_iterator;
struct _path_iterator
{
	const char *	base;
	const char *	cur;
	int				index;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* core functions */
static void fileio_exit(running_machine *machine);

/* file open/close */
static file_error fopen_internal(const char *searchpath, const char *filename, UINT32, UINT32 flags, mame_file **file);
static file_error fopen_attempt_zipped(char *fullname, const char *filename, UINT32 crc, UINT32 openflags, mame_file *file);

/* CHD callbacks */
static chd_interface_file *chd_open_cb(const char *filename, const char *mode);
static void chd_close_cb(chd_interface_file *file);
static UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chd_length_cb(chd_interface_file *file);

/* path iteration */
static int path_iterator_init(path_iterator *iterator, const char *searchpath);
static int path_iterator_get_next(path_iterator *iterator, char *buffer, int buflen);

/* misc helpers */
static file_error load_zipped_file(mame_file *file);
static int zip_filename_match(const zip_file_header *header, const char *filename, int filenamelen);



/***************************************************************************
    HARD DISK INTERFACE
***************************************************************************/

static chd_interface mame_chd_interface =
{
	chd_open_cb,
	chd_close_cb,
	chd_read_cb,
	chd_write_cb,
	chd_length_cb
};



/***************************************************************************
    CORE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    fileio_init - initialize the internal file
    I/O system; note that the OS layer is free to
    call mame_fopen before fileio_init
-------------------------------------------------*/

void fileio_init(running_machine *machine)
{
	chd_set_interface(&mame_chd_interface);
	add_exit_callback(machine, fileio_exit);
}


/*-------------------------------------------------
    fileio_exit - clean up behind ourselves
-------------------------------------------------*/

static void fileio_exit(running_machine *machine)
{
	zip_file_cache_clear();
}



/***************************************************************************
    FILE OPEN/CLOSE
***************************************************************************/

/*-------------------------------------------------
    mame_fopen - open a file for access and
    return an error code
-------------------------------------------------*/

file_error mame_fopen(const char *searchpath, const char *filename, UINT32 openflags, mame_file **file)
{
	return fopen_internal(searchpath, filename, 0, openflags, file);
}


/*-------------------------------------------------
    mame_fopen_crc - open a file by name or CRC
    and return an error code
-------------------------------------------------*/

file_error mame_fopen_crc(const char *searchpath, const char *filename, UINT32 crc, UINT32 openflags, mame_file **file)
{
	return fopen_internal(searchpath, filename, crc, openflags | OPEN_FLAG_HAS_CRC, file);
}


/*-------------------------------------------------
    mame_fopen_ram - open a "file" which is
    actually just an array of data in RAM
-------------------------------------------------*/

file_error mame_fopen_ram(const void *data, UINT32 length, UINT32 openflags, mame_file **file)
{
	file_error filerr;

	/* allocate the file itself */
	*file = malloc(sizeof(**file));
	if (*file == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* reset the file handle */
	memset(*file, 0, sizeof(**file));
	(*file)->openflags = openflags;
#ifdef DEBUG_COOKIE
	(*file)->debug_cookie = DEBUG_COOKIE;
#endif

	/* attempt to open the file directly */
	filerr = core_fopen_ram(data, length, openflags, &(*file)->file);
	if (filerr == FILERR_NONE)
		goto error;

	/* handle errors and return */
error:
	if (filerr != FILERR_NONE)
	{
		mame_fclose(*file);
		*file = NULL;
	}
	return filerr;
}


/*-------------------------------------------------
    fopen_internal - open a file
-------------------------------------------------*/

static file_error fopen_internal(const char *searchpath, const char *filename, UINT32 crc, UINT32 openflags, mame_file **file)
{
	file_error filerr = FILERR_NOT_FOUND;
	path_iterator iterator;
	int maxlen, pathlen;
	char *fullname;

	/* can either have a hash or open for write, but not both */
	if ((openflags & OPEN_FLAG_HAS_CRC) && (openflags & OPEN_FLAG_WRITE))
		return FILERR_INVALID_ACCESS;

	/* allocate the file itself */
	*file = malloc(sizeof(**file));
	if (*file == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* reset the file handle */
	memset(*file, 0, sizeof(**file));
	(*file)->openflags = openflags;
#ifdef DEBUG_COOKIE
	(*file)->debug_cookie = DEBUG_COOKIE;
#endif

	/* if the path is absolute, null out the search path */
	if (searchpath != NULL && osd_is_absolute_path(searchpath))
		searchpath = NULL;

	/* determine the maximum length of a composed filename, plus some extra space for .zip extensions */
	maxlen = (UINT32)strlen(filename) + 5 + path_iterator_init(&iterator, searchpath) + 1;

	/* allocate a temporary buffer to hold it */
	fullname = malloc(maxlen);
	if (fullname == NULL)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto error;
	}

	/* loop over paths */
	while ((pathlen = path_iterator_get_next(&iterator, fullname, maxlen)) != -1)
	{
		char *dest;

		/* compute the full pathname */
		dest = &fullname[pathlen];
		if (pathlen > 0)
			*dest++ = PATH_SEPARATOR[0];
		strncpy(dest, filename, &fullname[maxlen] - dest);

		/* attempt to open the file directly */
		filerr = core_fopen(fullname, openflags, &(*file)->file);
		if (filerr == FILERR_NONE)
			break;

		/* if we're opening for read-only we have other options */
		if ((openflags & (OPEN_FLAG_READ | OPEN_FLAG_WRITE)) == OPEN_FLAG_READ)
		{
			filerr = fopen_attempt_zipped(fullname, dest, crc, openflags, *file);
			if (filerr == FILERR_NONE)
				break;
		}
	}
	free(fullname);

	/* handle errors and return */
error:
	if (filerr != FILERR_NONE)
	{
		mame_fclose(*file);
		*file = NULL;
	}
	return filerr;
}


/*-------------------------------------------------
    fopen_attempt_zipped - attempt to open a
    ZIPped file
-------------------------------------------------*/

static file_error fopen_attempt_zipped(char *fullname, const char *filename, UINT32 crc, UINT32 openflags, mame_file *file)
{
	char *dirsep = fullname + strlen(fullname);
	zip_error ziperr;
	zip_file *zip;
	char saved[5];

	assert(filename >= fullname && filename < fullname + strlen(fullname));

	/* loop over directory parts up to the start of filename */
	while (1)
	{
		const zip_file_header *header;
		int filenamelen;

		/* find the previous path separator */
		for (dirsep--; dirsep >= filename && *dirsep != PATH_SEPARATOR[0]; dirsep--) ;

		/* if none, we're done */
		if (*dirsep != PATH_SEPARATOR[0])
			return FILERR_NOT_FOUND;

		/* truncate here and replace with .zip */
		dirsep[0] = '.';
		saved[1] = dirsep[1]; dirsep[1] = 'z';
		saved[2] = dirsep[2]; dirsep[2] = 'i';
		saved[3] = dirsep[3]; dirsep[3] = 'p';
		saved[4] = dirsep[4]; dirsep[4] = 0;

		/* attempt to open the ZIP file */
		ziperr = zip_file_open(fullname, &zip);

		/* fix the original buffer */
		dirsep[0] = PATH_SEPARATOR[0];
		dirsep[1] = saved[1];
		dirsep[2] = saved[2];
		dirsep[3] = saved[3];
		dirsep[4] = saved[4];

		/* if we didn't open this file, continue scanning */
		if (ziperr != ZIPERR_NONE)
			continue;

		/* precompute the filename length */
		filenamelen = (UINT32)strlen(dirsep + 1);

		/* see if we can find a file with the right name and (if available) crc */
		for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
		{
			if (zip_filename_match(header, dirsep + 1, filenamelen) &&
				(!(openflags & OPEN_FLAG_HAS_CRC) || header->crc == crc))
				break;
		}

		/* if that failed, look for a file with the right crc, but the wrong filename */
		if (header == NULL && (openflags & OPEN_FLAG_HAS_CRC)) {
			for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
			{
				if (header->crc == crc)
					break;
			}
		}

		/* if that failed, look for a file with the right name; reporting a bad checksum */
		/* is more helpful and less confusing than reporting "rom not found" */
		if (header == NULL) {
			for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
			{
				if (zip_filename_match(header, dirsep + 1, filenamelen))
					break;
			}
		}

		/* if we got it, read the data */
		if (header != NULL)
		{
			UINT8 crcs[4];

			file->zipfile = zip;
			file->ziplength = header->uncompressed_length;

			/* build a hash with just the CRC */
			hash_data_clear(file->hash);
			crcs[0] = header->crc >> 24;
			crcs[1] = header->crc >> 16;
			crcs[2] = header->crc >> 8;
			crcs[3] = header->crc >> 0;
			hash_data_insert_binary_checksum(file->hash, HASH_CRC, crcs);
			return FILERR_NONE;
		}

		/* close up the ZIP file and try the next level */
		zip_file_close(zip);
	}
}


/*-------------------------------------------------
    mame_fclose - closes a file
-------------------------------------------------*/

void mame_fclose(mame_file *file)
{
#ifdef DEBUG_COOKIE
	assert(file->debug_cookie == DEBUG_COOKIE);
	file->debug_cookie = 0;
#endif

	/* close files and free memory */
	if (file->zipfile != NULL)
		zip_file_close(file->zipfile);
	if (file->file != NULL)
		core_fclose(file->file);
	if (file->zipdata != NULL)
		free(file->zipdata);
	free(file);
}



/***************************************************************************
    FILE POSITIONING
***************************************************************************/

/*-------------------------------------------------
    mame_fseek - seek within a file
-------------------------------------------------*/

int mame_fseek(mame_file *file, INT64 offset, int whence)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* seek if we can */
	if (file->file != NULL)
		return core_fseek(file->file, offset, whence);

	return 1;
}


/*-------------------------------------------------
    mame_ftell - return the current file position
-------------------------------------------------*/

UINT64 mame_ftell(mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* tell if we can */
	if (file->file != NULL)
		return core_ftell(file->file);

	return 0;
}


/*-------------------------------------------------
    mame_feof - return 1 if we're at the end
    of file
-------------------------------------------------*/

int mame_feof(mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* return EOF if we can */
	if (file->file != NULL)
		return core_feof(file->file);

	return 0;
}


/*-------------------------------------------------
    mame_fsize - returns the size of a file
-------------------------------------------------*/

UINT64 mame_fsize(mame_file *file)
{
	/* use the ZIP length if present */
	if (file->zipfile != NULL)
		return file->ziplength;

	/* return length if we can */
	if (file->file != NULL)
		return core_fsize(file->file);

	return 0;
}



/***************************************************************************
    FILE READ
***************************************************************************/

/*-------------------------------------------------
    mame_fread - read from a file
-------------------------------------------------*/

UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* read the data if we can */
	if (file->file != NULL)
		return core_fread(file->file, buffer, length);

	return 0;
}


/*-------------------------------------------------
    mame_fgetc - read a character from a file
-------------------------------------------------*/

int mame_fgetc(mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* read the data if we can */
	if (file->file != NULL)
		return core_fgetc(file->file);

	return EOF;
}


/*-------------------------------------------------
    mame_ungetc - put back a character read from
    a file
-------------------------------------------------*/

int mame_ungetc(int c, mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* read the data if we can */
	if (file->file != NULL)
		return core_ungetc(c, file->file);

	return 1;
}


/*-------------------------------------------------
    mame_fgets - read a line from a text file
-------------------------------------------------*/

char *mame_fgets(char *s, int n, mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* read the data if we can */
	if (file->file != NULL)
		return core_fgets(s, n, file->file);

	return NULL;
}



/***************************************************************************
    FILE WRITE
***************************************************************************/

/*-------------------------------------------------
    mame_fwrite - write to a file
-------------------------------------------------*/

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	/* write the data if we can */
	if (file->file != NULL)
		return core_fwrite(file->file, buffer, length);

	return 0;
}


/*-------------------------------------------------
    mame_fputs - write a line to a text file
-------------------------------------------------*/

int mame_fputs(mame_file *file, const char *s)
{
	/* write the data if we can */
	if (file->file != NULL)
		return core_fputs(file->file, s);

	return 0;
}


/*-------------------------------------------------
    mame_vfprintf - vfprintf to a text file
-------------------------------------------------*/

static int mame_vfprintf(mame_file *file, const char *fmt, va_list va)
{
	/* write the data if we can */
	if (file->file != NULL)
	{
		char buf[1024];
		vsnprintf(buf, sizeof(buf), fmt, va);
		return core_fputs(file->file, buf);
	}
	return 0;
}


/*-------------------------------------------------
    mame_fprintf - vfprintf to a text file
-------------------------------------------------*/

int CLIB_DECL mame_fprintf(mame_file *file, const char *fmt, ...)
{
	int rc;
	va_list va;
	va_start(va, fmt);
	rc = mame_vfprintf(file, fmt, va);
	va_end(va);
	return rc;
}



/***************************************************************************
    MISCELLANEOUS
***************************************************************************/

/*-------------------------------------------------
    mame_core_file - return the core_file
    underneath the mame_file
-------------------------------------------------*/

core_file *mame_core_file(mame_file *file)
{
	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);

	/* return the core file */
	return file->file;
}


/*-------------------------------------------------
    mame_fhash - returns the hash for a file
-------------------------------------------------*/

const char *mame_fhash(mame_file *file, UINT32 functions)
{
	const UINT8 *filedata;
	UINT32 wehave;

	/* if we already have the functions we need, just return */
	wehave = hash_data_used_functions(file->hash);
	if ((wehave & functions) == functions)
		return file->hash;

	/* load the ZIP file now if we haven't yet */
	if (file->zipfile != NULL)
		load_zipped_file(file);
	if (file->file == NULL)
		return file->hash;

	/* read the data if we can */
	filedata = core_fbuffer(file->file);
	if (filedata == NULL)
		return file->hash;

	/* compute the hash */
	hash_compute(file->hash, filedata, core_fsize(file->file), wehave | functions);
	return file->hash;
}



/***************************************************************************
    CHD CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    chd_open_cb - interface for opening
    a hard disk image
-------------------------------------------------*/

chd_interface_file *chd_open_cb(const char *filename, const char *mode)
{
	file_error filerr;
	mame_file *file;

	/* look for read-only drives first in the ROM path */
	if (mode[0] == 'r' && !strchr(mode, '+'))
	{
		const game_driver *drv;
		char *path;

		/* allocate a temporary buffer to hold the path */
		path = malloc_or_die(strlen(filename) + 1 + MAX_DRIVER_NAME_CHARS + 1);

		/* attempt reading up the chain through the parents */
		for (drv = Machine->gamedrv; drv != NULL; drv = driver_get_clone(drv))
		{
			sprintf(path, "%s" PATH_SEPARATOR "%s", drv->name, filename);
			filerr = mame_fopen(SEARCHPATH_IMAGE, path, OPEN_FLAG_READ, &file);
			if (filerr == FILERR_NONE)
			{
				free(path);
				return (chd_interface_file *)file;
			}
		}
		free(path);
		return NULL;
	}

	/* look for read/write drives in the diff area */
	filerr = mame_fopen(SEARCHPATH_IMAGE_DIFF, filename, OPEN_FLAG_READ | OPEN_FLAG_WRITE, &file);
	if (filerr != FILERR_NONE)
		filerr = mame_fopen(SEARCHPATH_IMAGE_DIFF, filename, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);
	return (filerr == FILERR_NONE) ? (chd_interface_file *)file : NULL;
}


/*-------------------------------------------------
    chd_close_cb - interface for closing
    a hard disk image
-------------------------------------------------*/

void chd_close_cb(chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}


/*-------------------------------------------------
    chd_read_cb - interface for reading
    from a hard disk image
-------------------------------------------------*/

UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_write_cb - interface for writing
    to a hard disk image
-------------------------------------------------*/

UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_length_cb - interface for getting
    the length a hard disk image
-------------------------------------------------*/

UINT64 chd_length_cb(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}



/***************************************************************************
    PATH ITERATION
***************************************************************************/

/*-------------------------------------------------
    path_iterator_init - prepare to iterate over
    a given path type
-------------------------------------------------*/

static int path_iterator_init(path_iterator *iterator, const char *searchpath)
{
	const char *cur, *base;
	int maxlen = 0;

	/* reset the structure */
	memset(iterator, 0, sizeof(*iterator));
	iterator->base = (searchpath != NULL) ? options_get_string(searchpath) : "";
	iterator->cur = iterator->base;

	/* determine the maximum path embedded here */
	base = iterator->base;
	for (cur = iterator->base; *cur != 0; cur++)
		if (*cur == ';')
		{
			maxlen = MAX(maxlen, cur - base);
			base = cur + 1;
		}

	/* account for the last path */
	maxlen = MAX(maxlen, cur - base);
	return maxlen + 1;
}


/*-------------------------------------------------
    path_iterator_get_next - get the next entry
    in a multipath sequence
-------------------------------------------------*/

static int path_iterator_get_next(path_iterator *iterator, char *buffer, int buflen)
{
	char *dest = buffer;
	const char *cur;

	/* if none left, return -1 to indicate we are done */
	if (iterator->index != 0 && *iterator->cur == 0)
		return -1;

	/* copy up to the next semicolon */
	cur = iterator->cur;
	while (*cur != 0 && *cur != ';' && buflen > 1)
	{
		*dest++ = *cur++;
		buflen--;
	}
	*dest = 0;

	/* if we ran out of buffer space, skip forward to the semicolon */
	while (*cur != 0 && *cur != ';')
		cur++;
	iterator->cur = (*cur == ';') ? (cur + 1) : cur;

	/* bump the index and return the length */
	iterator->index++;
	return dest - buffer;
}



/***************************************************************************
    MISC HELPERS
***************************************************************************/

/*-------------------------------------------------
    load_zipped_file - load a ZIPped file
-------------------------------------------------*/

static file_error load_zipped_file(mame_file *file)
{
	file_error filerr;
	zip_error ziperr;

	assert(file->file == NULL);
	assert(file->zipdata == NULL);
	assert(file->zipfile != NULL);

	/* allocate some memory */
	file->zipdata = malloc(file->ziplength);
	if (file->zipdata == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* read the data into our buffer and return */
	ziperr = zip_file_decompress(file->zipfile, file->zipdata, file->ziplength);
	if (ziperr != ZIPERR_NONE)
	{
		free(file->zipdata);
		file->zipdata = NULL;
		return FILERR_FAILURE;
	}

	/* convert to RAM file */
	filerr = core_fopen_ram(file->zipdata, file->ziplength, file->openflags, &file->file);
	if (filerr != FILERR_NONE)
	{
		free(file->zipdata);
		file->zipdata = NULL;
		return FILERR_FAILURE;
	}

	/* close out the ZIP file */
	zip_file_close(file->zipfile);
	file->zipfile = NULL;
	return FILERR_NONE;
}


/*-------------------------------------------------
    zip_filename_match - compare zip filename
    to expected filename, ignoring any directory
-------------------------------------------------*/

static int zip_filename_match(const zip_file_header *header, const char *filename, int filenamelen)
{
	const char *zipfile = header->filename + header->filename_length - filenamelen;

	return (zipfile >= header->filename && mame_stricmp(zipfile, filename) == 0 &&
		(zipfile == header->filename || zipfile[-1] == '/'));
}
