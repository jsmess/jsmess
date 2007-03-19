/*********************************************************************

	osdmess.h

	OS dependent calls for MESS

*********************************************************************/

#ifndef __OSDMESS_H__
#define __OSDMESS_H__

#include "osdcore.h"
#include "mamecore.h"

struct _mame_file;
typedef struct _mess_image mess_image;



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



/***************************************************************************
    UNCATEGORIZED INTERFACES
***************************************************************************/

/* called by the filemanager code to allow the OS to override the file		*/
/* selecting with it's own code. Return 0 if the MESS core should handle	*/
/* file selection, -1 if the OS code does nothing or 1 if the OS code		*/
/* changed a file.															*/
int osd_select_file(mess_image *img, char *filename);

/* returns 1 if input of type IPT_KEYBOARD should be supressed */
int osd_keyboard_disabled(void);



/******************************************************************************

  Parallel processing (for SMP)

******************************************************************************/

/* 
  Called by core to distribute tasks across multiple processors.  When this is
  called, there will be 1 to max_tasks invocations of task(); where task_count
  specifies the number of calls, and task_num is a number from zero to
  task_count-1 to specify which call was made.  This can be used to subdivide
  tasks across mulitple processors.

  If max_tasks<1, then it should be treated as if it was 1

  A bogus implementation would look like this:

	void osd_parallelize(void (*task)(void *param, int task_num, int
		task_count), void *param, int max_tasks)
	{
		task(param, 0, 1);
	}
*/

void osd_parallelize(void (*task)(void *param, int task_num, int task_count), void *param, int max_tasks);

/******************************************************************************

  Device and file browsing

******************************************************************************/

int osd_num_devices(void);
const char *osd_get_device_name(int i);
void osd_change_device(const char *vol);

void osd_mess_config_init(running_machine *machine);
void osd_mess_options_init(void);

#endif /* __OSDMESS_H__ */
