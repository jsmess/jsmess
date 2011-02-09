/***************************************************************************

        Sharp pocket computers 1500

        More info:
            http://www.pc1500.com/

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/lh5801/lh5801.h"
#include "rendlay.h"


class pc1500_state : public driver_device
{
public:
	pc1500_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu")
		{ }

	required_device<cpu_device> m_maincpu;

	UINT8 *m_lcd_data;

	bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

static ADDRESS_MAP_START( pc1500_mem , ADDRESS_SPACE_PROGRAM, 8, pc1500_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff) AM_ROM	//module ROM/RAM
	AM_RANGE( 0x4000, 0x47ff) AM_RAM	//user RAM
	AM_RANGE( 0x4800, 0x6fff) AM_RAM	//expansion RAM
	AM_RANGE( 0x7000, 0x71ff) AM_RAM	AM_MIRROR(0x0600)	AM_BASE( m_lcd_data )
	AM_RANGE( 0x7800, 0x7bff) AM_RAM	AM_MIRROR(0x0400)
	AM_RANGE( 0xa000, 0xbfff) AM_ROM	//expansion ROM
	AM_RANGE( 0xc000, 0xffff) AM_ROM	//system ROM
ADDRESS_MAP_END


bool pc1500_state::video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	bitmap_fill(&bitmap, &cliprect, 0);

	for (int p=0; p<=1; p++)
		for (int a=0; a<0x4e; a++)
		{
			UINT8 data = m_lcd_data[a + (p<<8)];
			for (int b=0; b<8; b++)
			{
				if(b<4)
					*BITMAP_ADDR16(&bitmap, b + 4 * (BIT( a,0)), (a>>1) + 0x00 + 0x27*p) = BIT(data, b);
				else
					*BITMAP_ADDR16(&bitmap, b - 4 * (BIT(~a,0)), (a>>1) + 0x4e + 0x27*p) = BIT(data, b);
			}
		}

	return 0;
}


static INPUT_PORTS_START( pc1500 )

INPUT_PORTS_END


static PALETTE_INIT( pc1500 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static UINT8 lh5801_input(device_t *device)
{
	return 0xff;
}

static const lh5801_cpu_core lh5801_pc1500_config =
{
	lh5801_input
};


static MACHINE_CONFIG_START( pc1500, pc1500_state )
	MCFG_CPU_ADD("maincpu", LH5801, 1300000)			//1.3 MHz
	MCFG_CPU_PROGRAM_MAP( pc1500_mem )
	MCFG_CPU_CONFIG( lh5801_pc1500_config )

	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))	// not accurate
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(156, 8)
	MCFG_SCREEN_VISIBLE_AREA(0, 156-1, 0, 7-1)
	MCFG_DEFAULT_LAYOUT(layout_lcd)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(pc1500)
MACHINE_CONFIG_END


ROM_START( pc1500 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sys1500.rom", 0xc000, 0x4000, CRC(d480b50d) SHA1(4bf748ba4d7c2b7cd7da7f3fdefcdd2e4cd41c4e))
    ROM_REGION( 0x10000, "ce150", ROMREGION_ERASEFF )
	ROM_LOAD( "ce-150.rom", 0x0000, 0x2000, CRC(8fa1df6d) SHA1(a3aa02a641a46c27c0d4c0dc025b0dbe9b5b79c8))
ROM_END

/*    YEAR  NAME    PARENT  COMPAT   MACHINE INPUT   INIT         COMPANY     FULLNAME */
COMP( 198?, pc1500,	0,	0,	pc1500,	pc1500,	0,	 "Sharp", "Pocket Computer 1500", GAME_NOT_WORKING | GAME_NO_SOUND )
