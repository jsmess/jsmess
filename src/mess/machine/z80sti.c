/***************************************************************************

    Mostek Z80STI Serial Timer Interrupt Controller emulation

    Copyright (c) 2009, The MESS Team.
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

enum
{
	Z80STI_TIMER_A = 0,
	Z80STI_TIMER_B,
	Z80STI_TIMER_C,
	Z80STI_TIMER_D,
	Z80STI_MAX_TIMERS
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
	devcb_resolved_write_line	out_int_func;

	/* I/O state */
	UINT8 gpip;							/* general purpose I/O register */
	UINT8 aer;							/* active edge register */
	UINT8 ddr;							/* data direction register */

	/* interrupt state */
	UINT16 ier;							/* interrupt enable register */
	UINT16 ipr;							/* interrupt pending register */
	UINT16 isr;							/* interrupt in-service register */
	UINT16 imr;							/* interrupt mask register */
	UINT8 pvr;							/*  */

	/* timer state */
	UINT8 tabc;							/*  */
	UINT8 tcdc;							/*  */
	UINT8 tdr[Z80STI_MAX_TIMERS];		/* timer data registers */
	UINT8 tmc[Z80STI_MAX_TIMERS];		/* timer main counters */

	/* serial state */
	UINT8 scr;							/* synchronous character register */
	UINT8 ucr;							/* USART control register */
	UINT8 tsr;							/* transmitter status register */
	UINT8 rsr;							/* receiver status register */
	UINT8 udr;							/* USART data register */

	/* timers */
	emu_timer *timer[Z80STI_MAX_TIMERS];	/* counter timers */
	emu_timer *rx_timer;					/* serial receive timer */
	emu_timer *tx_timer;					/* serial transmit timer */
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
    check_interrupts - set the interrupt request
	line state
-------------------------------------------------*/
#ifdef UNUSED_CODE
static void check_interrupts(const device_config *device)
{
	z80sti_t *z80sti = get_safe_token(device);

	if (z80sti->ipr & z80sti->imr)
	{
		devcb_call_write_line(&z80sti->out_int_func, ASSERT_LINE);
	}
	else
	{
		devcb_call_write_line(&z80sti->out_int_func, CLEAR_LINE);
	}
}

/*-------------------------------------------------
    take_interrupt - mark an interrupt pending
-------------------------------------------------*/

static void take_interrupt(const device_config *device, UINT16 mask)
{
	z80sti_t *z80sti = get_safe_token(device);

	z80sti->ipr |= mask;

	check_interrupts(device);
}
#endif
/*-------------------------------------------------
    serial_receive - receive serial bit
-------------------------------------------------*/

static void serial_receive(z80sti_t *z80sti)
{
}

/*-------------------------------------------------
    TIMER_CALLBACK( rx_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( rx_tick )
{
	const device_config *device = ptr;
	z80sti_t *z80sti = get_safe_token(device);

	serial_receive(z80sti);
}

/*-------------------------------------------------
    serial_transmit - transmit serial bit
-------------------------------------------------*/

static void serial_transmit(z80sti_t *z80sti)
{
}

/*-------------------------------------------------
    TIMER_CALLBACK( tx_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( tx_tick )
{
	const device_config *device = ptr;
	z80sti_t *z80sti = get_safe_token(device);

	serial_transmit(z80sti);
}

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
	z80sti_t *z80sti = get_safe_token(device);

	if (state)
	{
		serial_receive(z80sti);
	}
}

/*-------------------------------------------------
    z80sti_tc_w - transmitter clock write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( z80sti_tc_w )
{
	z80sti_t *z80sti = get_safe_token(device);

	if (state)
	{
		serial_transmit(z80sti);
	}
}

/*-------------------------------------------------
    timer_count - timer count down
-------------------------------------------------*/

static void timer_count(z80sti_t *z80sti, int index)
{
}

/*-------------------------------------------------
    TIMER_CALLBACK( timer_a )
-------------------------------------------------*/

static TIMER_CALLBACK( timer_a ) { z80sti_t *z80sti = get_safe_token(ptr); timer_count(z80sti, Z80STI_TIMER_A); }
static TIMER_CALLBACK( timer_b ) { z80sti_t *z80sti = get_safe_token(ptr); timer_count(z80sti, Z80STI_TIMER_B); }
static TIMER_CALLBACK( timer_c ) { z80sti_t *z80sti = get_safe_token(ptr); timer_count(z80sti, Z80STI_TIMER_C); }
static TIMER_CALLBACK( timer_d ) { z80sti_t *z80sti = get_safe_token(ptr); timer_count(z80sti, Z80STI_TIMER_D); }

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
	devcb_resolve_write_line(&z80sti->out_int_func, &intf->out_int_func, device);

	/* create the counter timers */
	z80sti->timer[Z80STI_TIMER_A] = timer_alloc(device->machine, timer_a, (void *)device);
	z80sti->timer[Z80STI_TIMER_B] = timer_alloc(device->machine, timer_b, (void *)device);
	z80sti->timer[Z80STI_TIMER_C] = timer_alloc(device->machine, timer_c, (void *)device);
	z80sti->timer[Z80STI_TIMER_D] = timer_alloc(device->machine, timer_d, (void *)device);

	/* create serial receive clock timer */
	if (intf->rx_clock > 0)
	{
		z80sti->rx_timer = timer_alloc(device->machine, rx_tick, (void *)device);
		timer_adjust_periodic(z80sti->rx_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->rx_clock));
	}

	/* create serial transmit clock timer */
	if (intf->tx_clock > 0)
	{
		z80sti->tx_timer = timer_alloc(device->machine, tx_tick, (void *)device);
		timer_adjust_periodic(z80sti->tx_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->tx_clock));
	}

	/* register for state saving */
	state_save_register_device_item(device, 0, z80sti->gpip);
	state_save_register_device_item(device, 0, z80sti->aer);
	state_save_register_device_item(device, 0, z80sti->ddr);
	state_save_register_device_item(device, 0, z80sti->ier);
	state_save_register_device_item(device, 0, z80sti->ipr);
	state_save_register_device_item(device, 0, z80sti->isr);
	state_save_register_device_item(device, 0, z80sti->imr);
	state_save_register_device_item(device, 0, z80sti->pvr);
	state_save_register_device_item(device, 0, z80sti->tabc);
	state_save_register_device_item(device, 0, z80sti->tcdc);
	state_save_register_device_item_array(device, 0, z80sti->tdr);
	state_save_register_device_item_array(device, 0, z80sti->tmc);
	state_save_register_device_item(device, 0, z80sti->scr);
	state_save_register_device_item(device, 0, z80sti->ucr);
	state_save_register_device_item(device, 0, z80sti->rsr);
	state_save_register_device_item(device, 0, z80sti->tsr);
	state_save_register_device_item(device, 0, z80sti->udr);
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
