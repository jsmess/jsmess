/***************************************************************************

        Orion driver by Miodrag Milanovic

        22/04/2008 Orion Pro added
        02/04/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8085/i8085.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/mc146818.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "formats/rk_cas.h"
#include "includes/orion.h"
#include "includes/radio86.h"

/* Address maps */

/* Orion 128 */
static ADDRESS_MAP_START(orion128_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xefff ) AM_RAMBANK(1)
	AM_RANGE( 0xf000, 0xf3ff ) AM_RAMBANK(2)
    AM_RANGE( 0xf400, 0xf4ff ) AM_READWRITE(orion128_system_r,orion128_system_w)  // Keyboard and cassette
    AM_RANGE( 0xf500, 0xf5ff ) AM_READWRITE(orion128_romdisk_r,orion128_romdisk_w)
    AM_RANGE( 0xf700, 0xf7ff ) AM_READWRITE(orion128_floppy_r,orion128_floppy_w)
    AM_RANGE( 0xf800, 0xffff ) AM_ROM
    AM_RANGE( 0xf800, 0xf8ff ) AM_WRITE (orion128_video_mode_w)
    AM_RANGE( 0xf900, 0xf9ff ) AM_WRITE (orion128_memory_page_w)
    AM_RANGE( 0xfa00, 0xfaff ) AM_WRITE (orion128_video_page_w)
ADDRESS_MAP_END

/* Orion Z80 Card II */
static ADDRESS_MAP_START( orion128_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0xf8, 0xf8) AM_WRITE ( orion128_video_mode_w )
	AM_RANGE( 0xf9, 0xf9) AM_WRITE ( orion128_memory_page_w )
	AM_RANGE( 0xfa, 0xfa) AM_WRITE ( orion128_video_page_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(orionz80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_RAMBANK(1)
	AM_RANGE( 0x4000, 0xefff ) AM_RAMBANK(2)
	AM_RANGE( 0xf000, 0xf3ff ) AM_RAMBANK(3)
    AM_RANGE( 0xf400, 0xf7ff ) AM_RAMBANK(4)
    AM_RANGE( 0xf800, 0xffff ) AM_RAMBANK(5)
ADDRESS_MAP_END

/* Orion Pro */
static ADDRESS_MAP_START( orionz80_io , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x0000, 0xffff) AM_READWRITE ( orionz80_io_r, orionz80_io_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(orionpro_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_RAMBANK(1)
	AM_RANGE( 0x2000, 0x3fff ) AM_RAMBANK(2)
	AM_RANGE( 0x4000, 0x7fff ) AM_RAMBANK(3)
	AM_RANGE( 0x8000, 0xbfff ) AM_RAMBANK(4)
	AM_RANGE( 0xc000, 0xefff ) AM_RAMBANK(5)
	AM_RANGE( 0xf000, 0xf3ff ) AM_RAMBANK(6)
    AM_RANGE( 0xf400, 0xf7ff ) AM_RAMBANK(7)
    AM_RANGE( 0xf800, 0xffff ) AM_RAMBANK(8)
ADDRESS_MAP_END

static ADDRESS_MAP_START( orionpro_io , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x0000, 0xffff) AM_READWRITE ( orionpro_io_r, orionpro_io_w )
ADDRESS_MAP_END

static const cassette_config orion_cassette_config =
{
	rko_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};


/* Machine driver */
static MACHINE_DRIVER_START( orion128 )
    MDRV_CPU_ADD("maincpu", 8080, 2000000)
    MDRV_CPU_PROGRAM_MAP(orion128_mem)
    MDRV_CPU_IO_MAP(orion128_io)

    MDRV_MACHINE_START( orion128 )
    MDRV_MACHINE_RESET( orion128 )

	MDRV_I8255A_ADD( "ppi8255_1", orion128_ppi8255_interface_1 )

	MDRV_I8255A_ADD( "ppi8255_2", radio86_ppi8255_interface_1 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(18)
	MDRV_PALETTE_INIT( orion128 )


	MDRV_VIDEO_START(orion128)
    MDRV_VIDEO_UPDATE(orion128)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CASSETTE_ADD( "cassette", orion_cassette_config )
	
	MDRV_WD1793_ADD("wd1793", default_wd17xx_interface )	
	
	MDRV_CARTSLOT_ADD("cart")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( orion128ms )
	MDRV_IMPORT_FROM(orion128)
	MDRV_DEVICE_REMOVE("ppi8255_2")
	MDRV_I8255A_ADD( "ppi8255_2", rk7007_ppi8255_interface )	
MACHINE_DRIVER_END	

static const ay8910_interface orionz80_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

static MACHINE_DRIVER_START( orionz80 )
    MDRV_CPU_ADD("maincpu", Z80, 2500000)
    MDRV_CPU_PROGRAM_MAP(orionz80_mem)
    MDRV_CPU_IO_MAP(orionz80_io)
    MDRV_CPU_VBLANK_INT("screen",orionz80_interrupt)

    MDRV_MACHINE_START( orionz80 )
    MDRV_MACHINE_RESET( orionz80 )

	MDRV_I8255A_ADD( "ppi8255_1", orion128_ppi8255_interface_1 )

	MDRV_I8255A_ADD( "ppi8255_2", radio86_ppi8255_interface_1 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(18)
	MDRV_PALETTE_INIT( orion128 )

	MDRV_VIDEO_START(orion128)
	MDRV_VIDEO_UPDATE(orion128)

	MDRV_NVRAM_HANDLER( mc146818 )

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("ay8912", AY8912, 1773400)
	MDRV_SOUND_CONFIG(orionz80_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", orion_cassette_config )
	
	MDRV_WD1793_ADD("wd1793", default_wd17xx_interface )		
	
	MDRV_CARTSLOT_ADD("cart")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( orionz80ms )
	MDRV_IMPORT_FROM(orionz80)

	MDRV_DEVICE_REMOVE("ppi8255_2")
	MDRV_I8255A_ADD( "ppi8255_2", rk7007_ppi8255_interface )	
MACHINE_DRIVER_END	

static MACHINE_DRIVER_START( orionpro )
    MDRV_CPU_ADD("maincpu", Z80, 5000000)
    MDRV_CPU_PROGRAM_MAP(orionpro_mem)
    MDRV_CPU_IO_MAP(orionpro_io)

    MDRV_MACHINE_START( orionpro )
    MDRV_MACHINE_RESET( orionpro )

	MDRV_I8255A_ADD( "ppi8255_1", orion128_ppi8255_interface_1 )

	MDRV_I8255A_ADD( "ppi8255_2", radio86_ppi8255_interface_1 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(18)
	MDRV_PALETTE_INIT( orion128 )

	MDRV_VIDEO_START(orion128)
    MDRV_VIDEO_UPDATE(orion128)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("ay8912", AY8912, 1773400)
	MDRV_SOUND_CONFIG(orionz80_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", orion_cassette_config )
	
	MDRV_WD1793_ADD("wd1793", default_wd17xx_interface )
	
	MDRV_CARTSLOT_ADD("cart")
MACHINE_DRIVER_END

static FLOPPY_OPTIONS_START(orion)
	FLOPPY_OPTION(orion, "odi,img", "Orion disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([5])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(orion, "odi,img", "Lucksian Key Orion disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static void orion_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_orion; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(orion128)
	CONFIG_RAM_DEFAULT(256 * 1024)
	CONFIG_DEVICE(orion_floppy_getinfo);
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(orionz80)
	CONFIG_RAM_DEFAULT(512 * 1024)
	CONFIG_DEVICE(orion_floppy_getinfo);
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(orionpro)
	CONFIG_RAM_DEFAULT(512 * 1024)
	CONFIG_DEVICE(orion_floppy_getinfo);
SYSTEM_CONFIG_END

/* ROM definition */

ROM_START( orion128 )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_SYSTEM_BIOS( 0, "m2rk", "Version 3.2 rk" )
    ROMX_LOAD( "m2rk.bin",    0x0f800, 0x0800, CRC(2025c234) SHA1(caf86918629be951fe698cddcdf4589f07e2fb96), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "m2_2rk", "Version 3.2.2 rk" )
    ROMX_LOAD( "m2_2rk.bin",  0x0f800, 0x0800, CRC(fc662351) SHA1(7c6de67127fae5869281449de1c503597c0c058e), ROM_BIOS(2) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionms )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "ms7007.bin",   0x0f800, 0x0800, CRC(c6174ba3) SHA1(8f9a42c3e09684718fe4121a8408e7860129d26f) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionz80 )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_SYSTEM_BIOS( 0, "m31", "Version 3.1" )
    ROMX_LOAD( "m31.bin",     0x0f800, 0x0800, CRC(007c6dc6) SHA1(338ff95497c820338f7f79c75f65bc540a5541c4), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "m32zrk", "Version 3.2 zrk" )
    ROMX_LOAD( "m32zrk.bin",  0x0f800, 0x0800, CRC(4ec3f012) SHA1(6b0b2bfc515a80e7caf72c3c33cf2dcf192d4711), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "m33zrkd", "Version 3.3 zrkd" )
    ROMX_LOAD( "m33zrkd.bin", 0x0f800, 0x0800, CRC(f404032d) SHA1(088cd9ed05f0dda4fa0a005c609208d9f57ad3d9), ROM_BIOS(3) )
    ROM_SYSTEM_BIOS( 3, "m34zrk", "Version 3.4 zrk" )
    ROMX_LOAD( "m34zrk.bin",  0x0f800, 0x0800, CRC(787c3903) SHA1(476c1c0b88e5efb582292eebec15e24d054c8851), ROM_BIOS(4) )
    ROM_SYSTEM_BIOS( 4, "m35zrkd", "Version 3.5 zrkd" )
    ROMX_LOAD( "m35zrkd.bin", 0x0f800, 0x0800, CRC(9368b38f) SHA1(64a77f22119d40c9b18b64d78ad12acc6fff9efb), ROM_BIOS(5) )
    ROM_SYSTEM_BIOS( 5, "peter", "Peterburg '91" )
    ROMX_LOAD( "peter.bin",   0x0f800, 0x0800, CRC(df9b1d8c) SHA1(c7f1e074e58ad1c1799cf522161b4f4cffa5aefa), ROM_BIOS(6) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionide )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "m35zrkh.bin", 0x0f800, 0x0800, CRC(b7745f28) SHA1(c3bd3e662db7ec56ecbab54bf6b3a4c26200d0bb) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionzms )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_SYSTEM_BIOS( 0, "m32zms", "Version 3.2 zms" )
    ROMX_LOAD( "m32zms.bin",  0x0f800, 0x0800, CRC(44cfd2ae) SHA1(84d53fbc249938c56be76ee4e6ab297f0461835b), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "m34zms", "Version 3.4 zms" )
    ROMX_LOAD( "m34zms.bin",  0x0f800, 0x0800, CRC(0f87a80b) SHA1(ab1121092e61268d8162ed8a7d4fd081016a409a), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "m35zmsd", "Version 3.5 zmsd" )
    ROMX_LOAD( "m35zmsd.bin", 0x0f800, 0x0800, CRC(f714ff37) SHA1(fbe9514adb3384aff146cbedd4fede37ce9591e1), ROM_BIOS(3) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionidm )
    ROM_REGION( 0x30000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "m35zmsh.bin", 0x0f800, 0x0800, CRC(01e66df4) SHA1(8c785a3c32fe3eacda73ec79157b41a6e4b63ba8) )
    ROM_CART_LOAD("cart", 0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

ROM_START( orionpro )
	ROM_REGION( 0x32000, "maincpu", ROMREGION_ERASEFF )
    ROM_CART_LOAD("cart",   0x10000, 0x10000, ROM_FILL_FF | ROM_OPTIONAL)
	ROM_SYSTEM_BIOS( 0, "ver21", "Version 2.1" )
    ROMX_LOAD( "rom1-210.bin", 0x20000, 0x2000,  CRC(8e1a0c78) SHA1(61c8a5ed596ce7e3fd32da920dcc80dc5375b421), ROM_BIOS(1) )
	ROMX_LOAD( "rom2-210.bin", 0x22000, 0x10000, CRC(7cb7a49b) SHA1(601f3dd61db323407c4874fd7f23c10dccac0209), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "ver20", "Version 2.0" )
    ROMX_LOAD( "rom1-200.bin", 0x20000, 0x2000,  CRC(4fbe83cc) SHA1(9884d43770b4c0fbeb519b96618b01957c0b8511), ROM_BIOS(2) )
	ROMX_LOAD( "rom2-200.bin", 0x22000, 0x10000, CRC(618aaeb7) SHA1(3e7e5d3ff9d2c683708928558e69aa62db877811), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "ver10", "Version 1.0" )
    ROMX_LOAD( "rom1-100.bin", 0x20000, 0x2000, CRC(4fd6c408) SHA1(b0c2e4fb5be5a74a7efa9bba14b746865122af1d), ROM_BIOS(3) )
	ROMX_LOAD( "rom2-100.bin", 0x22000, 0x8000, CRC(370ffdca) SHA1(169e2acac2d0b382e2d0a144da0af18bfa38db5c), ROM_BIOS(3) )
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT           INIT    CONFIG  COMPANY              FULLNAME   FLAGS */
COMP( 1990, orion128, 	 0,  		0,		orion128, 	radio86, 	orion128, orion128,  "", 					 "Orion 128",	 0)
COMP( 1990, orionms, 	 orion128, 	0,		orion128ms,	ms7007, 	orion128, orion128,  "", 					 "Orion 128 (MS7007)",	 0)
COMP( 1990, orionz80, 	 orion128, 	0,		orionz80, 	radio86,	orion128, orionz80,  "", 					 "Orion 128 + Z80 Card II",	 0)
COMP( 1990, orionide, 	 orion128, 	0,		orionz80, 	radio86,	orion128, orionz80,  "", 					 "Orion 128 + Z80 Card II + IDE",	 0)
COMP( 1990, orionzms, 	 orion128, 	0,		orionz80ms,	ms7007, 	orion128, orionz80,  "", 					 "Orion 128 + Z80 Card II (MS7007)",	 0)
COMP( 1990, orionidm, 	 orion128, 	0,		orionz80ms,	ms7007, 	orion128, orionz80,  "", 					 "Orion 128 + Z80 Card II + IDE (MS7007)",	 0)
COMP( 1994, orionpro, 	 orion128, 	0,		orionpro, 	radio86, 	orion128, orionpro,  "", 					 "Orion Pro",	 0)
