/***************************************************************************

    Processor Technology Corp. SOL-20

    07/2009 Skeleton driver.

    Info from: http://www.sol20.org/

    Note that the SOLOS dump comes from the Solace emu. Not being sure if
    it has been verified with a real SOL-20 (even if I think it has been), I
    marked it as a BAD_DUMP

    Other OS ROMs to be dumped:
      - CONSOL
      - CUTER
      - BOOTLOAD
      - DPMON (3rd party)

    Hardware variants (ever shipped?):
      - SOL-10, i.e. a stripped down version without expansion backplane and
        numeric keypad
      - SOL-PC, i.e. the single board version, basically it consisted of the
        SOL-20 board only

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"
#include "devices/messram.h"


class ptcsol_state : public driver_device
{
public:
	ptcsol_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 sol20_fa;
	UINT8 sol20_fc;
	UINT8 sol20_fe;
	const UINT8 *FNT;
	const UINT8 *videoram;
	UINT8 framecnt;
};



static READ8_HANDLER( sol20_fa_r )
{
	ptcsol_state *state = space->machine->driver_data<ptcsol_state>();
	return state->sol20_fa;
}

static READ8_HANDLER( sol20_fc_r )
{
	ptcsol_state *state = space->machine->driver_data<ptcsol_state>();
	state->sol20_fa = 0xff;
	return state->sol20_fc;
}

static WRITE8_HANDLER( sol20_fe_w )
{
	ptcsol_state *state = space->machine->driver_data<ptcsol_state>();
	state->sol20_fe = data;
}

static ADDRESS_MAP_START( sol20_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0X0000, 0Xbfff) AM_RAM	// optional s100 ram
	AM_RANGE(0xc000, 0xc7ff) AM_ROM
	AM_RANGE(0Xc800, 0Xcbff) AM_RAM	// system ram
	AM_RANGE(0Xcc00, 0Xcfff) AM_RAM	AM_REGION("maincpu", 0xcc00)// video ram
	AM_RANGE(0Xd000, 0Xffff) AM_RAM	// optional s100 ram
ADDRESS_MAP_END

static ADDRESS_MAP_START( sol20_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xfa, 0xfa) AM_READ(sol20_fa_r)
	AM_RANGE(0xfc, 0xfc) AM_READ(sol20_fc_r)
	AM_RANGE(0xfe, 0xfe) AM_WRITE(sol20_fe_w)
/*  AM_RANGE(0xf8, 0xf8) serial status in (bit 6=data av, bit 7=tmbe)
    AM_RANGE(0xf9, 0xf9) serial data in, out
    AM_RANGE(0xfa, 0xfa) general status in (bit 0=keyb data av, bit 1=parin data av, bit 2=parout ready)
    AM_RANGE(0xfb, 0xfb) tape
    AM_RANGE(0xfc, 0xfc) keyboard data in
    AM_RANGE(0xfd, 0xfd) parallel data in, out
    AM_RANGE(0xfe, 0xfe) scroll register
    AM_RANGE(0xff, 0xff) sense switches */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sol20 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET( sol20 )
{
	ptcsol_state *state = machine->driver_data<ptcsol_state>();
	state->sol20_fa=0xff;
	state->sol20_fe=0;
}

static VIDEO_START( sol20 )
{
	ptcsol_state *state = machine->driver_data<ptcsol_state>();
	state->FNT = machine->region("chargen")->base();
	state->videoram = machine->region("maincpu")->base()+0xcc00;
}

static VIDEO_UPDATE( sol20 )
{
	ptcsol_state *state = screen->machine->driver_data<ptcsol_state>();
/* visible screen is 64 x 16, with start position controlled by scroll register */
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma,x,inv;

	state->framecnt++;

	ma = state->sol20_fe << 6; // scroll register

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 13; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x++)
			{
				chr = state->videoram[x & 0x3ff];
				inv = 0;

				/* Take care of flashing characters */
				if ((chr & 0x80) && (state->framecnt & 0x08))
					inv ^= 0xff;

				chr &= 0x7f;

				if (ra == 0)
					gfx = 0;
				else
				if (ra < 10)

					gfx = state->FNT[(chr<<4) | (ra-1) ] ^ inv;
				else
					gfx = inv;

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
		ma+=64;
	}
	return 0;
}

static WRITE8_DEVICE_HANDLER( sol20_kbd_put )
{
	ptcsol_state *state = device->machine->driver_data<ptcsol_state>();
	state->sol20_fa &= 0xfe;
	state->sol20_fc = data;
}

static GENERIC_TERMINAL_INTERFACE( sol20_terminal_intf )
{
	DEVCB_HANDLER(sol20_kbd_put)
};

static MACHINE_CONFIG_START( sol20, ptcsol_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_14_31818MHz/7)
	MCFG_CPU_PROGRAM_MAP(sol20_mem)
	MCFG_CPU_IO_MAP(sol20_io)

	MCFG_MACHINE_RESET(sol20)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(512, 208)
	MCFG_SCREEN_VISIBLE_AREA(0, 511, 0, 207)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(sol20)
	MCFG_VIDEO_UPDATE(sol20)

	/* internal ram */
//  MCFG_RAM_ADD("messram")
//  MCFG_RAM_DEFAULT_SIZE("8K")
//  MCFG_RAM_EXTRA_OPTIONS("16K,32K")
	MCFG_GENERIC_TERMINAL_ADD("terminal", sol20_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sol20 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "solos.rom", 0xc000, 0x0800, BAD_DUMP CRC(4d0af383) SHA1(ac4510c3380ed4a31ccf4f538af3cb66b76701ef) )	// from solace emu
	/* Character generator rom is missing */

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   COMPANY     FULLNAME   FLAGS */
COMP( 1976, sol20,  0,      0,      sol20,   sol20, 0,     "Processor Technology Corporation",  "SOL-20", GAME_NOT_WORKING | GAME_NO_SOUND)
