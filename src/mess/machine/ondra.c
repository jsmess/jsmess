/***************************************************************************

        Ondra driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/cassette.h"
#include "includes/ondra.h"
#include "devices/messram.h"

/* Driver initialization */
DRIVER_INIT(ondra)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,64*1024);
}

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}

static UINT8 ondra_bank1_status;
static UINT8 ondra_bank2_status;

static READ8_HANDLER( ondra_keyboard_r )
{
	UINT8 retVal = 0x00;
	UINT8 ondra_keyboard_line = offset & 0x000f;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	double valcas = cassette_input(cassette_device_image(space->machine));
	if ( valcas < 0.00) {
		retVal = 0x80;
	}
	if (ondra_keyboard_line > 9) {
		retVal |= 0x1f;
	} else {
		retVal |= input_port_read(space->machine, keynames[ondra_keyboard_line]);
	}
	return retVal;
 }

static void ondra_update_banks(running_machine *machine)
{
	UINT8 *mem = memory_region(machine, "maincpu");
	if (ondra_bank1_status==0) {
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
		memory_set_bankptr(machine, 1, mem + 0x010000);
	} else {
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
		memory_set_bankptr(machine, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
	}
	memory_set_bankptr(machine, 2, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
	if (ondra_bank2_status==0) {
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_BANK(3));
		memory_install_read8_handler (cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_BANK(3));
		memory_set_bankptr(machine, 3, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xe000);
	} else {
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_UNMAP);
		memory_install_read8_handler (cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, ondra_keyboard_r);
	}
}

WRITE8_HANDLER( ondra_port_03_w )
{
	ondra_video_enable = data & 1;
	ondra_bank1_status = (data >> 1) & 1;
	ondra_bank2_status = (data >> 2) & 1;
	ondra_update_banks(space->machine);
	cassette_output(cassette_device_image(space->machine), ((data >> 3) & 1) ? -1.0 : +1.0);
}

WRITE8_HANDLER( ondra_port_09_w )
{
}

WRITE8_HANDLER( ondra_port_0a_w )
{
}

static TIMER_CALLBACK(nmi_check_callback)
{
	if ((input_port_read(machine, "NMI") & 1) == 1)
	{
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
	}
}

MACHINE_RESET( ondra )
{
	ondra_bank1_status = 0;
	ondra_bank2_status = 0;
	ondra_update_banks(machine);

	timer_pulse(machine, ATTOTIME_IN_HZ(10), NULL, 0, nmi_check_callback);
}

MACHINE_START(ondra)
{
}
