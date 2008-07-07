/****************************************************************************

	ay31015.c by Robbbert, May 2008

	Code for the AY-3-1014A, AY-3-1015(D), AY-5-1013(A) and AY-6-1013 UARTs

	This is cycle-accurate according to the specifications.

	It supports independent receive and transmit clocks,
	and transmission and reception can occur simultaneously if desired.

	Note to any MESSDEVs who might happen to stray here:

	- If you find bugs, please fix them yourself.
	- If you want to convert it to a DEVICE, please do so.
	- If you want to add support for multiple instances, please do so.

*****************************************************************************

Differences between the chip types:
- All units have pull-up resistors on the inputs, except for the AY-3-1014A which is CMOS-compatible.
- AY-3-1014A and AY-3-1015 - 1.5 stop bits mode available.
- Max baud rate of 30k, except AY-5-1013 which has 20k.
- AY-5-1013 has extended temperature ratings.
- AY-5-1013 and AY-6-1013 require a -12 volt supply on pin 2. Pin is not used otherwise.
- AY-5-1013 and AY-6-1013 do not reset the received data register when XR pin is used.

******************************************************************************

It is not clear in the documentation as to which settings will reset the device.
	To be safe, we will always reset whenever the control register changes.

	Also, it not clear exactly what happens under various error conditions.
	What we do is:
	1. During receive the stop bit processing is cut short, so we can better
	synchronise the next byte.
	2. If a framing error occurs, we do not signal DAV, but instead skip
	the bad byte and start looking for the next.

	This device has only been tested successfully at 300 baud, with 8 bits,
	2 stop bits and no parity.

	The bit order of the status and control registers is set up to suit the
	Exidy driver. If you wish to use it for some other driver, use the BITSWAP8
	macro in your driver.

********************************************************************************

Device Data:

* Common Controls:
-- Pin 1 - Vcc - 5 volts
-- Pin 2 - not used (on AY-5-1013 and AY-6-1013 this is Voo = -12 volts)
-- Pin 3 - Gnd - 0 volts
-- Pin 21 - XR - External Reset - resets all registers to initial state except for the control register
-- Pin 35 - NP - No Parity - "1" will kill any parity processing
-- Pin 36 - TSB - Number of Stop Bits - "0" = 1 stop bit; "1" = 2 stop bits. If "1", and 5 bits per character, then we have 1.5 stop bits
-- pin 37 - NB1
-- pin 38 - NB2 - Number of bits per character = NB1 + (NB2 * 2) + 5
-- pin 39 - EPS - Odd or Even Parity Select - "0" = Odd parity; "1" = Even parity. Has no effect if NP is high.
-- Pin 34 - CS - Control Strobe - Read NP, TSB, EPS, NB1, NB2 into the control register.

Format of data stream:
Start bit (low), Bit 0, Bit 1... highest bit, Parity bit (if enabled), 1-2 stop bits (high)


* Receiver Controls:
-- Pin 17 - RCP - Clock which is 16x the desired baud rate
-- Pin 20 - SI - Serial input stream - "1" = Mark (waiting for input), "0" = Space (Start bit) initiates the transfer of a byte
-- Pin 4 - RDE - "0" causes the received data to appear on RD1 to RD8.
-- Pins 5 to 12 - RD8 to RD1 - These are the data lines (bits 7 to 0). Data is right-justified.
-- Pin 16 - SWE - Status word enable - causes the status bits (PE, FE, OR, DAV, TBMT) to appear at the pins.
-- Pin 19 - DAV - "1" indicates that a byte has been received by the UART, and should now be accepted by the computer
-- Pin 18 - RDAV - "0" will force DAV low.
-- Pin 13 - PE - Parity error - "1" indicates that a parity error occured
-- Pin 14 - FE - Framing error - "1" Indicates that the stop bit was missing
-- Pin 15 - OR - overrun - "1" indicates that a new character has become available before the computer had accepted the previous character

* Transmitter controls:
-- Pin 40 - TCP - Clock which is 16x the desired baud rate
-- Pin 25 - SO - Serial output stream - it will stay at "1" while no data is being transmitted
-- Pins 26 to 33 - DB1 to DB8 - These are the data lines containing the byte to be sent
-- Pin 23 - DS - Data Strobe - "0" will copy DB1 to DB8 into the transmit buffer
-- Pin 22 - TBMT - Transmit buffer Empty - "1" indicates to the computer that another byte may be sent to the UART
-- Pin 24 - EOC - End of Character - "0" means that a character is being sent.

******************************************* COMMON CONTROLS ********************************************************/

#include "driver.h"
#include "ay31015.h"

//typedef struct _ay31015_token ay31015_token;
static struct // _ay31015_token
{
	UINT8 control_reg;
	UINT8 status_reg;
	UINT16 second_stop_bit;	// 0, 8, 16
	UINT16 total_pulses;	// bits * 16
	UINT8 internal_sample;

	UINT8 rx_state;
	UINT8 rx_data;		// byte being received
	UINT8 rx_buffer;	// received byte waiting to be accepted by computer
	UINT8 rx_bit_count;
	UINT8 rx_parity;
	UINT16 rx_pulses;	// total pulses left

	UINT8 tx_state;
	UINT8 tx_data;		// byte being sent
	UINT8 tx_buffer;	// next byte to send
	UINT8 tx_parity;
	UINT16 tx_pulses;	// total pulses left
	UINT8 si;
} ay;

//static struct _ay31015_token ay;

/* state - must only be altered by the timer callbacks */

#define IDLE		0
#define START_BIT	1
#define PROCESSING	2
#define PARITY_BIT	3
#define FIRST_STOP_BIT	4
#define SECOND_STOP_BIT	5
#define PREP_TIME	6

#if 0
/*-------------------------------------------------
    get_token - safely gets the data
-------------------------------------------------*/

INLINE ay31015_token *get_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == AY31015);
	return (ay31015_token *) device->token;
}


/*-------------------------------------------------
    get_config - safely gets the config
-------------------------------------------------*/

INLINE const ay31015_config *get_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == ay31015);
	return (const ay31015_config *) device->static_config;
}


static DEVICE_START(ay31015)
{
	ay31015_token *ay;
//	const ay31015_config *config = get_config(device);

	ay = get_token(device);
}
#endif


/*-------------------------------------------------
    ay51013_xr - the reset pin
-------------------------------------------------*/
static void ay51013_xr( void )
{
	/* total pulses = 16 * data-bits */
	UINT8 t1;
	t1 = (ay.control_reg & 3) + 5;					/* data bits */
	ay.total_pulses = t1 << 4;					/* total clock pulses to load a byte */
	ay.second_stop_bit = ((ay.control_reg & AY31015_TSB) ? 16 : 0);		/* 2nd stop bit */
	ay.status_reg = AY31015_EOC | AY31015_TBMT | AY31015_SO;
	ay.tx_data = 0;
	ay.rx_state = PREP_TIME;
	ay.tx_state = IDLE;
	ay.si = 1;
}


/*-------------------------------------------------
    ay51013_init - drivers should call this at
    machine reset time
-------------------------------------------------*/
void ay51013_init( void )
{
	ay.control_reg = 0;
	ay.rx_data = 0;
	ay51013_xr();
}


/*-------------------------------------------------
    ay51013_cs - The entire control register is
    updated at once.
-------------------------------------------------*/
void ay51013_cs( UINT8 data )
{
	UINT8 t1 = ay.control_reg;
	ay.control_reg = data;
	if (ay.control_reg != t1) ay51013_xr();
}



/*-------------------------------------------------
    ay31015_xr - the reset pin
-------------------------------------------------*/
static void ay31015_xr( void )
{
	/* total pulses = 16 * data-bits */
	UINT8 t1;
	t1 = (ay.control_reg & 3) + 5;					/* data bits */
	ay.total_pulses = t1 << 4;					/* total clock pulses to load a byte */
	ay.second_stop_bit = ((ay.control_reg & AY31015_TSB) ? 16 : 0);		/* 2nd stop bit */
	if ((t1 == 5) && (ay.second_stop_bit == 16))
		ay.second_stop_bit = 8;				/* 5 data bits and 2 stop bits = 1.5 stop bits */
	ay.status_reg = AY31015_EOC | AY31015_TBMT | AY31015_SO;
	ay.rx_data = 0;
	ay.tx_data = 0;
	ay.rx_state = PREP_TIME;
	ay.tx_state = IDLE;
	ay.si = 1;
}


/*-------------------------------------------------
    ay31015_init - drivers should call this at
    machine reset time
-------------------------------------------------*/
void ay31015_init( void )
{
	ay.control_reg = 0;
	ay31015_xr();
}


/*-------------------------------------------------
    ay31015_cs - The entire control register is
    updated at once.
-------------------------------------------------*/
void ay31015_cs( UINT8 data )
{
	UINT8 t1 = ay.control_reg;
	ay.control_reg = data;
	if (ay.control_reg != t1) ay31015_xr();
}


/*************************************************** RECEIVE CONTROLS *************************************************/


/*-------------------------------------------------
    ay31015_rx_process - convert serial to parallel
-------------------------------------------------*/
static TIMER_CALLBACK( ay31015_rx_process )
{
	switch (ay.rx_state)
	{
		case PREP_TIME:							// assist sync by ensuring high bit occurs
			ay.rx_pulses--;
			if (ay.si)
				ay.rx_state = IDLE;
			return;

		case IDLE:
			ay.rx_pulses--;
			if (!ay.si)
			{
				ay.rx_state = START_BIT;
				ay.rx_pulses = 16;
			}
			return;

		case START_BIT:
			ay.rx_pulses--;
			if ((ay.rx_pulses == 8) && (ay.si))			// start bit must be low at sample time
				ay.rx_state = IDLE;
			else
			if (!ay.rx_pulses)					// end of start bit
			{
				ay.rx_state = PROCESSING;
				ay.rx_pulses = ay.total_pulses;
				ay.rx_bit_count = 0;
				ay.rx_parity = 0;
				ay.rx_data = 0;
			}
			return;

		case PROCESSING:
			ay.rx_pulses--;
			if (!ay.rx_pulses)					// end of a byte
			{
				ay.rx_pulses = 16;
				if (ay.control_reg & AY31015_NP)		// see if we need to get a parity bit
					ay.rx_state = FIRST_STOP_BIT;
				else
					ay.rx_state = PARITY_BIT;
			}
			else
			if (!(ay.rx_pulses & 15))				// end of a bit
				ay.rx_bit_count++;
			else
			if ((ay.rx_pulses & 15) == 8)				// sample input stream
			{
				ay.internal_sample = ay.si;
				ay.rx_parity ^= ay.internal_sample;		// calculate cumulative parity
				ay.rx_data |= ay.internal_sample << ay.rx_bit_count;
			}
			return;

		case PARITY_BIT:
			ay.rx_pulses--;

			if (ay.rx_pulses == 8)					// sample input stream
				ay.rx_parity ^= ay.si;				// calculate cumulative parity
			else
			if (!ay.rx_pulses)					// end of a byte
			{
				ay.rx_pulses = 16;
				ay.rx_state = FIRST_STOP_BIT;

				if ((ay.status_reg & AY31015_EPS) && (ay.rx_parity))
					ay.rx_parity = 0;			// odd parity, ok
				else
				if ((!(ay.status_reg & AY31015_EPS)) && (!ay.rx_parity))
					ay.rx_parity = 0;			// even parity, ok
				else
					ay.rx_parity = 1;			// parity error
			}
			return;

		case FIRST_STOP_BIT:
			ay.rx_pulses--;
			if (ay.rx_pulses == 8)				// sample input stream
				ay.internal_sample = ay.si;
			else
			if (ay.rx_pulses == 7)				// set error flags
			{
				if (!ay.internal_sample)
				{
					ay.status_reg |= AY31015_FE;		// framing error - the stop bit not high
					ay.rx_state = PREP_TIME;		// lost sync - start over
			//		return;
				}
				else
					ay.status_reg &= ~AY31015_FE;

				if ((ay.rx_parity) && (!(ay.control_reg & AY31015_NP)))
					ay.status_reg |= AY31015_PE;		// parity error
				else
					ay.status_reg &= ~AY31015_PE;

				if (ay.status_reg & AY31015_DAV)
					ay.status_reg |= AY31015_OR;		// overrun error - previous byte still in buffer
				else
					ay.status_reg &= ~AY31015_OR;

				ay.rx_buffer = ay.rx_data;		// bring received byte out for computer to read
			}
			else
			if (ay.rx_pulses == 6)
				ay.status_reg |= AY31015_DAV;		// tell computer that new byte is ready
			else
			if (ay.rx_pulses == 4)

	/* According to the specs, this commented-out code is what happens.
		However, it is not particularly effective at keeping sync.
		Therefore, since the input is HIGH, we simply jump to PREP_TIME and
		wait for the next start bit to begin. */

//			if (!ay.rx_pulses)				// end of first stop bit
//			{
//				if (ay.second_stop_bit)
//				{
//					ay.rx_state = SECOND_STOP_BIT;
//					ay.rx_pulses = ay.second_stop_bit;
//				}
//				else
//					ay.rx_state = PREP_TIME;
//			}
//			return;
//			
//		case SECOND_STOP_BIT:
//			ay.rx_pulses--;
//			if (!ay.rx_pulses)
				ay.rx_state = PREP_TIME;
			return;
	}
}


/*-------------------------------------------------
    ay31015_set_rx_clock_speed - set receive clock
-------------------------------------------------*/
void ay31015_set_rx_clock_speed( UINT32 data )
{
//	if (data)
		timer_pulse(ATTOTIME_IN_HZ(data),NULL,0,ay31015_rx_process);
//	else
//		turn timer off;
}

/*-------------------------------------------------
    ay31015_swe - The entire status register is
    returned at once.
-------------------------------------------------*/
UINT8 ay31015_swe( void )
{
	return ay.status_reg & 0x3f;
}


/*-------------------------------------------------
    ay31015_rde - return a byte to the computer
-------------------------------------------------*/
UINT8 ay31015_rde( void )
{
	return ay.rx_buffer;
}


/*-------------------------------------------------
    ay31015_rdav - tell uart computer has retrieved
    the above byte.
-------------------------------------------------*/
void ay31015_rdav( void )
{
	ay.status_reg &= ~AY31015_DAV;		/* set dav low */
}


/*-------------------------------------------------
    ay31015_si - send a serial bit to the uart
-------------------------------------------------*/
void ay31015_si( UINT8 data )
{
	ay.si = (data) ? 1 : 0;
}


/*************************************************** TRANSMIT CONTROLS *************************************************/


/*-------------------------------------------------
    ay31015_tx_process - convert parallel to serial
-------------------------------------------------*/
static TIMER_CALLBACK( ay31015_tx_process )
{
	UINT8 t1;
	switch (ay.tx_state)
	{
		case IDLE:
			if (!(ay.status_reg & AY31015_TBMT))
			{
				ay.tx_state = PREP_TIME;		// When idle, see if a byte has been sent to us
				ay.tx_pulses = 1;
			}
			return;

		case PREP_TIME:						// This phase lets the transmitter regain sync after an idle period
			ay.tx_pulses--;
			if (!ay.tx_pulses)
			{
				ay.tx_state = START_BIT;
				ay.tx_pulses = 16;
			}
			return;

		case START_BIT:
			if (ay.tx_pulses == 16)				// beginning of start bit
			{
				ay.tx_data = ay.tx_buffer;			// load the shift register
				ay.status_reg |= AY31015_TBMT;			// tell computer that another byte can be sent to uart
				ay.status_reg &= ~AY31015_SO;			// start bit begins now (we are "spacing")
				ay.status_reg &= ~AY31015_EOC;			// we are no longer idle
				ay.tx_parity = 0;
			}

			ay.tx_pulses--;
			if (!ay.tx_pulses)					// end of start bit
			{
				ay.tx_state = PROCESSING;
				ay.tx_pulses = ay.total_pulses;
			}
			return;

		case PROCESSING:
			if (!(ay.tx_pulses & 15))				// beginning of a data bit
			{
				if (ay.tx_data & 1)
				{
					ay.status_reg |= AY31015_SO;
					ay.tx_parity++;				// calculate cumulative parity
				}
				else
					ay.status_reg &= ~AY31015_SO;

				ay.tx_data >>= 1;				// adjust the shift register
			}
				
			ay.tx_pulses--;
			if (!ay.tx_pulses)					// all data bits sent
			{
				ay.tx_pulses = 16;
				if (ay.control_reg & AY31015_NP)		// see if we need to make a parity bit
					ay.tx_state = FIRST_STOP_BIT;
				else
					ay.tx_state = PARITY_BIT;
			}

			return;

		case PARITY_BIT:
			if (ay.tx_pulses == 16)
			{
				t1 = (ay.control_reg & AY31015_EPS) ? 0 : 1;
				t1 ^= (ay.tx_parity & 1);
				if (t1)
					ay.status_reg |= AY31015_SO;		// extra bit to set the correct parity
				else
					ay.status_reg &= ~AY31015_SO;		// it was already correct
			}

			ay.tx_pulses--;
			if (!ay.tx_pulses)
			{
				ay.tx_state = FIRST_STOP_BIT;
				ay.tx_pulses = 16;
			}
			return;

		case FIRST_STOP_BIT:
			if (ay.tx_pulses == 16)
				ay.status_reg |= AY31015_SO;			// create a stop bit (marking and soon idle)
			ay.tx_pulses--;
			if (!ay.tx_pulses)
			{
				ay.status_reg |= AY31015_EOC;			// character is completely sent
				if (ay.second_stop_bit)
				{
					ay.tx_state = SECOND_STOP_BIT;
					ay.tx_pulses = ay.second_stop_bit;
				}
				else
				if (ay.status_reg & AY31015_TBMT)
					ay.tx_state = IDLE;			// if nothing to send, go idle
				else
				{
					ay.tx_pulses = 16;
					ay.tx_state = START_BIT;		// otherwise immediately start next byte
				}
			}
			return;

		case SECOND_STOP_BIT:
			ay.tx_pulses--;
			if (!ay.tx_pulses)
			{
				if (ay.status_reg & AY31015_TBMT)
					ay.tx_state = IDLE;			// if nothing to send, go idle
				else
				{
					ay.tx_pulses = 16;
					ay.tx_state = START_BIT;		// otherwise immediately start next byte
				}
			}
			return;

	}
}


/*-------------------------------------------------
    ay31015_set_tx_clock_speed - set transmit clock
-------------------------------------------------*/
void ay31015_set_tx_clock_speed( UINT32 data )
{
//	if (data)
		timer_pulse(ATTOTIME_IN_HZ(data),NULL,0,ay31015_tx_process);
//	else
//		turn timer off;
}


/*-------------------------------------------------
    ay31015_so - return a serial bit to calling
    driver.
-------------------------------------------------*/
UINT8 ay31015_so( void )
{
	return (ay.status_reg & AY31015_SO) ? 1 : 0;
}


/*-------------------------------------------------
    ay31015_ds - accept a byte to transmit, if able
-------------------------------------------------*/
void ay31015_ds( UINT8 data )
{
	if (ay.status_reg & AY31015_TBMT)
	{
		ay.tx_buffer = data;
		ay.status_reg &= ~AY31015_TBMT;
	}
}



#if 0

void 
	ay31015_token *ay = get_token(device);
	const ay31015_config *config = get_config(device);


/*-------------------------------------------------
    DEVICE_GET_INFO(ay31015) - device getinfo
	function
-------------------------------------------------*/

DEVICE_GET_INFO(ay31015)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(ay31015_token); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0; break;
		case DEVINFO_INT_CLASS:				info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:			info->i = IO_PRINTER; break;
		case DEVINFO_INT_IMAGE_READABLE:		info->i = 0; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:		info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:		info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:				info->start = DEVICE_START_NAME(ay31015); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:				info->s = "ay31015"; break;
		case DEVINFO_STR_FAMILY:			info->s = "ay31015"; break;
		case DEVINFO_STR_SOURCE_FILE:			info->s = __FILE__; break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:		info->s = "prn"; break;
	}
}
#endif

