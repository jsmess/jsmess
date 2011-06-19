//============================================================
//
//  swconfig.c
//
//============================================================
// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

// standard C headers
#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include "emu.h"
#include "swconfig.h"
#include "mui_opts.h"
//============================================================
//  IMPLEMENTATION
//============================================================

software_config *software_config_alloc(int driver_index) //, hashfile_error_func error_proc)
{
	software_config *config;

	// allocate the software_config
	config = (software_config *)malloc(sizeof(software_config));
	memset(config,0,sizeof(software_config));

	// allocate the machine config
	windows_options pCurrentOpts;
	load_options(pCurrentOpts, OPTIONS_GLOBAL, driver_index);
	config->mconfig = global_alloc(machine_config(driver_list::driver(driver_index),pCurrentOpts));

	// other stuff
	config->driver_index = driver_index;
	config->gamedrv = &driver_list::driver(driver_index);

	return config;
}



void software_config_free(software_config *config)
{
	if (config->mconfig != NULL)
	{
		global_free(config->mconfig);
		config->mconfig = NULL;
	}
	/*if (config->hashfile != NULL)
    {
        hashfile_close(config->hashfile);
        config->hashfile = NULL;
    }*/
	free(config);
}
