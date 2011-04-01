/***************************************************************************

        Robotron K8915

        30/08/2010 Skeleton driver

        When it says DIAGNOSTIC RAZ P, press enter.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class k8915_state : public driver_device
{
public:
	k8915_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	UINT8 *m_charrom;
	UINT8 m_framecnt;
	UINT8 m_term_data;
	UINT8 m_k8915_53;
};

static READ8_HANDLER( k8915_52_r )
{
// get data from ascii keyboard
	k8915_state *state = space->machine().driver_data<k8915_state>();
	state->m_k8915_53 = 0;
	return state->m_term_data;
}

static READ8_HANDLER( k8915_53_r )
{
// keyboard status
	k8915_state *state = space->machine().driver_data<k8915_state>();
	return state->m_k8915_53;
}

static WRITE8_HANDLER( k8915_a8_w )
{
// seems to switch ram and rom around.
	if (data == 0x87)
		memory_set_bank(space->machine(), "boot", 0); // ram at 0000
	else
		memory_set_bank(space->machine(), "boot", 1); // rom at 0000
}

static ADDRESS_MAP_START(k8915_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("boot")
	AM_RANGE(0x1000, 0x17ff) AM_RAM AM_BASE_MEMBER(k8915_state, m_videoram)
	AM_RANGE(0x1800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( k8915_io, AS_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x52, 0x52) AM_READ(k8915_52_r)
	AM_RANGE(0x53, 0x53) AM_READ(k8915_53_r)
	AM_RANGE(0xa8, 0xa8) AM_WRITE(k8915_a8_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( k8915 )
INPUT_PORTS_END

static MACHINE_RESET(k8915)
{
	memory_set_bank(machine, "boot", 1);
}

static DRIVER_INIT(k8915)
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x10000);
}

static VIDEO_START( k8915 )
{
	k8915_state *state = machine.driver_data<k8915_state>();
	state->m_charrom = machine.region("chargen")->base();
}

static SCREEN_UPDATE( k8915 )
{
	k8915_state *state = screen->machine().driver_data<k8915_state>();
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	state->m_framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;

				if (ra < 9)
				{
					chr = state->m_videoram[x];

					/* Take care of flashing characters */
					if ((chr & 0x80) && (state->m_framecnt & 0x08))
						chr = 0x20;

					chr &= 0x7f;

					gfx = state->m_charrom[(chr<<4) | ra ];
				}

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
		ma+=80;
	}
	return 0;
}

PALETTE_INIT( k8915 )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color(machine, 1, MAKE_RGB(0, 220, 0)); /* green */
}

static WRITE8_DEVICE_HANDLER( k8915_kbd_put )
{
	k8915_state *state = device->machine().driver_data<k8915_state>();
	state->m_term_data = data;
	state->m_k8915_53 = 1;
}

static GENERIC_TERMINAL_INTERFACE( k8915_terminal_intf )
{
	DEVCB_HANDLER(k8915_kbd_put)
};

static MACHINE_CONFIG_START( k8915, k8915_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MCFG_CPU_PROGRAM_MAP(k8915_mem)
	MCFG_CPU_IO_MAP(k8915_io)

	MCFG_MACHINE_RESET(k8915)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MCFG_SCREEN_UPDATE(k8915)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(k8915)

	MCFG_VIDEO_START(k8915)

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, k8915_terminal_intf) // keyboard only
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( k8915 )
	ROM_REGION( 0x11000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k8915.bin", 0x10000, 0x1000, CRC(ca70385f) SHA1(a34c14adae9be821678aed7f9e33932ee1f3e61c))

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1982, k8915,  0,       0, 	k8915,	k8915,	 k8915, 	  "Robotron",   "K8915",		GAME_NOT_WORKING | GAME_NO_SOUND)
