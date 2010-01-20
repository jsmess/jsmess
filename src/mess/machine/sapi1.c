/***************************************************************************

        SAPI-1 driver by Miodrag Milanovic

        09/09/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "includes/sapi1.h"

static UINT8 sapi1_keyboard_mask;

/* Driver initialization */
DRIVER_INIT(sapi1)
{
}

MACHINE_RESET( sapi1 )
{
	sapi1_keyboard_mask = 0;
}

MACHINE_START(sapi1)
{
}

READ8_HANDLER (sapi1_keyboard_r )
{
	UINT8 key = 0xff;
	if ((sapi1_keyboard_mask & 0x01)!=0) { key &= input_port_read(space->machine,"LINE0"); }
	if ((sapi1_keyboard_mask & 0x02)!=0) { key &= input_port_read(space->machine,"LINE1"); }
	if ((sapi1_keyboard_mask & 0x04)!=0) { key &= input_port_read(space->machine,"LINE2"); }
	if ((sapi1_keyboard_mask & 0x08)!=0) { key &= input_port_read(space->machine,"LINE3"); }
	if ((sapi1_keyboard_mask & 0x10)!=0) { key &= input_port_read(space->machine,"LINE4"); }
	return key;
}
WRITE8_HANDLER(sapi1_keyboard_w )
{
	sapi1_keyboard_mask = (data ^ 0xff ) & 0x1f;
}

