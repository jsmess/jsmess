//============================================================
//
//  config.h - Win32 configuration routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef _WIN_CONFIG__
#define _WIN_CONFIG__


//============================================================
//  TYPE DEFINITIONS
//============================================================



//============================================================
//  PROTOTYPES
//============================================================

// Initializes and exits the configuration system
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

#endif // _WIN_CONFIG__
