//============================================================
//
//  swconfig.h
//
//============================================================

#ifndef __SWCONFIG_H__
#define __SWCONFIG_H__

#include "emu.h"
#include "mconfig.h"


//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _software_config software_config;
struct _software_config
{
	int driver_index;
	const game_driver *gamedrv;
	machine_config *mconfig;
};



//============================================================
//  PROTOTYPES
//============================================================

software_config *software_config_alloc(int driver_index); //, hashfile_error_func error_proc);
void software_config_free(software_config *config);

#endif // __SWCONFIG_H__
