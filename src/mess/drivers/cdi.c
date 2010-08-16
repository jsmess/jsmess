/******************************************************************************


    Philips CD-I
    -------------------

    Preliminary MAME driver by Roberto Fresca, David Haywood & Angelo Salese
    MESS improvements by Harmony
    Help provided by CD-i Fan


*******************************************************************************

STATUS:

Some games are playable, but many suffer from bugs and/or crashes.

TODO:

- Proper handling of the 68070's internal devices (UART,DMA,Timers etc.)

- Full emulation of the 66470 and/or MCD212 Video Chip

*******************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "sound/dmadac.h"
#include "devices/chd_cd.h"
#include "machine/timekpr.h"
#include "sound/cdda.h"
#include "includes/cdi.h"
#include "cdi.lh"

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
    if( VERBOSE_LEVEL >= n_level )
    {
        va_list v;
        char buf[ 32768 ];
        va_start( v, s_fmt );
        vsprintf( buf, s_fmt, v );
        va_end( v );
        logerror( "%08x: %s", cpu_get_pc(machine->device("maincpu")), buf );
    }
}
#else
#define verboselog(x,y,z,...)
#endif

/*************************
*      Memory maps       *
*************************/

WRITE16_HANDLER( cdic_ram_w )
{
	cdi_state *state = space->machine->driver_data<cdi_state>();
	cdic_regs_t *cdic = &state->cdic_regs;

	verboselog(space->machine, 0, "cdic_ram_w: %08x = %04x & %04x\n", 0x00300000 + offset*2, data, mem_mask);
	COMBINE_DATA(&cdic->ram[offset]);
}

READ16_HANDLER( cdic_ram_r )
{
	cdi_state *state = space->machine->driver_data<cdi_state>();
	cdic_regs_t *cdic = &state->cdic_regs;

	verboselog(space->machine, 0, "cdic_ram_r: %08x = %04x & %04x\n", 0x00300000 + offset*2, cdic->ram[offset], mem_mask);
	return cdic->ram[offset];
}

static ADDRESS_MAP_START( cdimono1_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE_MEMBER(cdi_state,planea)
	AM_RANGE(0x00200000, 0x0027ffff) AM_RAM AM_BASE_MEMBER(cdi_state,planeb)
#if ENABLE_UART_PRINTING
	AM_RANGE(0x00301400, 0x00301403) AM_READ(uart_loopback_enable)
#endif
	AM_RANGE(0x00300000, 0x00303bff) AM_READWRITE(cdic_ram_r, cdic_ram_w) AM_BASE_MEMBER(cdi_state,cdic_regs.ram)
	//AM_RANGE(0x00300000, 0x00303bff) AM_RAM AM_BASE_MEMBER(cdi_state,cdic_regs.ram)
	AM_RANGE(0x00303c00, 0x00303fff) AM_READWRITE(cdic_r, cdic_w)
	AM_RANGE(0x00310000, 0x00317fff) AM_READWRITE(slave_r, slave_w)
	//AM_RANGE(0x00318000, 0x0031ffff) AM_NOP
	AM_RANGE(0x00320000, 0x00323fff) AM_DEVREADWRITE8("mk48t08", timekeeper_r, timekeeper_w, 0xff00)	/* nvram (only low bytes used) */
	AM_RANGE(0x00400000, 0x0047ffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x004fffe0, 0x004fffff) AM_READWRITE(mcd212_r, mcd212_w)
	AM_RANGE(0x00500000, 0x0057ffff) AM_RAM
	AM_RANGE(0x00580000, 0x00ffffff) AM_NOP
	AM_RANGE(0x00e00000, 0x00efffff) AM_RAM // DVC
	AM_RANGE(0x80000000, 0x8000807f) AM_READWRITE(scc68070_periphs_r, scc68070_periphs_w)
ADDRESS_MAP_END

/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( cdi )
	PORT_START("MOUSEX")
	PORT_BIT(0x3ff, 0x000, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_MINMAX(0x000, 0x3ff) PORT_KEYDELTA(0) PORT_CHANGED(mouse_update, 0)

	PORT_START("MOUSEY")
	PORT_BIT(0x3ff, 0x000, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_MINMAX(0x000, 0x3ff) PORT_KEYDELTA(0) PORT_CHANGED(mouse_update, 0)

	PORT_START("MOUSEBTN")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("Mouse Button 1") PORT_CHANGED(mouse_update, 0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("Mouse Button 2") PORT_CHANGED(mouse_update, 0)
	PORT_BIT(0xfc, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("DEBUG")
	PORT_CONFNAME( 0x01, 0x00, "Plane A Disable")
	PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(    0x01, DEF_STR( On ) )
	PORT_CONFNAME( 0x02, 0x00, "Plane B Disable")
	PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(    0x02, DEF_STR( On ) )
	PORT_CONFNAME( 0x04, 0x00, "Force Backdrop Color")
	PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(    0x04, DEF_STR( On ) )
	PORT_CONFNAME( 0xf0, 0x00, "Backdrop Color")
	PORT_CONFSETTING(    0x00, "Black" )
	PORT_CONFSETTING(    0x10, "Half-Bright Blue" )
	PORT_CONFSETTING(    0x20, "Half-Bright Green" )
	PORT_CONFSETTING(    0x30, "Half-Bright Cyan" )
	PORT_CONFSETTING(    0x40, "Half-Bright Red" )
	PORT_CONFSETTING(    0x50, "Half-Bright Magenta" )
	PORT_CONFSETTING(    0x60, "Half-Bright Yellow" )
	PORT_CONFSETTING(    0x70, "Half-Bright White" )
	PORT_CONFSETTING(    0x80, "Black (Alternate)" )
	PORT_CONFSETTING(    0x90, "Blue" )
	PORT_CONFSETTING(    0xa0, "Green" )
	PORT_CONFSETTING(    0xb0, "Cyan" )
	PORT_CONFSETTING(    0xc0, "Red" )
	PORT_CONFSETTING(    0xd0, "Magenta" )
	PORT_CONFSETTING(    0xe0, "Yellow" )
	PORT_CONFSETTING(    0xf0, "White" )
INPUT_PORTS_END

static MACHINE_START( cdi )
{
	cdi_state *state = machine->driver_data<cdi_state>();

	scc68070_register_globals(machine, &state->scc68070_regs);
	cdic_register_globals(machine, &state->cdic_regs);
	slave_register_globals(machine, &state->slave_regs);

	state->scc68070_regs.timers.timer0_timer = timer_alloc(machine, scc68070_timer0_callback, 0);
	timer_adjust_oneshot(state->scc68070_regs.timers.timer0_timer, attotime_never, 0);

	state->slave_regs.interrupt_timer = timer_alloc(machine, slave_trigger_readback_int, 0);
	timer_adjust_oneshot(state->slave_regs.interrupt_timer, attotime_never, 0);

	state->cdic_regs.interrupt_timer = timer_alloc(machine, cdic_trigger_readback_int, 0);
	timer_adjust_oneshot(state->cdic_regs.interrupt_timer, attotime_never, 0);

	state->cdic_regs.audio_sample_timer = timer_alloc(machine, audio_sample_trigger, 0);
	timer_adjust_oneshot(state->cdic_regs.audio_sample_timer, attotime_never, 0);
}

static MACHINE_RESET( cdi )
{
	cdi_state *state = machine->driver_data<cdi_state>();
	UINT16 *src   = (UINT16*)memory_region(machine, "maincpu");
	UINT16 *dst   = state->planea;
	running_device *cdrom_dev = machine->device("cdrom");
	memcpy(dst, src, 0x8);

	scc68070_init(machine, &state->scc68070_regs);
	cdic_init(machine, &state->cdic_regs);
	slave_init(machine, &state->slave_regs);

	if( cdrom_dev )
	{
		state->cdic_regs.cd = mess_cd_get_cdrom_file(cdrom_dev);
		cdda_set_cdrom(machine->device("cdda"), state->cdic_regs.cd);
	}

	machine->device("maincpu")->reset();

	state->dmadac[0] = machine->device<dmadac_sound_device>("dac1");
	state->dmadac[1] = machine->device<dmadac_sound_device>("dac2");

	state->slave_regs.real_mouse_x = 0xffff;
	state->slave_regs.real_mouse_y = 0xffff;
}

/*************************
*    Machine Drivers     *
*************************/

static MACHINE_DRIVER_START( cdimono1 )

	MDRV_DRIVER_DATA( cdi_state )

	MDRV_CPU_ADD( "maincpu", SCC68070, CLOCK_A/2 )
	MDRV_CPU_PROGRAM_MAP( cdimono1_mem )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME( ATTOSECONDS_IN_USEC(0) )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 384, 262 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 384-1, 22, 262-1 ) //dynamic resolution,TODO

	MDRV_SCREEN_ADD( "lcd", RASTER )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME( ATTOSECONDS_IN_USEC(0) )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 192, 22 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 192-1, 0, 22-1 )

	MDRV_PALETTE_LENGTH( 0x100 )

	MDRV_DEFAULT_LAYOUT( layout_cdi )

	MDRV_VIDEO_START( cdimono1 )
	MDRV_VIDEO_UPDATE( cdimono1 )

	MDRV_MACHINE_RESET( cdi )
	MDRV_MACHINE_START( cdi )

	MDRV_CDROM_ADD( "cdrom" )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO( "lspeaker", "rspeaker" )

	MDRV_SOUND_ADD( "dac1", DMADAC, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 1.0 )

	MDRV_SOUND_ADD( "dac2", DMADAC, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 1.0 )

	MDRV_SOUND_ADD( "cdda", CDDA, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 1.0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 1.0 )

	MDRV_MK48T08_ADD( "mk48t08" )
MACHINE_DRIVER_END

/*************************
*        Rom Load        *
*************************/

ROM_START( cdimono1 )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "mcdi200", "Magnavox CD-i 200" )
	ROMX_LOAD( "cdi200.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "pcdi220", "Philips CD-i 220 F2" )
	ROMX_LOAD( "cdi220b.rom", 0x000000, 0x80000, CRC(279683ca) SHA1(53360a1f21ddac952e95306ced64186a3fc0b93e), ROM_BIOS(2) )
	// This one is a Mono-IV board, needs to be a separate driver
	//ROM_SYSTEM_BIOS( 2, "pcdi490", "Philips CD-i 490" )
	//ROMX_LOAD( "cdi490.rom", 0x000000, 0x80000, CRC(e115f45b) SHA1(f71be031a5dfa837de225081b2ddc8dcb74a0552), ROM_BIOS(3) )
	// This one is a Mini-MMC board, needs to be a separate driver
	//ROM_SYSTEM_BIOS( 3, "pcdi910m", "Philips CD-i 910" )
	//ROMX_LOAD( "cdi910.rom", 0x000000, 0x80000,  CRC(8ee44ed6) SHA1(3fcdfa96f862b0cb7603fb6c2af84cac59527b05), ROM_BIOS(4) )
ROM_END

/*************************
*      Game driver(s)    *
*************************/

/*    YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY     FULLNAME   FLAGS */
CONS( 1991, cdimono1, 0,        0,        cdimono1, cdi,      0,        "Philips",  "CD-i (Mono-I)",   GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
