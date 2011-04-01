/***************************************************************************

    Driver for Casio CFX-9850

To operate:
The unit is switched off by default, you have to switch it on by pressing 'Q'.

Debugging information:
First make sure system is initialized (g 10b3).
At cs=23, ip=d1a4,d1af,d1ba,d1c5,d1d0,d1db,d1ea,d1f9,d208,d217,d226,d235,d244
are routines which fill display ram with patterns.

***************************************************************************/

#include "emu.h"
#include "cpu/hcd62121/hcd62121.h"
#include "rendlay.h"

class cfx9850_state : public driver_device
{
public:
	cfx9850_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_video_ram;
	UINT8 *m_display_ram;

	UINT16 m_ko;				/* KO lines KO1 - KO14 */
};


static ADDRESS_MAP_START( cfx9850, AS_PROGRAM, 8 )
	AM_RANGE( 0x000000, 0x007fff ) AM_ROM
	AM_RANGE( 0x080000, 0x0807ff ) AM_RAM AM_BASE_MEMBER( cfx9850_state, m_video_ram )
//  AM_RANGE( 0x100000, 0x10ffff ) /* some memory mapped i/o? */
//  AM_RANGE( 0x110000, 0x11ffff ) /* some memory mapped i/o? */
	AM_RANGE( 0x200000, 0x27ffff ) AM_ROM AM_REGION( "bios", 0 )
	AM_RANGE( 0x400000, 0x40ffff ) AM_RAM
	AM_RANGE( 0x600000, 0x601fff ) AM_MIRROR(0xf800) AM_RAM AM_BASE_MEMBER( cfx9850_state, m_display_ram )
//  AM_RANGE( 0xe10000, 0xe1ffff ) /* some memory mapped i/o? */
ADDRESS_MAP_END


static WRITE8_HANDLER( cfx9850_kol_w )
{
	cfx9850_state *state = space->machine().driver_data<cfx9850_state>();

	state->m_ko = ( state->m_ko & 0xff00 ) | data;
}

static WRITE8_HANDLER( cfx9850_koh_w )
{
	cfx9850_state *state = space->machine().driver_data<cfx9850_state>();

	state->m_ko = ( state->m_ko & 0x00ff ) | ( data << 8 );
}


static READ8_HANDLER( cfx9850_ki_r )
{
	cfx9850_state *state = space->machine().driver_data<cfx9850_state>();
	UINT8 data = 0;

	if ( state->m_ko & 0x0001 )
		data |= input_port_read( space->machine(), "KO1" );
	if ( state->m_ko & 0x0002 )
		data |= input_port_read( space->machine(), "KO2" );
	if ( state->m_ko & 0x0004 )
		data |= input_port_read( space->machine(), "KO3" );
	if ( state->m_ko & 0x0008 )
		data |= input_port_read( space->machine(), "KO4" );
	if ( state->m_ko & 0x0010 )
		data |= input_port_read( space->machine(), "KO5" );
	if ( state->m_ko & 0x0020 )
		data |= input_port_read( space->machine(), "KO6" );
	if ( state->m_ko & 0x0040 )
		data |= input_port_read( space->machine(), "KO7" );
	if ( state->m_ko & 0x0080 )
		data |= input_port_read( space->machine(), "KO8" );
	if ( state->m_ko & 0x0100 )
		data |= input_port_read( space->machine(), "KO9" );
	if ( state->m_ko & 0x0200 )
		data |= input_port_read( space->machine(), "KO10" );
	/* KO11 is not connected */
	if ( state->m_ko & 0x0800 )
		data |= input_port_read( space->machine(), "KO12" );
	/* KO13 is not connected */
	/* KO14 is not connected */

	return data;
}


static ADDRESS_MAP_START( cfx9850_io, AS_IO, 8 )
	AM_RANGE( HCD62121_KOL, HCD62121_KOL ) AM_WRITE( cfx9850_kol_w )
	AM_RANGE( HCD62121_KOH, HCD62121_KOH ) AM_WRITE( cfx9850_koh_w )
	AM_RANGE( HCD62121_KI, HCD62121_KI ) AM_READ( cfx9850_ki_r )
ADDRESS_MAP_END


static INPUT_PORTS_START( cfx9850 )
	PORT_START( "KO1" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "AC On/Off" ) PORT_CODE(KEYCODE_Q)

	PORT_START( "KO2" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "EXE Enter" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "(-) Ares" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "EXP Pi" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( ". SPACE" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "0   Z" )

	PORT_START( "KO3" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "- ] Y" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "+ [ X" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "3   W" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "2   V" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "1   U" )

	PORT_START( "KO4" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "/ } T" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "* { S" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "6   R" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "5   Q" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "4   P" )

	PORT_START( "KO5" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "DEL DG" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "9   O" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "8   N" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "7   M" )

	PORT_START( "KO6" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "TAB L" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( ",   K" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( ") x^-1 J" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "( .. I" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F<=>0 H" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "a b/c G" )

	PORT_START( "KO7" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "tan tan^-1 F" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "cos cas^-1 E" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "sin sin^-1 D" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "ln e^x C" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "log 10^x B" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "x,d,t A" )

	PORT_START( "KO8" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "Right" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "Down" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "EXIT QUIT" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "/\\ .. .." )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "x^2 sqrt .." )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "ALPHA ..-LOCK" )

	PORT_START( "KO9" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "Up" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "Left" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "MENU SET UP" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "VARS PRGM" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "OPTN" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "SHIFT" )

	PORT_START( "KO10" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F6 G<=>T" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F5 G-Solv" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F4 Sketch" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F3 V-Window" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F2 Zoom" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "F1 Trace" )

	/* KO11 is not connected */
//  PORT_START( "KO11" )

	PORT_START( "KO12" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "TEST" )

	/* KO13 is not connected */
//  PORT_START( "KO13" )

	/* KO14 is not connected */
//  PORT_START( "KO14" )
INPUT_PORTS_END


static PALETTE_INIT( cfx9850 )
{
	palette_set_color_rgb( machine, 0, 0xff, 0xff, 0xff );
	palette_set_color_rgb( machine, 1, 0x00, 0x00, 0xff );
	palette_set_color_rgb( machine, 2, 0x00, 0xff, 0x00 );
	palette_set_color_rgb( machine, 3, 0xff, 0x00, 0x00 );
}


static SCREEN_UPDATE( cfx9850 )
{
	cfx9850_state *state = screen->machine().driver_data<cfx9850_state>();
	UINT16 offset = 0;

	for ( int i = 0; i < 16; i++ )
	{
		int x = 120 - i * 8;

		for ( int j = 0; j < 64; j++ )
		{
			UINT8 data1 = state->m_display_ram[ offset ];
			UINT8 data2 = state->m_display_ram[ offset + 0x400 ];

			for ( int b = 0; b < 8; b++ )
			{
				*BITMAP_ADDR16(bitmap, 63-j, x+b) = ( data1 & 0x80 ) ? ( data2 & 0x80 ? 3 : 2 ) : ( data2 & 0x80 ? 1 : 0 );
				data1 <<= 1;
				data2 <<= 1;
			}

			offset++;
		}
	}

	return 0;
}


static MACHINE_CONFIG_START( cfx9850, cfx9850_state )

	MCFG_CPU_ADD( "maincpu", HCD62121, 4300000 )	/* 4.3 MHz */
	MCFG_CPU_PROGRAM_MAP( cfx9850 )
	MCFG_CPU_IO_MAP( cfx9850_io )

	MCFG_SCREEN_ADD( "screen", LCD )
	MCFG_SCREEN_REFRESH_RATE( 60 )
	MCFG_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MCFG_SCREEN_SIZE( 128, 64 )
	MCFG_SCREEN_VISIBLE_AREA( 0, 127, 0, 63 )
	MCFG_SCREEN_UPDATE( cfx9850 )

	MCFG_DEFAULT_LAYOUT(layout_lcd)


	/* TODO: It uses a color display, but I'm being lazy here. 3 colour lcd */
	MCFG_PALETTE_LENGTH( 4 )
	MCFG_PALETTE_INIT( cfx9850 )

MACHINE_CONFIG_END


ROM_START( cfx9850 )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "hcd62121.bin", 0x0000, 0x8000, CRC(e72075f8) SHA1(f50d176e1c225dab69abfc67702c9dfb296b6a78) )
	ROM_REGION( 0x80000, "bios", 0 )
	/* No idea yet which rom is what version. */
	ROM_SYSTEM_BIOS( 0, "rom1", "rom1, version unknown" )
	ROMX_LOAD( "cfx9850.bin", 0x00000, 0x80000, CRC(6c9bd903) SHA1(d5b6677ab4e0d3f84e5769e89e8f3d101f98f848), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "rom2", "rom2, version unknown" )
	ROMX_LOAD( "cfx9850b.bin", 0x00000, 0x80000, CRC(cd3c497f) SHA1(1d1aa38205eec7aba3ed6bef7389767e38afe075), ROM_BIOS(2) )
ROM_END


COMP( 1996, cfx9850, 0, 0, cfx9850, cfx9850, 0, "Casio", "CFX-9850G", GAME_NO_SOUND | GAME_NOT_WORKING )
