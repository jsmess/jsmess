/***************************************************************************

    AT&T Unix PC series

    Skeleton driver

	Note: The 68k core needs restartable instruction support for this
	to have a chance to run.

***************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "devices/messram.h"
#include "unixpc.lh"


/***************************************************************************
    DRIVER STATE
***************************************************************************/

class unixpc_state : public driver_device
{
public:
	unixpc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_ram(*this, "messram")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<running_device> m_ram;

	virtual bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	virtual void machine_reset();

	DECLARE_WRITE16_MEMBER( misc_control_w );
	DECLARE_WRITE16_MEMBER( romlmap_w );

	UINT16 *m_videoram;
};


/***************************************************************************
    MEMORY
***************************************************************************/

WRITE16_MEMBER( unixpc_state::romlmap_w )
{
	if (BIT(data, 15))
		memory_install_ram(&space, 0x000000, 0x3fffff, 0, 0, messram_get_ptr(m_ram));
	else
		memory_install_rom(&space, 0x000000, 0x3fffff, 0, 0, memory_region(space.machine, "bootrom"));
}

void unixpc_state::machine_reset()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);

	// force ROM into lower mem on reset
	romlmap_w(*program, 0, 0, 0xffff);

	// reset cpu so that it can pickup the new values
	m_maincpu->reset();
}


/***************************************************************************
    MISC
***************************************************************************/

WRITE16_MEMBER( unixpc_state::misc_control_w )
{
	output_set_value("led_0", !BIT(data,  8));
	output_set_value("led_1", !BIT(data,  9));
	output_set_value("led_2", !BIT(data, 10));
	output_set_value("led_3", !BIT(data, 11));
}


/***************************************************************************
    VIDEO
***************************************************************************/

bool unixpc_state::video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	for (int y = 0; y < 348; y++)
		for (int x = 0; x < 720/16; x++)
			for (int b = 0; b < 16; b++)
				*BITMAP_ADDR16(&bitmap, y, x * 16 + b) = BIT(m_videoram[y * (720/16) + x], b);

	return 0;
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( unixpc_mem, ADDRESS_SPACE_PROGRAM, 16, unixpc_state )
	AM_RANGE(0x000000, 0x3fffff) AM_RAMBANK("bank1")
	AM_RANGE(0x420000, 0x427fff) AM_RAM AM_BASE(m_videoram)
	AM_RANGE(0x4a0000, 0x4a0001) AM_WRITE(misc_control_w)
	AM_RANGE(0x800000, 0xbfffff) AM_MIRROR(0x7fc000) AM_ROM AM_REGION("bootrom", 0)
	AM_RANGE(0xe43000, 0xe43001) AM_WRITE(romlmap_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( unixpc )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_CONFIG_START( unixpc, unixpc_state )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", M68010, XTAL_10MHz)
	MDRV_CPU_PROGRAM_MAP(unixpc_mem)

	// video hardware
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_20MHz, 896, 0, 720, 367, 0, 348)
	// vsync should actually last 17264 pixels

	MDRV_DEFAULT_LAYOUT(layout_unixpc)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	// internal ram
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1M")
	MDRV_RAM_EXTRA_OPTIONS("2M")
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

// ROMs were provided by Michael Lee und imaged by Philip Pemberton
ROM_START( 3b1 )
	ROM_REGION16_BE(0x400000, "bootrom", 0)
	ROM_LOAD16_BYTE("72-00617.15c", 0x000000, 0x002000, CRC(4e93ff40) SHA1(1a97c8d32ec862f7f5fa1032f1688b76ea0672cc))
	ROM_LOAD16_BYTE("72-00616.14c", 0x000001, 0x002000, CRC(c61f7ae0) SHA1(ab3ac29935a2a587a083c4d175a5376badd39058))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

//    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT   INIT  COMPANY  FULLNAME  FLAGS
COMP( 1985, 3b1,  0,      0,      unixpc,  unixpc, 0,    "AT&T",  "3B1",    GAME_NOT_WORKING | GAME_NO_SOUND )
