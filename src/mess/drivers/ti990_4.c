/*
	ti990/4 driver

	We emulate a basic ti990/4 board, with a FD800 disk controller and an 733
	ASR terminal.  A little floppy-based software for this computer is
	available thanks to efforts by Dave Pitts: mostly, Forth and TX990.


	Board setup options:
	8kb of DRAM (onboard option, with optional parity): base 0x0000 or 0x2000
	4 banks of 512 bytes of ROM or SRAM: base 0x0000, 0x0800, 0xF000 or 0xF800
	power-up vector: 0x0000 (level 0) or 0xFFFC (load)
	optional memerr interrupt (level 2)
	optional power fail interrupt (level 1)
	optional real-time clock interrupt (level 5 or 7)


	Setup for the emulated system:
	0x0000: 8kb on-board DRAM + 24kb extension RAM (total 32kb)
	0xF800: 512 bytes SRAM
	0xFA00: 512 bytes SRAM (or empty?)
	0xFC00: 512 bytes self-test ROM
	0xFE00: 512 bytes loader ROM
	power-up vector: 0xFFFC (load)

	Note that only interrupt levels 3-7 are supported by the board (8-15 are not wired).

TODO:
* finish ASR emulation
* programmer panel
* emulate other devices: card reader, printer

*/

/*if 1, use 911 VDT; if 0, use 733 ASR  */
#define VIDEO_911 0

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/ti990.h"
#include "machine/990_dk.h"
#if VIDEO_911
#include "video/911_vdt.h"
#else
#include "video/733_asr.h"
#endif
#include "devices/mflopimg.h"

static MACHINE_RESET(ti990_4)
{
	ti990_hold_load();

	ti990_reset_int();

#if !VIDEO_911
	asr733_reset(0);
#endif

	fd800_machine_init(ti990_set_int7);
}


static void ti990_4_line_interrupt(void)
{
#if VIDEO_911
	vdt911_keyboard(0);
#else
	asr733_keyboard(0);
#endif

	ti990_line_interrupt();
}

/*static void idle_callback(int state)
{
}*/

static WRITE8_HANDLER ( rset_callback )
{
	ti990_cpuboard_reset();

#if VIDEO_911
	vdt911_reset();
#endif
	/* ... */

	/* clear controller panel and smi fault LEDs */
}

static WRITE8_HANDLER ( ckon_ckof_callback )
{
	ti990_ckon_ckof_callback((offset & 0x1000) ? 1 : 0);
}

static WRITE8_HANDLER ( lrex_callback )
{
	/* right??? */
	ti990_hold_load();
}

#if VIDEO_911

/*
	TI990/4 video emulation.

	We emulate a single VDT911 CRT terminal.
*/

static int video_start_ti990_4(void)
{
	const vdt911_init_params_t params =
	{
		char_1920,
		vdt911_model_US,
		ti990_set_int3
	};

	return vdt911_init_term(0, & params);
}

static VIDEO_UPDATE( ti990_4 )
{
	vdt911_refresh(bitmap, 0, 0, 0);
	return 0;
}

#else

static VIDEO_START( ti990_4 )
{
	return asr733_init_term(0, ti990_set_int6);
}

static VIDEO_UPDATE( ti990_4 )
{
	asr733_refresh(bitmap, 0, 0, 0);
	return 0;
}

#endif

/*
	Memory map - see description above
*/

static ADDRESS_MAP_START(ti990_4_memmap, ADDRESS_SPACE_PROGRAM, 16)

	AM_RANGE(0x0000, 0x7fff) AM_READWRITE(MRA16_RAM, MWA16_RAM)	/* dynamic RAM */
	AM_RANGE(0x8000, 0xf7ff) AM_READWRITE(MRA16_NOP, MWA16_NOP)	/* reserved for expansion */
	AM_RANGE(0xf800, 0xfbff) AM_READWRITE(MRA16_RAM, MWA16_RAM)	/* static RAM? */
	AM_RANGE(0xfc00, 0xffff) AM_READWRITE(MRA16_ROM, MWA16_ROM)	/* LOAD ROM */

ADDRESS_MAP_END


/*
	CRU map

	0x000-0xF7F: user devices
	0xF80-0xF9F: CRU interrupt + expansion control
	0xFA0-0xFAF: TILINE coupler interrupt control
	0xFB0-0xFCF: reserved
	0xFD0-0xFDF: memory mapping and memory protect
	0xFE0-0xFEF: internal interrupt control
	0xFF0-0xFFF: front panel

	Default user map:
	0x000-0x00f: 733 ASR (int 6)
	0x010-0x01f: PROM programmer (wired to int 15, unused)
	0x020-0x02f: 804 card reader (int 4)
	0x030-0x03f: line printer (wired to int 14, unused)
	0x040-0x05f: FD800 floppy controller (int 7)
	0x060-0x07f: VDT1 (int 3 - wired to int 11, unused)
	0x080-0x09f: VDT2, or CRU expansion (int ??? - wired to int 10, unused)
	0x0a0-0x0bf: VDT3 (int ??? - wired to int 9, unused)
*/

static ADDRESS_MAP_START(ti990_4_writecru, ADDRESS_SPACE_IO, 8)

#if VIDEO_911
	AM_RANGE(0x80, 0x8f) AM_WRITE(vdt911_0_cru_w)
#else
	AM_RANGE(0x00, 0x0f) AM_WRITE(asr733_0_cru_w)
#endif

	AM_RANGE(0x40, 0x5f) AM_WRITE(fd800_cru_w)

	AM_RANGE(0xff0, 0xfff) AM_WRITE(ti990_panel_write)

	/* external instruction decoding */
/*	AM_RANGE(0x2000, 0x2fff) AM_WRITE(idle_callback)*/
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(rset_callback)
	AM_RANGE(0x5000, 0x6fff) AM_WRITE(ckon_ckof_callback)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(lrex_callback)

ADDRESS_MAP_END

static ADDRESS_MAP_START(ti990_4_readcru, ADDRESS_SPACE_IO, 8)

#if VIDEO_911
	AM_RANGE(0x10, 0x11) AM_READ(vdt911_0_cru_r)
#else
	AM_RANGE(0x00, 0x01) AM_READ(asr733_0_cru_r)
#endif

	AM_RANGE(0x08, 0x0b) AM_READ(fd800_cru_r)

	AM_RANGE(0x1fe, 0x1ff) AM_READ(ti990_panel_read)

ADDRESS_MAP_END

/*static tms9900reset_param reset_params =
{
	idle_callback
};*/

static MACHINE_DRIVER_START(ti990_4)

	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0(???) MHz */
	MDRV_CPU_ADD(TMS9900, 3000000)
	/*MDRV_CPU_CONFIG(reset_params)*/
	MDRV_CPU_PROGRAM_MAP(ti990_4_memmap, 0)
	MDRV_CPU_IO_MAP(ti990_4_readcru, ti990_4_writecru)
	/*MDRV_CPU_VBLANK_INT(NULL, 0)*/
	MDRV_CPU_PERIODIC_INT(ti990_4_line_interrupt, TIME_IN_HZ(120)/*or TIME_IN_HZ(100) in Europe*/)

	/* video hardware - we emulate a single 911 vdt display */
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_RESET( ti990_4 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(num, den)*/
#if VIDEO_911
	MDRV_SCREEN_SIZE(560, 280)
	MDRV_SCREEN_VISIBLE_AREA(0, 560-1, 0, /*250*/280-1)
#else
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
#endif

#if VIDEO_911
	MDRV_GFXDECODE(vdt911_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(vdt911_palette_size)
	MDRV_COLORTABLE_LENGTH(vdt911_colortable_size)
#else
	MDRV_GFXDECODE(asr733_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(asr733_palette_size)
	MDRV_COLORTABLE_LENGTH(asr733_colortable_size)
#endif

#if VIDEO_911
	MDRV_PALETTE_INIT(vdt911)
#else
	MDRV_PALETTE_INIT(asr733)
#endif
	MDRV_VIDEO_START(ti990_4)
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(ti990_4)

#if VIDEO_911
	/* 911 VDT has a beep tone generator */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(ti990_4)
	/*CPU memory space*/
	ROM_REGION16_BE(0x10000, REGION_CPU1,0)

#if 0
	/* ROM set 945121-5: "733 ASR ROM loader with self test (prototyping)"
	(cf 945401-9701 pp. 1-19) */

	/* test ROM */
	ROMX_LOAD("94519209.u39", 0xFC00, 0x100, CRC(0a0b0c42), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519210.u55", 0xFC00, 0x100, CRC(d078af61), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD("94519211.u61", 0xFC01, 0x100, CRC(6cf7d4a0), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519212.u78", 0xFC01, 0x100, CRC(d9522458), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))

	/* LOAD ROM */
	ROMX_LOAD("94519113.u3", 0xFE00, 0x100, CRC(8719b04e), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519114.u4", 0xFE00, 0x100, CRC(72a040e0), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))
	ROMX_LOAD("94519115.u6", 0xFE01, 0x100, CRC(9ccf8cca), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1))
	ROMX_LOAD("94519116.u7", 0xFE01, 0x100, CRC(fa387bf3), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1))

#else
	/* ROM set 945121-4(?): "Floppy disc loader with self test" (cf 945401-9701
	pp. 1-19) */

	ROM_LOAD16_WORD("ti9904.rom", 0xFC00, 0x400, CRC(691e7d19) SHA1(58d9bed80490fdf71c743bfd3077c70840b7df8c))

#endif

#if VIDEO_911
	/* VDT911 character definitions */
	ROM_REGION(vdt911_chr_region_len, vdt911_chr_region, 0)
#else
	ROM_REGION(asr733_chr_region_len, asr733_chr_region, 0)
#endif

ROM_END

static DRIVER_INIT( ti990_4 )
{
#if VIDEO_911
	vdt911_init();
#else
	asr733_init();
#endif
}

INPUT_PORTS_START(ti990_4)
#if VIDEO_911
	VDT911_KEY_PORTS
#else
	ASR733_KEY_PORTS
#endif
INPUT_PORTS_END

static void ti990_4_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_fd800; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(ti990_4)
	CONFIG_DEVICE(ti990_4_floppy_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1976,	ti990_4,	0,		0,		ti990_4,	ti990_4,	ti990_4,	ti990_4,	"Texas Instruments",	"TI Model 990/4 Microcomputer System" , 0)
