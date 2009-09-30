/******************************************************************************
 MC-10

  TODO
    RS232
    Printer
    Disk
 *****************************************************************************/



/*****************************************************************************
 Includes
*****************************************************************************/


#include "driver.h"
#include "video/m6847.h"
#include "includes/mc10.h"
#include "devices/cassette.h"
#include "sound/dac.h"



/*****************************************************************************
 Global variables
*****************************************************************************/


static UINT8 mc10_keyboard_strobe;



/*****************************************************************************
 Hardware initialization
*****************************************************************************/


MACHINE_START( mc10 )
{
	mc10_keyboard_strobe = 0x00;

	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x4000,	0x4000 + mess_ram_size - 1,	0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_set_bankptr(machine, 1, mess_ram);

	state_save_register_global(machine, mc10_keyboard_strobe);
}



/*****************************************************************************
 Memory mapped I/O
*****************************************************************************/


READ8_HANDLER ( mc10_bfff_r )
{
    /*   BIT 0 KEYBOARD ROW 1
     *   BIT 1 KEYBOARD ROW 2
     *   BIT 2 KEYBOARD ROW 3
     *   BIT 3 KEYBOARD ROW 4
     *   BIT 4 KEYBOARD ROW 5
     *   BIT 5 KEYBOARD ROW 6
     */

	int val = 0x40;

	if ((input_port_read(space->machine, "LINE0") | mc10_keyboard_strobe) == 0xff)
		val |= 0x01;
	if ((input_port_read(space->machine, "LINE1") | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;
	if ((input_port_read(space->machine, "LINE2") | mc10_keyboard_strobe) == 0xff)
		val |= 0x04;
	if ((input_port_read(space->machine, "LINE3") | mc10_keyboard_strobe) == 0xff)
		val |= 0x08;
	if ((input_port_read(space->machine, "LINE4") | mc10_keyboard_strobe) == 0xff)
		val |= 0x10;
	if ((input_port_read(space->machine, "LINE5") | mc10_keyboard_strobe) == 0xff)
		val |= 0x20;

	return val;
}


WRITE8_HANDLER ( mc10_bfff_w )
{
	const device_config *dac_device = devtag_get_device(space->machine, "dac");
	const device_config *mc6847 = devtag_get_device(space->machine, "mc6847");

	/*   BIT 2 GM2 6847 CONTROL & INT/EXT CONTROL
     *   BIT 3 GM1 6847 CONTROL
     *   BIT 4 GM0 6847 CONTROL
     *   BIT 5 A/G 684? CONTROL
     *   BIT 6 CSS 6847 CONTROL
     *   BIT 7 SOUND OUTPUT BIT
     */
	mc6847_gm2_w(mc6847, BIT(data, 2));
	mc6847_intext_w(mc6847, BIT(data, 2));
	mc6847_gm1_w(mc6847, BIT(data, 3));
	mc6847_gm0_w(mc6847, BIT(data, 4));
	mc6847_ag_w(mc6847, BIT(data, 5));
	mc6847_css_w(mc6847, BIT(data, 6));
	dac_data_w(dac_device, BIT(data, 7));
}



/*****************************************************************************
 MC6803 I/O ports
*****************************************************************************/


READ8_HANDLER ( mc10_port1_r )
{
	return mc10_keyboard_strobe;
}


WRITE8_HANDLER ( mc10_port1_w )
{
    /*   BIT 0  KEYBOARD COLUMN 1 STROBE
     *   BIT 1  KEYBOARD COLUMN 2 STROBE
     *   BIT 2  KEYBOARD COLUMN 3 STROBE
     *   BIT 3  KEYBOARD COLUMN 4 STROBE
     *   BIT 4  KEYBOARD COLUMN 5 STROBE
     *   BIT 5  KEYBOARD COLUMN 6 STROBE
     *   BIT 6  KEYBOARD COLUMN 7 STROBE
     *   BIT 7  KEYBOARD COLUMN 8 STROBE
     */
	mc10_keyboard_strobe = data;
}


READ8_HANDLER ( mc10_port2_r )
{
    /*   BIT 1 KEYBOARD SHIFT/CONTROL KEYS INPUT
     * ! BIT 2 PRINTER OTS INPUT
     * ! BIT 3 RS232 INPUT DATA
     *   BIT 4 CASSETTE TAPE INPUT
     */

	const device_config *img = devtag_get_device(space->machine, "cassette");
	int val = 0xed;

	if ((input_port_read(space->machine, "LINE6") | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;

	if (cassette_input(img) >= 0)
		val |= 0x10;

	return val;
}


WRITE8_HANDLER ( mc10_port2_w )
{
	const device_config *img = devtag_get_device(space->machine, "cassette");

	/*   BIT 0 PRINTER OUTFUT & CASS OUTPUT
     */

	cassette_output(img, (data & 0x01) ? +1.0 : -1.0);
}


/***************************************************************************
    MC6847
***************************************************************************/

READ8_DEVICE_HANDLER( mc10_mc6847_videoram_r )
{
	mc6847_inv_w(device, BIT(mess_ram[offset], 6));
	mc6847_as_w(device, BIT(mess_ram[offset], 7));

	return mess_ram[offset];
}

VIDEO_UPDATE( mc10 )
{
	const device_config *mc6847 = devtag_get_device(screen->machine, "mc6847");
	return mc6847_update(mc6847, bitmap, cliprect);
}
