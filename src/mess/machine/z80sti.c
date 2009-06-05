/***************************************************************************

    Mostek Z80STI Serial Timer Interrupt Controller emulation

    Copyright (c) 2008, The MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

/*

	TODO:

	- everything

*/

#include "driver.h"
#include "z80sti.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define VERBOSE 1

#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

enum
{
	Z80STI_REGISTER_IR = 0,
	Z80STI_REGISTER_GPIP,
	Z80STI_REGISTER_IPRB,
	Z80STI_REGISTER_IPRA,
	Z80STI_REGISTER_ISRB,
	Z80STI_REGISTER_ISRA,
	Z80STI_REGISTER_IMRB,
	Z80STI_REGISTER_IMRA,
	Z80STI_REGISTER_PVR,
	Z80STI_REGISTER_TABC,
	Z80STI_REGISTER_TBDR,
	Z80STI_REGISTER_TADR,
	Z80STI_REGISTER_UCR,
	Z80STI_REGISTER_RSR,
	Z80STI_REGISTER_TSR,
	Z80STI_REGISTER_UDR
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _z80sti_t z80sti_t;
struct _z80sti_t
{
	devcb_resolved_read8		in_gpio_func;
	devcb_resolved_write8		out_gpio_func;
	devcb_resolved_read_line	in_si_func;
	devcb_resolved_write_line	out_so_func;
	devcb_resolved_write_line	out_tao_func;
	devcb_resolved_write_line	out_tbo_func;
	devcb_resolved_write_line	out_tco_func;
	devcb_resolved_write_line	out_tdo_func;
	devcb_resolved_write_line	out_irq_func;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static int z80sti_irq_state(const device_config *device);
static void z80sti_irq_reti(const device_config *device);

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE z80sti_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == Z80STI);
	return (z80sti_t *)device->token;
}

INLINE const z80sti_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == Z80STI));
	return (const z80sti_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    z80sti_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( z80sti_r )
{
	return 0;
}

/*-------------------------------------------------
    z80sti_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( z80sti_w )
{
}

/*-------------------------------------------------
    z80sti_rc_w - receiver clock write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_rc_w )
{
}

/*-------------------------------------------------
    z80sti_tc_w - transmitter clock write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_tc_w )
{
}

/*-------------------------------------------------
    z80sti_i0_w - input 0 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i0_w )
{
}

/*-------------------------------------------------
    z80sti_i1_w - input 1 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i1_w )
{
}

/*-------------------------------------------------
    z80sti_i2_w - input 2 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i2_w )
{
}

/*-------------------------------------------------
    z80sti_i3_w - input 3 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i3_w )
{
}

/*-------------------------------------------------
    z80sti_i4_w - input 4 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i4_w )
{
}

/*-------------------------------------------------
    z80sti_i5_w - input 5 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i5_w )
{
}

/*-------------------------------------------------
    z80sti_i6_w - input 6 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i6_w )
{
}

/*-------------------------------------------------
    z80sti_i7_w - input 7 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_i7_w )
{
}

/*-------------------------------------------------
    z80sti_irq_state - get interrupt status
-------------------------------------------------*/

static int z80sti_irq_state(const device_config *device)
{
	return 0;
}

/*-------------------------------------------------
    z80sti_irq_ack - interrupt acknowledge
-------------------------------------------------*/

static int z80sti_irq_ack(const device_config *device)
{
	return 0;
}

/*-------------------------------------------------
    z80sti_irq_reti - return from interrupt
-------------------------------------------------*/

static void z80sti_irq_reti(const device_config *device)
{
}

/*-------------------------------------------------
    DEVICE_START( z80sti )
-------------------------------------------------*/

static DEVICE_START( z80sti )
{
	z80sti_t *z80sti = get_safe_token(device);
	const z80sti_interface *intf = device->static_config;

	/* resolve callbacks */
	devcb_resolve_read8(&z80sti->in_gpio_func, &intf->in_gpio_func, device);
	devcb_resolve_write8(&z80sti->out_gpio_func, &intf->out_gpio_func, device);
	devcb_resolve_read_line(&z80sti->in_si_func, &intf->in_si_func, device);
	devcb_resolve_write_line(&z80sti->out_so_func, &intf->out_so_func, device);
	devcb_resolve_write_line(&z80sti->out_tao_func, &intf->out_tao_func, device);
	devcb_resolve_write_line(&z80sti->out_tbo_func, &intf->out_tbo_func, device);
	devcb_resolve_write_line(&z80sti->out_tco_func, &intf->out_tco_func, device);
	devcb_resolve_write_line(&z80sti->out_tdo_func, &intf->out_tdo_func, device);
	devcb_resolve_write_line(&z80sti->out_irq_func, &intf->out_irq_func, device);
}

/*-------------------------------------------------
    DEVICE_RESET( z80sti )
-------------------------------------------------*/

static DEVICE_RESET( z80sti )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( z80sti )
-------------------------------------------------*/

DEVICE_GET_INFO( z80sti )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(z80sti_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(z80sti);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(z80sti);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)z80sti_irq_state;				break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)z80sti_irq_ack;				break;
		case DEVINFO_FCT_IRQ_RETI:						info->f = (genf *)z80sti_irq_reti;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Mostek MK3801");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Z80 STI");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");		break;
	}
}
