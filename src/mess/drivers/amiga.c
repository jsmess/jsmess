/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "sound/custom.h"
#include "includes/amiga.h"
#include "machine/amigafdc.h"
#include "machine/amigakbd.h"
#include "devices/chd_cd.h"
#include "inputx.h"

/***************************************************************************
  Address maps
***************************************************************************/

static ADDRESS_MAP_START(amiga_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x07ffff) AM_MIRROR(0x80000) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram) AM_SIZE(&amiga_chip_ram_size)
#if AMIGA_ACTION_REPLAY_1
	AM_RANGE(0x9fc000, 0x9fffff) AM_RAMBANK(2) AM_BASE(&amiga_ar_ram) AM_SIZE(&amiga_ar_ram_size)
#endif
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xc7ffff) AM_RAM
	AM_RANGE(0xc80000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
#if AMIGA_ACTION_REPLAY_1
	AM_RANGE(0xf00000, 0xf7ffff) AM_ROM AM_REGION(REGION_USER2, 0)	/* Cart ROM */
#else
	AM_RANGE(0xf00000, 0xf7ffff) AM_NOP
#endif
	AM_RANGE(0xf80000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* System ROM - mirror */
ADDRESS_MAP_END

/*
 * CDTV memory map (source: http://www.l8r.net/technical/cdtv-technical.html)
 * 
 * 000000-0FFFFF Chip memory
 * 100000-1FFFFF Space for extra chip memory (Megachip)
 * 200000-9FFFFF Space for AutoConfig memory
 * A00000-BFFFFF CIA chips
 * C00000-C7FFFF Space for slow-fast memory
 * C80000-DBFFFF Space
 * DC0000-DC7FFF Power backed-up real time clock
 * DC8000-DC87FF Non-volatile RAM
 * DC8800-DCFFFF Space in non-volatile RAM decoded area
 * DD0000-DEFFFF Space
 * DF0000-DFFFFF Custom chips
 * E00000-E7FFFF Memory card address space for front panel memory card
 * E80000-E8FFFF AutoConfig configuration space
 * E90000-E9FFFF First AutoConfig device, used by DMAC
 * EA0000-EFFFFF Space for other AutoConfig devices
 * F00000-F3FFFF CDTV ROM
 * F40000-F7FFFF Space in CDTV ROM decoded area
 * F80000-FBFFFF Space in Kickstart ROM decoded area (used by Kickstart 2)
 * FC0000-FFFFFF Kickstart ROM
 * 
 */

static ADDRESS_MAP_START(cdtv_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x0fffff) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0x100000, 0x9fffff) AM_NOP
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xdeffff) AM_NOP
	AM_RANGE(0xdf0000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xf00000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* CDTV & System ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START(a1000_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x03ffff) AM_MIRROR(0xc0000) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xdf0000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xf80000, 0xfbffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* Bootstrap ROM */
	AM_RANGE(0xfc0000, 0xffffff) AM_RAMBANK(2)	/* Kickstart RAM */
ADDRESS_MAP_END

/***************************************************************************
  Inputs
***************************************************************************/

INPUT_PORTS_START( amiga )
	PORT_START_TAG("config")
	PORT_CONFNAME( 0x20, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x20, DEF_STR(Joystick) )
	PORT_CONFNAME( 0x10, 0x10, "Input Port 1 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x10, DEF_STR(Joystick) )
	
	PORT_START_TAG("CIA0PORTA")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("JOY0DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P1JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("JOY1DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P2JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("POTGO")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xaaff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P1JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START_TAG("P2JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	
	PORT_START_TAG("P0MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)	
	
	PORT_START_TAG("P0MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)

	PORT_START_TAG("P1MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(2)	
	
	PORT_START_TAG("P1MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(2)
	
	PORT_INCLUDE( amiga_keyboard )
		
INPUT_PORTS_END

/***************************************************************************
  Machine drivers
***************************************************************************/

static struct CustomSound_interface amiga_custom_interface =
{
	amiga_sh_start
};

static MACHINE_DRIVER_START( ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(amiga_mem, 0)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 262)
	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( amiga )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512*2, 262)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 244+8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_PALETTE_INIT( amiga )

	MDRV_VIDEO_START(amiga)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, 3579545)
	MDRV_SOUND_CONFIG(amiga_custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
	MDRV_SOUND_ROUTE(2, "right", 0.50)
	MDRV_SOUND_ROUTE(3, "left", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cdtv )
	MDRV_IMPORT_FROM(ntsc)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(cdtv_mem, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a1000 )
	MDRV_IMPORT_FROM(ntsc)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(a1000_mem, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pal )
	MDRV_IMPORT_FROM(ntsc)

	/* adjust for PAL specs */
	MDRV_CPU_REPLACE("main", M68000, 7093790)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 312)

	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_SCREEN_SIZE(512*2, 312)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 300+8-1)
MACHINE_DRIVER_END

/***************************************************************************

  Amiga specific stuff

***************************************************************************/

static UINT8 amiga_cia_0_portA_r( void )
{
	UINT8 ret = readinputportbytag("CIA0PORTA") & 0xc0;	/* Gameport 1 and 0 buttons */
	ret |= amiga_fdc_status_r();
	return ret;
}

static void amiga_cia_0_portA_w( UINT8 data )
{
	/* switch banks as appropriate */
	memory_set_bank(1, data & 1);

	/* swap the write handlers between ROM and bank 1 based on the bit */
	if ((data & 1) == 0) {
		UINT32 mirror_mask = amiga_chip_ram_size;
		
		while( (mirror_mask<<1) < 0x100000 ) {
			mirror_mask |= ( mirror_mask << 1 );
		}
		
		/* overlay disabled, map RAM on 0x000000 */
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, amiga_chip_ram_size - 1, 0, mirror_mask, MWA16_BANK1);
	}
	else
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_ROM);

	set_led_status( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led on Amiga */		
}

static UINT16 amiga_read_joy0dat(void)
{
	if ( readinputportbytag("config") & 0x20 ) {
		/* Joystick */
		return readinputportbytag_safe("JOY0DAT", 0xffff);
	} else {
		/* Mouse */
		int input;
		input  = ( readinputportbytag("P0MOUSEX") & 0xff );
		input |= ( readinputportbytag("P0MOUSEY") & 0xff ) << 8;
		return input;
	}
}

static UINT16 amiga_read_joy1dat(void)
{
	if ( readinputportbytag("config") & 0x10 ) {
		/* Joystick */
		return readinputportbytag_safe("JOY1DAT", 0xffff);
	} else {
		/* Mouse */
		int input;
		input  = ( readinputportbytag("P1MOUSEX") & 0xff );
		input |= ( readinputportbytag("P1MOUSEY") & 0xff ) << 8;
		return input;
	}
}

static UINT16 amiga_read_dskbytr(void)
{
	return amiga_fdc_get_byte();
}

static void amiga_write_dsklen(UINT16 data)
{
	if ( data & 0x8000 ) {
		if ( CUSTOM_REG(REG_DSKLEN) & 0x8000 )
			amiga_fdc_setup_dma();
	}
}

static DRIVER_INIT( amiga )
{
	static const amiga_machine_interface amiga_intf =
	{
		ANGUS_CHIP_RAM_MASK,
		amiga_cia_0_portA_r, NULL,               /* CIA0 port A & B read */
		amiga_cia_0_portA_w, NULL,               /* CIA0 port A & B write */
		NULL, NULL,                              /* CIA1 port A & B read */
		NULL, amiga_fdc_control_w,               /* CIA1 port A & B write */
		amiga_read_joy0dat,	amiga_read_joy1dat,  /* joy0dat_r & joy1dat_r */
		NULL,                                    /* potgo_w */
		amiga_read_dskbytr,	amiga_write_dsklen,  /* dskbytr_r & dsklen_w */
		NULL,                                    /* serdat_w */
		NULL,                                    /* scanline0_callback */
		NULL                                     /* reset_callback */
	};

	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);
	
	/* initialize keyboard */
	amigakbd_init();
}

static DRIVER_INIT( cdtv )
{
	static const amiga_machine_interface amiga_intf =
	{
		FAT_ANGUS_CHIP_RAM_MASK,
		amiga_cia_0_portA_r, NULL,               /* CIA0 port A & B read */
		amiga_cia_0_portA_w, NULL,               /* CIA0 port A & B write */
		NULL, NULL,                              /* CIA1 port A & B read */
		NULL, NULL,                              /* CIA1 port A & B write */
		amiga_read_joy0dat,	amiga_read_joy1dat,  /* joy0dat_r & joy1dat_r */
		NULL,                                    /* potgo_w */
		amiga_read_dskbytr,	amiga_write_dsklen,  /* dskbytr_r & dsklen_w */
		NULL,                                    /* serdat_w */
		NULL,                                    /* scanline0_callback */
		NULL                                     /* reset_callback */
	};

	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);
}

/***************************************************************************
  ROM loading
***************************************************************************/

SYSTEM_BIOS_START(amiga)
	SYSTEM_BIOS_ADD(0, "kick13",  "Kickstart 1.3 (34.5)")
	SYSTEM_BIOS_ADD(1, "kick12",  "Kickstart 1.2 (33.180)")
	SYSTEM_BIOS_ADD(2, "kick204", "Kickstart 2.04 (37.175)")
	SYSTEM_BIOS_ADD(3, "kick31",  "Kickstart 3.1 (40.63)")
SYSTEM_BIOS_END

ROM_START(a500n)
	ROM_REGION16_BE(0x080000, REGION_USER1, 0)
	ROMX_LOAD("315093.01", 0x000000, 0x040000, CRC(a6ce1636) SHA1(11f9e62cf299f72184835b7b2a70a16333fc0d88), ROM_GROUPWORD | ROM_BIOS(2))
	ROMX_LOAD("315093.02", 0x000000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785), ROM_GROUPWORD | ROM_BIOS(1))
	ROM_COPY(REGION_USER1, 0x000000, 0x040000, 0x040000)
	ROMX_LOAD("390979.01", 0x000000, 0x080000, CRC(c3bdb240) SHA1(c5839f5cb98a7a8947065c3ed2f14f5f42e334a1), ROM_GROUPWORD | ROM_BIOS(3))	/* identical to 363968.01 */
	ROMX_LOAD("kick40063", 0x000000, 0x080000, CRC(fc24ae0d) SHA1(3b7f1493b27e212830f989f26ca76c02049f09ca), ROM_GROUPWORD | ROM_BIOS(4))	/* part number? */

#if AMIGA_ACTION_REPLAY_1
	ROM_REGION16_BE(0x080000, REGION_USER2, 0)
	ROM_LOAD_OPTIONAL("ar1.bin", 0x000000, 0x010000, CRC(f82c4258) SHA1(843b433b2c56640e045d5fdc854dc6b1a4964e7c))
#endif
ROM_END

ROM_START(a500p)
	ROM_REGION16_BE(0x080000, REGION_USER1, 0)
	ROMX_LOAD("315093.01", 0x000000, 0x040000, CRC(a6ce1636) SHA1(11f9e62cf299f72184835b7b2a70a16333fc0d88), ROM_GROUPWORD | ROM_BIOS(2))
	ROMX_LOAD("315093.02", 0x000000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785), ROM_GROUPWORD | ROM_BIOS(1))
	ROM_COPY(REGION_USER1, 0x000000, 0x040000, 0x040000)
	ROMX_LOAD("390979.01", 0x000000, 0x080000, CRC(c3bdb240) SHA1(c5839f5cb98a7a8947065c3ed2f14f5f42e334a1), ROM_GROUPWORD | ROM_BIOS(3))	/* identical to 363968.01 */
	ROMX_LOAD("kick40063", 0x000000, 0x080000, CRC(fc24ae0d) SHA1(3b7f1493b27e212830f989f26ca76c02049f09ca), ROM_GROUPWORD | ROM_BIOS(4))	/* part number? */

#if AMIGA_ACTION_REPLAY_1
	ROM_REGION16_BE(0x080000, REGION_USER2, 0)
	ROM_LOAD_OPTIONAL("ar1.bin", 0x000000, 0x010000, CRC(f82c4258) SHA1(843b433b2c56640e045d5fdc854dc6b1a4964e7c))
#endif
ROM_END

ROM_START(a1000n)
	ROM_REGION16_BE(0x080000, REGION_USER1, 0)
	ROMX_LOAD("a1000.bin", 0x000000, 0x002000, CRC(62f11c04) SHA1(c87f9fada4ee4e69f3cca0c36193be822b9f5fe6), ROM_GROUPWORD)
ROM_END

ROM_START(cdtv)
	ROM_REGION16_BE(0x100000, REGION_USER1, 0)
	ROM_LOAD16_BYTE("391008.01", 0x000000, 0x020000, CRC(791cb14b) SHA1(277a1778924496353ffe56be68063d2a334360e4))
	ROM_LOAD16_BYTE("391009.01", 0x000001, 0x020000, CRC(accbbc2e) SHA1(41b06d1679c6e6933c3378b7626025f7641ebc5c))
	ROM_COPY(REGION_USER1, 0x000000, 0x040000, 0x040000)
	ROMX_LOAD(      "315093.02", 0x080000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785), ROM_GROUPWORD)    
	ROM_COPY(REGION_USER1, 0x080000, 0x0c0000, 0x040000)
ROM_END

/***************************************************************************
  System config
***************************************************************************/

SYSTEM_CONFIG_START(amiga)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END

static void cdtv_cd_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* CHD CD-ROM */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default: cdrom_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cdtv)
	CONFIG_DEVICE(cdtv_cd_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************
  Game drivers
***************************************************************************/

/*     YEAR  NAME      PARENT   BIOS     COMPAT   MACHINE  INPUT    INIT     CONFIG   COMPANY                             FULLNAME             FLAGS */
COMP(  1985, a1000n,   0,                0,       a1000,   amiga,   amiga,   amiga,   "Commodore Business Machines Co.",  "Commodore Amiga 1000 (NTSC-OCS)", GAME_COMPUTER | GAME_NOT_WORKING )
COMPB( 1987, a500n,    0,       amiga,   0,       ntsc,    amiga,   amiga,   amiga,   "Commodore Business Machines Co.",  "Commodore Amiga 500 (NTSC-OCS)", GAME_COMPUTER | GAME_IMPERFECT_GRAPHICS )
COMPB( 1987, a500p,    a500n,   amiga,   0,       pal,     amiga,   amiga,   amiga,   "Commodore Business Machines Co.",  "Commodore Amiga 500 (PAL-OCS)", GAME_COMPUTER | GAME_IMPERFECT_GRAPHICS )
COMP(  1991, cdtv,     0,                0,       cdtv,    amiga,   cdtv,    cdtv,    "Commodore Business Machines Co.",  "Commodore Amiga CDTV (NTSC)", GAME_NOT_WORKING )
