/****************************************************************************

 PC-Engine / Turbo Grafx 16 driver
 by Charles Mac Donald
 E-Mail: cgfm2@hooked.net

 Thanks to David Shadoff and Brian McPhail for help with the driver.

****************************************************************************/

/**********************************************************************
          To-Do List:
- convert h6280-based drivers to internal memory map for the I/O region
- test sprite collision and overflow interrupts
- sprite precaching
- rewrite the base renderer loop
- Add CD support
- Add 6 button joystick support
- Add 263 line mode
- Sprite DMA should use vdc VRAM functions
- properly implement the pixel clocks instead of the simple scaling we do now

Banking
=======

Normally address spacebanks 00-F6 are assigned to regular HuCard ROM space. There
are a couple of special situations:

Street Fighter II:
  - address space banks 40-7F switchable by writing to 1FF0-1FF3
    1FF0 - select rom banks 40-7F
    1FF1 - select rom banks 80-BF
    1FF2 - select rom banks C0-FF
    1FF3 - select rom banks 100-13F

Populous:
  - address space banks 40-43 contains 32KB RAM

CDRom units:
  - address space banks 80-87 contains 64KB RAM

Super System Card:
  - address space banks 68-7F contains 192KB RAM

**********************************************************************/

/**********************************************************************
                          Known Bugs
***********************************************************************
- TV Sports Basketball game freezes.
- Ankoku Densetsu: graphics flake out during intro (DMA issues)
**********************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "devices/cartslot.h"
#include "devices/chd_cd.h"
#include "sound/c6280.h"
#include "sound/cdda.h"
#include "sound/msm5205.h"
#include "hash.h"

#define	MAIN_CLOCK		21477270
#define PCE_CD_CLOCK	9216000


static ADDRESS_MAP_START( pce_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x000000, 0x07FFFF) AM_ROMBANK("bank1")
	AM_RANGE( 0x080000, 0x087FFF) AM_ROMBANK("bank2")
	AM_RANGE( 0x088000, 0x0CFFFF) AM_ROMBANK("bank3")
	AM_RANGE( 0x0D0000, 0x0FFFFF) AM_ROMBANK("bank4")
	AM_RANGE( 0x100000, 0x10FFFF) AM_RAM AM_BASE( &pce_cd_ram )
	AM_RANGE( 0x110000, 0x1EDFFF) AM_NOP
	AM_RANGE( 0x1EE000, 0x1EE7FF) AM_ROMBANK("bank10") AM_WRITE( pce_cd_bram_w )
	AM_RANGE( 0x1EE800, 0x1EFFFF) AM_NOP
	AM_RANGE( 0x1F0000, 0x1F1FFF) AM_RAM AM_MIRROR(0x6000) AM_BASE( &pce_user_ram )
	AM_RANGE( 0x1FE000, 0x1FE3FF) AM_READWRITE( vdc_0_r, vdc_0_w )
	AM_RANGE( 0x1FE400, 0x1FE7FF) AM_READWRITE( vce_r, vce_w )
	AM_RANGE( 0x1FE800, 0x1FEBFF) AM_DEVREADWRITE(C6280_TAG, c6280_r, c6280_w )
	AM_RANGE( 0x1FEC00, 0x1FEFFF) AM_READWRITE( h6280_timer_r, h6280_timer_w )
	AM_RANGE( 0x1FF000, 0x1FF3FF) AM_READWRITE( pce_joystick_r, pce_joystick_w )
	AM_RANGE( 0x1FF400, 0x1FF7FF) AM_READWRITE( h6280_irq_status_r, h6280_irq_status_w )
	AM_RANGE( 0x1FF800, 0x1FFBFF) AM_READWRITE( pce_cd_intf_r, pce_cd_intf_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( pce_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0x03) AM_READWRITE( vdc_0_r, vdc_0_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( sgx_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x000000, 0x07FFFF) AM_ROMBANK("bank1")
	AM_RANGE( 0x080000, 0x087FFF) AM_ROMBANK("bank2")
	AM_RANGE( 0x088000, 0x0CFFFF) AM_ROMBANK("bank3")
	AM_RANGE( 0x0D0000, 0x0FFFFF) AM_ROMBANK("bank4")
	AM_RANGE( 0x100000, 0x10FFFF) AM_RAM AM_BASE( &pce_cd_ram )
	AM_RANGE( 0x110000, 0x1EDFFF) AM_NOP
	AM_RANGE( 0x1EE000, 0x1EE7FF) AM_ROMBANK("bank10") AM_WRITE( pce_cd_bram_w )
	AM_RANGE( 0x1EE800, 0x1EFFFF) AM_NOP
	AM_RANGE( 0x1F0000, 0x1F7FFF) AM_RAM AM_BASE( &pce_user_ram )
	AM_RANGE( 0x1FE000, 0x1FE007) AM_READWRITE( vdc_0_r, vdc_0_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE008, 0x1FE00F) AM_READWRITE( vpc_r, vpc_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE010, 0x1FE017) AM_READWRITE( vdc_1_r, vdc_1_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE400, 0x1FE7FF) AM_READWRITE( vce_r, vce_w )
	AM_RANGE( 0x1FE800, 0x1FEBFF) AM_DEVREADWRITE(C6280_TAG, c6280_r, c6280_w )
	AM_RANGE( 0x1FEC00, 0x1FEFFF) AM_READWRITE( h6280_timer_r, h6280_timer_w )
	AM_RANGE( 0x1FF000, 0x1FF3FF) AM_READWRITE( pce_joystick_r, pce_joystick_w )
	AM_RANGE( 0x1FF400, 0x1FF7FF) AM_READWRITE( h6280_irq_status_r, h6280_irq_status_w )
	AM_RANGE( 0x1FF800, 0x1FFBFF) AM_READWRITE( pce_cd_intf_r, pce_cd_intf_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( sgx_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0x03) AM_READWRITE( sgx_vdc_r, sgx_vdc_w )
ADDRESS_MAP_END

/* todo: alternate forms of input (multitap, mouse, etc.) */
static INPUT_PORTS_START( pce )

    PORT_START("JOY")  /* Player 1 controls */
    /* II is left of I on the original pad so we map them in reverse order */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button I")
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button II")
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SELECT  )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START   ) PORT_NAME("P1 Run")
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
#if 0
    PORT_START  /* Fake dipswitches for system config */
    PORT_DIPNAME( 0x01, 0x01, "Console type")
    PORT_DIPSETTING(    0x00, "Turbo-Grafx 16")
    PORT_DIPSETTING(    0x01, "PC-Engine")

    PORT_DIPNAME( 0x01, 0x01, "Joystick type")
    PORT_DIPSETTING(    0x00, "2 Button")
    PORT_DIPSETTING(    0x01, "6 Button")
#endif
INPUT_PORTS_END


static void pce_partialhash(char *dest, const unsigned char *data,
        unsigned long length, unsigned int functions)
{
        if ( ( length <= PCE_HEADER_SIZE ) || ( length & PCE_HEADER_SIZE ) ) {
	        hash_compute(dest, &data[PCE_HEADER_SIZE], length - PCE_HEADER_SIZE, functions);
	} else {
		hash_compute(dest, data, length, functions);
	}
}

static const c6280_interface c6280_config =
{
	"maincpu"
};


static MACHINE_DRIVER_START( pce_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("pce,bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(pce_cart)
	MDRV_CARTSLOT_PARTIALHASH(pce_partialhash)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pce )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", H6280, MAIN_CLOCK/3)
	MDRV_CPU_PROGRAM_MAP(pce_mem)
	MDRV_CPU_IO_MAP(pce_io)
	MDRV_CPU_VBLANK_INT_HACK(pce_interrupt, VDC_LPF)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( pce )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(MAIN_CLOCK/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)
	/* MDRV_GFXDECODE( pce ) */
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT( vce )

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD(C6280_TAG, C6280, MAIN_CLOCK/6)
	MDRV_SOUND_CONFIG(c6280_config)
	MDRV_SOUND_ROUTE( 0, "lspeaker", 1.00 )
	MDRV_SOUND_ROUTE( 1, "rspeaker", 1.00 )

	MDRV_SOUND_ADD( "msm5205", MSM5205, PCE_CD_CLOCK / 6 )
	MDRV_SOUND_CONFIG( pce_cd_msm5205_interface )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 0.00 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 0.00 )

	MDRV_SOUND_ADD( "cdda", CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "lspeaker", 1.00 )
	MDRV_SOUND_ROUTE( 1, "rspeaker", 1.00 )

	MDRV_CDROM_ADD( "cdrom" )

	MDRV_IMPORT_FROM( pce_cartslot )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sgx )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", H6280, MAIN_CLOCK/3)
	MDRV_CPU_PROGRAM_MAP(sgx_mem)
	MDRV_CPU_IO_MAP(sgx_io)
	MDRV_CPU_VBLANK_INT_HACK(sgx_interrupt, VDC_LPF)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( pce )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(MAIN_CLOCK/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT( vce )

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD(C6280_TAG, C6280, MAIN_CLOCK/6)
	MDRV_SOUND_CONFIG(c6280_config)
	MDRV_SOUND_ROUTE(0, "lspeaker", 1.00)
	MDRV_SOUND_ROUTE(1, "rspeaker", 1.00)

	MDRV_SOUND_ADD( "msm5205", MSM5205, PCE_CD_CLOCK / 6 )
	MDRV_SOUND_CONFIG( pce_cd_msm5205_interface )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 0.00 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 0.00 )

	MDRV_SOUND_ADD( "cdda", CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "lspeaker", 1.00 )
	MDRV_SOUND_ROUTE( 1, "rspeaker", 1.00 )

	MDRV_IMPORT_FROM( pce_cartslot )
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pce )
	ROM_REGION( PCE_ROM_MAXSIZE, "user1", 0 )		/* Cartridge ROM area */
	ROM_FILL( 0, PCE_ROM_MAXSIZE, 0xFF )
ROM_END

#define rom_tg16 rom_pce
#define rom_sgx rom_pce

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT    INIT   COMPANY  FULLNAME */
CONS( 1987, pce,    0,      0,      pce,    pce,     pce,   "Nippon Electronic Company", "PC Engine", GAME_IMPERFECT_SOUND )
CONS( 1989, tg16,   pce,    0,      pce,    pce,     tg16,  "Nippon Electronic Company", "TurboGrafx 16", GAME_IMPERFECT_SOUND )
CONS( 1989, sgx,    pce,    0,      sgx,    pce,     sgx,   "Nippon Electronic Company", "SuperGrafx", GAME_IMPERFECT_SOUND )
