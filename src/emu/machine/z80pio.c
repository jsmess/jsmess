/***************************************************************************

    Z80 PIO Parallel Input/Output Controller emulation

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "z80pio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"

/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE 0

#define LOGERROR if (VERBOSE) logerror

/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	PIO_MODE_OUTPUT = 0,
	PIO_MODE_INPUT,
	PIO_MODE_BIDIRECTIONAL,
	PIO_MODE_CONTROL
};

enum
{
	PIO_STATE_NORMAL = 0,
	PIO_STATE_DDR,
	PIO_STATE_MASK
};

enum
{
	PIO_PORT_A = 0,
	PIO_PORT_B,
	PIO_PORT_COUNT
};

#define PIO_MODE_CONTROL_WORD	0x0f
#define PIO_IRQ_CONTROL_WORD	0x07
#define PIO_IRQ_DISABLE_WORD	0x03

#define PIO_IRQ_MASK_FOLLOWS	0x10
#define PIO_IRQ_ACTIVE_HIGH		0x20
#define PIO_IRQ_AND				0x40
#define PIO_IRQ_ENABLE			0x80

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _z80pio z80pio_t;
struct _z80pio
{
	const z80pio_interface *intf;			/* interface */

	int clock;								/* clock frequency */

	int state[PIO_PORT_COUNT];				/* control register state */
	int mode[PIO_PORT_COUNT];				/* mode control register */
	int irq_enable[PIO_PORT_COUNT];			/* interrupt enable flag */
	int irq_pending[PIO_PORT_COUNT];		/* interrupt pending flag */
	int int_state[PIO_PORT_COUNT];			/* interrupt status */
	UINT8 enable[PIO_PORT_COUNT];			/* interrupt control word */
	UINT8 input[PIO_PORT_COUNT];			/* data input register */
	UINT8 output[PIO_PORT_COUNT];			/* data output register */
	UINT8 vector[PIO_PORT_COUNT];			/* interrupt vector */
	UINT8 mask[PIO_PORT_COUNT];				/* mask register */
	UINT8 ddr[PIO_PORT_COUNT];				/* input/output select register */
	int strobe[PIO_PORT_COUNT];				/* strobe pulse */
	int match[PIO_PORT_COUNT];				/* control mode bit logic equation match */

	/* timers */
	emu_timer *poll_timer;					/* mode 3 poll timer */
	emu_timer *irq_timer;					/* interrupt enable timer */
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static int z80pio_irq_state(const device_config *device);
static int z80pio_irq_ack(const device_config *device);
static void z80pio_irq_reti(const device_config *device);

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE z80pio_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == Z80PIO);

	return (z80pio_t *)device->token;
}

/***************************************************************************
    INTERNAL STATE MANAGEMENT
***************************************************************************/

static void z80pio_check_interrupt(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token(device);
	int channel;

	for (channel = PIO_PORT_A; channel < PIO_PORT_COUNT; channel++)
	{
		if (z80pio->irq_enable[channel])
		{
			if (z80pio->irq_pending[channel])
			{
				/* trigger interrupt request */
				z80pio->int_state[channel] |= Z80_DAISY_INT;

				/* reset interrupt pending */
				z80pio->irq_pending[channel] = 0;

				LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Request\n", device->tag, 'A' + channel);
			}
		}
		else
		{
			/* clear interrupt request */
			z80pio->int_state[channel] &= ~Z80_DAISY_INT;
		}
	}

	if (z80pio->intf->on_int_changed)
	{
		z80pio->intf->on_int_changed(device, (z80pio_irq_state(device) & Z80_DAISY_INT) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static void z80pio_trigger_interrupt(const device_config *device, int channel)
{
	z80pio_t *z80pio = get_safe_token(device);

	if (z80pio->irq_enable[channel])
	{
		z80pio->int_state[channel] |= Z80_DAISY_INT;
		z80pio_check_interrupt(device);

		LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Request\n", device->tag, 'A' + channel);
	}
	else
	{
		/* set interrupt pending */
		z80pio->irq_pending[channel] = 1;
		z80pio_check_interrupt(device);

		LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Pending\n", device->tag, 'A' + channel);
	}
}

static TIMER_CALLBACK( z80pio_irq_tick )
{
	const device_config *device = (device_config *) ptr;
	z80pio_t *z80pio = get_safe_token(device);

	int channel = BIT(param, 1);
	int state = BIT(param, 0);

	/* set interrupt enable */
	z80pio->irq_enable[channel] = state;

	LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Enable %u\n", device->tag, 'A' + channel, state);

	/* check interrupt status */
	z80pio_check_interrupt(device);
}

static void z80pio_set_irq_enable(const device_config *device, int channel, int state)
{
	z80pio_t *z80pio = get_safe_token(device);

	if (state)
	{
		/* enable interrupt on next _M1 */
		int param = (channel << 1) | state;

		timer_adjust_oneshot(z80pio->irq_timer, ATTOTIME_IN_HZ(z80pio->clock * 4), param);
	}
	else
	{
		/* disable interrupt immediately */
		z80pio->irq_enable[channel] = state;

		LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Enable %u\n", device->tag, 'A' + channel, state);
	}
}

static void z80pio_ready_w(const device_config *device, int channel, int state)
{
	z80pio_t *z80pio = get_safe_token(device);

	LOGERROR("Z80PIO \"%s\" Port %c : Ready %u\n", device->tag, 'A' + channel, state);

	if (channel == PIO_PORT_A)
	{
		if (z80pio->intf->on_ardy_changed)
		{
			z80pio->intf->on_ardy_changed(device, state);
		}
	}
	else
	{
		if (z80pio->intf->on_brdy_changed)
		{
			z80pio->intf->on_brdy_changed(device, state);
		}
	}
}

static UINT8 z80pio_port_r(const device_config *device, int channel)
{
	z80pio_t *z80pio = get_safe_token(device);
	UINT8 data = 0;

	if (channel == PIO_PORT_A)
	{
		if (z80pio->intf->port_a_r)
		{
			data = z80pio->intf->port_a_r(device, 0);
		}
	}
	else
	{
		if (z80pio->intf->port_b_r)
		{
			data = z80pio->intf->port_b_r(device, 0);
		}
	}

	//LOGERROR("Z80PIO \"%s\" Port %c : Input Data '%02x'\n", device->tag, 'A' + channel, data);

	return data;
}

static void z80pio_port_w(const device_config *device, int channel)
{
	z80pio_t *z80pio = get_safe_token(device);
	UINT8 data = z80pio->output[channel];

	if (z80pio->mode[channel] == PIO_MODE_CONTROL)
	{
		/* mask input pins */
		data &= ~z80pio->ddr[channel];
	}

	if (channel == PIO_PORT_A)
	{
		if (z80pio->intf->port_a_w)
		{
			z80pio->intf->port_a_w(device, 0, data);
		}
	}
	else
	{
		if (z80pio->intf->port_b_w)
		{
			z80pio->intf->port_b_w(device, 0, data);
		}
	}

	//LOGERROR("Z80PIO \"%s\" Port %c : Output Data '%02x'\n", device->tag, 'A' + channel, data);
}

static void z80pio_set_mode(const device_config *device, int channel, UINT8 data)
{
	z80pio_t *z80pio = get_safe_token(device);

	int mode = data >> 6;

	switch (mode)
	{
	case PIO_MODE_OUTPUT:
		LOGERROR("Z80PIO \"%s\" Port %c : Mode %u (Byte Output)\n", device->tag, 'A' + channel, mode);

		/* set mode */
		z80pio->mode[channel] = mode;

		/* enable data output */
		z80pio_port_w(device, channel);

		/* assert ready line */
		z80pio_ready_w(device, channel, 1);
		break;

	case PIO_MODE_INPUT:
		LOGERROR("Z80PIO \"%s\" Port %c : Mode %u (Byte Input)\n", device->tag, 'A' + channel, mode);

		/* set mode */
		z80pio->mode[channel] = mode;
		break;

	case PIO_MODE_BIDIRECTIONAL:
		if (channel == PIO_PORT_A)
		{
			LOGERROR("Z80PIO \"%s\" Port %c : Mode %u (Bidirectional)\n", device->tag, 'A' + channel, mode);

			/* set mode */
			z80pio->mode[channel] = mode;
		}
		break;

	case PIO_MODE_CONTROL:
		LOGERROR("Z80PIO \"%s\" Port %c : Mode %u (Bit Control)\n", device->tag, 'A' + channel, mode);

		/* set mode */
		z80pio->mode[channel] = mode;

		if ((channel == PIO_PORT_A) || (z80pio->mode[PIO_PORT_A] != PIO_MODE_BIDIRECTIONAL))
		{
			/* clear ready line */
			z80pio_ready_w(device, channel, 0);
		}

		/* disable interrupts until DDR is written */
		z80pio_set_irq_enable(device, channel, 0);

		/* set logic equation to false */
		z80pio->match[channel] = 0;

		/* next word is I/O control */
		z80pio->state[channel] = PIO_STATE_DDR;
		break;
	}
}

/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( lh0081_c_r )
{
	z80pio_t *z80pio = get_safe_token(device);

	return (z80pio->enable[PIO_PORT_A] & 0xc0) | (z80pio->enable[PIO_PORT_B] >> 4);
}

READ8_DEVICE_HANDLER( z80pio_c_r )
{
	return 0;
}

WRITE8_DEVICE_HANDLER( z80pio_c_w )
{
	z80pio_t *z80pio = get_safe_token(device);
	int channel = offset & 0x01;

	switch (z80pio->state[channel])
	{
	case PIO_STATE_NORMAL:
		if (!BIT(data, 0))
		{
			LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Vector Word '%02x'\n", device->tag, 'A' + channel, z80pio->vector[channel]);

			/* load interrupt vector */
			z80pio->vector[channel] = data & 0xfe;
		}
		else if ((data & 0x0f) == PIO_IRQ_DISABLE_WORD)
		{
			LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Disable Word '%02x'\n", device->tag, 'A' + channel, data);

			/* set interrupt enable */
			z80pio->enable[channel] = (data & 0x80) | (z80pio->enable[channel] & 0x7f);
			z80pio_set_irq_enable(device, channel, BIT(data, 7));
		}
		else if ((data & 0x0f) == PIO_MODE_CONTROL_WORD)
		{
			LOGERROR("Z80PIO \"%s\" Port %c : Mode Control Word '%02x'\n", device->tag, 'A' + channel, data);

			/* set operating mode */
			z80pio_set_mode(device, channel, data);
		}
		else if ((data & 0x0f) == PIO_IRQ_CONTROL_WORD)
		{
			LOGERROR("Z80PIO \"%s\" Port %c : Interrupt Control Word '%02x'\n", device->tag, 'A' + channel, data);

			/* set interrupt control word */
			z80pio->enable[channel] = data & 0xf0;

			if (data & PIO_IRQ_MASK_FOLLOWS)
			{
				/* disable interrupts until mask is written */
				z80pio_set_irq_enable(device, channel, 0);

				/* reset pending interrupt */
				z80pio->irq_pending[channel] = 0;

				/* set logic equation to false */
				z80pio->match[channel] = 0;

				/* next word is mask control */
				z80pio->state[channel] = PIO_STATE_MASK;
			}
			else
			{
				/* set interrupt enable */
				z80pio_set_irq_enable(device, channel, BIT(data, 7));
			}
		}
		break;

	case PIO_STATE_DDR:
		LOGERROR("Z80PIO \"%s\" Port %c : I/O Control Word '%02x'\n", device->tag, 'A' + channel, data);

		/* set data direction */
		z80pio->ddr[channel] = data;

		/* set interrupt enable */
		z80pio_set_irq_enable(device, channel, BIT(z80pio->enable[channel], 7));

		/* next word is indeterminate */
		z80pio->state[channel] = PIO_STATE_NORMAL;
		break;

	case PIO_STATE_MASK:
		LOGERROR("Z80PIO \"%s\" Port %c : Mask Control Word '%02x'\n", device->tag, 'A' + channel, data);

		/* set interrupt mask */
		z80pio->mask[channel] = data;

		/* set interrupt enable */
		z80pio_set_irq_enable(device, channel, BIT(z80pio->enable[channel], 7));

		/* next word is indeterminate */
		z80pio->state[channel] = PIO_STATE_NORMAL;
		break;
	}
}

/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80pio_d_r )
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel = offset & 0x01;
	UINT8 data = 0;

	switch (z80pio->mode[channel])
	{
	case PIO_MODE_OUTPUT:
		data = z80pio->output[channel];
		break;

	case PIO_MODE_INPUT:
		data = z80pio->input[channel];

		/* clear ready line */
		z80pio_ready_w(device, channel, 0);

		/* assert ready line */
		z80pio_ready_w(device, channel, 1);
		break;

	case PIO_MODE_BIDIRECTIONAL:
		data = z80pio->input[PIO_PORT_A] | (z80pio->output[PIO_PORT_A] & (~z80pio->ddr[PIO_PORT_A]));

		/* clear ready line */
		z80pio_ready_w(device, 1, 0);

		/* assert ready line */
		z80pio_ready_w(device, 1, 1);
		break;

	case PIO_MODE_CONTROL:
		z80pio->input[channel] = z80pio_port_r(device, channel);

		data = (z80pio->input[channel] & z80pio->ddr[channel]) | (z80pio->output[channel] & (~z80pio->ddr[channel]));
		break;
	}

	LOGERROR("Z80PIO \"%s\" Port %c : Data Register Read '%02x'\n", device->tag, 'A' + channel, data);

	return data;
}

WRITE8_DEVICE_HANDLER( z80pio_d_w )
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel = offset & 0x01;

	LOGERROR("Z80PIO \"%s\" Port %c : Data Register Write '%02x'\n", device->tag, 'A' + channel, data);

	switch (z80pio->mode[channel])
	{
	case PIO_MODE_OUTPUT:
		/* clear ready line */
		z80pio_ready_w(device, channel, 0);

		/* output data */
		z80pio->output[channel] = data;
		z80pio_port_w(device, channel);

		/* assert ready line */
		z80pio_ready_w(device, channel, 1);
		break;

	case PIO_MODE_INPUT:
		/* do nothing */
		break;

	case PIO_MODE_BIDIRECTIONAL:
		/* clear ready line */
		z80pio_ready_w(device, channel, 0);

		/* latch output data */
		z80pio->output[channel] = data;

		if (!z80pio->strobe[channel])
		{
			/* output data if ASTB active */
			z80pio_port_w(device, channel);
		}

		/* assert ready line */
		z80pio_ready_w(device, channel, 1);
		break;

	case PIO_MODE_CONTROL:
		/* output data */
		z80pio->output[channel] = data;
		z80pio_port_w(device, channel);
		break;
	}
}

/***************************************************************************
    STROBE STATE MANAGEMENT
***************************************************************************/

static TIMER_CALLBACK( z80pio_poll_tick )
{
	const device_config *device = (device_config *) ptr;
	z80pio_t *z80pio = get_safe_token(device);
	int channel;

	for (channel = PIO_PORT_A; channel < PIO_PORT_COUNT; channel++)
	{
		if ((z80pio->mode[channel] == PIO_MODE_CONTROL) && (z80pio->mask[channel] != 0xff))
		{
			int bit;
			int match = 0;

			int edge = (z80pio->enable[channel] & PIO_IRQ_ACTIVE_HIGH) ? 1 : 0;

			UINT8 mask = z80pio->mask[channel];
			UINT8 ddr = z80pio->ddr[channel];
			UINT8 old_data = z80pio->input[channel] & ddr;
			UINT8 data = z80pio_port_r(device, channel) & ddr;

			if (old_data == data) continue;

			for (bit = 0; bit < 8; bit++)
			{
				if (BIT(mask, bit)) continue; /* masked out */
				if (!BIT(ddr, bit)) continue; /* output bit */

				if ((BIT(old_data, bit) ^ edge) && !(BIT(data, bit) ^ edge)) /* falling edge */
				{
					/* logic equation is true, until proven otherwise */
					match = 1;

					LOGERROR("Z80PIO \"%s\" Port %c : Edge Transition detected on bit %u, data %02x %02x\n", device->tag, 'A' + channel, bit, old_data, data);
				}
				else if (z80pio->enable[channel] & PIO_IRQ_AND)
				{
					/* all bits must match in AND mode, so logic equation is false */
					match = 0;
					break;
				}
			}

			if (!z80pio->match[channel] && match) /* rising edge */
			{
				/* trigger interrupt only once */
				z80pio_trigger_interrupt(device, channel);
				z80pio->input[channel] = data;
			}

			z80pio->match[channel] = match;
		}
	}
}

/***************************************************************************
    STROBE STATE MANAGEMENT
***************************************************************************/

static void z80pio_strobe_bidirectional(const device_config *device, int channel, int state)
{
	z80pio_t *z80pio = get_safe_token(device);

	switch (channel)
	{
	case PIO_PORT_A:
		if (!state) /* low level */
		{
			/* output data */
			z80pio_port_w(device, PIO_PORT_A);
		}
		else if (!z80pio->strobe[channel] && state) /* rising edge */
		{
			/* trigger interrupt */
			z80pio_trigger_interrupt(device, channel);

			/* clear ready line */
			z80pio_ready_w(device, channel, 0);
		}
		break;

	case PIO_PORT_B:
		if (!state) /* low level */
		{
			/* input data */
			z80pio->input[channel] = z80pio_port_r(device, PIO_PORT_A);
		}
		else if (!z80pio->strobe[channel] && state) /* rising edge */
		{
			/* trigger interrupt */
			z80pio_trigger_interrupt(device, channel);

			/* clear ready line */
			z80pio_ready_w(device, channel, 0);
		}
		break;
	}

	/* latch strobe state */
	z80pio->strobe[channel] = state;
}

static void z80pio_strobe(const device_config *device, int channel, int state)
{
	z80pio_t *z80pio = get_safe_token(device);

	switch (z80pio->mode[channel])
	{
	case PIO_MODE_OUTPUT:
		if (!z80pio->strobe[channel] && state) /* rising edge */
		{
			/* trigger interrupt */
			z80pio_trigger_interrupt(device, channel);

			/* clear ready line */
			z80pio_ready_w(device, channel, 0);
		}
		break;

	case PIO_MODE_INPUT:
		if (!state) /* low level */
		{
			/* strobe data in */
			z80pio->input[channel] = z80pio_port_r(device, channel);
		}
		else if (!z80pio->strobe[channel] && state) /* rising edge */
		{
			/* trigger interrupt */
			z80pio_trigger_interrupt(device, channel);

			/* clear ready line */
			z80pio_ready_w(device, channel, 0);
		}
		break;

	case PIO_MODE_CONTROL:
		/* ignore strobes */
		break;
	}

	/* latch strobe state */
	z80pio->strobe[channel] = state;
}

static void z80pio_strobe_w(const device_config *device, int channel, int state)
{
	z80pio_t *z80pio = get_safe_token(device);

	LOGERROR("Z80PIO \"%s\" Port %c : Strobe %u\n", device->tag, 'A' + channel, state);

	if (z80pio->mode[PIO_PORT_A] == PIO_MODE_BIDIRECTIONAL)
	{
		/* port A bidirectional mode */
		z80pio_strobe_bidirectional(device, channel, state);
	}
	else
	{
		z80pio_strobe(device, channel, state);
	}
}

void z80pio_astb_w(const device_config *device, int state)
{
	z80pio_strobe_w(device, PIO_PORT_A, state);
}

void z80pio_bstb_w(const device_config *device, int state)
{
	z80pio_strobe_w(device, PIO_PORT_B, state);
}

/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

static int z80pio_irq_state(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int state = 0;
	int channel;

	/* loop over all channels */
	for (channel = PIO_PORT_A; channel < PIO_PORT_COUNT; channel++)
	{
		/* if we're servicing a request, don't indicate more interrupts */
		if (z80pio->int_state[channel] & Z80_DAISY_IEO)
		{
			state |= Z80_DAISY_IEO;
			break;
		}

		state |= z80pio->int_state[channel];
	}

	LOGERROR("Z80PIO \"%s\" : Interrupt State %u\n", device->tag, state);

	return state;
}

static int z80pio_irq_ack(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel;

	LOGERROR("Z80PIO \"%s\" Interrupt Acknowledge\n", device->tag);

	/* loop over all channels */
	for (channel = PIO_PORT_A; channel < PIO_PORT_COUNT; channel++)
	{
		/* find the first channel with an interrupt requested */
		if (z80pio->int_state[channel] & Z80_DAISY_INT)
		{
			/* clear interrupt, switch to the IEO state, and update the IRQs */
			z80pio->int_state[channel] = Z80_DAISY_IEO;
			z80pio_check_interrupt(device);

			return z80pio->vector[channel];
		}
	}

	LOGERROR("z80pio_irq_ack: failed to find an interrupt to ack!");

	return z80pio->vector[0];
}

static void z80pio_irq_reti(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel;

	LOGERROR("Z80PIO \"%s\" Return from Interrupt\n", device->tag);

	/* loop over all channels */
	for (channel = PIO_PORT_A; channel < PIO_PORT_COUNT; channel++)
	{
		/* find the first channel with an IEO pending */
		if (z80pio->int_state[channel] & Z80_DAISY_IEO)
		{
			/* clear the IEO state and update the IRQs */
			z80pio->int_state[channel] &= ~Z80_DAISY_IEO;
			z80pio_check_interrupt(device);

			return;
		}
	}

	LOGERROR("z80pio_irq_reti: failed to find an interrupt to clear IEO on!");
}

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

READ8_DEVICE_HANDLER(z80pio_r)
{
	return (offset & 2) ? z80pio_c_r(device, offset & 1) : z80pio_d_r(device, offset & 1);
}

WRITE8_DEVICE_HANDLER(z80pio_w)
{
	if (offset & 2)
		z80pio_c_w(device, offset & 1, data);
	else
		z80pio_d_w(device, offset & 1, data);
}

READ8_DEVICE_HANDLER(z80pio_alt_r)
{
	int channel = BIT(offset, 1);

	return (offset & 1) ? z80pio_c_r(device, channel) : z80pio_d_r(device, channel);
}

WRITE8_DEVICE_HANDLER(z80pio_alt_w)
{
	int channel = BIT(offset, 1);

	if (offset & 1)
		z80pio_c_w(device, channel, data);
	else
		z80pio_d_w(device, channel, data);
}

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( z80pio )
{
	const z80pio_interface *intf = device->static_config;
	z80pio_t *z80pio = get_safe_token( device );
	char unique_tag[30];
	int cpunum = -1;

	assert(intf != NULL);
	z80pio->intf = intf;

	/* get clock */

	if (intf->cpu != NULL)
	{
		cpunum = mame_find_cpu_index(device->machine, intf->cpu);
	}

	if (cpunum != -1)
	{
		z80pio->clock = device->machine->config->cpu[cpunum].clock;
	}
	else
	{
		assert(intf->clock > 0);
		z80pio->clock = intf->clock;
	}

	/* allocate poll timer */

	z80pio->poll_timer = timer_alloc(z80pio_poll_tick, (void *)device);
	timer_adjust_periodic(z80pio->poll_timer, attotime_zero, 0, ATTOTIME_IN_HZ(z80pio->clock / 16));

	/* allocate interrupt enable timer */

	z80pio->irq_timer = timer_alloc(z80pio_irq_tick, (void *)device);

	/* register for state saving */

	state_save_combine_module_and_tag(unique_tag, "z80pio", device->tag);

	state_save_register_item_array(unique_tag, 0, z80pio->state);
	state_save_register_item_array(unique_tag, 0, z80pio->mode);
	state_save_register_item_array(unique_tag, 0, z80pio->irq_enable);
	state_save_register_item_array(unique_tag, 0, z80pio->irq_pending);
	state_save_register_item_array(unique_tag, 0, z80pio->int_state);
	state_save_register_item_array(unique_tag, 0, z80pio->enable);
	state_save_register_item_array(unique_tag, 0, z80pio->input);
	state_save_register_item_array(unique_tag, 0, z80pio->output);
	state_save_register_item_array(unique_tag, 0, z80pio->vector);
	state_save_register_item_array(unique_tag, 0, z80pio->mask);
	state_save_register_item_array(unique_tag, 0, z80pio->ddr);
	state_save_register_item_array(unique_tag, 0, z80pio->strobe);
	state_save_register_item_array(unique_tag, 0, z80pio->match);

	return DEVICE_START_OK;
}

static DEVICE_RESET( z80pio )
{
	z80pio_t *z80pio = get_safe_token(device);
	int channel;

	LOGERROR("Z80PIO \"%s\" : Reset\n", device->tag);

	for (channel = 0; channel < 2; channel++)
	{
		/* set mode 1 */
		z80pio->mode[channel] = PIO_MODE_INPUT;

		/* disable interrupt */
		z80pio->enable[channel] &= 0x7f;
		z80pio->irq_enable[channel] = 0;
		z80pio->irq_pending[channel] = 0;
		z80pio->int_state[channel] = 0;
		z80pio->match[channel] = 0;

		/* reset all bits of the data I/O register */
		z80pio->ddr[channel] = 0;

		/* set all bits of the mask control register */
		z80pio->mask[channel] = 0xff;

		/* reset output register */
		z80pio->output[channel] = 0;

		/* clear ready line */
		z80pio_ready_w(device, channel, 0);
	}

	z80pio_check_interrupt(device);
}

static DEVICE_SET_INFO( z80pio )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( z80pio )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(z80pio_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;							break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(z80pio); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(z80pio);break;
		case DEVINFO_FCT_STOP:							/* Nothing */							break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(z80pio);break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)z80pio_irq_state;		break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)z80pio_irq_ack;		break;
		case DEVINFO_FCT_IRQ_RETI:						info->f = (genf *)z80pio_irq_reti;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Zilog Z80 PIO";				break;
		case DEVINFO_STR_FAMILY:						info->s = "Z80";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";						break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;						break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright Nicola Salmoria and the MAME Team"; break;
	}
}

DEVICE_GET_INFO( z8420 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Zilog Z8420";				break;
		case DEVINFO_STR_FAMILY:						info->s = "Z80 PIO";					break;

		default:										DEVICE_GET_INFO_CALL(z80pio);
	}
}

DEVICE_GET_INFO( lh0081 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Sharp LH0081";				break;
		case DEVINFO_STR_FAMILY:						info->s = "Z80 PIO";					break;

		default:										DEVICE_GET_INFO_CALL(z80pio);
	}
}

DEVICE_GET_INFO( mk3881 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Mostek MK3881";				break;
		case DEVINFO_STR_FAMILY:						info->s = "Z80 PIO";					break;

		default:										DEVICE_GET_INFO_CALL(z80pio);
	}
}
