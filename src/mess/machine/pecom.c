/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"
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
	state->reset_timer = timer_alloc(reset_tick, NULL);
}

MACHINE_RESET( pecom )
{
	UINT8 *rom = memory_region(machine, "main");
	pecom_state *state = machine->driver_data;
	
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, SMH_UNMAP);
	memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, SMH_BANK3);
	memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, SMH_BANK4);
	memory_set_bankptr(1, rom + 0x8000);	
	memory_set_bankptr(2, mess_ram + 0x4000);
	memory_set_bankptr(3, rom + 0xf400);
	memory_set_bankptr(4, rom + 0xf800);
	
	pecom_caps_state = 4;
	pecom_prev_caps_state = 4;

	state->cdp1802_mode = CDP1802_MODE_RESET;
	state->dma = 0;
	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(5), 0);
}

WRITE8_HANDLER( pecom_bank_w )
{
	const device_config *cdp1869 = device_list_find_by_tag(machine->config->devicelist, CDP1869_VIDEO, CDP1869_TAG);
	UINT8 *rom = memory_region(machine, "main");
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, mess_ram + 0x0000);
		
	if (data==2) {
		memory_install_read8_device_handler (cdp1869, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, cdp1869_charram_r);
		memory_install_write8_device_handler(cdp1869, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, cdp1869_charram_w);
		memory_install_read8_device_handler (cdp1869, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, cdp1869_pageram_r);
		memory_install_write8_device_handler(cdp1869, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, cdp1869_pageram_w);	
	} else {
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, SMH_UNMAP);
		memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf7ff, 0, 0, SMH_BANK3);
		memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, SMH_BANK4);
		memory_set_bankptr(3, rom + 0xf400);
		memory_set_bankptr(4, rom + 0xf800);
	}
}

READ8_HANDLER (pecom_keyboard_r)
{
	static const char *keynames[] = { 	"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
										"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25" };
	/* 
	   INP command BUS -> M(R(X)) BUS -> D 
	   so on each input, address is also set, 8 lower bits are used as input for keyboard 
	   Address is available on address bus during reading of value from port, and that is 
	   used to determine keyboard line reading
	*/
	UINT16 addr = cpunum_get_reg(0,CDP1802_R0 + cpunum_get_reg(0,CDP1802_X));
	/* just in case somone is reading non existing ports */
	if (addr<0x7cca || addr>0x7ce3) return 0; 
	return input_port_read(machine, keynames[addr - 0x7cca]) & 0x03;
}

/* CDP1802 Interface */
static CDP1802_MODE_READ( pecom64_mode_r )
{
	pecom_state *state = machine->driver_data;

	return state->cdp1802_mode;	
}

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, CASSETTE, "cassette");
}

static CDP1802_EF_READ( pecom64_ef_r )
{
	int flags = 0x0f;
	double valcas = cassette_input(cassette_device_image(machine));
	UINT8 val = input_port_read(machine, "CNT");
	
	if ((val & 0x04)==0x04 && pecom_prev_caps_state==0) {
		pecom_caps_state = (pecom_caps_state==4) ? 0 : 4; // Change CAPS state
	}
	pecom_prev_caps_state = val & 0x04;
	if (valcas!=0.0) { // If there is any cassette signal
		val = (val & 0x0D); // remove EF2 from SHIFT
		flags -= EF2;
		if ( valcas > 0.00) flags += EF2;
	}	
	flags -= (val & 0x0b) + pecom_caps_state;
	return flags;
}

static CDP1802_Q_WRITE( pecom64_q_w )
{	
	cassette_output(cassette_device_image(machine), level ? -1.0 : +1.0);
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
		cpunum_set_input_line(machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
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
	pecom64_q_w,
	NULL,
	NULL
};
