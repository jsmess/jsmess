/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "c1551.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/g64_dsk.h"
#include "machine/6525tpi.h"
#include "machine/cbmserial.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6510T_TAG		"u2"
#define M6525_TAG		"u3"
#define M6523_TAG		"ci_u2"

#define FLOPPY_TAG		"c1551_floppy"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1551_t c1551_t;
struct _c1551_t
{
	/* devices */
	const device_config *cpu;
	const device_config *tpi;
	const device_config *image;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1551_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == C1551);
	return (c1551_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static READ8_HANDLER( c1551_port_r )
{
	/*

        bit     description

        P0      STEP0
        P1      STEP1
        P2      MOTOR ON
        P3      ACT
        P4      WPRT
        P5      DS0
        P6      DS1
        P7      ATN

    */

	return 0;
}

static WRITE8_HANDLER( c1551_port_w )
{
	/*

        bit     description

        P0      STEP0
        P1      STEP1
        P2      MOTOR ON
        P3      ACT
        P4      WPRT
        P5      DS0
        P6      DS1
        P7      ATN

    */
}


/*-------------------------------------------------
    ADDRESS_MAP( c1551_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1551_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0001) AM_READWRITE(c1551_port_r, c1551_port_w)
	AM_RANGE(0x0002, 0x07ff) AM_RAM
	AM_RANGE(0x4000, 0x4007) AM_DEVREADWRITE(M6525_TAG, tpi6525_r, tpi6525_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c1551", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    tpi6525_interface c1551_tpi_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c1551_tpi_pa_r )
{
	/*

        bit     description

        PA0     6523 P0
        PA1     6523 P1
        PA2     6523 P2
        PA3     6523 P3
        PA4     6523 P4
        PA5     6523 P5
        PA6     6523 P6
        PA7     6523 P7

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pa_w )
{
	/*

        bit     description

        PA0     6523 P0
        PA1     6523 P1
        PA2     6523 P2
        PA3     6523 P3
        PA4     6523 P4
        PA5     6523 P5
        PA6     6523 P6
        PA7     6523 P7

    */
}

static READ8_DEVICE_HANDLER( c1551_tpi_pb_r )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pb_w )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */
}

static READ8_DEVICE_HANDLER( c1551_tpi_pc_r )
{
	/*

        bit     description

        PC0     6523 _IRQ
        PC1     6523 _RES
        PC2     interface J1
        PC3     6523 pin 20
        PC4     SOE
        PC5     JP1
        PC6     _SYNC
        PC7     6523 phi2

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pc_w )
{
	/*

        bit     description

        PC0     6523 _IRQ
        PC1     6523 _RES
        PC2     interface J1
        PC3     6523 pin 20
        PC4     SOE
        PC5     JP1
        PC6     _SYNC
        PC7     6523 phi2

    */
}

static const tpi6525_interface c1551_tpi_intf =
{
	c1551_tpi_pa_r,
	c1551_tpi_pb_r,
	c1551_tpi_pc_r,
	c1551_tpi_pa_w,
	c1551_tpi_pb_w,
	c1551_tpi_pc_w,
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1551 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1551 )
	FLOPPY_OPTION( c1551, "g64", "Commodore 1541 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
//  FLOPPY_OPTION( c1551, "d64", "Commodore 1541 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1551_floppy_config
-------------------------------------------------*/

static const floppy_config c1551_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c1551),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1551 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1551 )
	MDRV_CPU_ADD(M6510T_TAG, M6510T, 2000000)
	MDRV_CPU_PROGRAM_MAP(c1551_map)

	MDRV_TPI6525_ADD(M6525_TAG, c1551_tpi_intf)
//  MDRV_MOS6523_ADD(M6523_TAG, 2000000, c1551_mos6523_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_TAG, c1551_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c1551 )
-------------------------------------------------*/

ROM_START( c1551 )
	ROM_REGION( 0x4000, "c1551", ROMREGION_LOADBYNAME )
	ROM_LOAD( "318001-01.u4", 0x0000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1551 )
-------------------------------------------------*/

static DEVICE_START( c1551 )
{
}

/*-------------------------------------------------
    DEVICE_RESET( c1551 )
-------------------------------------------------*/

static DEVICE_RESET( c1551 )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1551 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1551_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1551);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1551);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1551);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1551);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1551");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-1540");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
