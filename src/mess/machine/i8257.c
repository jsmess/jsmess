/***************************************************************************

    Intel 8257 Programmable DMA Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

/*

    TODO:

    - use cr/ar directly

*/

#include "emu.h"
#include "i8257.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG 0

#define I8257_STATUS_TC0	0x01
#define I8257_STATUS_TC1	0x02
#define I8257_STATUS_TC2	0x04
#define I8257_STATUS_TC3	0x08
#define I8257_STATUS_UP		0x10

#define I8257_MODE_EN0		0x01
#define I8257_MODE_EN1		0x02
#define I8257_MODE_EN2		0x04
#define I8257_MODE_EN3		0x08
#define I8257_MODE_RP		0x10
#define I8257_MODE_EW		0x20
#define I8257_MODE_TCS		0x40
#define I8257_MODE_AL		0x80

enum
{
	STATE_SI,
	STATE_S0,
	STATE_S1,
	STATE_S2,
	STATE_S3,
	STATE_SW,
	STATE_SU,
	STATE_S4
};

enum
{
	MODE_VERIFY = 0,
	MODE_WRITE,
	MODE_READ,
	MODE_ILLEGAL
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _dma8257_t i8257_t;
struct _dma8257_t
{
	devcb_resolved_write_line	out_hrq_func;
	devcb_resolved_write_line	out_tc_func;
	devcb_resolved_write_line	out_mark_func;
	devcb_resolved_read8		in_memr_func[I8257_CHANNELS];
	devcb_resolved_write8		out_memw_func[I8257_CHANNELS];
	devcb_resolved_read8		in_ior_func[I8257_CHANNELS];
	devcb_resolved_write8		out_iow_func[I8257_CHANNELS];
	devcb_resolved_write_line	out_dack_func[I8257_CHANNELS];

	/* registers */
	UINT8 mr;					/* mode register */
	UINT8 sr;					/* status register */
	UINT16 ar[I8257_CHANNELS];	/* address register */
	UINT16 cr[I8257_CHANNELS];	/* count register */
	int fl;						/* first/last flip-flop */

	/* inputs */
	int drq[I8257_CHANNELS];	/* data request */
	int hlda;					/* hold acknowledge */
	int ready;					/* ready */

	/* outputs */
	int tc;						/* terminal count */
	int mark;					/* mark */

	/* transfer */
	int state;					/* DMA state */
	int channel;				/* current channel being serviced */
	int priority;				/* highest priority channel */
	UINT8 data;					/* DMA data */
	UINT16 address;				/* DMA address */
	UINT16 count;				/* DMA counter */
	int mode;					/* DMA transfer type */

	/* timers */
	emu_timer *dma_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE i8257_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == I8257);
	return (i8257_t *) downcast<legacy_device_base *>(device)->token();
}

INLINE const i8257_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == I8257));
	return (const i8257_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static void set_hrq(device_t *device, int state)
{
	i8257_t *i8257 = get_safe_token(device);

	if (LOG) logerror("I8257 '%s' Hold Request: %u\n", device->tag(), state);

	devcb_call_write_line(&i8257->out_hrq_func, state);
}

static void set_tc(device_t *device, int state)
{
	i8257_t *i8257 = get_safe_token(device);

	if (i8257->tc != state)
	{
		if (LOG) logerror("I8257 '%s' Terminal Count: %u\n", device->tag(), state);
		i8257->tc = state;
		devcb_call_write_line(&i8257->out_tc_func, state);
	}
}

static void set_mark(device_t *device, int state)
{
	i8257_t *i8257 = get_safe_token(device);

	if (i8257->mark != state)
	{
		if (LOG) logerror("I8257 '%s' Mark: %u\n", device->tag(), state);
		i8257->mark = state;
		devcb_call_write_line(&i8257->out_mark_func, state);
	}
}

static void set_dack(device_t *device, int active_channel)
{
	i8257_t *i8257 = get_safe_token(device);
	int ch;

	for (ch = 0; ch < I8257_CHANNELS; ch++)
	{
		int state = (ch != active_channel);

		if (LOG) logerror("I8257 '%s' DMA Acknowledge %u: %u\n", device->tag(), ch, state);

		devcb_call_write_line(&i8257->out_dack_func[ch], state);
	}
}

static int sample_drq(device_t *device)
{
	i8257_t *i8257 = get_safe_token(device);
	int ch;

	for (ch = 0; ch < I8257_CHANNELS; ch++)
	{
		if (i8257->drq[ch] && BIT(i8257->mr, ch))
		{
			return 1;
		}
	}

	return 0;
}

static void resolve_priorities(device_t *device)
{
	i8257_t *i8257 = get_safe_token(device);
	int i;

	for (i = 0; i < I8257_CHANNELS; i++)
	{
		int ch = (i + i8257->priority) % I8257_CHANNELS;

		if (i8257->drq[ch] && BIT(i8257->mr, ch))
		{
			i8257->channel = ch;
			i8257->address = i8257->ar[ch];
			i8257->count = i8257->cr[ch] & 0x3fff;
			i8257->mode = i8257->cr[ch] >> 14;
			break;
		}
	}
}

static void dma_read(device_t *device)
{
	i8257_t *i8257 = get_safe_token(device);

	switch (i8257->mode)
	{
	case MODE_VERIFY:
		break;

	case MODE_WRITE:
		i8257->data = devcb_call_read8(&i8257->in_memr_func[i8257->channel], i8257->address);
		if (LOG) logerror("I8257 '%s' DMA Memory Read %04x: %02x\n", device->tag(), i8257->address, i8257->data);
		break;

	case MODE_READ:
		i8257->data = devcb_call_read8(&i8257->in_ior_func[i8257->channel], i8257->address);
		if (LOG) logerror("I8257 '%s' DMA I/O Read %04x: %02x\n", device->tag(), i8257->address, i8257->data);
		break;

	case MODE_ILLEGAL:
		break;
	}
}

static void dma_write(device_t *device)
{
	i8257_t *i8257 = get_safe_token(device);

	switch (i8257->mode)
	{
	case MODE_VERIFY:
		break;

	case MODE_WRITE:
		devcb_call_write8(&i8257->out_iow_func[i8257->channel], i8257->address, i8257->data);
		if (LOG) logerror("I8257 '%s' DMA I/O Write %04x: %02x\n", device->tag(), i8257->address, i8257->data);
		break;

	case MODE_READ:
		devcb_call_write8(&i8257->out_memw_func[i8257->channel], i8257->address, i8257->data);
		if (LOG) logerror("I8257 '%s' DMA Memory Write %04x: %02x\n", device->tag(), i8257->address, i8257->data);
		break;

	case MODE_ILLEGAL:
		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( dma_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( dma_tick )
{
	device_t *device = (device_t *)ptr;
	i8257_t *i8257 = get_safe_token(device);
	int drq = 0;

	switch (i8257->state)
	{
	case STATE_SI:
		/* sample DRQn lines */
		drq = sample_drq(device);

		/* set HRQ if DRQn = 1 */
		if (drq)
		{
			set_hrq(device, 1);
			i8257->state = STATE_S0;
		}
		break;

	case STATE_S0:
		/* sample HLDA */
		if (i8257->hlda)
		{
			i8257->state = STATE_S1;
		}

		/* resolve DRQ priorities */
		resolve_priorities(device);
		break;

	case STATE_S1:
		/* present and data upper address */
		/* present lower address */
		i8257->state = STATE_S2;
		break;

	case STATE_S2:
		/* activate DACKn */
		set_dack(device, i8257->channel);

		/* activate read command */
		dma_read(device);

		/* advanced write command */
		if (i8257->mr & I8257_MODE_EW)
		{
			dma_write(device);
		}

		i8257->state = STATE_S3;
		break;

	case STATE_S3:
		/* activate write command */
		if (!(i8257->mr & I8257_MODE_EW))
		{
			dma_write(device);
		}

		i8257->address++;
		i8257->count--;

		/* activate MARK */
		switch (i8257->count & 0xff)
		{
		case 0:	case 0x80:
			set_mark(device, 1);
			break;
		}

		/* activate TC */
		if (i8257->count == 0)
		{
			set_tc(device, 1);
		}

		if (!i8257->ready)
		{
			i8257->state = STATE_SW;
		}
		else if (i8257->tc && (i8257->channel == 2) && (i8257->mr & I8257_MODE_AL))
		{
			i8257->state = STATE_SU;
		}
		else
		{
			i8257->state = STATE_S4;
		}
		break;

	case STATE_SW:
		/* sample READY line */
		if (i8257->ready)
		{
			if (i8257->tc && (i8257->channel == 2) && (i8257->mr & I8257_MODE_AL))
			{
				i8257->state = STATE_SU;
			}
			else
			{
				i8257->state = STATE_S4;
			}
		}
		break;

	case STATE_SU:
		i8257->sr |= I8257_STATUS_UP;

		/* auto load channel 2 parameters */
		i8257->ar[2] = i8257->ar[3];
		i8257->cr[2] = i8257->cr[3];

		if (LOG)
		{
			logerror("I8257 '%s' DMA Channel 2 Address Register: %04x\n", device->tag(), i8257->ar[2]);
			logerror("I8257 '%s' DMA Channel 2 Count Register: %04x\n", device->tag(), i8257->cr[2] & 0x3fff);
			logerror("I8257 '%s' DMA Channel 2 Mode: %u\n", device->tag(), i8257->cr[2] >> 14);
		}

		i8257->state = STATE_S4;
		break;

	case STATE_S4:
		i8257->sr &= ~I8257_STATUS_UP;

		if (i8257->tc)
		{
			/* reset enable for channel n if TC stop and TC are active */
			if ((i8257->mr & I8257_MODE_TCS) &&
				!((i8257->channel == 2) && (i8257->mr & I8257_MODE_AL)))
			{
				i8257->mr &= ~(1 << i8257->channel);
				if (LOG) logerror("I8257 '%s' DMA Channel %u: disabled\n", device->tag(), i8257->channel);
			}

			if (i8257->mr & I8257_MODE_RP)
			{
				i8257->priority = (i8257->channel + 1) % I8257_CHANNELS;
			}

			/* resolve DRQ priorities */
			resolve_priorities(device);
		}

		/* deactivate DACKn, MARK and TC */
		set_dack(device, -1);
		set_mark(device, 0);
		set_tc(device, 0);

		/* sample DRQn */
		drq = sample_drq(device);

		/* reset HRQ if HLDA = 0 or DRQ = 0 */
		if (!i8257->hlda || !drq)
		{
			set_hrq(device, 0);
			i8257->state = STATE_SI;
			i8257->dma_timer->enable(0);
		}
		else
		{
			i8257->state = STATE_S1;
		}
		break;
	}
}

READ8_DEVICE_HANDLER( i8257_r )
{
	i8257_t *i8257 = get_safe_token(device);
	UINT8 data = 0;
	int ch;

	switch (offset & 0x0f)
	{
	case 0:
	case 2:
	case 4:
	case 6:
		ch = offset / 2;

		if (i8257->fl)
		{
			data = i8257->ar[ch] >> 8;
		}
		else
		{
			data = i8257->ar[ch] & 0xff;
		}

		i8257->fl = !i8257->fl;
		break;

	case 1:
	case 3:
	case 5:
	case 7:
		ch = (offset - 1) / 2;

		if (i8257->fl)
		{
			data = i8257->cr[ch] >> 8;
		}
		else
		{
			data = i8257->cr[ch] & 0xff;
		}

		i8257->fl = !i8257->fl;
		break;

	case 8:
		data = i8257->sr;
		i8257->sr &= ~(I8257_STATUS_TC3 | I8257_STATUS_TC2 | I8257_STATUS_TC1 | I8257_STATUS_TC0);
		break;
	}

	return data;
}

WRITE8_DEVICE_HANDLER( i8257_w )
{
	i8257_t *i8257 = get_safe_token(device);
	int ch;

	switch (offset & 0x0f)
	{
	case 0:
	case 2:
	case 4:
	case 6:
		ch = offset / 2;

		if (i8257->fl)
		{
			i8257->ar[ch] = (data << 8) | (i8257->ar[ch] & 0xff);
			if (LOG) logerror("I8257 '%s' DMA Channel %u Address Register: %04x\n", device->tag(), ch, i8257->ar[ch]);
		}
		else
		{
			i8257->ar[ch] = (i8257->ar[ch] & 0xff00) | data;
		}

		if ((i8257->mr & I8257_MODE_AL) && (ch == 2))
		{
			i8257->ar[3] = i8257->ar[2];
		}

		i8257->fl = !i8257->fl;
		break;

	case 1:
	case 3:
	case 5:
	case 7:
		ch = (offset - 1) / 2;

		if (i8257->fl)
		{
			i8257->cr[ch] = (data << 8) | (i8257->cr[ch] & 0xff);
			if (LOG) logerror("I8257 '%s' DMA Channel %u Count Register: %04x\n", device->tag(), ch, i8257->cr[ch] & 0x3fff);
			if (LOG) logerror("I8257 '%s' DMA Channel %u Mode: %u\n", device->tag(), ch, i8257->cr[ch] >> 14);
		}
		else
		{
			i8257->cr[ch] = (i8257->cr[ch] & 0xff00) | data;
		}

		if ((i8257->mr & I8257_MODE_AL) && (ch == 2))
		{
			i8257->cr[3] = i8257->cr[2];
		}

		i8257->fl = !i8257->fl;
		break;

	case 8:
		i8257->mr = data;
		i8257->fl = 0;

		if ((i8257->state == STATE_SI) && sample_drq(device))
		{
			i8257->dma_timer->enable(1);
		}

		if (LOG)
		{
			logerror("I8257 '%s' DMA Channel 0: %s\n", device->tag(), BIT(i8257->mr, 0) ? "enabled" : "disabled");
			logerror("I8257 '%s' DMA Channel 1: %s\n", device->tag(), BIT(i8257->mr, 1) ? "enabled" : "disabled");
			logerror("I8257 '%s' DMA Channel 2: %s\n", device->tag(), BIT(i8257->mr, 2) ? "enabled" : "disabled");
			logerror("I8257 '%s' DMA Channel 3: %s\n", device->tag(), BIT(i8257->mr, 3) ? "enabled" : "disabled");
			if (BIT(i8257->mr, 4)) logerror("I8257 '%s' Rotating Priority\n", device->tag());
			if (BIT(i8257->mr, 5)) logerror("I8257 '%s' Extended Write\n", device->tag());
			if (BIT(i8257->mr, 6)) logerror("I8257 '%s' TC Stop\n", device->tag());
			if (BIT(i8257->mr, 7)) logerror("I8257 '%s' Auto Load\n", device->tag());
		}
		break;
	}
}

WRITE_LINE_DEVICE_HANDLER( i8257_hlda_w )
{
	i8257_t *i8257 = get_safe_token(device);

	i8257->hlda = state;

	if (LOG) logerror("I8257 '%s' Hold Acknowledge: %u\n", device->tag(), state);
}

WRITE_LINE_DEVICE_HANDLER( i8257_ready_w )
{
	i8257_t *i8257 = get_safe_token(device);

	i8257->ready = state;

	if (LOG) logerror("I8257 '%s' Ready: %u\n", device->tag(), state);
}

static void drq_w(device_t *device, int ch, int state)
{
	i8257_t *i8257 = get_safe_token(device);

	i8257->drq[ch] = state;

	if (LOG) logerror("I8257 '%s' Data Request %u: %u\n", device->tag(), ch, state);

	if (state && (i8257->state == STATE_SI))
	{
		i8257->dma_timer->enable(1);
	}
}

WRITE_LINE_DEVICE_HANDLER( i8257_drq0_w ) { drq_w(device, 0, state); }
WRITE_LINE_DEVICE_HANDLER( i8257_drq1_w ) { drq_w(device, 1, state); }
WRITE_LINE_DEVICE_HANDLER( i8257_drq2_w ) { drq_w(device, 2, state); }
WRITE_LINE_DEVICE_HANDLER( i8257_drq3_w ) { drq_w(device, 3, state); }

/*-------------------------------------------------
    DEVICE_START( i8257 )
-------------------------------------------------*/

static DEVICE_START( i8257 )
{
	i8257_t *i8257 = get_safe_token(device);
	i8257_interface *intf = (i8257_interface *)device->baseconfig().static_config();
	int ch;

	/* resolve callbacks */
	devcb_resolve_write_line(&i8257->out_hrq_func, &intf->out_hrq_func, device);
	devcb_resolve_write_line(&i8257->out_tc_func, &intf->out_tc_func, device);
	devcb_resolve_write_line(&i8257->out_mark_func, &intf->out_mark_func, device);

	for (ch = 0; ch < I8257_CHANNELS; ch++)
	{
		devcb_resolve_read8(&i8257->in_memr_func[ch], &intf->in_memr_func[ch], device);
		devcb_resolve_write8(&i8257->out_memw_func[ch], &intf->out_memw_func[ch], device);
		devcb_resolve_read8(&i8257->in_ior_func[ch], &intf->in_ior_func[ch], device);
		devcb_resolve_write8(&i8257->out_iow_func[ch], &intf->out_iow_func[ch], device);
		devcb_resolve_write_line(&i8257->out_dack_func[ch], &intf->out_dack_func[ch], device);
	}

	/* create the DMA timer */
	i8257->dma_timer = device->machine().scheduler().timer_alloc(FUNC(dma_tick), (void *)device);
	i8257->dma_timer->adjust(attotime::zero, 0, attotime::from_hz(device->clock()));

	/* register for state saving */
	device->save_item(NAME(i8257->mr));
	device->save_item(NAME(i8257->sr));
	device->save_item(NAME(i8257->ar));
	device->save_item(NAME(i8257->cr));
	device->save_item(NAME(i8257->fl));
	device->save_item(NAME(i8257->drq));
	device->save_item(NAME(i8257->hlda));
	device->save_item(NAME(i8257->ready));
	device->save_item(NAME(i8257->tc));
	device->save_item(NAME(i8257->mark));
	device->save_item(NAME(i8257->state));
	device->save_item(NAME(i8257->channel));
	device->save_item(NAME(i8257->priority));
	device->save_item(NAME(i8257->data));
	device->save_item(NAME(i8257->address));
	device->save_item(NAME(i8257->count));
	device->save_item(NAME(i8257->mode));
}

/*-------------------------------------------------
    DEVICE_RESET( i8257 )
-------------------------------------------------*/

static DEVICE_RESET( i8257 )
{
	i8257_t *i8257 = get_safe_token(device);

	i8257->fl = 0;
	i8257->mr = 0;
	i8257->sr = 0;
	i8257->state = STATE_SI;
	i8257->priority = 0;
	i8257->ready = 1;

	set_hrq(device, 0);
	set_mark(device, 0);
	set_tc(device, 0);
	set_dack(device, -1);

	i8257->dma_timer->enable(0);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( i8257 )
-------------------------------------------------*/

DEVICE_GET_INFO( i8257 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(i8257_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(i8257);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(i8257);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Intel 8257");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Intel 8085");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(I8257, i8257);
