/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1551__
#define __C1551__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C1551, c1551);

#define MCFG_C1551_ADD(_tag, _cpu_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1551, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1551_config, cpu_tag, _cpu_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1551_config, address, _address)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1551_config c1551_config;
struct _c1551_config
{
	const char *cpu_tag;		/* CPU to hook into */
	int address;				/* bus address */
};
#endif
