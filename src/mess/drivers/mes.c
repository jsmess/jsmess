/***************************************************************************

        Schleicher MES

        30/08/2010 Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class mes_state : public driver_device
{
public:
	mes_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	const UINT8 *FNT;
	const UINT8 *videoram;
};



static ADDRESS_MAP_START(mes_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xffff) AM_RAM AM_REGION("maincpu", 0x1000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mes_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mes )
INPUT_PORTS_END

static MACHINE_RESET(mes)
{
}

static VIDEO_START( mes )
{
	mes_state *state = machine->driver_data<mes_state>();
	state->FNT = machine->region("chargen")->base();
	state->videoram = machine->region("maincpu")->base()+0xf000;
}

/* This system appears to have 2 screens. Not implemented.
    Also the screen dimensions are a guess. */
static VIDEO_UPDATE( mes )
{
	mes_state *state = screen->machine->driver_data<mes_state>();
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
		ma+=80;
	}
	return 0;
}

static MACHINE_CONFIG_START( mes, mes_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MCFG_CPU_PROGRAM_MAP(mes_mem)
	MCFG_CPU_IO_MAP(mes_io)

    MCFG_MACHINE_RESET(mes)

    /* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(mes)
    MCFG_VIDEO_UPDATE(mes)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( mes )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mescpu.bin",   0x0000, 0x1000, CRC(b6d90cf4) SHA1(19e608af5bdaabb00a134e1106b151b00e2a0b04))
    ROM_REGION( 0x10000, "xebec", ROMREGION_ERASEFF )
	ROM_LOAD( "mesxebec.bin", 0x0000, 0x2000, CRC(061b7212) SHA1(c5d600116fb7563c69ebd909eb9613269b2ada0f))

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 198?, mes,  0,       0,	mes,	mes,	 0, 	  "Schleicher",   "MES",		GAME_NOT_WORKING | GAME_NO_SOUND)
