/***************************************************************************

    Netronics Elf-II

****************************************************************************/

/*

	TODO:

	- TTL74C923
	- keyboard
	- Q led
	- DMAIN
	- memory protect
	- 7 segment displays

*/

#include "driver.h"
#include "includes/elf.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1861.h"
#include "video/dm9368.h"
#include "devices/cassette.h"
#include "elf2.lh"

/* Read/Write Handlers */

static WRITE8_HANDLER( digit_w )
{
//	elf_state *state = space->machine->driver_data;

	output_set_digit_value(0, 0xff);
	output_set_digit_value(1, 0xff);
}

/* Memory Maps */

static ADDRESS_MAP_START( elf2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x00ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( elf2_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861_TAG, vip_cdp1861_dispon_r, vip_cdp1861_dispoff_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(digit_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( input_w )
{
	if (newval)
	{
		cputag_set_input_line(field->port->machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAIN, ASSERT_LINE);
	}
}

static INPUT_PORTS_START( elf2 )
	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RUN") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LOAD") PORT_CODE(KEYCODE_L) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("M/P") PORT_CODE(KEYCODE_M) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("INPUT") PORT_CODE(KEYCODE_R) PORT_CHANGED(input_w, 0)
INPUT_PORTS_END

/* CDP1802 Configuration */

static CDP1802_MODE_READ( elf2_mode_r )
{
//	elf_state *state = device->machine->driver_data;

	UINT8 special = input_port_read(device->machine, "SPECIAL");
	int run = BIT(special, 0);
	int load = !BIT(special, 1);
	cdp1802_control_mode mode = CDP1802_MODE_RESET;

	switch ((run << 1) | load)
	{
	case 0: mode = CDP1802_MODE_LOAD; break;
	case 1: mode = CDP1802_MODE_RESET; break;
	case 2: mode = CDP1802_MODE_PAUSE; break;
	case 3: mode = CDP1802_MODE_RUN; break;
	}

	return mode;
}

static CDP1802_EF_READ( elf2_ef_r )
{
	/*
        EF1     CDP1861
        EF2     
        EF3     
        EF4     
    */

	elf_state *state = device->machine->driver_data;

	UINT8 flags = 0x0f;

	/* CDP1861 */
	if (state->cdp1861_efx) flags -= EF1;

	return flags;
}

static CDP1802_SC_WRITE( elf2_sc_w )
{
//	elf_state *driver_state = device->machine->driver_data;

	if (state == CDP1802_STATE_CODE_S2_DMA)
	{
		// DMA acknowledge clears the DMAOUT request
		cputag_set_input_line(device->machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAIN, CLEAR_LINE);
	}
}

static WRITE_LINE_DEVICE_HANDLER( elf2_q_w )
{
	output_set_led_value(0, state);
}

static READ8_DEVICE_HANDLER( elf2_dma_r )
{
//	elf_state *state = device->machine->driver_data;

	return 0;
}

static WRITE8_DEVICE_HANDLER( elf2_dma_w )
{
	elf_state *state = device->machine->driver_data;

	cdp1861_dma_w(state->cdp1861, data);
}

static CDP1802_INTERFACE( elf2_config )
{
	elf2_mode_r,
	elf2_ef_r,
	elf2_sc_w,
	DEVCB_LINE(elf2_q_w),
	DEVCB_HANDLER(elf2_dma_r),
	DEVCB_HANDLER(elf2_dma_w)
};

/* CDP1861 Interface */

static VIDEO_UPDATE( elf2 )
{
	elf_state *state = screen->machine->driver_data;

	cdp1861_update(state->cdp1861, bitmap, cliprect);

	return 0;
}

static WRITE_LINE_DEVICE_HANDLER( elf2_efx_w )
{
	elf_state *driver_state = device->machine->driver_data;

	driver_state->cdp1861_efx = state;
}

static CDP1861_INTERFACE( elf2_cdp1861_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(elf2_efx_w)
};

/* Machine Initialization */

static MACHINE_START( elf2 )
{
	elf_state *state = machine->driver_data;

	/* find devices */
	state->cdp1861 = devtag_get_device(machine, CDP1861_TAG);
	state->_74c923 = devtag_get_device(machine, _74C923_TAG);
	state->dm9368_l = devtag_get_device(machine, DM9368_L_TAG);
	state->dm9368_h = devtag_get_device(machine, DM9368_H_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static const cassette_config elf_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( elf2 )
	MDRV_DRIVER_DATA(elf_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_3_579545MHz/2)
    MDRV_CPU_PROGRAM_MAP(elf2_mem)
    MDRV_CPU_IO_MAP(elf2_io)
	MDRV_CPU_CONFIG(elf2_config)

    MDRV_MACHINE_START(elf2)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_elf2 )

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_3_579545MHz/2, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(elf2)

	MDRV_DM9368_ADD(DM9368_H_TAG, 0, NULL)
	MDRV_DM9368_ADD(DM9368_L_TAG, 1, NULL)
	MDRV_CDP1861_ADD(CDP1861_TAG, XTAL_3_579545MHz/2, elf2_cdp1861_intf)

	/* devices */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, elf_cassette_config)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( elf2 )
	ROM_REGION( 0x10000, CDP1802_TAG, ROMREGION_ERASE00 )
ROM_END

/* System Configuration */

static SYSTEM_CONFIG_START( elf2 )
	CONFIG_RAM_DEFAULT( 1 * 1024 )
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY			FULLNAME	FLAGS */
COMP( 1978, elf2,	0,		0,		elf2,	elf2,	0,		elf2,	"Netronics",	"Elf-II",	GAME_NOT_WORKING )
