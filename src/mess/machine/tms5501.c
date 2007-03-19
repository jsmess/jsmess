/*
	TMS5501 input/output controller
	Krzysztof Strzecha, Nathan Woods, 2003
	Based on TMS9901 emulator by Raphael Nabet

	21-May-2004 -	Fixed interrupt queue overflow bug (not really fixed
			previously).
	06-Mar-2004 -   Fixed bug in sensor input.
	01-Mar-2004 -	Interrupt queue overrun problem fixed.
	19-Oct-2003 -	Status register added. Reset fixed. Some cleanups.
			INTA enable/disable.

	TODO:
	- SIO
*/

#include "driver.h"
#include "tms5501.h"

#define DEBUG_TMS5501	0

#if DEBUG_TMS5501
	#define LOG_TMS5501(n, message, data) logerror ("TMS5501 %d: %s %02x\n", n, message, data)
#else
	#define LOG_TMS5501(n, message, data)
#endif

#define MAX_TMS5501			1

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

typedef struct tms5501_t
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

	/* PIO callbacks */
	UINT8 (*pio_read_callback)(void);
	void (*pio_write_callback)(UINT8);

	/* SIO handlers */

	/* interrupt callback to notify driver about interruot and its vector */
	void (*interrupt_callback)(int intreq, UINT8 vector);

	/* internal timers */
	UINT8 timer_counter[5];
	mame_timer * timer[5];
	double clock_rate;
} tms5501_t;

static tms5501_t tms5501[MAX_TMS5501];

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

static void tms5501_field_interrupts(int which)
{
	UINT8 int_vectors[] = { 0xc7, 0xcf, 0xd7, 0xdf, 0xe7, 0xef, 0xf7, 0xff };
	UINT8 current_ints = tms5501[which].pending_interrupts;

	/* disabling masked interrupts */
	current_ints &= tms5501[which].interrupt_mask;

	LOG_TMS5501(which, "Pending interrupts", tms5501[which].pending_interrupts);
	LOG_TMS5501(which, "Interrupt mask", tms5501[which].interrupt_mask);
	LOG_TMS5501(which, "Current interrupts", current_ints);

	if (current_ints)
	{
		/* selecting interrupt with highest priority */
		int level = find_first_bit(current_ints);	
		LOG_TMS5501(which, "Interrupt level", level);

		/* reseting proper bit in pending interrupts register */
		tms5501[which].pending_interrupts &= ~(1<<level);

		/* selecting  interrupt vector */
		tms5501[which].interrupt_address = int_vectors[level];
		LOG_TMS5501(which, "Interrupt vector", int_vectors[level]);

		if ((tms5501[which].command & TMS5501_INT_ACK_ENABLE))
		{
			if (tms5501[which].interrupt_callback)
				(*tms5501[which].interrupt_callback)(1, int_vectors[level]);
		}
		else
			tms5501[which].status |= TMS5501_INTERRUPT_PENDING;
	}
	else
	{
		if ((tms5501[which].command & TMS5501_INT_ACK_ENABLE))
		{
			if (tms5501[which].interrupt_callback)
				(*tms5501[which].interrupt_callback)(0, 0);
		}
		else
			tms5501[which].status &= ~TMS5501_INTERRUPT_PENDING;
	}
}

static void tms5501_timer_0_decrementer_callback(int which)
{
	tms5501[which].pending_interrupts |= TMS5501_TIMER_0_INT;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_1_decrementer_callback(int which)
{
	tms5501[which].pending_interrupts |= TMS5501_TIMER_1_INT;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_2_decrementer_callback(int which)
{
	tms5501[which].pending_interrupts |= TMS5501_TIMER_2_INT;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_3_decrementer_callback(int which)
{
	tms5501[which].pending_interrupts |= TMS5501_TIMER_3_INT;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_4_decrementer_callback(int which)
{
	if (!(tms5501[which].command & TMS5501_INT_7_SELECT))
		tms5501[which].pending_interrupts |= TMS5501_TIMER_4_INT;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_reload(int which, int timer)
{
	if (tms5501[which].timer_counter[timer])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[timer], (double) tms5501[which].timer_counter[0] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[timer] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		switch (timer)
		{
			case 0: tms5501_timer_0_decrementer_callback(which); break;
			case 1: tms5501_timer_1_decrementer_callback(which); break;
			case 2: tms5501_timer_2_decrementer_callback(which); break;
			case 3: tms5501_timer_3_decrementer_callback(which); break;
			case 4: tms5501_timer_4_decrementer_callback(which); break;
		}
		timer_enable(tms5501[which].timer[timer], 0);
	}
}

static void tms5501_reset (int which)
{
	int i;

	tms5501[which].status &= ~(TMS5501_RCV_BUFFER_LOADED|TMS5501_FULL_BIT_DETECT|TMS5501_START_BIT_DETECT|TMS5501_OVERRUN_ERROR);
	tms5501[which].status |= TMS5501_XMIT_BUFFER_EMPTY|TMS5501_SERIAL_RCVD;

	tms5501[which].pending_interrupts = TMS5501_SERIAL_XMIT_EMPTY_INT;

	for (i=0; i<5; i++)
	{
		tms5501[which].timer_counter[i] = 0;
		timer_enable(tms5501[which].timer[i], 0);
	}

	LOG_TMS5501(which, "Reset", 0);
}

void tms5501_init (int which, const tms5501_init_param *param)
{
	tms5501[which].clock_rate = param->clock_rate;
	tms5501[which].timer[0] = timer_alloc(tms5501_timer_0_decrementer_callback);
	tms5501[which].timer[1] = timer_alloc(tms5501_timer_1_decrementer_callback);
	tms5501[which].timer[2] = timer_alloc(tms5501_timer_2_decrementer_callback);
	tms5501[which].timer[3] = timer_alloc(tms5501_timer_3_decrementer_callback);
	tms5501[which].timer[4] = timer_alloc(tms5501_timer_4_decrementer_callback);

	tms5501[which].pio_read_callback = param->pio_read_callback;
	tms5501[which].pio_write_callback = param->pio_write_callback;

	tms5501[which].interrupt_callback = param->interrupt_callback;

	tms5501[which].interrupt_mask = 0;
	tms5501[which].interrupt_address = 0;

	tms5501[which].sensor = 0;
	tms5501[which].sio_rate = 0;
	tms5501[which].sio_input_buffer = 0;
	tms5501[which].sio_output_buffer = 0;
	tms5501[which].pio_input_buffer = 0;
	tms5501[which].pio_output_buffer = 0;

	tms5501[which].command = 0;

	tms5501_reset(which);
	LOG_TMS5501(which, "Init", 0);
}

void tms5501_cleanup (int which)
{
	int i;

	for (i=0; i<5; i++)
	{
		if (tms5501[which].timer[i])
		{
			timer_reset(tms5501[which].timer[i], TIME_NEVER);	/* FIXME - timers should only be allocated once */
			tms5501[which].timer[i] = NULL;
		}
	}
	LOG_TMS5501(which, "Cleanup", 0);
}

void tms5501_set_pio_bit_7 (int which, UINT8 data)
{
	if (tms5501[which].command & TMS5501_INT_7_SELECT)
	{
		if (!(tms5501[which].pio_input_buffer & TMS5501_PIO_INT_7) && data)
			tms5501[which].pending_interrupts |= TMS5501_INT_7_INT;
		else
			tms5501[which].pending_interrupts &= ~TMS5501_INT_7_INT;
	}

	tms5501[which].pio_input_buffer &= ~TMS5501_PIO_INT_7;
	if (data)
		tms5501[which].pio_input_buffer |= TMS5501_PIO_INT_7;

	if (tms5501[which].pending_interrupts & TMS5501_INT_7_INT)
		tms5501_field_interrupts(which);
}

void tms5501_sensor (int which, UINT8 data)
{
	if (!(tms5501[which].sensor) && data)
		tms5501[which].pending_interrupts |= TMS5501_SENSOR_INT;
	else
		tms5501[which].pending_interrupts &= ~TMS5501_SENSOR_INT;

	tms5501[which].sensor = data;

	if (tms5501[which].pending_interrupts &= TMS5501_SENSOR_INT)
		tms5501_field_interrupts(which);
}

UINT8 tms5501_read (int which, UINT16 offset)
{
	UINT8 data = 0x00;
	switch (offset&0x000f)
	{
		case 0x00:	/* Serial input buffer */
			data = tms5501[which].sio_input_buffer;
			tms5501[which].status &= ~TMS5501_RCV_BUFFER_LOADED;
			LOG_TMS5501(which, "Reading from serial input buffer", data);
			break;
		case 0x01:	/* PIO input port */
			if (tms5501[which].pio_read_callback)
				data = tms5501[which].pio_read_callback();
			LOG_TMS5501(which, "Reading from PIO", data);
			break;
		case 0x02:	/* Interrupt address register */
			data = tms5501[which].interrupt_address;
			tms5501[which].status &= ~TMS5501_INTERRUPT_PENDING;
			break;
		case 0x03:	/* Status register */
			data = tms5501[which].status;
			break;
		case 0x04:	/* Command register */
			data = tms5501[which].command;
			LOG_TMS5501(which, "Command register read", data);	 
			break;
		case 0x05:	/* Serial rate register */
			data = tms5501[which].sio_rate;
			LOG_TMS5501(which, "Serial rate read", data);
			break;
		case 0x06:	/* Serial output buffer */
		case 0x07:	/* PIO output */
			break;
		case 0x08:	/* Interrupt mask register */
			data = tms5501[which].interrupt_mask;
			LOG_TMS5501(which, "Interrupt mask read", data);	 
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

void tms5501_write (int which, UINT16 offset, UINT8 data)
{
	switch (offset&0x000f)
	{
		case 0x00:	/* Serial input buffer */
		case 0x01:	/* Keyboard input port, Page blanking signal */
		case 0x02:	/* Interrupt address register */
		case 0x03:	/* Status register */
			LOG_TMS5501(which, "Writing to read only port", offset&0x000f);
			LOG_TMS5501(which, "Data", data);
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

			tms5501[which].command = data & TMS5501_COMMAND_LATCHED_BITS;
			LOG_TMS5501(which, "Command register write", data);

			if (data&TMS5501_RESET)	tms5501_reset(which);
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

			tms5501[which].sio_rate = data;
			LOG_TMS5501(which, "Serial rate write", data);
			break;
		case 0x06:	/* Serial output buffer */
			tms5501[which].sio_output_buffer = data;
			LOG_TMS5501(which, "Serial output data", data);
			break;
		case 0x07:	/* PIO output */
			tms5501[which].pio_output_buffer = data;
			if (tms5501[which].pio_write_callback)
				tms5501[which].pio_write_callback(tms5501[which].pio_output_buffer);
			LOG_TMS5501(which, "Writing to PIO", data);
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

			tms5501[which].interrupt_mask = data;
			LOG_TMS5501(which, "Interrupt mask write", data);
			break;
		case 0x09:	/* Timer 0 counter */
		case 0x0a:	/* Timer 1 counter */
		case 0x0b:	/* Timer 2 counter */
		case 0x0c:	/* Timer 3 counter */
		case 0x0d:	/* Timer 4 counter */
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload(which, (offset&0x0f)-0x09);
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x09);
			LOG_TMS5501(which, "Timer counter set", data);
			break;
	}
}

 READ8_HANDLER( tms5501_0_r )
{
	return tms5501_read(0, offset);
}

WRITE8_HANDLER( tms5501_0_w )
{
	tms5501_write(0, offset, data);
}
