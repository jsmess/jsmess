/***************************************************************************

    Atari Clay Shoot hardware

    driver by Zsolt Vasvari

****************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "includes/clayshoo.h"


static UINT8 input_port_select;
static UINT8 analog_port_val;


/*************************************
 *
 *  Digital control handling functions
 *
 *************************************/

static WRITE8_HANDLER( input_port_select_w )
{
	input_port_select = data;
}


static UINT8 difficulty_input_port_r(int bit)
{
	UINT8 ret = 0;

	/* read fake port and remap the buttons to 2 bits */
	UINT8	raw = readinputportbytag("FAKE");

	if (raw & (1 << (bit + 1)))
		ret = 0x03;		/* expert */
	else if (raw & (1 << (bit + 2)))
		ret = 0x01;		/* pro */
	else
		ret = 0x00;		/* amateur otherwise */

	return ret;
}


static READ8_HANDLER( input_port_r )
{
	UINT8 ret = 0;


	switch (input_port_select)
	{
	case 0x01:	ret = readinputportbytag("IN0"); break;
	case 0x02:	ret = readinputportbytag("IN1"); break;
	case 0x04:	ret = (readinputportbytag("IN2") & 0xf0) |
					   difficulty_input_port_r(0) |
					  (difficulty_input_port_r(3) << 2); break;
	case 0x08:	ret = readinputportbytag("IN3"); break;
	case 0x10:
	case 0x20:	break;	/* these two are not really used */
	default: logerror("Unexpected port read: %02X\n", input_port_select);
	}

	return ret;
}


static ppi8255_interface ppi8255_intf =
{
	2, 							/* 2 chips */
	{ 0, 0 },					/* Port A read */
	{ 0, input_port_r },		/* Port B read */
	{ 0, 0 },					/* Port C read */
	{ 0, input_port_select_w },	/* Port A write */
	{ 0, 0 },					/* Port B write */
	{ 0, 0 /* sound effects */},/* Port C write */
};


MACHINE_RESET( clayshoo )
{
	ppi8255_init(&ppi8255_intf);
}



/*************************************
 *
 *  Analog control handling functions
 *
 *************************************/

static void reset_analog_bit(int bit)
{
	analog_port_val &= ~bit;
}


static double compute_duration(int analog_pos)
{
	/* the 58 comes from the length of the loop used to
       read the analog position */
	return TIME_IN_CYCLES(58*analog_pos, 0);
}


WRITE8_HANDLER( clayshoo_analog_reset_w )
{
	/* reset the analog value, and start two times that will fire
       off in a short period proportional to the position of the
       analog control and set the appropriate bit. */

	analog_port_val = 0xff;

	timer_set(compute_duration(readinputportbytag("AN1")), 0x02, reset_analog_bit);
	timer_set(compute_duration(readinputportbytag("AN2")), 0x01, reset_analog_bit);
}


READ8_HANDLER( clayshoo_analog_r )
{
	return analog_port_val;
}
