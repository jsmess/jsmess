/*********************************************************************

    tms5501.c

    TMS5501 input/output controller

    Krzysztof Strzecha, Nathan Woods, 2003
    Based on TMS9901 emulator by Raphael Nabet

    21-May-2004 -   Fixed interrupt queue overflow bug (not really fixed
            previously).
    06-Mar-2004 -   Fixed bug in sensor input.
    01-Mar-2004 -   Interrupt queue overrun problem fixed.
    19-Oct-2003 -   Status register added. Reset fixed. Some cleanups.
            INTA enable/disable.

    TODO:
    - SIO

*********************************************************************/

#include "emu.h"
#include "tms5501.h"


/***************************************************************************
    PARAMETERS/MACROS
***************************************************************************/

#define DEBUG_TMS5501	0

#define LOG_TMS5501(device, message, data) do { if (DEBUG_TMS5501) logerror ("\nTMS5501 %s: %s %02x", device->tag(), message, data); } while (0)

/* status register */
#define TMS5501_FRAME_ERROR		0x01
#define TMS5501_OVERRUN_ERROR		0x02
#define TMS5501_SERIAL_RCVD		0x04
#define TMS5501_RCV_BUFFER_LOADED	0x08
#define TMS5501_XMIT_BUFFER_EMPTY	0x10
#define TMS5501_INTERRUPT_PENDING	0x20
#define TMS5501_FULL_BIT_DETECT		0x40
#define TMS5501_START_BIT_DETECT	0x80

/* command */
#define TMS5501_RESET			0x01
#define TMS5501_BREAK			0x02
#define TMS5501_INT_7_SELECT		0x04
#define TMS5501_INT_ACK_ENABLE		0x08
#define TMS5501_TEST_BIT_1		0x10
#define TMS5501_TEST_BIT_2		0x20
#define TMS5501_COMMAND_LATCHED_BITS	0x3e

/* interrupt mask register */
#define TMS5501_TIMER_0_INT		0x01
#define TMS5501_TIMER_1_INT		0x02
#define TMS5501_SENSOR_INT		0x04
#define TMS5501_TIMER_2_INT		0x08
#define TMS5501_SERIAL_RCV_LOADED_INT	0x10
#define TMS5501_SERIAL_XMIT_EMPTY_INT	0x20
#define TMS5501_TIMER_3_INT		0x40
#define TMS5501_TIMER_4_INT		0x80
#define TMS5501_INT_7_INT		0x80

#define TMS5501_PIO_INT_7		0x80


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tms5501_t tms5501_t;
struct _tms5501_t
{
	/* internal registers */
	UINT8 status;			/* status register */
	UINT8 command;			/* command register, bits 1-5 are latched */

	UINT8 sio_rate;			/* SIO configuration register */
	UINT8 sio_input_buffer;		/* SIO input buffer */
	UINT8 sio_output_buffer;	/* SIO output buffer */

	UINT8 pio_input_buffer;		/* PIO input buffer */
	UINT8 pio_output_buffer;	/* PIO output buffer */

	UINT8 interrupt_mask;		/* interrupt mask register */
	UINT8 pending_interrupts;	/* pending interrupts register */
	UINT8 interrupt_address;	/* interrupt vector register */

	UINT8 sensor;			/* sensor input */

	/* internal timers */
	UINT8 timer_counter[5];
	emu_timer * timer[5];
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE tms5501_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TMS5501);
	return (tms5501_t *) downcast<legacy_device_base *>(device)->token();
}


INLINE const tms5501_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TMS5501);
	return (const tms5501_interface *) device->baseconfig().static_config();
}

static const UINT8 timer_name[] = { TMS5501_TIMER_0_INT, TMS5501_TIMER_1_INT, TMS5501_TIMER_2_INT, TMS5501_TIMER_3_INT, TMS5501_TIMER_4_INT };

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    find_first_bit
-------------------------------------------------*/

static int find_first_bit(int value)
{
	int bit = 0;

	if (! value)
		return -1;

	while (! (value & 1))
	{
		value >>= 1;	/* try next bit */
		bit++;
	}
	return bit;
}



/*-------------------------------------------------
    tms5501_field_interrupts
-------------------------------------------------*/

static void tms5501_field_interrupts(device_t *device)
{
	static const UINT8 int_vectors[] = { 0xc7, 0xcf, 0xd7, 0xdf, 0xe7, 0xef, 0xf7, 0xff };

	tms5501_t *tms = get_token(device);
	const tms5501_interface *intf = get_interface(device);
	UINT8 current_ints = tms->pending_interrupts;

	/* disabling masked interrupts */
	current_ints &= tms->interrupt_mask;

	LOG_TMS5501(device, "Pending interrupts", tms->pending_interrupts);
	LOG_TMS5501(device, "Interrupt mask", tms->interrupt_mask);
	LOG_TMS5501(device, "Current interrupts", current_ints);

	if (current_ints)
	{
		/* selecting interrupt with highest priority */
		int level = find_first_bit(current_ints);
		LOG_TMS5501(device, "Interrupt level", level);

		/* resetting proper bit in pending interrupts register */
		tms->pending_interrupts &= ~(1<<level);

		/* selecting  interrupt vector */
		tms->interrupt_address = int_vectors[level];
		LOG_TMS5501(device, "Interrupt vector", int_vectors[level]);

		if ((tms->command & TMS5501_INT_ACK_ENABLE))
		{
			if (intf->interrupt_callback)
				(*intf->interrupt_callback)(device, 1, int_vectors[level]);
		}
		else
			tms->status |= TMS5501_INTERRUPT_PENDING;
	}
	else
	{
		if ((tms->command & TMS5501_INT_ACK_ENABLE))
		{
			if (intf->interrupt_callback)
				(*intf->interrupt_callback)(device, 0, 0);
		}
		else
			tms->status &= ~TMS5501_INTERRUPT_PENDING;
	}
}


/*-------------------------------------------------
    tms5501_timer_decrementer
-------------------------------------------------*/

static void tms5501_timer_decrementer(device_t *device, UINT8 mask)
{
	tms5501_t *tms = get_token(device);

	if ((mask != TMS5501_TIMER_4_INT) || ((mask == TMS5501_TIMER_4_INT) && (!(tms->command & TMS5501_INT_7_SELECT))))
		tms->pending_interrupts |= mask;

	tms5501_field_interrupts(device);
}


/*-------------------------------------------------
    TIMER_CALLBACK(tms5501_timer_decrementer_callback)
-------------------------------------------------*/

static TIMER_CALLBACK(tms5501_timer_decrementer_callback)
{
	device_t *device = (device_t *) ptr;
	UINT8 mask = param;

	tms5501_timer_decrementer(device, mask);
}


/*-------------------------------------------------
    tms5501_timer_reload
-------------------------------------------------*/

static void tms5501_timer_reload(device_t *device, int timer)
{
	tms5501_t *tms = get_token(device);
	const tms5501_interface *intf = get_interface(device);

	if (tms->timer_counter[timer])
	{	/* reset clock interval */
		tms->timer[timer]->adjust(attotime::from_double((double) tms->timer_counter[0] / (intf->clock_rate / 128.)), timer_name[timer], attotime::from_double((double) tms->timer_counter[timer] / (intf->clock_rate / 128.)));
	}
	else
	{	/* clock interval == 0 -> no timer */
		switch (timer)
		{
			case 0: tms5501_timer_decrementer(device, TMS5501_TIMER_0_INT); break;
			case 1: tms5501_timer_decrementer(device, TMS5501_TIMER_1_INT); break;
			case 2: tms5501_timer_decrementer(device, TMS5501_TIMER_2_INT); break;
			case 3: tms5501_timer_decrementer(device, TMS5501_TIMER_3_INT); break;
			case 4: tms5501_timer_decrementer(device, TMS5501_TIMER_4_INT); break;
		}
		tms->timer[timer]->enable(0);
	}
}


/*-------------------------------------------------
    DEVICE_RESET( tms5501 )
-------------------------------------------------*/

static DEVICE_RESET( tms5501 )
{
	tms5501_t *tms = get_token(device);
	int i;

	tms->status &= ~(TMS5501_RCV_BUFFER_LOADED|TMS5501_FULL_BIT_DETECT|TMS5501_START_BIT_DETECT|TMS5501_OVERRUN_ERROR);
	tms->status |= TMS5501_XMIT_BUFFER_EMPTY|TMS5501_SERIAL_RCVD;

	tms->pending_interrupts = TMS5501_SERIAL_XMIT_EMPTY_INT;

	for (i=0; i<5; i++)
	{
		tms->timer_counter[i] = 0;
		tms->timer[i]->enable(0);
	}

	LOG_TMS5501(device, "Reset", 0);
}


/*-------------------------------------------------
    DEVICE_START( tms5501 )
-------------------------------------------------*/

static DEVICE_START( tms5501 )
{
	int i;
	tms5501_t *tms = get_token(device);

	for (i = 0; i < 5; i++)
	{
		tms->timer[i] = device->machine().scheduler().timer_alloc(FUNC(tms5501_timer_decrementer_callback), (void *) device);
		tms->timer[i]->set_param(i);
	}

	tms->interrupt_mask = 0;
	tms->interrupt_address = 0;

	tms->sensor = 0;
	tms->sio_rate = 0;
	tms->sio_input_buffer = 0;
	tms->sio_output_buffer = 0;
	tms->pio_input_buffer = 0;
	tms->pio_output_buffer = 0;

	tms->command = 0;
	LOG_TMS5501(device, "Init", 0);
}


/*-------------------------------------------------
    tms5501_set_pio_bit_7
-------------------------------------------------*/

void tms5501_set_pio_bit_7 (device_t *device, UINT8 data)
{
	tms5501_t *tms = get_token(device);

	if (tms->command & TMS5501_INT_7_SELECT)
	{
		if (!(tms->pio_input_buffer & TMS5501_PIO_INT_7) && data)
			tms->pending_interrupts |= TMS5501_INT_7_INT;
		else
			tms->pending_interrupts &= ~TMS5501_INT_7_INT;
	}

	tms->pio_input_buffer &= ~TMS5501_PIO_INT_7;
	if (data)
		tms->pio_input_buffer |= TMS5501_PIO_INT_7;

	if (tms->pending_interrupts & TMS5501_INT_7_INT)
		tms5501_field_interrupts(device);
}


/*-------------------------------------------------
    tms5501_sensor
-------------------------------------------------*/

void tms5501_sensor (device_t *device, UINT8 data)
{
	tms5501_t *tms = get_token(device);

	if (!(tms->sensor) && data)
		tms->pending_interrupts |= TMS5501_SENSOR_INT;
	else
		tms->pending_interrupts &= ~TMS5501_SENSOR_INT;

	tms->sensor = data;

	if (tms->pending_interrupts &= TMS5501_SENSOR_INT)
		tms5501_field_interrupts(device);
}


/*-------------------------------------------------
    READ8_DEVICE_HANDLER( tms5501_r )
-------------------------------------------------*/

READ8_DEVICE_HANDLER( tms5501_r )
{
	tms5501_t *tms = get_token(device);
	const tms5501_interface *intf = get_interface(device);

	UINT8 data = 0x00;
	offset &= 0x0f;

	switch (offset)
	{
		case 0x00:	/* Serial input buffer */
			data = tms->sio_input_buffer;
			tms->status &= ~TMS5501_RCV_BUFFER_LOADED;
			LOG_TMS5501(device, "Reading from serial input buffer", data);
			break;
		case 0x01:	/* PIO input port */
			if (intf->pio_read_callback)
				data = (*intf->pio_read_callback)(device);
			LOG_TMS5501(device, "Reading from PIO", data);
			break;
		case 0x02:	/* Interrupt address register */
			data = tms->interrupt_address;
			tms->status &= ~TMS5501_INTERRUPT_PENDING;
			break;
		case 0x03:	/* Status register */
			data = tms->status;
			break;
		case 0x04:	/* Command register */
			data = tms->command;
			LOG_TMS5501(device, "Command register read", data);
			break;
		case 0x05:	/* Serial rate register */
			data = tms->sio_rate;
			LOG_TMS5501(device, "Serial rate read", data);
			break;
		case 0x06:	/* Serial output buffer */
		case 0x07:	/* PIO output */
			break;
		case 0x08:	/* Interrupt mask register */
			data = tms->interrupt_mask;
			LOG_TMS5501(device, "Interrupt mask read", data);
			break;
		case 0x09:	/* Timer 0 address */
		case 0x0a:	/* Timer 1 address */
		case 0x0b:	/* Timer 2 address */
		case 0x0c:	/* Timer 3 address */
		case 0x0d:	/* Timer 4 address */
			break;
	}
	return data;
}


/*-------------------------------------------------
    WRITE8_DEVICE_HANDLER( tms5501_w )
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( tms5501_w )
{
	tms5501_t *tms = get_token(device);
	const tms5501_interface *intf = get_interface(device);
	offset &= 0x0f;

	switch (offset)
	{
		case 0x00:	/* Serial input buffer */
		case 0x01:	/* Keyboard input port, Page blanking signal */
		case 0x02:	/* Interrupt address register */
		case 0x03:	/* Status register */
			LOG_TMS5501(device, "Writing to read only port", offset&0x000f);
			LOG_TMS5501(device, "Data", data);
			break;
		case 0x04:
			/* Command register
                bit 0: reset
                bit 1: send break, '1' - serial output is high impedance
                bit 2: int 7 select: '0' - timer 5, '1' - IN7 of the DCE-bus
                bit 3: int ack enable, '0' - disabled, '1' - enabled
                bits 4-5: test bits, normally '0'
                bits 6-7: not used, normally '0'
               bits 1-5 are latched */

			tms->command = data & TMS5501_COMMAND_LATCHED_BITS;
			LOG_TMS5501(device, "Command register write", data);

			if (data & TMS5501_RESET)
				device->reset();
			break;
		case 0x05:
			/* Serial rate register
                bit 0: 110 baud
                bit 1: 150 baud
                bit 2: 300 baud
                bit 3: 1200 baud
                bit 4: 2400 baud
                bit 5: 4800 baud
                bit 6: 9600 baud
                bit 7: '0' - two stop bits, '1' - one stop bit */

			tms->sio_rate = data;
			LOG_TMS5501(device, "Serial rate write", data);
			break;
		case 0x06:	/* Serial output buffer */
			tms->sio_output_buffer = data;
			LOG_TMS5501(device, "Serial output data", data);
			break;
		case 0x07:	/* PIO output */
			tms->pio_output_buffer = data;
			if (intf->pio_write_callback)
				(*intf->pio_write_callback)(device, tms->pio_output_buffer);
			LOG_TMS5501(device, "Writing to PIO", data);
			break;
		case 0x08:
			/* Interrupt mask register
                bit 0: Timer 1 has expired (UTIM)
                bit 1: Timer 2 has expired
                bit 2: External interrupt (STKIM)
                bit 3: Timer 3 has expired (SNDIM)
                bit 4: Serial receiver loaded
                bit 5: Serial transmitter empty
                bit 6: Timer 4 has expired (KBIM)
                bit 7: Timer 5 has expired or IN7 (CLKIM) */

			tms->interrupt_mask = data;
			LOG_TMS5501(device, "Interrupt mask write", data);
			break;
		case 0x09:	/* Timer 0 counter */
		case 0x0a:	/* Timer 1 counter */
		case 0x0b:	/* Timer 2 counter */
		case 0x0c:	/* Timer 3 counter */
		case 0x0d:	/* Timer 4 counter */
			offset -= 9;
			tms->timer_counter[offset] = data;
			tms5501_timer_reload(device, offset);
			LOG_TMS5501(device, "Write timer", offset);
			LOG_TMS5501(device, "Timer counter set", data);
			break;
	}
}


/*-------------------------------------------------
    DEVICE_GET_INFO( tms5501 )
-------------------------------------------------*/

DEVICE_GET_INFO( tms5501 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(tms5501_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(tms5501);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(tms5501);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "TMS5501");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "TMS5501");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(TMS5501, tms5501);
