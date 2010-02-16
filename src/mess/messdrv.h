/***************************************************************************

    messdrv.h

    MESS specific driver stuff

***************************************************************************/

#ifndef __MESSDRV_H__
#define __MESSDRV_H__

/******************************************************************************
 * MESS' version of the GAME() macros of MAME
 * CONS is for consoles
 * COMP is for computers
 ******************************************************************************/

#define CONS(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver GAME_NAME(NAME) =	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	MACHINE_DRIVER_NAME(MACHINE),			\
	INPUT_PORTS_NAME(INPUT),				\
	DRIVER_INIT_NAME(INIT),					\
	ROM_NAME(NAME),							\
	#COMPAT,								\
	ROT0|(FLAGS),							\
	NULL									\
};

#define COMP(YEAR,NAME,PARENT,COMPAT,MACHINE,INPUT,INIT,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver GAME_NAME(NAME) =	\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	MACHINE_DRIVER_NAME(MACHINE),			\
	INPUT_PORTS_NAME(INPUT),				\
	DRIVER_INIT_NAME(INIT),					\
	ROM_NAME(NAME),							\
	#COMPAT,								\
	ROT0|(FLAGS),				\
	NULL									\
};

#endif /* __MESSDRV_H__ */
