/*********************************************************************

    osdmess.h

    OS dependent calls for MESS

*********************************************************************/

#ifndef __OSDMESS_H__
#define __OSDMESS_H__

#include "osdcore.h"
#include "options.h"
enum
{
	OSD_FOPEN_READ,
	OSD_FOPEN_WRITE,
	OSD_FOPEN_RW,
	OSD_FOPEN_RW_CREATE
};


struct _mame_file;



/***************************************************************************
    FILE I/O INTERFACES
***************************************************************************/

/*-----------------------------------------------------------------------------
    osd_copyfile: copies a file

    Parameters:

        destfile - path to destination

        srcfile - path to source file

    Return value:

        a file_error describing any error that occurred while copying
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_copyfile(const char *destfile, const char *srcfile);


/*-----------------------------------------------------------------------------
    osd_get_temp_filename: given a filename, return a full path with that
    filename but in a temporary directory

    Parameters:

        buffer - pointer to buffer

        buffer_len - length of buffer

        basename - basename of file to create

    Return value:

        a file_error describing any error that occurred while copying
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_get_temp_filename(char *buffer, size_t buffer_len, const char *basename);


/***************************************************************************
    DIRECTORY INTERFACES
***************************************************************************/

/*-----------------------------------------------------------------------------
    osd_stat: return a directory entry for a path

    Parameters:

        path - path in question

    Return value:

        an allocated pointer to an osd_directory_entry representing
        info on the path; even if the file does not exist

-----------------------------------------------------------------------------*/
osd_directory_entry *osd_stat(const char *path);


/*-----------------------------------------------------------------------------
    osd_mkdir: creates a directory

    Parameters:

        dir - path to directory to create

    Return value:

        a file_error describing any error that occurred while creating
        the directory, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_mkdir(const char *dir);


/*-----------------------------------------------------------------------------
    osd_rmdir: removes a directory

    Parameters:

        dir - path to directory to removes

    Return value:

        a file_error describing any error that occurred while deleting
        the directory, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_rmdir(const char *dir);


/*-----------------------------------------------------------------------------
    osd_getcurdir: retrieves the current working directory

    Parameters:

        buffer - place to store current working directory

        buffer_len - length of buffer

    Return value:

        a file_error describing any error that occurred while deleting
        the directory, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_getcurdir(char *buffer, size_t buffer_len);


/*-----------------------------------------------------------------------------
    osd_setcurdir: sets the current working directory

    Parameters:

        dir - path to directory to which to change

    Return value:

        a file_error describing any error that occurred while deleting
        the directory, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
file_error osd_setcurdir(const char *dir);


/***************************************************************************
    PATH INTERFACES
***************************************************************************/

/*-----------------------------------------------------------------------------
    osd_get_emulator_directory: returns the path containing the emulator

    Parameters:

        dir - space to output directory
        dir_size - size of path

-----------------------------------------------------------------------------*/

void osd_get_emulator_directory(char *dir, size_t dir_size);

/*-----------------------------------------------------------------------------
    osd_is_path_separator: returns whether a character is a path separator

    Parameters:

        c - the character in question

    Return value:

        non-zero if the character is a path separator, zero otherwise

-----------------------------------------------------------------------------*/
int osd_is_path_separator(char c);


/*-----------------------------------------------------------------------------
    osd_dirname: returns the base directory of a file path

    Parameters:

        filename - the path in question

    Return value:

        an allocated path to the directory containing this file

-----------------------------------------------------------------------------*/
char *osd_dirname(const char *filename);


/*-----------------------------------------------------------------------------
    osd_basename: returns the file or directory name from a full path

    Parameters:

        filename - the path in question

    Return value:

        a pointer to the base name of the file

-----------------------------------------------------------------------------*/
char *osd_basename(char *filename);


/*-----------------------------------------------------------------------------
    osd_get_full_path: retrieves the full path

    Parameters:

        path - the path in question
        dst - pointer to receive new path; the returned string needs to be free()-ed!

    Return value:

        file error

-----------------------------------------------------------------------------*/
file_error osd_get_full_path(char **dst, const char *path);



/***************************************************************************
    UNCATEGORIZED INTERFACES
***************************************************************************/


/* reads text from the clipboard - the returned string needs to be free()-ed! */
char *osd_get_clipboard_text(void);



/******************************************************************************

  Device and file browsing

******************************************************************************/

int osd_num_devices(void);
const char *osd_get_device_name(int idx);

void osd_mess_options_init(core_options *opts);

#endif /* __OSDMESS_H__ */
