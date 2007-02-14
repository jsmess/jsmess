/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "8080bw.h"


static int shift_data1,shift_data2,shift_amount;


WRITE8_HANDLER( c8080bw_shift_amount_w )
{
	shift_amount = data;
}

WRITE8_HANDLER( c8080bw_shift_data_w )
{
	shift_data2 = shift_data1;
	shift_data1 = data;
}


#define SHIFT  (((((shift_data1 << 8) | shift_data2) << (shift_amount & 0x07)) >> 8) & 0xff)


READ8_HANDLER( c8080bw_shift_data_r )
{
	return SHIFT;
}

INTERRUPT_GEN( c8080bw_interrupt )
{
	int vector = cpu_getvblank() ? 0xcf : 0xd7;  /* RST 08h/10h */

	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);
}
