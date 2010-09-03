//============================================================
//
//  swconfig.c
//
//============================================================

#include "swconfig.h"


//============================================================
//  IMPLEMENTATION
//============================================================

software_config *software_config_alloc(int driver_index, core_options *opts, hashfile_error_func error_proc)
{
	const game_driver *driver;
	software_config *config;

	// allocate the software_config
	config = (software_config *)malloc(sizeof(software_config));
	memset(config,0,sizeof(software_config));

	// allocate the machine config
	config->mconfig = global_alloc(machine_config(*drivers[driver_index]));

	// allocate the hash file
	driver = drivers[driver_index];
	while ((driver) && (!config->hashfile))
	{
		config->hashfile = hashfile_open_options(opts, driver->name, TRUE, error_proc);
		driver = driver_get_compatible(driver);
	}

	// other stuff
	config->driver_index = driver_index;
	config->gamedrv = drivers[driver_index];

	return config;
}



void software_config_free(software_config *config)
{
	if (config->mconfig != NULL)
	{
		global_free(config->mconfig);
		config->mconfig = NULL;
	}
	if (config->hashfile != NULL)
	{
		hashfile_close(config->hashfile);
		config->hashfile = NULL;
	}
	free(config);
}
