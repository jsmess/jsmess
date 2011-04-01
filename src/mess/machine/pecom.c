/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1869.h"
#include "imagedev/cassette.h"
#include "includes/pecom.h"
#include "machine/ram.h"

static TIMER_CALLBACK( reset_tick )
{
	pecom_state *state = machine.driver_data<pecom_state>();

	state->m_reset = 1;
}

MACHINE_START( pecom )
{
	pecom_state *state = machine.driver_data<pecom_state>();
	state->m_reset_timer = machine.scheduler().timer_alloc(FUNC(reset_tick));
}

MACHINE_RESET( pecom )
{
	UINT8 *rom = machine.region(CDP1802_TAG)->base();
	address_space *space = machine.device(CDP1802_TAG)->memory().space(AS_PROGRAM);

	pecom_state *state = machine.driver_data<pecom_state>();

	space->unmap_write(0x0000, 0x3fff);
	space->install_write_bank(0x4000, 0x7fff, "bank2");
	space->unmap_write(0xf000, 0xf7ff);
	space->unmap_write(0xf800, 0xffff);
	space->install_read_bank (0xf000, 0xf7ff, "bank3");
	space->install_read_bank (0xf800, 0xffff, "bank4");
	memory_set_bankptr(machine, "bank1", rom + 0x8000);
	memory_set_bankptr(machine, "bank2", ram_get_ptr(machine.device(RAM_TAG)) + 0x4000);
	memory_set_bankptr(machine, "bank3", rom + 0xf000);
	memory_set_bankptr(machine, "bank4", rom + 0xf800);

	state->m_reset = 0;
	state->m_dma = 0;
	state->m_reset_timer->adjust(attotime::from_msec(5));
}

static READ8_HANDLER( pecom_cdp1869_charram_r )
{
	pecom_state *state = space->machine().driver_data<pecom_state>();
	return state->m_cdp1869->char_ram_r(*space, offset);
}

static WRITE8_HANDLER( pecom_cdp1869_charram_w )
{
	pecom_state *state = space->machine().driver_data<pecom_state>();
	return state->m_cdp1869->char_ram_w(*space, offset, data);
}

static READ8_HANDLER( pecom_cdp1869_pageram_r )
{
	pecom_state *state = space->machine().driver_data<pecom_state>();
	return state->m_cdp1869->page_ram_r(*space, offset);
}

static WRITE8_HANDLER( pecom_cdp1869_pageram_w )
{
	pecom_state *state = space->machine().driver_data<pecom_state>();
	return state->m_cdp1869->page_ram_w(*space, offset, data);
}

WRITE8_HANDLER( pecom_bank_w )
{
//  pecom_state *state = space->machine().driver_data<pecom_state>();
	address_space *space2 = space->machine().device(CDP1802_TAG)->memory().space(AS_PROGRAM);
	UINT8 *rom = space->machine().region(CDP1802_TAG)->base();
	space->machine().device(CDP1802_TAG)->memory().space(AS_PROGRAM)->install_write_bank(0x0000, 0x3fff, "bank1");
	memory_set_bankptr(space->machine(), "bank1", ram_get_ptr(space->machine().device(RAM_TAG)) + 0x0000);

	if (data==2)
	{
		space2->install_legacy_read_handler (0xf000, 0xf7ff, FUNC(pecom_cdp1869_charram_r));
		space2->install_legacy_write_handler(0xf000, 0xf7ff, FUNC(pecom_cdp1869_charram_w));
		space2->install_legacy_read_handler (0xf800, 0xffff, FUNC(pecom_cdp1869_pageram_r));
		space2->install_legacy_write_handler(0xf800, 0xffff, FUNC(pecom_cdp1869_pageram_w));
	}
	else
	{
		space2->unmap_write(0xf000, 0xf7ff);
		space2->unmap_write(0xf800, 0xffff);
		space2->install_read_bank (0xf000, 0xf7ff, "bank3");
		space2->install_read_bank (0xf800, 0xffff, "bank4");
		memory_set_bankptr(space->machine(), "bank3", rom + 0xf000);
		memory_set_bankptr(space->machine(), "bank4", rom + 0xf800);
	}
}

READ8_HANDLER (pecom_keyboard_r)
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
		"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25"
	};
	/*
       INP command BUS -> M(R(X)) BUS -> D
       so on each input, address is also set, 8 lower bits are used as input for keyboard
       Address is available on address bus during reading of value from port, and that is
       used to determine keyboard line reading
    */
	UINT16 addr = cpu_get_reg(space->machine().device(CDP1802_TAG), COSMAC_R0 + cpu_get_reg(space->machine().device(CDP1802_TAG), COSMAC_X));
	/* just in case somone is reading non existing ports */
	if (addr<0x7cca || addr>0x7ce3) return 0;
	return input_port_read(space->machine(), keynames[addr - 0x7cca]) & 0x03;
}

/* CDP1802 Interface */

static READ_LINE_DEVICE_HANDLER( clear_r )
{
	pecom_state *state = device->machine().driver_data<pecom_state>();

	return state->m_reset;
}

static READ_LINE_DEVICE_HANDLER( ef2_r )
{
	int shift = BIT(input_port_read(device->machine(), "CNT"), 1);
	double cas = cassette_input(device);

	return (cas > 0.0) | shift;
}

/*
static COSMAC_EF_READ( pecom64_ef_r )
{
    int flags = 0x0f;
    double valcas = cassette_input(cassette_device_image(device->machine()));
    UINT8 val = input_port_read(device->machine(), "CNT");

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
*/
static WRITE_LINE_DEVICE_HANDLER( pecom64_q_w )
{
	cassette_output(device, state ? -1.0 : +1.0);
}

static COSMAC_SC_WRITE( pecom64_sc_w )
{
	switch (sc)
	{
	case COSMAC_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case COSMAC_STATE_CODE_S1_EXECUTE:
		break;

	case COSMAC_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		cputag_set_input_line(device->machine(), CDP1802_TAG, COSMAC_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;
	case COSMAC_STATE_CODE_S3_INTERRUPT:
		break;
	}
}

COSMAC_INTERFACE( pecom64_cdp1802_config )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("cassette", ef2_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("cassette", pecom64_q_w),
	DEVCB_NULL,
	DEVCB_NULL,
	pecom64_sc_w,
	DEVCB_NULL,
	DEVCB_NULL
};
