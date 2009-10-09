/***************************************************************************

    Epson TF-20

    Dual floppy drive with HX-20 factory option

    Skeleton driver, not working

***************************************************************************/

#include "driver.h"
#include "tf20.h"
#include "cpu/z80/z80.h"
#include "machine/upd7201.h"
#include "machine/nec765.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tf20_state tf20_state;
struct _tf20_state
{
	UINT8 dummy;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE tf20_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == TF20);

	return (tf20_state *)device->token;
}


/*****************************************************************************
    ADDRESS MAPS
*****************************************************************************/

static ADDRESS_MAP_START( tf20_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("tf20", 0)
	AM_RANGE(0x0800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tf20_io, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END


/*****************************************************************************
    MACHINE CONFIG
*****************************************************************************/

static const nec765_interface tf20_nec765a_intf =
{
	DEVCB_NULL, /* interrupt line */
	NULL,
	NULL,
	NEC765_RDY_PIN_NOT_CONNECTED, /* ??? */
	{NULL, NULL, NULL, NULL}
};

static MACHINE_DRIVER_START( tf20 )
	MDRV_CPU_ADD("tf20", Z80, XTAL_8MHz / 2) /* uPD780C */
	MDRV_CPU_PROGRAM_MAP(tf20_mem)
	MDRV_CPU_IO_MAP(tf20_io)

	MDRV_NEC765A_ADD("nec765a", tf20_nec765a_intf)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( tf20 )
	ROM_REGION(0x0800, "tf20", ROMREGION_LOADBYNAME)
	ROM_LOAD("tfx.15e", 0x0000, 0x0800, CRC(af34f084) SHA1(c9bdf393f757ba5d8f838108ceb2b079be1d616e))
ROM_END


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* serial output signal (to the host computer) */
READ_LINE_DEVICE_HANDLER( tf20_rxs_r )
{
	logerror("%s: tf20_rxs_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

READ_LINE_DEVICE_HANDLER( tf20_pins_r )
{
	logerror("%s: tf20_pins_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

/* serial input signal (from host computer) */
WRITE_LINE_DEVICE_HANDLER( tf20_txs_w )
{
	logerror("%s: tf20_rxd1_w %u\n", cpuexec_describe_context(device->machine), state);
}

WRITE_LINE_DEVICE_HANDLER( tf20_pouts_w )
{
	logerror("%s: tf20_pouts_w %u\n", cpuexec_describe_context(device->machine), state);
}

/* serial output signal (to another terminal) */
READ_LINE_DEVICE_HANDLER( tf20_txc_r )
{
	logerror("%s: tf20_txc_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

/* serial input signal (from another terminal) */
WRITE_LINE_DEVICE_HANDLER( tf20_rxc_w )
{
	logerror("%s: tf20_rxc_w %u\n", cpuexec_describe_context(device->machine), state);
}

READ_LINE_DEVICE_HANDLER( tf20_poutc_r )
{
	logerror("%s: tf20_poutc_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

WRITE_LINE_DEVICE_HANDLER( tf20_pinc_w )
{
	logerror("%s: tf20_pinc_w %u\n", cpuexec_describe_context(device->machine), state);
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( tf20 )
{
	tf20_state *tf20 = get_safe_token(device);

	tf20->dummy = 0;
}

static DEVICE_RESET( tf20 )
{
}

DEVICE_GET_INFO( tf20 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(tf20_state);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;									break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_OTHER;					break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_DRIVER_NAME(tf20);	break;
		case DEVINFO_PTR_ROM_REGION:			info->romregion = ROM_NAME(tf20); 				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(tf20);			break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(tf20);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "TF-20");						break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Floppy drive");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");			break;
	}
}
