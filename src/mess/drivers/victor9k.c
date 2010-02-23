/***************************************************************************

    Victor 9000

	- very exciting hardware, disk controller is a direct descendant
	  of the Commodore drives (designed by Chuck Peddle)

    Skeleton driver

***************************************************************************/

#include "emu.h"
#include "includes/victor9k.h"
#include "cpu/i86/i86.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/upd7201.h"
#include "video/mc6845.h"

/* Memory Maps */

static ADDRESS_MAP_START( victor9k_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
/*	AM_RANGE(0xe0000, 0xe0001) AM_DEVREADWRITE(I8259A_TAG, pic8259_r, pic8259_w)
	AM_RANGE(0xe0020, 0xe0023) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xe0040, 0xe0043) AM_DEVREADWRITE(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)*/
	AM_RANGE(0xe8000, 0xe8000) AM_DEVREADWRITE(HD46505S_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0xe8001, 0xe8001) AM_DEVREADWRITE(HD46505S_TAG, mc6845_status_r, mc6845_address_w)
/*	AM_RANGE(0xe8020, 0xe802f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0xe8040, 0xe804f) AM_DEVREADWRITE(M6522_2_TAG, via_r, via_w)
	AM_RANGE(0xe8060, 0xe806f) AM_DEVREADWRITE(M6852_TAG, m6852_r, m6852_w)
	AM_RANGE(0xe8080, 0xe808f) AM_DEVREADWRITE(M6522_3_TAG, via_r, via_w)
	AM_RANGE(0xe80a0, 0xe80af) AM_DEVREADWRITE(M6522_4_TAG, via_r, via_w)
	AM_RANGE(0xe80c0, 0xe80cf) AM_DEVREADWRITE(M6522_6_TAG, via_r, via_w)
	AM_RANGE(0xe80e0, 0xe80ef) AM_DEVREADWRITE(M6522_5_TAG, via_r, via_w)*/
	AM_RANGE(0xf0000, 0xf0fff) AM_RAM
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION(I8088_TAG, 0)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( victor9k )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( victor9k_update_row )
{
}

static const mc6845_interface hd46505s_intf = 
{
	SCREEN_TAG,
	8,
	NULL,
	victor9k_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static VIDEO_START( victor9k )
{
	victor9k_state *state = (victor9k_state *)machine->driver_data;

	/* find devices */
	state->crt = devtag_get_device(machine, HD46505S_TAG);
}

static VIDEO_UPDATE( victor9k )
{
	victor9k_state *state = (victor9k_state *)screen->machine->driver_data;

	mc6845_update(state->crt, bitmap, cliprect);

	return 0;
}

/* Floppy Configuration */

static const floppy_config victor9k_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

/* IEEE-488 Interface */

static IEEE488_DAISY( ieee488_daisy )
{
//	{ M6522_1_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( victor9k )
{
	victor9k_state *state = (victor9k_state *)machine->driver_data;

	/* find devices */
	state->ieee488 = devtag_get_device(machine, IEEE488_TAG);
}

/* Machine Driver */

static MACHINE_DRIVER_START( victor9k )
	MDRV_DRIVER_DATA(victor9k_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8088_TAG, I8088, 5000000)
	MDRV_CPU_PROGRAM_MAP(victor9k_mem)

	MDRV_MACHINE_START(victor9k)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(monochrome_green)

	MDRV_VIDEO_START(victor9k)
	MDRV_VIDEO_UPDATE(victor9k)

	MDRV_MC6845_ADD(HD46505S_TAG, H46505, 1000000, hd46505s_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_M6852_ADD(M6852_TAG, 0, m6852_intf)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)
//	MDRV_PIC8259_ADD(I8259A_TAG, i8259_intf)
//	MDRV_PIT8253_ADD(I8253_TAG, i8253_intf)
//	MDRV_UPD7201_ADD(UPD7201_TAG, 0, upd7201_intf)
//	MDRV_VIA6522_ADD(M6522_1_TAG, 0, m6522_1_intf)
//	MDRV_VIA6522_ADD(M6522_2_TAG, 0, m6522_2_intf)
//	MDRV_VIA6522_ADD(M6522_3_TAG, 0, m6522_3_intf)
//	MDRV_VIA6522_ADD(M6522_4_TAG, 0, m6522_4_intf)
//	MDRV_VIA6522_ADD(M6522_5_TAG, 0, m6522_5_intf)
//	MDRV_VIA6522_ADD(M6522_6_TAG, 0, m6522_6_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(victor9k_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("512K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( victor9k )
	ROM_REGION( 0x2000, I8088_TAG, 0 )
	ROM_LOAD( "102320.7j", 0x0000, 0x1000, CRC(3d615fd7) SHA1(b22f7e5d66404185395d8effbf57efded0079a92) )
	ROM_LOAD( "102322.8j", 0x1000, 0x1000, CRC(9209df0e) SHA1(3ee8e0c15186bbd5768b550ecc1fa3b6b1dbb928) )
	ROM_LOAD( "v9000_univ._fe_f3f7_13db.7j", 0x0000, 0x1000, CRC(25c7a59f) SHA1(8784e9aa7eb9439f81e18b8e223c94714e033911) )
	ROM_LOAD( "v9000_univ._ff_f3f7_39fe.8j", 0x1000, 0x1000, CRC(496c7467) SHA1(eccf428f62ef94ab85f4a43ba59ae6a066244a66) )

	ROM_REGION( 0x800, "gcr", 0 )
	ROM_LOAD( "100836-01.4k", 0x000, 0x800, CRC(adc601bd) SHA1(6eeff3d2063ae2d97452101aa61e27ef83a467e5) )

	ROM_REGION( 0x800, "motor", 0)
	ROM_LOAD( "disk spindle motor control i8048", 0x000, 0x800, NO_DUMP )

	ROM_REGION( 0x800, "keyboard", 0)
	ROM_LOAD( "keyboard controller i8048", 0x000, 0x800, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY						FULLNAME		FLAGS */
COMP( 1982, victor9k, 0,      0,      victor9k, victor9k, 0,    "Victor Business Products", "Victor 9000",	GAME_NOT_WORKING | GAME_NO_SOUND )
