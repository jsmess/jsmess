/***************************************************************************

    BEEHIVE DM3270

    25/05/2009 Skeleton driver [Robbbert]

    This is a conventional computer terminal using a serial link.

    The character gen rom is not dumped. Using the one from 'c10'
    for the moment.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"


class beehive_state : public driver_device
{
public:
	beehive_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	const UINT8 *FNT;
	const UINT8 *videoram;
};



static ADDRESS_MAP_START(beehive_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x17ff ) AM_ROM
	AM_RANGE( 0x1800, 0xffff ) AM_RAM AM_REGION("maincpu", 0x1800)
ADDRESS_MAP_END

static ADDRESS_MAP_START( beehive_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( beehive )
INPUT_PORTS_END


static MACHINE_RESET(beehive)
{
}

static VIDEO_START( beehive )
{
	beehive_state *state = machine->driver_data<beehive_state>();
	state->FNT = machine->region("chargen")->base();
	state->videoram = machine->region("maincpu")->base()+0x81fa;
}

/* This system appears to have inline attribute bytes of unknown meaning.
    Currently they are ignored. */
static VIDEO_UPDATE( beehive )
{
	beehive_state *state = screen->machine->driver_data<beehive_state>();
	//static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x,xx;

	//framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			xx = ma;
			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;
				if (ra < 9)
				{
					chr = state->videoram[xx++];

				//	/* Take care of flashing characters */
				//	if ((chr < 0x80) && (framecnt & 0x08))
				//		chr |= 0x80;

					if (chr & 0x80)  // ignore attribute bytes
						x--;
					else
					{
						gfx = state->FNT[(chr<<4) | ra ];

						/* Display a scanline of a character */
						*p++ = ( gfx & 0x80 ) ? 1 : 0;
						*p++ = ( gfx & 0x40 ) ? 1 : 0;
						*p++ = ( gfx & 0x20 ) ? 1 : 0;
						*p++ = ( gfx & 0x10 ) ? 1 : 0;
						*p++ = ( gfx & 0x08 ) ? 1 : 0;
						*p++ = ( gfx & 0x04 ) ? 1 : 0;
						*p++ = ( gfx & 0x02 ) ? 1 : 0;
						*p++ = ( gfx & 0x01 ) ? 1 : 0;
					}
				}
			}
		}
		ma+=106;
	}
	return 0;
}

static MACHINE_CONFIG_START( beehive, beehive_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8085A, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(beehive_mem)
    MCFG_CPU_IO_MAP(beehive_io)

    MCFG_MACHINE_RESET(beehive)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 250)
    MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(beehive)
    MCFG_VIDEO_UPDATE(beehive)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( beehive )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "dm3270-1.rom", 0x0000, 0x0800, CRC(781bde32) SHA1(a3fe25baadd2bfc2b1791f509bb0f4960281ee32) )
	ROM_LOAD( "dm3270-2.rom", 0x0800, 0x0800, CRC(4d3476b7) SHA1(627ad42029ca6c8574cda8134d047d20515baf53) )
	ROM_LOAD( "dm3270-3.rom", 0x1000, 0x0800, CRC(dbf15833) SHA1(ae93117260a259236c50885c5cecead2aad9b3c4) )

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, beehive,  0,       0,	beehive,	beehive,	 0,  "BeeHive",   "DM3270",		GAME_NOT_WORKING | GAME_NO_SOUND)

