/**********************************************************************

    Commodore 8280 Dual 8" Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "c8280.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "machine/6532riot.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_DOS_TAG	"5c"
#define M6502_FDC_TAG	"9e"
#define M6532_0_TAG		"9f"
#define M6532_1_TAG		"9g"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c8280_t c8280_t;
struct _c8280_t
{
	/* abstractions */
	int address;						/* bus address - 8 */

	/* devices */
	device_t *cpu_dos;
	device_t *cpu_fdc;
	device_t *riot0;
	device_t *riot1;
	device_t *bus;
	device_t *image[2];
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c8280_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C8280);
	return (c8280_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c8280_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C8280);
	return (c8280_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    c8280_ieee488_atn_w - IEEE-488 bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_atn_w )
{
}

/*-------------------------------------------------
    c8280_ieee488_ifc_w - IEEE-488 bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_ifc_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c8280_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8280_dos_map, AS_PROGRAM, 8 )
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c8280", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c8280_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8280_fdc_map, AS_PROGRAM, 8 )
ADDRESS_MAP_END

/*-------------------------------------------------
    riot6532_interface riot0_intf
-------------------------------------------------*/

static const riot6532_interface riot0_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/*-------------------------------------------------
    riot6532_interface riot1_intf
-------------------------------------------------*/

static const riot6532_interface riot1_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c8280 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c8280 )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c8280_floppy_config
-------------------------------------------------*/

static const floppy_config c8280_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(c8280),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c8280 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c8280 )
	MCFG_CPU_ADD(M6502_DOS_TAG, M6502, 1000000)
	MCFG_CPU_PROGRAM_MAP(c8280_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, 1000000, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, 1000000, riot1_intf)

	MCFG_CPU_ADD(M6502_FDC_TAG, M6502, 1000000)
	MCFG_CPU_PROGRAM_MAP(c8280_fdc_map)

	MCFG_FLOPPY_2_DRIVES_ADD(c8280_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c8280 )
-------------------------------------------------*/

ROM_START( c8280 )
	ROM_REGION( 0x4800, "c8280", ROMREGION_LOADBYNAME )
	ROM_LOAD( "300542-001.10c", 0x0000, 0x2000, CRC(3c6eee1e) SHA1(0726f6ab4de4fc9c18707fe87780ffd9f5ed72ab) )
	ROM_LOAD( "300543-001.10d", 0x2000, 0x2000, CRC(f58e665e) SHA1(9e58b47c686c91efc6ef1a27f72dbb5e26c485ec) )
	ROM_LOAD( "300541-001.3c",  0x4000, 0x0800, CRC(cb07b2db) SHA1(a1f9c5a7bd3798f5a97dc0b465c3bf5e3513e148) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c8280 )
-------------------------------------------------*/

static DEVICE_START( c8280 )
{
	c8280_t *c8280 = get_safe_token(device);
	const c8280_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 16));
	c8280->address = config->address - 8;

	/* find our CPU */
	c8280->cpu_dos = device->subdevice(M6502_DOS_TAG);
	c8280->cpu_fdc = device->subdevice(M6502_FDC_TAG);
}

/*-------------------------------------------------
    DEVICE_RESET( c8280 )
-------------------------------------------------*/

static DEVICE_RESET( c8280 )
{
	c8280_t *c8280 = get_safe_token(device);

	c8280->cpu_dos->reset();
	c8280->cpu_fdc->reset();
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c8280 )
-------------------------------------------------*/

DEVICE_GET_INFO( c8280 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c8280_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c8280_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c8280);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c8280);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c8280);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c8280);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 8280");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore PET");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(C8280, c8280);
