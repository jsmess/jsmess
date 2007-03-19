/***************************************************************************

	messdrv.h

	MESS specific driver stuff

***************************************************************************/

#ifndef MESSDRV_H
#define MESSDRV_H

#include <assert.h>

#include "formats/flopimg.h"
#include "unicode.h"
#include "device.h"



struct SystemConfigurationParamBlock
{
	/* for RAM options */
	int max_ram_options;
	int actual_ram_options;
	int default_ram_option;
	UINT32 *ram_options;

	/* device specification */
	int device_slotcount;
	device_getinfo_handler *device_handlers;
	int *device_countoverrides;
	int device_position;
};

#define SYSTEM_CONFIG_START(name)															\
	static void construct_sysconfig_##name(struct SystemConfigurationParamBlock *cfg)		\
	{																						\

#define SYSTEM_CONFIG_END																	\
	}																						\

#define CONFIG_IMPORT_FROM(name)															\
		construct_sysconfig_##name(cfg);													\

#define CONFIG_RAM(ram)																		\
	if (cfg->max_ram_options > 0)															\
	{																						\
		assert(cfg->actual_ram_options < cfg->max_ram_options);								\
		assert(cfg->ram_options);															\
		cfg->ram_options[cfg->actual_ram_options++] = (ram);								\
	}																						\

#define CONFIG_RAM_DEFAULT(ram)																\
	if (cfg->max_ram_options > 0)															\
	{																						\
		cfg->default_ram_option = cfg->actual_ram_options;									\
		CONFIG_RAM(ram);																	\
	}																						\

#define CONFIG_DEVICE(getinfo)										\
	if (cfg->device_position < cfg->device_slotcount)				\
	{																\
		cfg->device_handlers[cfg->device_position++] = (getinfo);	\
	}																\

#define CONFIG_DEVICE_COUNT(getinfo, count)							\
	if (cfg->device_position < cfg->device_slotcount)				\
	{																\
		cfg->device_handlers[cfg->device_position] = (getinfo);		\
		cfg->device_countoverrides[cfg->device_position] = (count);	\
		cfg->device_position++;										\
	}																\

/******************************************************************************
 * MESS' version of the GAME() macros of MAME
 * CONS is for consoles
 * COMP is for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	ipt_##INPUT,							\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|(FLAGS),							\
	NULL									\
};

#define CONSB(YEAR,NAME,PARENT,BIOS,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_##BIOS,						\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	ipt_##INPUT,							\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|(FLAGS),							\
	NULL									\
};

#define COMP(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	ipt_##INPUT,							\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|GAME_COMPUTER|(FLAGS),				\
	NULL									\
};

#define COMPB(YEAR,NAME,PARENT,BIOS,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;   \
extern const game_driver driver_##COMPAT;   \
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_##BIOS,						\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	ipt_##INPUT,							\
	init_##INIT,							\
	rom_##NAME,								\
	construct_sysconfig_##CONFIG,			\
	&driver_##COMPAT,						\
	ROT0|GAME_COMPUTER|(FLAGS),				\
	NULL									\
};

#define construct_sysconfig_NULL	(NULL)

#endif /* MESSDRV_H */


