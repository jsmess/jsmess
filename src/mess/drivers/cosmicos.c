/*

	COSMICOS

	http://retro.hansotten.nl/index.php?page=1802-cosmicos

*/

/*

	TODO:

	- type in ROM
	- switches
	- hex keypad
	- PPI 8255
	- Floppy WD1793
	- COM8017 UART to printer

*/

#include "driver.h"
#include "includes/cosmicos.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "video/dm9368.h"
#include "cosmicos.lh"
#include "devices/messram.h"
/* Memory Maps */

static ADDRESS_MAP_START( cosmicos_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicos_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x00, 0x00)
//	AM_RANGE(0x01, 0x01)
//	AM_RANGE(0x02, 0x02)
//	AM_RANGE(0x03, 0x03)
//	AM_RANGE(0x04, 0x04)
//	AM_RANGE(0x05, 0x05) AM_READWRITE(hex_keyboard_r, hex_keylatch_w)
//	AM_RANGE(0x06, 0x06) AM_READWRITE(reset_cathode_counter_r, segment_w)
//	AM_RANGE(0x07, 0x07) AM_READWRITE(data_r, display_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( cosmicos )
	PORT_START("DATA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SINGLE STEP")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RUN")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LOAD")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAUSE")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MEMORY PROTECT") PORT_TOGGLE
INPUT_PORTS_END

static INPUT_PORTS_START( cosmicos_hex )
	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RET") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEC") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("REQ") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SEQ") PORT_CODE(KEYCODE_F4)
INPUT_PORTS_END

/* CDP1802 Configuration */

static CDP1802_MODE_READ( cosmicos_mode_r )
{
	cdp1802_control_mode mode = CDP1802_MODE_RUN;

	return mode;
}

static CDP1802_EF_READ( cosmicos_ef_r )
{
	/*
        EF1     
        EF2		cassette input
        EF3
        EF4     
    */

	cosmicos_state *state = device->machine->driver_data;

	UINT8 flags = 0x0f;

	/* cassette input */
	if (cassette_input(state->cassette) < 0.0) flags -= EF2;

	return flags;
}

static CDP1802_SC_WRITE( cosmicos_sc_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( cosmicos_q_w )
{
	cosmicos_state *driver_state = device->machine->driver_data;

	/* cassette */
	cassette_output(driver_state->cassette, state ? +1.0 : -1.0);
}

static CDP1802_INTERFACE( cosmicos_config )
{
	cosmicos_mode_r,
	cosmicos_ef_r,
	cosmicos_sc_w,
	DEVCB_LINE(cosmicos_q_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( cosmicos )
{
	cosmicos_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->dm9368 = devtag_get_device(machine, DM9368_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* initialize LED display */
	dm9368_rbi_w(state->dm9368, 1);

	/* setup memory banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, CDP1802_TAG), 0);
	memory_set_bank(machine, 1, 0);

	switch (messram_get_size(devtag_get_device(machine, "messram")))
	{
	case 256:
		memory_install_readwrite8_handler(program, 0x0000, 0x00ff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x0100, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 4*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x1000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 48*1024:
		memory_install_readwrite8_handler(program, 0x0000, 0xbfff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;
	}

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static const cassette_config cosmicos_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( cosmicos )
	MDRV_DRIVER_DATA(cosmicos_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_1_75MHz)
    MDRV_CPU_PROGRAM_MAP(cosmicos_mem)
    MDRV_CPU_IO_MAP(cosmicos_io)
	MDRV_CPU_CONFIG(cosmicos_config)

    MDRV_MACHINE_START(cosmicos)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_cosmicos )
	MDRV_DM9368_ADD(DM9368_TAG, 0, NULL)

	/* devices */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, cosmicos_cassette_config)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256")
	MDRV_RAM_EXTRA_OPTIONS("4K,48K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( cosmicos )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "cosmicos.rom", 0xc000, 0x0800, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME		PARENT  COMPAT  MACHINE		INPUT		INIT    CONFIG		COMPANY				FULLNAME    FLAGS */
COMP( 1979, cosmicos,	0,		0,		cosmicos,	cosmicos,	0,		0,	"Radio Bulletin",	"Cosmicos",	GAME_NOT_WORKING )
