/***************************************************************************

        Z1013 driver by Miodrag Milanovic

        22/04/2009 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "includes/z1013.h"
#include "cpu/z80/z80.h"

static UINT8 z1013_keyboard_line;
static UINT8 z1013_keyboard_part;

/* Wires on Z80 PIO are switched */
READ8_DEVICE_HANDLER(z1013_z80pio_r)
{
	if ((offset & 1) ==0) return z80pio_d_r(device, offset >> 1); else return z80pio_c_r(device, offset >> 1);
}

WRITE8_DEVICE_HANDLER(z1013_z80pio_w)
{
	if ((offset & 1) ==0)
		z80pio_d_w(device, offset >> 1, data);
	else
		z80pio_c_w(device, offset >> 1, data);
}

/* Driver initialization */
DRIVER_INIT(z1013)
{
}

MACHINE_RESET( z1013 )
{
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC, 0xF000);
	z1013_keyboard_part = 0;
	z1013_keyboard_line = 0;
}

WRITE8_HANDLER(z1013_keyboard_w) {
	z1013_keyboard_line	= data;
}

static READ8_DEVICE_HANDLER (z1013_port_b_r)
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3",
		"LINE4", "LINE5", "LINE6", "LINE7"
	};
	UINT8 data = input_port_read(device->machine, keynames[z1013_keyboard_line & 7]);
	if (z1013_keyboard_part==0x10) {
		return (data >> 4) & 0x0f;
	} else {
		return data & 0x0f;
	}
}

static WRITE8_DEVICE_HANDLER (z1013_port_b_w)
{
	z1013_keyboard_part = data;
}
const z80pio_interface z1013_z80pio_intf =
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_HANDLER(z1013_port_b_r),
	DEVCB_NULL,
	DEVCB_HANDLER(z1013_port_b_w),
	DEVCB_NULL,
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
	DEVCB_HANDLER(z1013k7659_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

SNAPSHOT_LOAD( z1013 )
{
	UINT8* data= auto_alloc_array(image->machine, UINT8, snapshot_size);
	UINT16 startaddr,endaddr,runaddr;

	image_fread(image, data, snapshot_size);

	startaddr = data[0] + data[1]*256;
	endaddr   = data[2] + data[3]*256;
	runaddr   = data[4] + data[5]*256;
	if (data[12]!='C') return INIT_FAIL;
	if (data[13]!=0xD3 || data[14]!=0xD3 || data[14]!=0xD3) return INIT_FAIL;

	memcpy (memory_get_read_ptr(cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM),  startaddr ),
		 data+0x20, endaddr - startaddr + 1);

	cpu_set_reg(cputag_get_cpu(image->machine, "maincpu"), Z80_PC, runaddr);

	return INIT_PASS;
}

