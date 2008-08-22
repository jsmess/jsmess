//============================================================
//
//  output.c - Generic implementation of MAME output routines
//
//  Copyright Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#if !defined(SDLMAME_WIN32)

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "deprecat.h"

// MAMEOS headers
#include "output.h"



//============================================================
//  CONSTANTS
//============================================================

#define SDLMAME_OUTPUT	"/tmp/sdlmame_out"

//============================================================
//  TYPEDEFS
//============================================================

//============================================================
//  PRIVATE VARIABLES
//============================================================

static FILE *output;

//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static void sdloutput_exit(running_machine *machine);
static void notifier_callback(const char *outname, INT32 value, void *param);

//============================================================
//  sdloutput_init
//============================================================

void sdloutput_init(running_machine *machine)
{
	int fildes;
	
	add_exit_callback(machine, sdloutput_exit);

	fildes = open(SDLMAME_OUTPUT, O_RDWR | O_NONBLOCK);
	
	if (fildes < 0)
	{
		output = NULL;
		mame_printf_verbose("ouput: unable to open output notifier file %s\n", SDLMAME_OUTPUT);
	}
	else
	{
		output = fdopen(fildes, "w");
		
		mame_printf_verbose("ouput: opened output notifier file %s\n", SDLMAME_OUTPUT);
		fprintf(output, "MAME %d START %s\n", getpid(), machine->gamedrv->name);
		fflush(output);
	}
	
	output_set_notifier(NULL, notifier_callback, NULL);
}


//============================================================
//  winoutput_exit
//============================================================

static void sdloutput_exit(running_machine *machine)
{
	if (output != NULL)
	{
		fprintf(output, "MAME %d STOP %s\n", getpid(), machine->gamedrv->name);
		fflush(output);
		fclose(output);
		output = NULL;
		mame_printf_verbose("ouput: closed output notifier file\n");
	}
}

//============================================================
//  notifier_callback
//============================================================

static void notifier_callback(const char *outname, INT32 value, void *param)
{
	if (output != NULL)
	{
		fprintf(output, "OUT %d %s %d\n", getpid(), outname, value);
		fflush(output);
	}
}

#else  /* SDLMAME_WIN32 */

#include "driver.h"

//============================================================
//  Stub for win32
//============================================================

void sdloutput_init(running_machine *machine)
{
}

#endif  /* SDLMAME_WIN32 */
