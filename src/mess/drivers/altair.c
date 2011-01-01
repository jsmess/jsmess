/***************************************************************************

        MITS Altair 8800b Turnkey

        04/12/2009 Initial driver by Miodrag Milanovic

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"
#include "devices/snapquik.h"


class altair_state : public driver_device
{
public:
	altair_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 term_data;
	UINT8* ram;
};



static READ8_HANDLER(sio_status_r)
{
	altair_state *state = space->machine->driver_data<altair_state>();
	if (state->term_data!=0) return 0x01; // data in
	return 0x02; // ready
}

static WRITE8_HANDLER(sio_command_w)
{

}

static READ8_HANDLER(sio_data_r)
{
	altair_state *state = space->machine->driver_data<altair_state>();
	UINT8 retVal = state->term_data;
	state->term_data = 0;
	return retVal;
}

static WRITE8_HANDLER(sio_data_w)
{
	device_t *devconf = space->machine->device("terminal");
	terminal_write(devconf,0,data);
}

static READ8_HANDLER(sio_key_status_r)
{
	altair_state *state = space->machine->driver_data<altair_state>();
	return (state->term_data!=0) ? 0x40 : 0x01;
}

static ADDRESS_MAP_START(altair_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xfcff ) AM_RAM  AM_BASE_MEMBER(altair_state, ram)
	AM_RANGE( 0xfd00, 0xfdff ) AM_ROM
	AM_RANGE( 0xff00, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( altair_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00, 0x00 ) AM_READ(sio_key_status_r)
	AM_RANGE( 0x01, 0x01 ) AM_READWRITE(sio_data_r,sio_data_w)
	AM_RANGE( 0x10, 0x10 ) AM_READWRITE(sio_status_r,sio_command_w)
	AM_RANGE( 0x11, 0x11 ) AM_READWRITE(sio_data_r,sio_data_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( altair )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


QUICKLOAD_LOAD(altair)
{
	altair_state *state = image.device().machine->driver_data<altair_state>();
	int quick_length;
	int read_;
	quick_length = image.length();
	if (quick_length >= 0xfd00)
		return IMAGE_INIT_FAIL;
	read_ = image.fread(state->ram, quick_length);
	if (read_ != quick_length)
		return IMAGE_INIT_FAIL;

	return IMAGE_INIT_PASS;
}


static MACHINE_RESET(altair)
{
	altair_state *state = machine->driver_data<altair_state>();
	// Set startup addess done by turn-key
	cpu_set_reg(machine->device("maincpu"), I8085_PC, 0xFD00);

	state->term_data = 0;
}

static WRITE8_DEVICE_HANDLER( altair_kbd_put )
{
	altair_state *state = device->machine->driver_data<altair_state>();
	state->term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( altair_terminal_intf )
{
	DEVCB_HANDLER(altair_kbd_put)
};

static MACHINE_CONFIG_START( altair, altair_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8080, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(altair_mem)
    MCFG_CPU_IO_MAP(altair_io)

    MCFG_MACHINE_RESET(altair)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,altair_terminal_intf)

	/* quickload */
	MCFG_QUICKLOAD_ADD("quickload", altair, "bin", 0)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( al8800bt )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "turnmon.bin",  0xfd00, 0x0100, CRC(5c629294) SHA1(125c76216954b681721fff84a3aca05094b21a28))
	ROM_LOAD( "88dskrom.bin", 0xff00, 0x0100, CRC(7c5232f3) SHA1(24f940ad70ad2829e1bc800c6790b6e993e6ebf6))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1975, al8800bt,  0,       0,	altair, 	altair, 	 0,   "MITS",   "Altair 8800bt",		GAME_NOT_WORKING | GAME_NO_SOUND)

