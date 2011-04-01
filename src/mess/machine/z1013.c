/***************************************************************************

        Z1013 driver by Miodrag Milanovic

        22/04/2009 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "includes/z1013.h"
#include "cpu/z80/z80.h"


/* Driver initialization */
DRIVER_INIT(z1013)
{
}

MACHINE_RESET( z1013 )
{
	z1013_state *state = machine.driver_data<z1013_state>();
	cpu_set_reg(machine.device("maincpu"), Z80_PC, 0xF000);
	state->m_keyboard_part = 0;
	state->m_keyboard_line = 0;
}

WRITE8_HANDLER(z1013_keyboard_w) {
	z1013_state *state = space->machine().driver_data<z1013_state>();
	state->m_keyboard_line	= data;
}

static READ8_DEVICE_HANDLER (z1013_port_b_r)
{
	z1013_state *state = device->machine().driver_data<z1013_state>();
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3",
		"LINE4", "LINE5", "LINE6", "LINE7"
	};
	UINT8 data = input_port_read(device->machine(), keynames[state->m_keyboard_line & 7]);
	if (state->m_keyboard_part==0x10) {
		return (data >> 4) & 0x0f;
	} else {
		return data & 0x0f;
	}
}

static WRITE8_DEVICE_HANDLER (z1013_port_b_w)
{
	z1013_state *state = device->machine().driver_data<z1013_state>();
	state->m_keyboard_part = data;
}
const z80pio_interface z1013_z80pio_intf =
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(z1013_port_b_r),
	DEVCB_HANDLER(z1013_port_b_w),
	DEVCB_NULL
};

static READ8_DEVICE_HANDLER (z1013k7659_port_b_r)
{
	return 0xff;
}

const z80pio_interface z1013k7659_z80pio_intf =
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(z1013k7659_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL
};

SNAPSHOT_LOAD( z1013 )
{
	UINT8* data= auto_alloc_array(image.device().machine(), UINT8, snapshot_size);
	UINT16 startaddr,endaddr,runaddr;

	image.fread( data, snapshot_size);

	startaddr = data[0] + data[1]*256;
	endaddr   = data[2] + data[3]*256;
	runaddr   = data[4] + data[5]*256;
	if (data[12]!='C') return IMAGE_INIT_FAIL;
	if (data[13]!=0xD3 || data[14]!=0xD3 || data[14]!=0xD3) return IMAGE_INIT_FAIL;

	memcpy (image.device().machine().device("maincpu")->memory().space(AS_PROGRAM)->get_read_ptr(startaddr),
		 data+0x20, endaddr - startaddr + 1);

	cpu_set_reg(image.device().machine().device("maincpu"), Z80_PC, runaddr);

	return IMAGE_INIT_PASS;
}

