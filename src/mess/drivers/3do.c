/***************************************************************************

  3do.c

  Driver file to handle emulation of the 3DO systems

Hardware descriptions:

Processors:
- 32bit 12.5MHZ RISC CPU (ARM60 - ARM6 core)
- Seperate BUS for video refresh updates (VRAM is dual ported)
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
- 17 seperate 16bit DMA channels to and from system memory.
- on chip instruction SRAM and register memory.
- 20bit internal processing.
- special filtering capable of creeating effects such as 3D sound.

Sound:
- 16bit stereo sound
- 44.1KHz sound sampling rate
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

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
//#include "includes/3do.h"
#include "devices/chd_cd.h"

/* The 3DO has an ARM6 core which is a bit different from the current
   ARM cpu cores. This define 'hack' can be removed once the ARM6 core
   is fully supported. */
#define CPU_ARM6	CPU_ARM

#define	MAIN_CLOCK_NTSC	25048948
#define MAIN_CLOCK_PAL	29500000

static ADDRESS_MAP_START( 3do_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x000FFFFF) AM_ROM				/* BIOS */
ADDRESS_MAP_END

INPUT_PORTS_START( 3do )
INPUT_PORTS_END

static MACHINE_DRIVER_START( 3do )
	/* Basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", ARM6, MAIN_CLOCK_NTSC/2 )
	MDRV_CPU_PROGRAM_MAP( 3do_mem, 0 )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 640, 525 )
	MDRV_SCREEN_VISIBLE_AREA( 0,639,0,479 )
	MDRV_SCREEN_REFRESH_RATE( 60 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( 3do_pal )
	/* Basic machine hardware */
	MDRV_CPU_ADD_TAG("main", ARM6, MAIN_CLOCK_PAL/2 )
	MDRV_CPU_PROGRAM_MAP( 3do_mem, 0 )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_SIZE( 640, 625 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_SCREEN_REFRESH_RATE( 50 )
MACHINE_DRIVER_END

SYSTEM_BIOS_START( 3do )
	SYSTEM_BIOS_ADD( 0, "panafz10", "Panasonic FZ-10 R.E.A.L. 3DO Interactive Multiplayer" )
	SYSTEM_BIOS_ADD( 1, "goldstar", "Goldstar 3DO Interactive Multiplayer v1.01m" )
	SYSTEM_BIOS_ADD( 2, "panafz1", "Panasonic FZ-1 R.E.A.L. 3DO Interactive Multiplayer" )
	SYSTEM_BIOS_ADD( 3, "gsalive2", "Goldstar 3DO Alive II" )
	SYSTEM_BIOS_ADD( 4, "sanyotry", "Sanyo TRY 3DO Interactive Multiplayer" )
SYSTEM_BIOS_END

ROM_START(3do)
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROMX_LOAD( "panafz10.bin", 0x000000, 0x100000, CRC(58242cee) SHA1(3c912300775d1ad730dc35757e279c274c0acaad), ROM_BIOS(1) | ROM_GROUPDWORD | ROM_REVERSE )
	ROMX_LOAD( "goldstar.bin", 0x000000, 0x100000, CRC(b6f5028b) SHA1(c4a2e5336f77fb5f743de1eea2cda43675ee2de7), ROM_BIOS(2) )
	ROMX_LOAD( "panafz1.bin", 0x000000, 0x100000, NO_DUMP, ROM_BIOS(3) )
	ROMX_LOAD( "gsalive2.bin", 0x000000, 0x100000, NO_DUMP, ROM_BIOS(4) )
	ROMX_LOAD( "sanyotry.bin", 0x000000, 0x100000, NO_DUMP, ROM_BIOS(5) )
ROM_END

SYSTEM_BIOS_START( 3do_pal )
        SYSTEM_BIOS_ADD( 0, "panafz10", "Panasonic FZ-10 R.E.A.L. 3DO Interactive Multiplayer" )
        SYSTEM_BIOS_ADD( 1, "goldstar", "Goldstar 3DO Interactive Multiplayer v1.01m" )
        SYSTEM_BIOS_ADD( 2, "panafz1", "Panasonic FZ-1 R.E.A.L. 3DO Interactive Multiplayer" )
SYSTEM_BIOS_END

ROM_START(3do_pal)
        ROM_REGION( 0x100000, REGION_CPU1, 0 )
        ROMX_LOAD( "panafz10.bin", 0x000000, 0x100000, CRC(58242cee) SHA1(3c912300775d1ad730dc35757e279c274c0acaad), ROM_BIOS(1) | ROM_GROUPDWORD | ROM_REVERSE )
        ROMX_LOAD( "goldstar.bin", 0x000000, 0x100000, CRC(b6f5028b) SHA1(c4a2e5336f77fb5f743de1eea2cda43675ee2de7), ROM_BIOS(2) )
        ROMX_LOAD( "panafz1.bin", 0x000000, 0x100000, NO_DUMP, ROM_BIOS(3) )
ROM_END

static void chdcd_3do_getinfo( const device_class *devclass, UINT32 state, union devinfo *info ) {
	/* CHD CD-ROM */
	switch( state ) {
	case DEVINFO_INT_COUNT:		info->i = 1; break;
	default:			cdrom_device_getinfo( devclass, state, info ); break;
	}
}

SYSTEM_CONFIG_START( 3do )
	CONFIG_DEVICE( chdcd_3do_getinfo )
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME  PARENT  BIOS  COMPAT  MACHINE INPUT  INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
CONSB(1991, 3do,       0,     3do,   0,     3do, 3do, 0, 3do, "3DO", "3DO", GAME_NOT_WORKING )
CONSB(1991, 3do_pal, 3do, 3do_pal, 	 0, 3do_pal, 3do, 0, 3do, "3DO", "3DO (PAL)", GAME_NOT_WORKING )

