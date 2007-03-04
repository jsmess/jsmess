/******************************************************************************
    uPD7002 Analogue to Digital Converter

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/


#include "ctype.h"
#include "driver.h"
#include "includes/upd7002.h"

// local copy of the uPD7002 external procedure calls
static struct uPD7002_interface
uPD7002_calls={
	0,
	0,
};


void uPD7002_config(const struct uPD7002_interface *intf)
{
	uPD7002_calls.get_analogue_func=*intf->get_analogue_func;
	uPD7002_calls.EOC_func=*intf->EOC_func;
}


/* Status Register
	D0 and D1 define the currently selected input channel
	D2 flag output
	D3 0 = 8 bit mode   1 = 12 bit mode
	D4 2nd MSB of conversion
	D5     MSB of conversion
	D6 0 = busy, 1 = not busy    (~busy)
	D7 0 = conversion completed, 1 = conversion not completed  (~EOC)
*/
static int uPD7002_status=0;

/* High data byte
	This byte contains the 8 most significant bits of the analogue to digital conversion. */
static int uPD7002_data1=0;

/* Low data byte
	In 12 bit mode: Bits 7 to 4 define the four low order bits of the conversion.
	In  8 bit mode. All bits 7 to 4 are inaccurate.
	Bits 3 to 0 are always set to low. */
static int uPD7002_data0=0;


/* temporary store of the next A to D conversion */
static int uPD7002_digitalvalue=0;


 READ8_HANDLER ( uPD7002_EOC_r )
{
	return (uPD7002_status>>7)&0x01;
}




/* this counter is used to check a full end of conversion has been reached
   if the uPD7002 is half way through one conversion and a new conversion is requested
   the counter at the end of the first conversion will not match and not be processed
   only then at the end of the second conversion will the conversion complete function run */
static int conversion_counter=0;

static void uPD7002_conversioncomplete(int counter_value)
{
	if (counter_value==conversion_counter)
	{
		// this really always does a 12 bit conversion
		uPD7002_data1=uPD7002_digitalvalue>>8;
		uPD7002_data0=uPD7002_digitalvalue&0xf0;

		// set the status register with top 2 MSB, not busy and conversion complete
		uPD7002_status=(uPD7002_status&0x0f)|((uPD7002_data1&0xc0)>>2)|0x40;

		// call the EOC function with EOC from status
		// uPD7002_EOC_r(0) this has just been set to 0
		if (uPD7002_calls.EOC_func) (uPD7002_calls.EOC_func)(0);
		conversion_counter=0;
	}
}




 READ8_HANDLER ( uPD7002_r )
{
	switch(offset&0x03)
	{
		case 0:
			return uPD7002_status;
			break;

		case 1:
			return uPD7002_data1;
			break;

		case 2: case 3:
			return uPD7002_data0;
			break;
	}
	return 0;
}



WRITE8_HANDLER ( uPD7002_w )
{

	/* logerror("write to uPD7002 $%02X = $%02X\n",offset,data); */

	switch(offset&0x03)
	{
		case 0:
		/*
		Data Latch/AD start
			D0 and D1 together define which one of the four input channels is selected
			D2 flag input, normally set to 0????
			D3 defines whether an 8 (0) or 12 (1) bit resolution conversion should occur
			D4 to D7 not used.

			an 8  bit conversion typically takes 4ms
			an 12 bit conversion typically takes 10ms

			writing to this register will initiate a conversion.
		*/

		/* set D6=0 busy ,D7=1 conversion not complete */
		uPD7002_status=(data & 0x0f) | 0x80;

		// call the EOC function with EOC from status
		// uPD7002_EOC_r(0) this has just been set to 1
		if (uPD7002_calls.EOC_func) (uPD7002_calls.EOC_func)(1);

		/* the uPD7002 works by sampling the analogue value at the start of the conversion
		   so it is read hear and stored until the end of the A to D conversion */

		// this function should return a 16 bit value.
		uPD7002_digitalvalue=(uPD7002_calls.get_analogue_func)(uPD7002_status&0x03);

		conversion_counter+=1;

		// call a timer to start the conversion
		if (uPD7002_status&0x08)
		{
			// 12 bit conversion takes 10ms
			timer_set(TIME_IN_MSEC(10), conversion_counter, uPD7002_conversioncomplete);
		} else {
			// 8 bit conversion takes 4ms
			timer_set(TIME_IN_MSEC(4),  conversion_counter, uPD7002_conversioncomplete);
		}
		break;

		case 1: case 2:
		/* Nothing */
		break;

		case 3:
		/* Test Mode: Used for inspecting the device, The data input-output terminals assume an input
			  state and are connected to the A/D counter. Therefore, the A/D conversion data
			  read out after this is meaningless.
		*/
		break;
	}
}





