/***************************************************************************

  RIOT 6532 emulation

***************************************************************************/

#include "driver.h"


struct R6532interface
{
	UINT32	base_clock;
	UINT32	reset_delay_cycles;
	read8_handler portA_r;
	read8_handler portB_r;

	write8_handler portA_w;
	write8_handler portB_w;

	void (*irq_func)(int state);
};


void r6532_init(int n, const struct R6532interface* RI);

READ8_HANDLER( r6532_0_r );
READ8_HANDLER( r6532_1_r );

WRITE8_HANDLER( r6532_0_w );
WRITE8_HANDLER( r6532_1_w );

WRITE8_HANDLER( r6532_0_PA7_w );	/* for edge detect irq generation; the function checks bit 7 of data */
WRITE8_HANDLER( r6532_1_PA7_w );
