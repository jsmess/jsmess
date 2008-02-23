/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Business Machines Co.

Preliminary driver by:

Ernesto Corvi

Note 1: The 'fast-mem' memory detector in Kickstart 1.2 expects to
see the custom chips at the end of any present fast memory (mapped
from $C00000 onwards). I assume this is a bug, given that the routine
was entirely rewritten for Kickstart 1.3. So the strategy is, for
any machine that can run Kickstart 1.2 (a500,a1000 and a2000), we
map a mirror of the custom chips right after any fast-mem we mapped.
If we didn't map any, then we still put a mirror, but where fast-mem
would commence ($C00000).

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/amiga.h"

/* Components */
#include "sound/custom.h"
#include "machine/amigafdc.h"
#include "machine/amigakbd.h"
#include "machine/amigacd.h"
#include "machine/amigacrt.h"
#include "machine/msm6242.h"

/* Devices */
#include "devices/chd_cd.h"
#include "devices/cartslot.h"


/***************************************************************************
  Battery Backed-Up Clock (MSM6264)
***************************************************************************/
static READ16_HANDLER( amiga_clock_r ) { return msm6242_r( offset / 2 ); }
static WRITE16_HANDLER( amiga_clock_w ) { msm6242_w( offset / 2, data ); }

/***************************************************************************
  Address maps
***************************************************************************/

static ADDRESS_MAP_START(amiga_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x07ffff) AM_MIRROR(0x80000) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xc7ffff) AM_RAM /* slow-mem */
	AM_RANGE(0xc80000, 0xcfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w)	/* see Note 1 above */
	AM_RANGE(0xdf0000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
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
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xdc0000, 0xdc003f) AM_READWRITE(amiga_clock_r, amiga_clock_w)
	AM_RANGE(0xdc8000, 0xdc87ff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xdf0000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xf00000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* CDTV & System ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START(a1000_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x03ffff) AM_MIRROR(0xc0000) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xc3ffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) /* See Note 1 above */
	AM_RANGE(0xdf0000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)	/* Custom Chips */
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xf80000, 0xfbffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* Bootstrap ROM */
	AM_RANGE(0xfc0000, 0xffffff) AM_RAMBANK(2)	/* Writable Control Store RAM */
ADDRESS_MAP_END


/***************************************************************************
  Inputs
***************************************************************************/

static INPUT_PORTS_START( amiga_common )
	PORT_START_TAG("input")
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


static INPUT_PORTS_START( amiga )
	PORT_START_TAG("hardware")
	PORT_CONFNAME( 0x08, 0x08, "Battery backed-up RTC")
	PORT_CONFSETTING( 0x00, "Not Installed" )
	PORT_CONFSETTING( 0x08, "Installed" )

	PORT_INCLUDE( amiga_common )
INPUT_PORTS_END


/* TODO: Support for the CDTV remote control */
static INPUT_PORTS_START( cdtv )
	PORT_INCLUDE( amiga_common )
INPUT_PORTS_END


/***************************************************************************
  Machine drivers
***************************************************************************/

static const struct CustomSound_interface amiga_custom_interface =
{
	amiga_sh_start
};

static MACHINE_DRIVER_START( ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, AMIGA_68000_NTSC_CLOCK)
	MDRV_CPU_PROGRAM_MAP(amiga_mem, 0)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 262)

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))

	MDRV_MACHINE_RESET( amiga )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(228*4, 262)
	MDRV_SCREEN_VISIBLE_AREA(214, (228*4)-1, 34, 262-1)
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
	MDRV_CPU_REPLACE("main", M68000, CDTV_CLOCK_X1 / 4)
	MDRV_CPU_PROGRAM_MAP(cdtv_mem, 0)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_SOUND_ADD( CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "left", 1.0 )
	MDRV_SOUND_ROUTE( 1, "right", 1.0 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a1000n )
	MDRV_IMPORT_FROM(ntsc)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(a1000_mem, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pal )
	MDRV_IMPORT_FROM(ntsc)

	/* adjust for PAL specs */
	MDRV_CPU_REPLACE("main", M68000, AMIGA_68000_PAL_CLOCK)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 312)

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_SIZE(228*4, 312)
	MDRV_SCREEN_VISIBLE_AREA(214, (228*4)-1, 34, 312-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a1000p )
	MDRV_IMPORT_FROM(pal)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(a1000_mem, 0)
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

		amiga_cart_check_overlay();
	}
	else
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, amiga_chip_ram_size - 1, 0, 0, MWA16_UNMAP);

	set_led_status( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led on Amiga */
	output_set_value("power_led", ( data & 2 ) ? 0 : 1);
}

static UINT16 amiga_read_joy0dat(void)
{
	if ( readinputportbytag("input") & 0x20 ) {
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
	if ( readinputportbytag("input") & 0x10 ) {
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

static void amiga_reset(void)
{
	if (readinputportbytag("hardware") & 0x08)
	{
		/* Install RTC */
		memory_install_readwrite16_handler(0, ADDRESS_SPACE_PROGRAM, 0xdc0000, 0xdc003f, 0, 0, amiga_clock_r, amiga_clock_w);
	}
	else
	{
		/* No RTC support */
		memory_install_readwrite16_handler(0, ADDRESS_SPACE_PROGRAM, 0xdc0000, 0xdc003f, 0, 0, MRA16_UNMAP, MWA16_UNMAP);
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
		amiga_reset,                             /* reset_callback */
		amiga_cart_nmi,                          /* nmi_callback */
		0                                        /* flags */
	};

	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);

	/* initialize cartridge (if present) */
	amiga_cart_init();

	/* initialize keyboard */
	amigakbd_init();
}

#ifdef UNUSED_FUNCTION
static DRIVER_INIT( amiga_ecs )
{
	static const amiga_machine_interface amiga_intf =
	{
		ECS_CHIP_RAM_MASK,
		amiga_cia_0_portA_r, NULL,               /* CIA0 port A & B read */
		amiga_cia_0_portA_w, NULL,               /* CIA0 port A & B write */
		NULL, NULL,                              /* CIA1 port A & B read */
		NULL, amiga_fdc_control_w,               /* CIA1 port A & B write */
		amiga_read_joy0dat,	amiga_read_joy1dat,  /* joy0dat_r & joy1dat_r */
		NULL,                                    /* potgo_w */
		amiga_read_dskbytr,	amiga_write_dsklen,  /* dskbytr_r & dsklen_w */
		NULL,                                    /* serdat_w */
		NULL,                                    /* scanline0_callback */
		amiga_reset,                             /* reset_callback */
		amiga_cart_nmi,                          /* nmi_callback */
		0                                        /* flags */
	};

	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);

	/* initialize Action Replay (if present) */
	amiga_cart_init();

	/* initialize keyboard */
	amigakbd_init();
}
#endif

static DRIVER_INIT( cdtv )
{
	static const amiga_machine_interface amiga_intf =
	{
		ECS_CHIP_RAM_MASK,
		amiga_cia_0_portA_r, NULL,               /* CIA0 port A & B read */
		amiga_cia_0_portA_w, NULL,               /* CIA0 port A & B write */
		NULL, NULL,                              /* CIA1 port A & B read */
		NULL, NULL,                              /* CIA1 port A & B write */
		amiga_read_joy0dat,	amiga_read_joy1dat,  /* joy0dat_r & joy1dat_r */
		NULL,                                    /* potgo_w */
		amiga_read_dskbytr,	amiga_write_dsklen,  /* dskbytr_r & dsklen_w */
		NULL,                                    /* serdat_w */
		NULL,                                    /* scanline0_callback */
		NULL,                                    /* reset_callback */
		NULL,                                    /* nmi_callback */
		0                                        /* flags */
	};

	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);

	/* initialize the cdrom controller */
	amigacd_init();
}

/***************************************************************************
  ROM definitions
***************************************************************************/

#define AMIGA_BIOS			\
	ROM_REGION16_BE(0x080000, REGION_USER1, 0)	\
	ROM_SYSTEM_BIOS(0, "kick13",  "Kickstart 1.3 (34.5)")	\
	ROMX_LOAD("315093-02.u6", 0x000000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785), ROM_GROUPWORD | ROM_BIOS(1))	\
	ROM_SYSTEM_BIOS(1, "kick12",  "Kickstart 1.2 (33.180)")	\
	ROMX_LOAD("315093-01.u6", 0x000000, 0x040000, CRC(a6ce1636) SHA1(11f9e62cf299f72184835b7b2a70a16333fc0d88), ROM_GROUPWORD | ROM_BIOS(2))	\
	ROM_COPY(REGION_USER1, 0x000000, 0x040000, 0x040000)	\
	ROM_SYSTEM_BIOS(2, "kick204", "Kickstart 2.04 (37.175)")	\
	ROMX_LOAD("390979-01.u6", 0x000000, 0x080000, CRC(c3bdb240) SHA1(c5839f5cb98a7a8947065c3ed2f14f5f42e334a1), ROM_GROUPWORD | ROM_BIOS(3))	/* identical to 363968.01 */	\
	ROM_SYSTEM_BIOS(3, "kick31",  "Kickstart 3.1 (40.63)")	\
	ROMX_LOAD("kick40063.u6", 0x000000, 0x080000, CRC(fc24ae0d) SHA1(3b7f1493b27e212830f989f26ca76c02049f09ca), ROM_GROUPWORD | ROM_BIOS(4))	/* part number? */	\


#define AMIGA_CART			\
	ROM_REGION16_BE(0x080000, REGION_USER2, 0)	\
	ROM_CART_LOAD(0, "rom,bin", 0x0000, 0x080000, ROM_NOMIRROR | ROM_FILL_FF | ROM_OPTIONAL)	\



ROM_START(a500n)
	AMIGA_BIOS
	AMIGA_CART
ROM_END

#define rom_a500p    rom_a500n

ROM_START(a1000n)
	ROM_REGION16_BE(0x080000, REGION_USER1, 0)
	ROM_LOAD16_BYTE("252179-01.u5n", 0x000000, 0x001000, CRC(42553bc4) SHA1(8855a97f7a44e3f62d1c88d938fee1f4c606af5b))
	ROM_LOAD16_BYTE("252180-01.u5p", 0x000001, 0x001000, CRC(8e5b9a37) SHA1(d10f1564b99f5ffe108fa042362e877f569de2c3))
ROM_END

#define rom_a1000p    rom_a1000n

ROM_START(cdtv)
	ROM_REGION16_BE(0x100000, REGION_USER1, 0)
	ROM_LOAD16_BYTE("391008-01.u34", 0x000000, 0x020000, CRC(791cb14b) SHA1(277a1778924496353ffe56be68063d2a334360e4))
	ROM_LOAD16_BYTE("391009-01.u35", 0x000001, 0x020000, CRC(accbbc2e) SHA1(41b06d1679c6e6933c3378b7626025f7641ebc5c))
	ROM_COPY(REGION_USER1, 0x000000, 0x040000, 0x040000)
	ROMX_LOAD(      "315093-02.u13", 0x080000, 0x040000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785), ROM_GROUPWORD)
	ROM_COPY(REGION_USER1, 0x080000, 0x0c0000, 0x040000)
ROM_END


/***************************************************************************
  System config
***************************************************************************/

SYSTEM_CONFIG_START(amiga)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(a1000)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END

static void cdtv_cd_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* CHD CD-ROM */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default: cdrom_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cdtv)
	CONFIG_DEVICE(cdtv_cd_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************
  Game drivers
***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY                             FULLNAME                 FLAGS */
COMP( 1985, a1000n, 0,      0,      a1000n, amiga,  amiga,  a1000,  "Commodore Business Machines Co.",  "Amiga 1000 (NTSC)",     GAME_IMPERFECT_GRAPHICS )
COMP( 1985, a1000p, a1000n, 0,      a1000p, amiga,  amiga,  a1000,  "Commodore Business Machines Co.",  "Amiga 1000 (PAL)",      GAME_IMPERFECT_GRAPHICS )
COMP( 1987, a500n,  0,      0,      ntsc,   amiga,  amiga,  amiga,  "Commodore Business Machines Co.",  "Amiga 500 (NTSC, OCS)", GAME_IMPERFECT_GRAPHICS )
COMP( 1987, a500p,  a500n,  0,      pal,    amiga,  amiga,  amiga,  "Commodore Business Machines Co.",  "Amiga 500 (PAL, OCS)",  GAME_IMPERFECT_GRAPHICS )
COMP( 1991, cdtv,   0,      0,      cdtv,   cdtv,   cdtv,   cdtv,   "Commodore Business Machines Co.",  "CDTV (NTSC)",           GAME_IMPERFECT_GRAPHICS )
