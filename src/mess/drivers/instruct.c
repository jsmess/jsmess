/***************************************************************************

        instruct

        08/04/2010 Skeleton driver.

        The eprom and 128 bytes of ram are in a 2656 chip. There is no
        useful info on this device on the net, particularly where in the
        memory map the ram resides.

        The Instructor has 512 bytes of ram, plus the 128 bytes mentioned
        above. The clock speed is supposedly 875kHz.

        There is a big lack of info on this computer. No schematics, no i/o
        list or memory map. There is a block diagram which imparts little.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/s2650/s2650.h"


class instruct_state : public driver_device
{
public:
	instruct_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		m_maincpu(*this, "maincpu")
		{ }

	required_device<cpu_device> m_maincpu;
	DECLARE_READ8_MEMBER( instruct_fc_r );
	DECLARE_READ8_MEMBER( instruct_fd_r );
	DECLARE_READ8_MEMBER( instruct_sense_r );
	DECLARE_WRITE8_MEMBER( instruct_f8_w );
};

WRITE8_MEMBER(instruct_state::instruct_f8_w)
{
}

READ8_MEMBER(instruct_state::instruct_fc_r)
{
	return 0;
}

READ8_MEMBER(instruct_state::instruct_fd_r)
{
	return 0;
}

READ8_MEMBER(instruct_state::instruct_sense_r)
{
	return 0;
}

static ADDRESS_MAP_START(instruct_mem, AS_PROGRAM, 8, instruct_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x1700, 0x17ff) AM_RAM
	AM_RANGE( 0x1f00, 0x1fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( instruct_io, AS_IO, 8, instruct_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xf8, 0xf8) AM_WRITE(instruct_f8_w)
	AM_RANGE(0xfc, 0xfc) AM_READ(instruct_fc_r)
	AM_RANGE(0xfd, 0xfd) AM_READ(instruct_fd_r)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(instruct_sense_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( instruct )
INPUT_PORTS_END


static MACHINE_RESET(instruct)
{
}

static VIDEO_START( instruct )
{
}

static SCREEN_UPDATE( instruct )
{
	return 0;
}

static MACHINE_CONFIG_START( instruct, instruct_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(instruct_mem)
	MCFG_CPU_IO_MAP(instruct_io)

	MCFG_MACHINE_RESET(instruct)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(instruct)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(instruct)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( instruct )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "instruct.rom", 0x0000, 0x0800, CRC(131715a6) SHA1(4930b87d09046113ab172ba3fb31f5e455068ec7) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1978, instruct,  0,       0,	instruct,	instruct,	 0,  "Signetics",   "Signetics Instructor 50",		GAME_NOT_WORKING | GAME_NO_SOUND )

