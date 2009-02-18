/***************************************************************************

  cgenie.c

***************************************************************************/

#include "driver.h"

#include "includes/cgenie.h"
#include "sound/ay8910.h"

static UINT8 control_port;

READ8_DEVICE_HANDLER( cgenie_sh_control_port_r )
{
	return control_port;
}

WRITE8_DEVICE_HANDLER( cgenie_sh_control_port_w )
{
	control_port = data;
	ay8910_address_w(device, offset, data);
}
