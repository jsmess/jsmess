/***************************************************************************

  3do.c

  Driver file to handle emulation of the 3DO systems

Hardware descriptions:

Processors:
- 32bit 12.5MHZ RISC CPU (ARM60 - ARM6 core)
- Separate BUS for video refresh updates (VRAM is dual ported)
- Super Fast BUS Speed (50 Megabytes per second)
- Math Co-Processor custom designed by NTG for accelerating fixed-point
  matrix operations (_not_ the ARM FPA)
- Multitaking 32-bit operating system

Resolution:
- 640x480 pixel resolution
- 16.7 million colors

Two accelerated video co-processors:
- 25MHZ clock rate (NTSC), 29.5MHZ clock rate (PAL)
- Capable of producing 9-16 million real pixels per second (36-64 Mpix/sec
  interpolated), distorted, scaled, rotated and texture mapped.
- able to map a rectangular bitmap onto any arbitrary 4-point polygon.
- texturemap source bitmaps can be 1, 2, 4, 6, 8, or 16 bits per pixel and
  are RLE compressed for a maximum combination of both high resolution and
  small storage space.
- supports transparency, translucency, and color-shading effects.

Custom 16bit DSP:
- specifically designed for mixing, manipulating, and synthesizing CD quality
  sound.
- can decompress sound 2:1 or 4:1 on the fly saving memory and bus bandwidth.
- 25MHz clock rate.
- pipelined CISC architecture
- 16bit register size
- 17 separate 16bit DMA channels to and from system memory.
- on chip instruction SRAM and register memory.
- 20bit internal processing.
- special filtering capable of creating effects such as 3D sound.

Sound:
- 16bit stereo sound
- 44.1 kHz sound sampling rate
- Fully support Dolby(tm) Surround Sound

Memory:
- 2 megabytes of DRAM
- 1 megabyte of VRAM (also capable of holding/executing code and data)
- 1 megabyte of ROM
- 32KB battery backed SRAM

CD-ROM drive:
- 320ms access time
- double speed 300kbps data transfer
- 32KB RAM buffer

Ports:
- 2 expansion ports:
  - 1 high-speed 68 pin x 1 AV I/O port (for FMV cartridge)
  - 1 high-speed 30 pin x 1 I/O expansion port
- 1 control port, capable of daisy chaining together up to 8 peripherals

Models:
- Panasonic FZ-1 R.E.A.L. 3DO Interactive Multiplayer (Japan, Asia, North America, Europe)
- Panasonic FZ-10 R.E.A.L. 3DO Interactive Multiplayer (Japan, North America, Europe)
- Goldstar 3DO Interactive Multiplayer (South Korea, North America, Europe)
- Goldstar 3DO ALIVE II (South Korea)
- Sanyo TRY 3DO Interactive Multiplayer (Japan)
- Creative 3DO Blaster - PC Card (ISA)

===========================================================================

Part list of Goldstar 3DO Interactive Multiplayer

- X1 = 50.0000 MHz KONY 95-08 50.0000 KCH089C
- X2 = 59.0000 MHz KONY 95-21 59.0000 KCH089C (NTSC would use 49.09MHz)
- IC303 BOB = 3DO BOB ADG 00919-001-IC 517A4611 - 100 pins
- IC1 ANVIL = 3DO Anvil rev4 00745-004-02 521U5L36 - 304 pins
- IC302 DSP = SONY CXD2500BQ 447HE5V - 80 pins
- IC601 ADAC = BB PCM1710U 9436 GG2553 - 28 pins
- X601 16.934MHz = 16.93440 KONY
- IC101/102/103/104 DRAM = Goldstar GM71C4800AJ70 9520 KOREA - 28 pins
- IC105/106/107/108 VRAM = Toshiba TC528267J-70 9513HBK - 40 pins
- IC3 ROM = Goldstar [202M] GM23C8000AFW-325 9524 - 32 pins
- IC4 SRAM = Goldstar GM76C256ALLFW70 - 28 pins
- IC2 ARM = ARM P60ARMCP 9516C - 100 pins
- IC6 = Philips 74HCT14D 974230Q - 14 pins
- IC301 u-COM = MC68HSC 705C8ACFB 3E20T HLAH9446 - 44 pins

***************************************************************************/

#include "driver.h"
#include "includes/3do.h"
#include "devices/chd_cd.h"
#include "cpu/arm7/arm7.h"


/* The 3DO has an ARM6 core which is a bit different from the current
   ARM cpu cores. This define 'hack' can be removed once the ARM6 core
   is fully supported. */
#define CPU_ARM6	CPU_ARM7


#define X2_CLOCK_PAL	59000000
#define X2_CLOCK_NTSC	49090000
#define X601_CLOCK		XTAL_16_9344MHz


static UINT32	*dram;
static UINT32	*vram;


static ADDRESS_MAP_START( 3do_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x001FFFFF) AM_RAMBANK(1) AM_BASE(&dram)					/* DRAM */
	AM_RANGE(0x00200000, 0x002FFFFF) AM_RAM	AM_BASE(&vram)							/* VRAM */
	AM_RANGE(0x03000000, 0x030FFFFF) AM_ROMBANK(2)									/* BIOS */
	AM_RANGE(0x03140000, 0x0315FFFF) AM_READWRITE(nvarea_r, nvarea_w)				/* NVRAM */
	AM_RANGE(0x03180000, 0x031FFFFF) AM_READWRITE(unk_318_r, unk_318_w)				/* ???? */
	AM_RANGE(0x03200000, 0x032FFFFF) AM_READWRITE(vram_sport_r, vram_sport_w)		/* special vram access1 */
	AM_RANGE(0x03300000, 0x033FFFFF) AM_READWRITE(madam_r, madam_w)					/* address decoder */
	AM_RANGE(0x03400000, 0x034FFFFF) AM_READWRITE(clio_r, clio_w)					/* io controller */
ADDRESS_MAP_END


static MACHINE_RESET( 3do )
{
	memory_set_bankptr(machine, 2,memory_region(machine, "user1"));

	/* configure overlay */
	memory_configure_bank(machine, 1, 0, 1, dram, 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "user1"), 0);

	/* start with overlay enabled */
	memory_set_bank(machine, 1, 1);

	madam_init();
	clio_init();
}


static MACHINE_DRIVER_START( 3do )
	/* Basic machine hardware */
	MDRV_CPU_ADD( "maincpu", ARM6, XTAL_50MHz/4 )
	MDRV_CPU_PROGRAM_MAP( 3do_mem)

	MDRV_MACHINE_RESET( 3do )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 640, 525 )
	MDRV_SCREEN_VISIBLE_AREA( 0,639,0,479 )
	MDRV_SCREEN_REFRESH_RATE( 60 )

	MDRV_CDROM_ADD( "cdrom" )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 3do_pal )
	/* Basic machine hardware */
	MDRV_CPU_ADD("maincpu", ARM6, XTAL_50MHz/4 )
	MDRV_CPU_PROGRAM_MAP( 3do_mem)

	MDRV_MACHINE_RESET( 3do )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 640, 625 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_SCREEN_REFRESH_RATE( 50 )

	MDRV_CDROM_ADD( "cdrom" )
MACHINE_DRIVER_END


ROM_START(3do)
	ROM_REGION32_BE( 0x100000, "user1", 0 )
	ROM_SYSTEM_BIOS( 0, "panafz10", "Panasonic FZ-10 R.E.A.L. 3DO Interactive Multiplayer" )
	ROMX_LOAD( "panafz10.bin", 0x000000, 0x100000, CRC(58242cee) SHA1(3c912300775d1ad730dc35757e279c274c0acaad), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "goldstar", "Goldstar 3DO Interactive Multiplayer v1.01m" )
	ROMX_LOAD( "goldstar.bin", 0x000000, 0x100000, CRC(b6f5028b) SHA1(c4a2e5336f77fb5f743de1eea2cda43675ee2de7), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "panafz1", "Panasonic FZ-1 R.E.A.L. 3DO Interactive Multiplayer" )
	ROMX_LOAD( "panafz1.bin", 0x000000, 0x100000, CRC(c8c8ff89) SHA1(34bf189111295f74d7b7dfc1f304d98b8d36325a), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "gsalive2", "Goldstar 3DO Alive II" )
	ROMX_LOAD( "gsalive2.bin", 0x000000, 0x100000, NO_DUMP, ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "sanyotry", "Sanyo TRY 3DO Interactive Multiplayer" )
	ROMX_LOAD( "sanyotry.bin", 0x000000, 0x100000, CRC(d5cbc509) SHA1(b01c53da256dde43ffec4ad3fc3adfa8d635e943), ROM_BIOS(5) )
ROM_END


ROM_START(3do_pal)
    ROM_REGION32_BE( 0x100000, "user1", 0 )
    ROM_SYSTEM_BIOS( 0, "panafz10", "Panasonic FZ-10 R.E.A.L. 3DO Interactive Multiplayer" )
    ROMX_LOAD( "panafz10.bin", 0x000000, 0x100000, CRC(58242cee) SHA1(3c912300775d1ad730dc35757e279c274c0acaad), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "goldstar", "Goldstar 3DO Interactive Multiplayer v1.01m" )
    ROMX_LOAD( "goldstar.bin", 0x000000, 0x100000, CRC(b6f5028b) SHA1(c4a2e5336f77fb5f743de1eea2cda43675ee2de7), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "panafz1", "Panasonic FZ-1 R.E.A.L. 3DO Interactive Multiplayer" )
    ROMX_LOAD( "panafz1.bin", 0x000000, 0x100000, CRC(c8c8ff89) SHA1(34bf189111295f74d7b7dfc1f304d98b8d36325a), ROM_BIOS(3) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY FULLNAME        FLAGS */
CONS( 1991, 3do,        0,      0,      3do,        0,    	0,      0,      "3DO",  "3DO (NTSC)",   GAME_NOT_WORKING )
CONS( 1991, 3do_pal,    3do,    0,      3do_pal,    0,    	0,      0,      "3DO",  "3DO (PAL)",    GAME_NOT_WORKING )
