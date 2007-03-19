#include "driver.h"
#include "includes/intv.h"
#include "sound/ay8910.h"

/* The Intellivision has a AY-3-8914, which is the same
 * as the AY-3-8910, _except_ the registers are remapped as follows!
 *
 * also, the CP1610 can directly address the registers,
 * instead of needing a control and data port
 */

static const int mapping8914to8910[16] = { 0, 2, 4, 11, 1, 3, 5, 12, 7, 6, 13, 8, 9, 10, 14, 15 };

READ16_HANDLER( AY8914_directread_port_0_lsb_r )
{
	int rv;
	AY8910_control_port_0_w(0,mapping8914to8910[offset&0xff]);
	rv = (int)AY8910_read_port_0_r(0);
	return rv;
}

WRITE16_HANDLER( AY8914_directwrite_port_0_lsb_w )
{
	AY8910_control_port_0_w(0,mapping8914to8910[offset&0xff]);
	AY8910_write_port_0_w(0,data&0xff);
}
