/**********************************************************************

    Commodore 1581 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "c1581.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/g64_dsk.h"
#include "machine/6526cia.h"
#include "machine/cbmserial.h"
#include "machine/wd17xx.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_TAG		"u1"
#define M8520_TAG		"u5"
#define WD1770_TAG		"u4"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1581_t c1581_t;
struct _c1581_t
{
	/* devices */
	const device_config *cpu;
	const device_config *cia;
	const device_config *wd1770;
	const device_config *serial_bus;
	const device_config *image;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1581_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == C1581);
	return (c1581_t *)device->token;
}

INLINE c1581_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == C1581);
	return (c1581_config *)device->inline_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( c1581_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1581_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x4000, 0x4010) AM_DEVREADWRITE(M8520_TAG, cia_r, cia_w)
	AM_RANGE(0x6000, 0x6003) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1581", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    cia6526_interface c1581_cia_intf
-------------------------------------------------*/

static const cia6526_interface c1581_cia_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	10, /* 1/10 second */
	{
		{ DEVCB_NULL, DEVCB_NULL },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

/*-------------------------------------------------
    wd17xx_interface c1581_wd1770_intf
-------------------------------------------------*/

static const wd17xx_interface c1581_wd1770_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1581 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1581 )
//	FLOPPY_OPTION( c1581, "d81", "Commodore 1581 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1581_floppy_config
-------------------------------------------------*/

static const floppy_config c1581_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(c1581),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1581 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1581 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c1581_map)

	MDRV_CIA8520_ADD(M8520_TAG, XTAL_16MHz/16, c1581_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, c1581_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c1581 )
-------------------------------------------------*/

ROM_START( c1581 )
	ROM_REGION( 0x8000, "c1581", ROMREGION_LOADBYNAME )
	ROM_LOAD( "beta.u2",	  0x0000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594) )
	ROM_LOAD( "318045-01.u2", 0x0000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164) )
	ROM_LOAD( "318045-02.u2", 0x0000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393) )
ROM_END

/*-------------------------------------------------
    ROM( c1563 )
-------------------------------------------------*/

ROM_START( c1563 )
	ROM_REGION( 0x8000, "c1563", ROMREGION_LOADBYNAME )
	ROM_LOAD( "1563-rom.bin", 0x0000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1581 )
-------------------------------------------------*/

static DEVICE_START( c1581 )
{
}

/*-------------------------------------------------
    DEVICE_RESET( c1581 )
-------------------------------------------------*/

static DEVICE_RESET( c1581 )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1581 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1581 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1581_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1581_config);								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1581);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1581);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1581);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1581);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
