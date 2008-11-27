/***************************************************************************

  cgenie.c

***************************************************************************/

#include "driver.h"

#include "includes/cgenie.h"
#include "sound/ay8910.h"

static int control_port;

READ8_HANDLER( cgenie_sh_control_port_r )
{
	return control_port;
}

READ8_HANDLER( cgenie_sh_data_port_r )
{
	return ay8910_read_port_0_r(space, offset);
}

WRITE8_HANDLER( cgenie_sh_control_port_w )
{
	control_port = data;
	ay8910_control_port_0_w(space, offset, data);
}

WRITE8_HANDLER( cgenie_sh_data_port_w )
{
	ay8910_write_port_0_w(space, offset, data);
}

