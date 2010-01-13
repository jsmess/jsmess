/**********************************************************************

    Commodore VIC-1112 IEEE-488 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "vic1112.h"
#include "machine/6522via.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6522_0_TAG		"u4"
#define M6522_1_TAG		"u5"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vic1112_t vic1112_t;
struct _vic1112_t
{
	const device_config *via0;
	const device_config *via1;
	const device_config *ieee_bus;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vic1112_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == VIC1112);
	return (vic1112_t *)device->token;
}

INLINE vic1112_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == VIC1112);
	return (vic1112_config *)device->inline_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    via6522_interface vic1112_via0_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */
}

static const via6522_interface vic1112_via0_intf =
{
	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL
};

/*-------------------------------------------------
    via6522_interface vic1112_via1_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( via1_pa_r )
{
	/*

        bit     description

        PA0     DI1
        PA1     DI2
        PA2     DI3
        PA3     DI4
        PA4     DI5
        PA5     DI6
        PA6     DI7
        PA7     DI8

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     description

        PB0     DO1
        PB1     DO2
        PB2     DO3
        PB3     DO4
        PB4     DO5
        PB5     DO6
        PB6     DO7
        PB7     DO8

    */
}

static const via6522_interface vic1112_via1_intf =
{
	DEVCB_HANDLER(via1_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL, // _SRQ IN
	DEVCB_NULL, // _ATN OUT
	DEVCB_NULL, // _EOI

	DEVCB_NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( vic1112 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( vic1112 )
	MDRV_VIA6522_ADD(M6522_0_TAG, 0, vic1112_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, 0, vic1112_via1_intf)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( vic1112 )
-------------------------------------------------*/

ROM_START( vic1112 )
	ROM_REGION( 0x800, "vic1112", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325329-03.u2", 0x000, 0x800, CRC(dd6147ba) SHA1(3945b9aa417f5905fa52f6f667a3e22add7ab15f) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( vic1112 )
-------------------------------------------------*/

static DEVICE_START( vic1112 )
{
	vic1112_t *vic1112 = get_safe_token(device);
	const vic1112_config *config = get_safe_config(device);

	/* set IEEE address */

	/* find devices */
	vic1112->via0 = device_find_child_by_tag(device, M6522_0_TAG);
	vic1112->via1 = device_find_child_by_tag(device, M6522_1_TAG);
	vic1112->ieee_bus = devtag_get_device(device->machine, config->ieee_bus_tag);

	/* set VIA clocks */
	device_set_clock(vic1112->via0, cpu_get_clock(device->machine->firstcpu));
	device_set_clock(vic1112->via1, cpu_get_clock(device->machine->firstcpu));

	/* register for state saving */
//	state_save_register_device_item(device, 0, vic1112->);
}

/*-------------------------------------------------
    DEVICE_RESET( vic1112 )
-------------------------------------------------*/

static DEVICE_RESET( vic1112 )
{
	vic1112_t *vic1112 = get_safe_token(device);

	/* reset VIAs */
	device_reset(vic1112->via0);
	device_reset(vic1112->via1);

	/* _IFC */
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vic1112 )
-------------------------------------------------*/

DEVICE_GET_INFO( vic1112 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vic1112_t);								break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(vic1112_config);							break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(vic1112);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(vic1112);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vic1112);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vic1112);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore VIC-1112");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-20");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
