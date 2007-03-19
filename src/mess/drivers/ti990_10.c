/*
	TI990/10 driver

	This driver boots the DX10 build tape and build a bootable system disk.
	I have been able to run a few programs (including most games from the
	"fun and games" tape), but I have been unable to perform system generation
	and install BASIC/COBOL/PASCAL.

TODO :
* programmer panel
* emulate TILINE fully: timings, tiline timeout, possibly memory error
* finish tape emulation (write support)
* add additional devices as need appears (931 VDT, FD800, card reader, ASR/KSR, printer)
* emulate 990/10A and 990/12 CPUs?
* find out why the computer locks when executing ALGS and BASIC/COBOL/PASCAL installation
*/

/*
	CRU map:

	990/10 CPU board:
	1fa0-1fbe: map file CRU interface
	1fc0-1fde: error interrupt register
	1fe0-1ffe: control panel

	optional hardware (default configuration):
	0000-001e: 733 ASR
	0020-003e: PROM programmer
	0040-005e: card reader
	0060-007e: line printer
	0080-00be: FD800 floppy disc
	00c0-00ee: 913 VDT, or 911 VDT
	0100-013e: 913 VDT #2, or 911 VDT
	0140-017e: 913 VDT #3, or 911 VDT
	1700-177e (0b00-0b7e, 0f00-0f7e): CI402 serial controller #0 (#1, #2) (for 931/940 VDT)
		(note that CRU base 1700 is used by the integrated serial controller in newer S300,
		S300A, 990/10A (and 990/5?) systems)
	1f00-1f1e: CRU expander #1 interrupt register
	1f20-1f3e: CRU expander #2 interrupt register
	1f40-1f5e: TILINE coupler interrupt control #1-8


	TPCS map:
	1ff800: disk controller #1 (system disk)
	1ff810->1ff870: extra disk controllers #2 through #8
	1ff880 (1ff890): tape controller #1 (#2)
	1ff900->1ff950: communication controllers #1 through #6
	1ff980 (1ff990, 1ff9A0): CI403/404 serial controller #1 (#2, #3) (for 931/940 VDT)
	1ffb00, 1ffb04, etc: ECC memory controller #1, #2, etc, diagnostic
	1ffb10, 1ffb14, etc: cache memory controller #1, #2, etc, diagnostic


	interrupt map (default configuration):
	0,1,2: CPU board
	3: free
	4: card reader
	5: line clock
	6: 733 ASR/KSR
	7: FD800 floppy (or FD1000 floppy)
	8: free
	9: 913 VDT #3
	10: 913 VDT #2
	11: 913 VDT
	12: free
	13: hard disk
	14 line printer
	15: PROM programmer (actually not used)
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/ti990.h"
#include "machine/990_hd.h"
#include "machine/990_tap.h"
#include "video/911_vdt.h"

static MACHINE_RESET( ti990_10 )
{
	ti990_hold_load();

	ti990_reset_int();

	ti990_tpc_init(ti990_set_int9);
	ti990_hdc_init(ti990_set_int13);
}

static void ti990_10_line_interrupt(void)
{
	vdt911_keyboard(0);

	ti990_line_interrupt();
}

/*static void idle_callback(int state)
{
}*/

static void rset_callback(void)
{
	ti990_cpuboard_reset();

	vdt911_reset();
	/* ... */

	/* clear controller panel and smi fault LEDs */
}

static void lrex_callback(void)
{
	/* right??? */
	ti990_hold_load();
}

/*
	TI990/10 video emulation.

	We emulate a single VDT911 CRT terminal.
*/


static VIDEO_START( ti990_10 )
{
	const vdt911_init_params_t params =
	{
		char_1920,
		vdt911_model_US/*vdt911_model_UK*//*vdt911_model_French*//*vdt911_model_French*/
		/*vdt911_model_German*//*vdt911_model_Swedish*//*vdt911_model_Norwegian*/
		/*vdt911_model_Japanese*//*vdt911_model_Arabic*//*vdt911_model_FrenchWP*/,
		ti990_set_int10
	};

	return vdt911_init_term(0, & params);
}

static VIDEO_UPDATE( ti990_10 )
{
	vdt911_refresh(bitmap, 0, 0, 0);
	return 0;
}

/*
  Memory map - see description above
*/

static ADDRESS_MAP_START(ti990_10_memmap, ADDRESS_SPACE_PROGRAM, 16)

	AM_RANGE(0x000000, 0x0fffff) AM_READWRITE(MRA16_RAM, MWA16_RAM)		/* let's say we have 1MB of RAM */
	AM_RANGE(0x100000, 0x1ff7ff) AM_READWRITE(MRA16_NOP, MWA16_NOP)		/* free TILINE space */
	AM_RANGE(0x1ff800, 0x1ff81f) AM_READWRITE(ti990_hdc_r, ti990_hdc_w)	/* disk controller TPCS */
	AM_RANGE(0x1ff820, 0x1ff87f) AM_READWRITE(MRA16_NOP, MWA16_NOP)		/* free TPCS */
	AM_RANGE(0x1ff880, 0x1ff89f) AM_READWRITE(ti990_tpc_r, ti990_tpc_w)	/* tape controller TPCS */
	AM_RANGE(0x1ff8a0, 0x1ffbff) AM_READWRITE(MRA16_NOP, MWA16_NOP)		/* free TPCS */
	AM_RANGE(0x1ffc00, 0x1fffff) AM_READWRITE(MRA16_ROM, MWA16_ROM)		/* LOAD ROM */

ADDRESS_MAP_END


/*
  CRU map
*/

static ADDRESS_MAP_START(ti990_10_writecru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x80, 0x8f) AM_WRITE(vdt911_0_cru_w)

	AM_RANGE(0xfd0, 0xfdf) AM_WRITE(ti990_10_mapper_cru_w)
	AM_RANGE(0xfe0, 0xfef) AM_WRITE(ti990_10_eir_cru_w)
	AM_RANGE(0xff0, 0xfff) AM_WRITE(ti990_panel_write)

ADDRESS_MAP_END

static ADDRESS_MAP_START(ti990_10_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x10, 0x11) AM_READ(vdt911_0_cru_r)

	AM_RANGE(0x1fa, 0x1fb) AM_READ(ti990_10_mapper_cru_r)
	AM_RANGE(0x1fc, 0x1fd) AM_READ(ti990_10_eir_cru_r)
	AM_RANGE(0x1fe, 0x1ff) AM_READ(ti990_panel_read)

ADDRESS_MAP_END

static ti990_10reset_param reset_params =
{
	/*idle_callback*/NULL,
	rset_callback,
	lrex_callback,
	ti990_ckon_ckof_callback,

	ti990_set_int2
};



static MACHINE_DRIVER_START(ti990_10)

	/* basic machine hardware */
	/* TI990/10 CPU @ 4.0(???) MHz */
	MDRV_CPU_ADD(TI990_10, 4000000)
	MDRV_CPU_CONFIG(reset_params)
	MDRV_CPU_PROGRAM_MAP(ti990_10_memmap, 0)
	MDRV_CPU_IO_MAP(ti990_10_readcru, ti990_10_writecru)
	/*MDRV_CPU_VBLANK_INT(NULL, 0)*/
	MDRV_CPU_PERIODIC_INT(ti990_10_line_interrupt, TIME_IN_HZ(120)/*or TIME_IN_HZ(100) in Europe*/)

	/* video hardware - we emulate a single 911 vdt display */
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_RESET( ti990_10 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(560, 280)
	MDRV_SCREEN_VISIBLE_AREA(0, 560-1, 0, /*250*/280-1)

	MDRV_GFXDECODE(vdt911_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(vdt911_palette_size)
	MDRV_COLORTABLE_LENGTH(vdt911_colortable_size)

	MDRV_PALETTE_INIT(vdt911)
	MDRV_VIDEO_START(ti990_10)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(ti990_10)

	/* 911 VDT has a beep tone generator */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(ti990_10)

	/*CPU memory space*/
#if 0

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : older boot ROMs for floppy-disk */
	ROM_LOAD16_BYTE("975383.31", 0x1FFC00, 0x100, CRC(64fcd040))
	ROM_LOAD16_BYTE("975383.32", 0x1FFC01, 0x100, CRC(64277276))
	ROM_LOAD16_BYTE("975383.29", 0x1FFE00, 0x100, CRC(af92e7bf))
	ROM_LOAD16_BYTE("975383.30", 0x1FFE01, 0x100, CRC(b7b40cdc))

#elif 1

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : newer "universal" boot ROMs  */
	ROM_LOAD16_BYTE("975383.45", 0x1FFC00, 0x100, CRC(391943c7) SHA1(bbd4da60b221d146542a6b547ae1570024e41b8a))
	ROM_LOAD16_BYTE("975383.46", 0x1FFC01, 0x100, CRC(f40f7c18) SHA1(03613bbf2263a4335c25dfc63cf2878c06bfe280))
	ROM_LOAD16_BYTE("975383.47", 0x1FFE00, 0x100, CRC(1ba571d8) SHA1(adaa18f149b643cc842fea8d7107ee868d6ffaf4))
	ROM_LOAD16_BYTE("975383.48", 0x1FFE01, 0x100, CRC(8852b09e) SHA1(f0df2abb438716832c16ab111e475da3ae612673))

#else

	ROM_REGION16_BE(0x202000, REGION_CPU1,0)

	/* TI990/12 ROMs - actually incompatible with TI990/10, but I just wanted to disassemble them. */
	ROM_LOAD16_BYTE("ti2025-7", 0x1FFC00, 0x1000, CRC(4824f89c))
	ROM_LOAD16_BYTE("ti2025-8", 0x1FFC01, 0x1000, CRC(51fef543))
	/* the other half of this ROM is not loaded - it makes no sense as TI990/12 machine code, as
	it is microcode... */

#endif


	/* VDT911 character definitions */
	ROM_REGION(vdt911_chr_region_len, vdt911_chr_region, 0)

ROM_END

static DRIVER_INIT( ti990_10 )
{
#if 0
	/* load specific ti990/12 rom page */
	const int page = 3;

	memmove(memory_region(REGION_CPU1)+0x1FFC00, memory_region(REGION_CPU1)+0x1FFC00+(page*0x400), 0x400);
#endif
	vdt911_init();
}

INPUT_PORTS_START(ti990_10)
	VDT911_KEY_PORTS
INPUT_PORTS_END

static void ti990_10_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* harddisk */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_HARDDISK; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_ti990_hd; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti990_hd; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti990_hd; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "hd"); break;
	}
}

static void ti990_10_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_ti990_tape; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ti990_tape; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_ti990_tape; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap"); break;
	}
}

SYSTEM_CONFIG_START(ti990_10)
	CONFIG_DEVICE(ti990_10_harddisk_getinfo)
	CONFIG_DEVICE(ti990_10_cassette_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1975,	ti990_10,	0,		0,		ti990_10,	ti990_10,	ti990_10,	ti990_10,	"Texas Instruments",	"TI Model 990/10 Minicomputer System" , 0)
