/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1869.h"
#include "devices/cassette.h"
#include "includes/pecom.h"
 
static UINT8 pecom_caps_state = 4;
static UINT8 pecom_prev_caps_state = 4;

/* Driver initialization */
DRIVER_INIT(pecom)
{
	memset(mess_ram,0,32*1024);
}

static TIMER_CALLBACK( reset_tick )
{
	pecom_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RUN;
}

MACHINE_START( pecom )
{
	pecom_state *state = machine->driver_data;
	state->reset_timer = timer_alloc(machine, reset_tick, NULL);
}

MACHINE_RESET( pecom )
{
	UINT8 *rom = memory_region(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	pecom_state *state = machine->driver_data;
	
	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(space, 0xf000, 0xf7ff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0xf800, 0xffff, 0, 0, SMH_UNMAP);
	memory_install_read8_handler (space, 0xf000, 0xf7ff, 0, 0, SMH_BANK(3));
	memory_install_read8_handler (space, 0xf800, 0xffff, 0, 0, SMH_BANK(4));
	memory_set_bankptr(machine, 1, rom + 0x8000);	
	memory_set_bankptr(machine, 2, mess_ram + 0x4000);
	memory_set_bankptr(machine, 3, rom + 0xf000);
	memory_set_bankptr(machine, 4, rom + 0xf800);
	
	pecom_caps_state = 4;
	pecom_prev_caps_state = 4;

	state->cdp1802_mode = CDP1802_MODE_RESET;
	state->dma = 0;
	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(5), 0);
}

WRITE8_HANDLER( pecom_bank_w )
{
	const device_config *cdp1869 = devtag_get_device(space->machine, CDP1869_TAG);
	const address_space *space2 = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *rom = memory_region(space->machine, "maincpu");
	memory_install_write8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
	memory_set_bankptr(space->machine, 1, mess_ram + 0x0000);
		
	if (data==2) 
	{
		memory_install_read8_device_handler (space2, cdp1869, 0xf000, 0xf7ff, 0, 0, cdp1869_charram_r);
		memory_install_write8_device_handler(space2, cdp1869, 0xf000, 0xf7ff, 0, 0, cdp1869_charram_w);
		memory_install_read8_device_handler (space2, cdp1869, 0xf800, 0xffff, 0, 0, cdp1869_pageram_r);
		memory_install_write8_device_handler(space2, cdp1869, 0xf800, 0xffff, 0, 0, cdp1869_pageram_w);	
	} 
	else 
	{
		memory_install_write8_handler(space2, 0xf000, 0xf7ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(space2, 0xf800, 0xffff, 0, 0, SMH_UNMAP);
		memory_install_read8_handler (space2, 0xf000, 0xf7ff, 0, 0, SMH_BANK(3));
		memory_install_read8_handler (space2, 0xf800, 0xffff, 0, 0, SMH_BANK(4));
		memory_set_bankptr(space->machine, 3, rom + 0xf000);
		memory_set_bankptr(space->machine, 4, rom + 0xf800);
	}
}

READ8_HANDLER (pecom_keyboard_r)
{
	static const char *const keynames[] = { 	"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
										"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25" };
	/* 
	   INP command BUS -> M(R(X)) BUS -> D 
	   so on each input, address is also set, 8 lower bits are used as input for keyboard 
	   Address is available on address bus during reading of value from port, and that is 
	   used to determine keyboard line reading
	*/
	UINT16 addr = cpu_get_reg(cputag_get_cpu(space->machine, "maincpu"), CDP1802_R0 + cpu_get_reg(cputag_get_cpu(space->machine, "maincpu"), CDP1802_X));
	/* just in case somone is reading non existing ports */
	if (addr<0x7cca || addr>0x7ce3) return 0; 
	return input_port_read(space->machine, keynames[addr - 0x7cca]) & 0x03;
}

/* CDP1802 Interface */
static CDP1802_MODE_READ( pecom64_mode_r )
{
	pecom_state *state = device->machine->driver_data;

	return state->cdp1802_mode;	
}

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}

static CDP1802_EF_READ( pecom64_ef_r )
{
	int flags = 0x0f;
	double valcas = cassette_input(cassette_device_image(device->machine));
	UINT8 val = input_port_read(device->machine, "CNT");
	
	if ((val & 0x04)==0x04 && pecom_prev_caps_state==0) 
	{
		pecom_caps_state = (pecom_caps_state==4) ? 0 : 4; // Change CAPS state
	}
	pecom_prev_caps_state = val & 0x04;
	if (valcas!=0.0) 
	{ // If there is any cassette signal
		val = (val & 0x0D); // remove EF2 from SHIFT
		flags -= EF2;
		if ( valcas > 0.00) flags += EF2;
	}	
	flags -= (val & 0x0b) + pecom_caps_state;
	return flags;
}

static WRITE_LINE_DEVICE_HANDLER( pecom64_q_w )
{	
	cassette_output(cassette_device_image(device->machine), state ? -1.0 : +1.0);
}

static CDP1802_SC_WRITE( pecom64_sc_w )
{
	switch (state)
	{
	case CDP1802_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case CDP1802_STATE_CODE_S1_EXECUTE:
		break;
		
	case CDP1802_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		cputag_set_input_line(device->machine, "maincpu", CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;
	case CDP1802_STATE_CODE_S3_INTERRUPT:
		break;
	}
}

CDP1802_INTERFACE( pecom64_cdp1802_config )
{
	pecom64_mode_r,
	pecom64_ef_r,
	pecom64_sc_w,
	DEVCB_LINE(pecom64_q_w),
	DEVCB_NULL,
	DEVCB_NULL
};
