/**********************************************************************

    Motorola MC6852 Synchronous Serial Data Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - FIFO
    - receive
    - transmit
    - parity
    - 1-sync-character mode
    - 2-sync-character mode
    - external sync mode
    - interrupts

*/

#include "mc6852.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

#define S_RDA			0x01
#define S_TDRA			0x02
#define S_DCD			0x04
#define S_CTS			0x08
#define S_TUF			0x10
#define S_RX_OVRN		0x20
#define S_PE			0x40
#define S_IRQ			0x80

#define C1_RX_RS		0x01
#define C1_TX_RS		0x02
#define C1_STRIP_SYNC	0x04
#define C1_CLEAR_SYNC	0x08
#define C1_TIE			0x10
#define C1_RIE			0x20
#define C1_AC_MASK		0xc0
#define C1_AC_C2		0x00
#define C1_AC_C3		0x40
#define C1_AC_SYNC		0x80
#define C1_AC_TX_FIFO	0xc0

#define C2_PC1			0x01
#define C2_PC2			0x02
#define C2_1_2_BYTE		0x04
#define C2_WS_MASK		0x38
#define C2_WS_6_E		0x00
#define C2_WS_6_O		0x08
#define C2_WS_7			0x10
#define C2_WS_8			0x18
#define C2_WS_7_E		0x20
#define C2_WS_7_O		0x28
#define C2_WS_8_E		0x30
#define C2_WS_8_O		0x38
#define C2_TX_SYNC		0x40
#define C2_EIE			0x80

#define C3_E_I_SYNC		0x01
#define C3_1_2_SYNC		0x02
#define C3_CLEAR_CTS	0x04
#define C3_CTUF			0x08

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mc6852_t mc6852_t;
struct _mc6852_t
{
	devcb_resolved_read_line		in_rx_data_func;
	devcb_resolved_write_line		out_tx_data_func;
	devcb_resolved_write_line		out_irq_func;
	devcb_resolved_read_line		in_cts_func;
	devcb_resolved_read_line		in_dcd_func;
	devcb_resolved_write_line		out_sm_dtr_func;
	devcb_resolved_write_line		out_tuf_func;

	/* registers */
	UINT8 status;			/* status register */
	UINT8 cr[3];			/* control registers */
	UINT8 scr;				/* sync code register */
	UINT8 rx_fifo[3];		/* receiver FIFO */
	UINT8 tx_fifo[3];		/* transmitter FIFO */
	UINT8 tdr;				/* transmit data register */
	UINT8 tsr;				/* transmit shift register */
	UINT8 rdr;				/* receive data register */
	UINT8 rsr;				/* receive shift register */

	int cts;				/* clear to send */
	int dcd;				/* data carrier detect */
	int sm_dtr;				/* sync match/data terminal ready */
	int tuf;				/* transmitter underflow */

	/* timers */
	emu_timer *rx_timer;
	emu_timer *tx_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mc6852_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MC6852);
	return (mc6852_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const mc6852_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MC6852);
	return (const mc6852_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    receive - receiver
-------------------------------------------------*/

static void receive(device_t *device)
{
};

/*-------------------------------------------------
    transmit - transmitter
-------------------------------------------------*/

static void transmit(device_t *device)
{
};

/*-------------------------------------------------
    TIMER_CALLBACK( receive_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( receive_tick )
{
	device_t *device = (device_t *)ptr;

	receive(device);
}

/*-------------------------------------------------
    TIMER_CALLBACK( transmit_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( transmit_tick )
{
	device_t *device = (device_t *)ptr;

	transmit(device);
}

/*-------------------------------------------------
    mc6852_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mc6852_r )
{
	mc6852_t *mc6852 = get_safe_token(device);

	UINT8 data = 0;

	if (BIT(offset, 0))
	{
		/* receive data FIFO */
	}
	else
	{
		/* status */
		data = mc6852->status;
	}

	return data;
}

/*-------------------------------------------------
    mc6852_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( mc6852_w )
{
	mc6852_t *mc6852 = get_safe_token(device);

	if (BIT(offset, 0))
	{
		switch (mc6852->cr[0] & C1_AC_MASK)
		{
		case C1_AC_C2:
			/* control 2 */
			mc6852->cr[1] = data;
			break;

		case C1_AC_C3:
			/* control 3 */
			mc6852->cr[2] = data;
			break;

		case C1_AC_SYNC:
			/* sync code */
			mc6852->scr = data;
			break;

		case C1_AC_TX_FIFO:
			/* transmit data FIFO */
			break;
		}
	}
	else
	{
		/* receiver reset */
		if (data & C1_RX_RS)
		{
			/* When Rx Rs is set, it clears the receiver
            control logic, sync logic, error logic, Rx Data FIFO Control,
            Parity Error status bit, and DCD interrupt. The Receiver Shift
            Register is set to ones.
            */

			if (LOG) logerror("MC6852 '%s' Receiver Reset\n", device->tag());

			mc6852->status &= ~(S_RX_OVRN | S_PE | S_DCD | S_RDA);
			mc6852->rsr = 0xff;
		}

		/* transmitter reset */
		if (data & C1_TX_RS)
		{
			/* When Tx Rs is set, it clears the transmitter
            control section, Transmitter Shift Register, Tx Data FIFO
            Control (the Tx Data FIFO can be reloaded after one E clock
            pulse), the Transmitter Underflow status bit, and the CTS interrupt,
            and inhibits the TDRA status bit (in the one-sync-character
            and two-sync-character modes).*/

			if (LOG) logerror("MC6852 '%s' Transmitter Reset\n", device->tag());

			mc6852->status &= ~(S_TUF | S_CTS | S_TDRA);
		}

		if (LOG)
		{
			if (data & C1_STRIP_SYNC) logerror("MC6852 '%s' Strip Synchronization Characters\n", device->tag());
			if (data & C1_CLEAR_SYNC) logerror("MC6852 '%s' Clear Synchronization\n", device->tag());
		}

		mc6852->cr[0] = data;
	}
}

/*-------------------------------------------------
    mc6852_rx_clk_w - receive clock
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc6852_rx_clk_w )
{
	if (state) receive(device);
}

/*-------------------------------------------------
    mc6852_tx_clk_w - transmit clock
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc6852_tx_clk_w )
{
	if (state) transmit(device);
}

/*-------------------------------------------------
    mc6852_cts_w - clear to send
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc6852_cts_w )
{
	mc6852_t *mc6852 = get_safe_token(device);

	mc6852->cts = state;
}

/*-------------------------------------------------
    mc6852_dcd_w - data carrier detect
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc6852_dcd_w )
{
	mc6852_t *mc6852 = get_safe_token(device);

	mc6852->dcd = state;
}

/*-------------------------------------------------
    mc685mc6852_sm_dtr_r2_dcd_w - sync match /
    data terminal ready
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( mc6852_sm_dtr_r )
{
	mc6852_t *mc6852 = get_safe_token(device);

	return mc6852->sm_dtr;
}

/*-------------------------------------------------
    mc6852_tuf_r - transmitter underflow
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( mc6852_tuf_r )
{
	mc6852_t *mc6852 = get_safe_token(device);

	return mc6852->tuf;
}

/*-------------------------------------------------
    DEVICE_START( mc6852 )
-------------------------------------------------*/

static DEVICE_START( mc6852 )
{
	mc6852_t *mc6852 = get_safe_token(device);
	const mc6852_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read_line(&mc6852->in_rx_data_func, &intf->in_rx_data_func, device);
	devcb_resolve_write_line(&mc6852->out_tx_data_func, &intf->out_tx_data_func, device);
	devcb_resolve_write_line(&mc6852->out_irq_func, &intf->out_irq_func, device);
	devcb_resolve_read_line(&mc6852->in_cts_func, &intf->in_cts_func, device);
	devcb_resolve_read_line(&mc6852->in_dcd_func, &intf->in_dcd_func, device);
	devcb_resolve_write_line(&mc6852->out_sm_dtr_func, &intf->out_sm_dtr_func, device);
	devcb_resolve_write_line(&mc6852->out_tuf_func, &intf->out_tuf_func, device);

	/* allocate timers */
	mc6852->rx_timer = device->machine().scheduler().timer_alloc(FUNC(receive_tick), (void *) device);
	mc6852->tx_timer = device->machine().scheduler().timer_alloc(FUNC(transmit_tick), (void *) device);
}

/*-------------------------------------------------
    DEVICE_RESET( mc6852 )
-------------------------------------------------*/

static DEVICE_RESET( mc6852 )
{
	mc6852_t *mc6852 = get_safe_token(device);

	/* set receiver shift register to all 1's */
	mc6852->rsr = 0xff;

	/* reset and inhibit receiver/transmitter sections */
	mc6852->cr[0] |= (C1_TX_RS | C1_RX_RS);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mc6852 )
-------------------------------------------------*/

DEVICE_GET_INFO( mc6852 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mc6852_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mc6852);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mc6852);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MC6852");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MC6800");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

DEFINE_LEGACY_DEVICE(MC6852, mc6852);
