/***************************************************************************

    messdrv.h

    MESS specific driver stuff

***************************************************************************/

#ifndef MESSDRV_H
#define MESSDRV_H

#include <assert.h>

#include "inputx.h"
#include "unicode.h"

/******************************************************************************
 * MESS' version of the GAME() macros of MAME
 * CONS is for consoles
 * COMP is for computers
 ******************************************************************************/
#define CONS(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	machine_config_##MACHINE,				\
	ipt_##INPUT,							\
	driver_init_##INIT,						\
	rom_##NAME,								\
	#COMPAT,								\
	ROT0|(FLAGS),							\
	NULL									\
};

#define COMP(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##NAME;   \
const game_driver driver_##NAME = 	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	machine_config_##MACHINE,				\
	ipt_##INPUT,							\
	driver_init_##INIT,						\
	rom_##NAME,								\
	#COMPAT,								\
	ROT0|GAME_COMPUTER|(FLAGS),				\
	NULL									\
};

#endif /* MESSDRV_H */
