/*

	PROF-80

	http://www.prof80.de/

*/

#include "driver.h"
#include "includes/prof80.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "machine/upd1990a.h"

/* Memory Maps */

static ADDRESS_MAP_START( prof80_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
    AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(2)
    AM_RANGE(0x2000, 0x2fff) AM_RAMBANK(3)
    AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(4)
    AM_RANGE(0x4000, 0x4fff) AM_RAMBANK(5)
    AM_RANGE(0x5000, 0x5fff) AM_RAMBANK(6)
    AM_RANGE(0x6000, 0x6fff) AM_RAMBANK(7)
    AM_RANGE(0x7000, 0x7fff) AM_RAMBANK(8)
    AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(9)
    AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(10)
    AM_RANGE(0xa000, 0xafff) AM_RAMBANK(11)
    AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(12)
    AM_RANGE(0xc000, 0xcfff) AM_RAMBANK(13)
    AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(14)
    AM_RANGE(0xe000, 0xefff) AM_RAMBANK(15)
    AM_RANGE(0xf000, 0xffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( prof80_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0xd8, 0xd8) AM_WRITE(flr_w)
//	AM_RANGE(0xda, 0xda) AM_READ(status_r)
//	AM_RANGE(0xdb, 0xdb) AM_READ(status2_r)
//	AM_RANGE(0xdc, 0xdc) AM_READ(fdc_status_r)
//	AM_RANGE(0xdd, 0xdd) AM_DEVREADWRITE(NEC765_TAG, fdc_r, fdc_w)
//	AM_RANGE(0xde, 0xdf) AM_WRITE(mmu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM
    AM_RANGE(0x4000, 0x47ff) AM_RAM
    AM_RANGE(0x8000, 0xffff) AM_RAMBANK(17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x00, 0x00) AM_READWRITE(stb_r, stb_w)
//	AM_RANGE(0x10, 0x10) AM_WRITE(cc3_w)
//	AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//	AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
//	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//	AM_RANGE(0x14, 0x14) AM_WRITE(cc1_w)
//	AM_RANGE(0x15, 0x15) AM_WRITE(cc2_w)
//	AM_RANGE(0x16, 0x16) AM_WRITE(flash_w)
//	AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
//	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE(Z80STI_TAG, z80sti_r, z80sti_w)
//	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
//	AM_RANGE(0x40, 0x40) AM_READ(status_r)
//	AM_RANGE(0x50, 0x50) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
//	AM_RANGE(0x52, 0x52) AM_DEVWRITE(MC6845_TAG, mc6845_register_w)
//	AM_RANGE(0x53, 0x53) AM_DEVREAD(MC6845_TAG, mc6845_register_r)
//	AM_RANGE(0x60, 0x60) AM_DEVWRITE("centronics", centronics_data_w)
//	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE("ppi8255", ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( prof80 )
INPUT_PORTS_END

/* Video */

static VIDEO_START( prof80 )
{
}

static VIDEO_UPDATE( prof80 )
{
    return 0;
}

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( prof80_upd1990a_data_w )
{
	prof80_state *driver_state = device->machine->driver_data;

	driver_state->rtc_data = state;
}

static UPD1990A_INTERFACE( prof80_upd1990a_intf )
{
	DEVCB_LINE(prof80_upd1990a_data_w),
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( prof80 )
{
	prof80_state *state = machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* register for state saving */
	state_save_register_global(machine, state->rtc_data);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( prof80 )
	MDRV_DRIVER_DATA(prof80_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_6MHz)
    MDRV_CPU_PROGRAM_MAP(prof80_mem)
    MDRV_CPU_IO_MAP(prof80_io)
 
	MDRV_MACHINE_START(prof80)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(2)
 
    MDRV_VIDEO_START(prof80)
    MDRV_VIDEO_UPDATE(prof80)

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, prof80_upd1990a_intf)
//	MDRV_PPI8255_ADD(PPI8255_TAG, grip_ppi8255_interface)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( prof80 )
	ROM_REGION( 0x800, "prof80", 0 )
	ROM_DEFAULT_BIOS( "v17" )
	ROM_SYSTEM_BIOS( 0, "v15", "v1.5" )
	ROMX_LOAD( "prof80v15.z7", 0x0000, 0x0800, CRC(8f74134c) SHA1(83f9dcdbbe1a2f50006b41d406364f4d580daa1f), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v16", "v1.6" )
	ROMX_LOAD( "prof80v16.z7", 0x0000, 0x0800, CRC(7d3927b3) SHA1(bcc15fd04dbf1d6640115be595255c7b9d2a7281), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v17", "v1.7" )
	ROMX_LOAD( "prof80v17.z7", 0x0000, 0x0800, CRC(53305ff4) SHA1(3ea209093ac5ac8a5db618a47d75b705965cdf44), ROM_BIOS(3) )

	ROM_REGION( 0x10000, "grip", 0 )
	ROM_LOAD( "grip.z2", 0x0000, 0x4000, NO_DUMP )
ROM_END

/* System Configurations */

static SYSTEM_CONFIG_START( prof80 )
	CONFIG_RAM_DEFAULT	(128 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   INIT  CONFIG COMPANY  FULLNAME   FLAGS */
COMP( 1984, prof80,     0,      0, prof80, prof80, 0, prof80,  "Conitec Datensysteme", "PROF-80", GAME_NO_SOUND | GAME_NOT_WORKING)
