/***************************************************************************

    Z80 PIO implementation

    based on original version (c) 1997, Tatsuyuki Satoh

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

#define LOG 0

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

#define PIO_IRQ_MASK_FOLLOWS	0x10
#define PIO_IRQ_HIGH_LOW		0x20
#define PIO_IRQ_AND_OR			0x40
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
	int irq_pending[PIO_PORT_COUNT];		/* interrupt pending */
	int int_state[PIO_PORT_COUNT];			/* interrupt status */
	UINT8 enable[PIO_PORT_COUNT];			/* interrupt control word */
	UINT8 input[PIO_PORT_COUNT];			/* data input register */
	UINT8 output[PIO_PORT_COUNT];			/* data output register */
	UINT8 vector[PIO_PORT_COUNT];			/* interrupt vector */
	UINT8 mask[PIO_PORT_COUNT];				/* mask register */
	UINT8 ddr[PIO_PORT_COUNT];				/* input/output select register */
	int strobe[PIO_PORT_COUNT];				/* strobe pulse */
	int match[PIO_PORT_COUNT];				/* mode 3 match equation match */

	/* timers */
	emu_timer *poll_timer;					/* mode 3 poll timer */
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
				if (LOG) logerror("Z80PIO Port %c Interrupt Request\n", 'A' + channel);

				/* request interrupt */
				z80pio->irq_pending[channel] = 0;
				z80pio->int_state[channel] |= Z80_DAISY_INT;
				break;
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

static void z80pio_set_ready_line(const device_config *device, int channel, int level)
{
	z80pio_t *z80pio = get_safe_token(device);

	if (LOG) logerror("Z80PIO Port %c Ready %u\n", 'A' + channel, level);

	if (channel == PIO_PORT_A)
	{
		if (z80pio->intf->on_ardy_changed)
		{
			z80pio->intf->on_ardy_changed(device, level);
		}
	}
	else
	{
		if (z80pio->intf->on_brdy_changed)
		{
			z80pio->intf->on_brdy_changed(device, level);
		}
	}
}

static void z80pio_port_read(const device_config *device, int channel)
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

	if (z80pio->mode[channel] == PIO_MODE_BIDIRECTIONAL)
	{
		/* mask out output bits */
		data &= z80pio->ddr[channel];
	}

	//if (LOG) logerror("Z80PIO Port %c Read Data %02x\n", 'A' + channel, data);

	z80pio->input[channel] = data;
}

static void z80pio_port_write(const device_config *device, int channel)
{
	z80pio_t *z80pio = get_safe_token(device);
	UINT8 data = z80pio->output[channel];

	switch (z80pio->mode[channel])
	{
	case PIO_MODE_BIDIRECTIONAL:
	case PIO_MODE_CONTROL:
		/* mask out input bits */
		data &= ~z80pio->ddr[channel];
		break;
	
	default:
		break;
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

	if (LOG) logerror("Z80PIO Port %c Write Data %02x\n", 'A' + channel, data);
}

static void z80pio_set_mode(const device_config *device, int channel, int mode)
{
	z80pio_t *z80pio = get_safe_token(device);

	if (LOG) logerror("Z80PIO Port %c Mode %u\n", 'A' + channel, mode);

	/* clear interrupt */
	z80pio->irq_pending[channel] = 0;
	z80pio->int_state[channel] = 0;
	z80pio->match[channel] = 0;

	switch (mode)
	{
	case PIO_MODE_OUTPUT:
		z80pio->mode[channel] = mode;

		/* enable data output */
		z80pio_port_write(device, channel);

		/* assert ready line */
		z80pio_set_ready_line(device, channel, 1);
		break;

	case PIO_MODE_INPUT:
		z80pio->mode[channel] = mode;
		break;

	case PIO_MODE_BIDIRECTIONAL:
		if (channel == PIO_PORT_A)
		{
			z80pio->mode[channel] = mode;
		}
		break;

	case PIO_MODE_CONTROL:
		z80pio->mode[channel] = mode;

		if ((channel == PIO_PORT_A) || (z80pio->mode[PIO_PORT_A] != PIO_MODE_BIDIRECTIONAL))
		{
			/* clear ready line */
			z80pio_set_ready_line(device, channel, 0);
		}
		break;
	}
}

/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80pio_c_r )
{
	return z80pio_d_r(device, offset);
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
			/* load interrupt vector */
			z80pio->vector[channel] = data & 0xfe;

			if (LOG) logerror("Z80PIO Port %c Interrupt Vector %02x\n", 'A' + channel, z80pio->vector[channel]);
		}
		else if ((data & 0x0f) == 0x0f)
		{
			/* select operating mode */
			int mode = data >> 6;

			z80pio_set_mode(device, channel, mode);

			if (mode == PIO_MODE_CONTROL)
			{
				z80pio->state[channel] = PIO_STATE_DDR;
			}
		}
		else if ((data & 0x0f) == 0x07)
		{
			/* set interrupt control word */
			z80pio->enable[channel] = data & 0xf0;

			if (data & PIO_IRQ_MASK_FOLLOWS)
			{
				/* mask follows */
				z80pio->state[channel] = PIO_STATE_MASK;

				/* clear pending interrupt */
				z80pio->irq_pending[channel] = 0;
			}
			else
			{
				/* set interrupt enable */
				z80pio->irq_enable[channel] = BIT(data, 7);
			}

			z80pio_check_interrupt(device);

			if (LOG) logerror("Z80PIO Port %c Interrupt Control %02x\n", 'A' + channel, data);
		}
		break;


	case PIO_STATE_DDR:
		/* set data direction */
		z80pio->ddr[channel] = data;
		z80pio->match[channel] = 0;

		z80pio->state[channel] = PIO_STATE_NORMAL;

		if (LOG) logerror("Z80PIO Port %c Direction %02x\n", 'A' + channel, data);
		break;

	case PIO_STATE_MASK:
		/* set interrupt mask */
		z80pio->mask[channel] = data;
		z80pio->match[channel] = 0;

		/* set interrupt enable */
		z80pio->irq_enable[channel] = BIT(z80pio->enable[channel], 7);
		z80pio_check_interrupt(device);

		z80pio->state[channel] = PIO_STATE_NORMAL;

		if (LOG) logerror("Z80PIO Port %c Mask %02x\n", 'A' + channel, data);
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
		z80pio_set_ready_line(device, channel, 0);

		/* assert ready line */
		z80pio_set_ready_line(device, channel, 1);
		break;

	case PIO_MODE_BIDIRECTIONAL:
		data = z80pio->input[PIO_PORT_A] | (z80pio->output[PIO_PORT_A] & (~z80pio->ddr[PIO_PORT_A]));

		/* clear ready line */
		z80pio_set_ready_line(device, 1, 0);

		/* assert ready line */
		z80pio_set_ready_line(device, 1, 1);
		break;

	case PIO_MODE_CONTROL:
		data = z80pio->input[channel] | (z80pio->output[channel] & (~z80pio->ddr[channel]));
		break;
	}

	if (LOG) logerror("Z80PIO Port %c Data Register Read %02x\n", 'A' + channel, data);

	return data;
}

WRITE8_DEVICE_HANDLER( z80pio_d_w )
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel = offset & 0x01;

	if (LOG) logerror("Z80PIO Port %c Data Register Write %02x\n", 'A' + channel, data);

	switch (z80pio->mode[channel])
	{
	case PIO_MODE_OUTPUT:
		/* clear ready line */
		z80pio_set_ready_line(device, channel, 0);

		/* output data */
		z80pio->output[channel] = data;
		z80pio_port_write(device, channel);

		/* assert ready line */
		z80pio_set_ready_line(device, channel, 1);
		break;

	case PIO_MODE_INPUT:
		/* do nothing */
		break;

	case PIO_MODE_BIDIRECTIONAL:
		/* clear ready line */
		z80pio_set_ready_line(device, PIO_PORT_A, 0);

		/* latch output data */
		z80pio->output[channel] = data;

		if (!z80pio->strobe[PIO_PORT_A])
		{
			/* output data if ASTB active */
			z80pio_port_write(device, channel);
		}

		/* assert ready line */
		z80pio_set_ready_line(device, PIO_PORT_A, 1);
		break;

	case PIO_MODE_CONTROL:
		/* output data */
		z80pio->output[channel] = data;
		z80pio_port_write(device, channel);
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
		if (z80pio->mode[channel] == PIO_MODE_CONTROL)
		{
			int match = 0;
			UINT8 data;

			/* input data */
			z80pio_port_read(device, channel);

			if (z80pio->enable[channel] & PIO_IRQ_HIGH_LOW)
			{
				/* active polarity is high */
				data = z80pio->input[channel] & ~z80pio->mask[channel];
			}
			else
			{
				/* active polarity is low */
				data = ~z80pio->input[channel] & ~z80pio->mask[channel];
			}

			if (z80pio->enable[channel] & PIO_IRQ_AND_OR)
			{
				/* logical operation is AND */
				match = (data == ~z80pio->mask[channel]);
			}
			else
			{
				/* logical operation is OR */
				match = (data != 0);
			}

			if (match)
			{
				if (!z80pio->match[channel])
				{
					/* trigger interrupt */
					z80pio->match[channel] = 1;
					z80pio->irq_pending[channel] = 1;

					if (LOG) logerror("Z80PIO Port %c Interrupt Pending\n", 'A' + channel);

					z80pio_check_interrupt(device);
				}
			}
			else
			{
				z80pio->match[channel] = 0;
			}
		}
	}
}

/***************************************************************************
    STROBE STATE MANAGEMENT
***************************************************************************/

static void z80pio_strobe(const device_config *device, int channel, int level)
{
	z80pio_t *z80pio = get_safe_token(device);

	if (LOG) logerror("Z80PIO Port %c Strobe %u\n", 'A' + channel, level);

	if (z80pio->mode[PIO_PORT_A] == PIO_MODE_BIDIRECTIONAL)
	{
		/* port A bidirectional mode */

		switch (channel)
		{
		case PIO_PORT_A:
			if (!level)
			{
				/* output data */
				z80pio_port_write(device, PIO_PORT_A);
			}
			break;
		
		case PIO_PORT_B:
			if (!level)
			{
				/* input data */
				z80pio_port_read(device, PIO_PORT_A);
			}
			else if (!z80pio->strobe[PIO_PORT_A] && level) // rising edge
			{
				/* generate interrupt */
				z80pio->irq_pending[channel] = 1;
				z80pio_check_interrupt(device);

				/* clear ready line */
				z80pio_set_ready_line(device, channel, 0);

				if (LOG) logerror("Z80PIO Port %c Interrupt Pending\n", 'A' + channel);
			}
			break;
		}
	}
	else
	{
		switch (z80pio->mode[channel])
		{
		case PIO_MODE_OUTPUT:
			if (!z80pio->strobe[channel] && level) // rising edge
			{
				/* generate interrupt */
				z80pio->irq_pending[channel] = 1;
				z80pio_check_interrupt(device);

				/* clear ready line */
				z80pio_set_ready_line(device, channel, 0);

				if (LOG) logerror("Z80PIO Port %c Interrupt Pending\n", 'A' + channel);
			}
			break;

		case PIO_MODE_INPUT:
			if (!level)
			{
				/* strobe data in */
				z80pio_port_read(device, channel);
			}
			else if (!z80pio->strobe[channel] && level) // rising edge
			{
				/* generate interrupt */
				z80pio->irq_pending[channel] = 1;
				z80pio_check_interrupt(device);

				/* clear ready line */
				z80pio_set_ready_line(device, channel, 0);

				if (LOG) logerror("Z80PIO Port %c Interrupt Pending\n", 'A' + channel);
			}
			break;

		case PIO_MODE_CONTROL:
			/* ignore strobes */
			break;
		}
	}

	z80pio->strobe[channel] = level;
}

void z80pio_astb_w(const device_config *device, int level)
{
	z80pio_strobe(device, PIO_PORT_A, level);
}

void z80pio_bstb_w(const device_config *device, int level)
{
	z80pio_strobe(device, PIO_PORT_B, level);
}

/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

static int z80pio_irq_state(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int state = 0;
	int channel;

	logerror("Z80PIO IRQ STATE\n");

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

	return state;
}

static int z80pio_irq_ack(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel;

	logerror("Z80PIO IRQ ACK\n");

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
	
	if (LOG) logerror("z80pio_irq_ack: failed to find an interrupt to ack!");

	return z80pio->vector[0];
}

static void z80pio_irq_reti(const device_config *device)
{
	z80pio_t *z80pio = get_safe_token( device );
	int channel;

	logerror("Z80PIO IRQ RETI\n");

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
	
	if (LOG) logerror("z80pio_irq_reti: failed to find an interrupt to clear IEO on!");
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

	/* allocate poll timers */

	z80pio->poll_timer = timer_alloc(z80pio_poll_tick, (void *)device);
	timer_adjust_periodic(z80pio->poll_timer, attotime_zero, 0, ATTOTIME_IN_HZ(z80pio->clock));

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

	if (LOG) logerror("Z80PIO Reset\n");

	for (channel = 0; channel < 2; channel++)
	{
		/* set port mask register to inhibit all port data bits */
		z80pio->mask[channel] = 0xff;
		
		/* clear ready line */
		z80pio_set_ready_line(device, channel, 0);

		/* set mode 1 */
		z80pio->mode[channel] = PIO_MODE_INPUT;

		/* reset interrupt */
		z80pio->enable[channel] &= 0x7f;
		z80pio->irq_enable[channel] = 0;
		z80pio->irq_pending[channel] = 0;
		z80pio->int_state[channel] = 0;
		z80pio->match[channel] = 0;

		/* reset output register */
		z80pio->output[channel] = 0;
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
