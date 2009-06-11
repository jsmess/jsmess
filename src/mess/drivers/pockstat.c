/***************************************************************************

    Sony PocketStation

    05/2009 Skeleton driver.


    This should be emulated alongside PS1 (and especially its memory cards, 
    since Pocket Station games were dowloaded from PS1 games into flash RAM
    after the unit had been inserted in the memory card slot). 
    While waiting for full PS1 emulation in MESS, we collect here info on the 
    system and its BIOS.

    CPU: ARM7T (32 bit RISC Processor)
    Memory: SRAM 2K bytes, Flash RAM 128K bytes
    Graphics: 32 x 32 dot monochrome LCD
    Sound: Miniature speaker (12 bit PCM) x 1 unit
    Input: 5 input buttons, 1 reset button
    Infrared communication: Bi-directional (supports IrDA based and 
        conventional remote control systems)
    Other: 1 LED indicator

    Info available at:
      * http://exophase.devzero.co.uk/ps_info.txt
      * http://members.at.infoseek.co.jp/DrHell/pocket/index.html

****************************************************************************/

#include "driver.h"
#include "cpu/arm7/arm7.h"

static ADDRESS_MAP_START(pockstat_mem, ADDRESS_SPACE_PROGRAM, 32)
/*	AM_RANGE(0x00000000, 0x000007ff) AM_RAM
	AM_RANGE(0x02000000, 0x0201ffff) Flash ROM
	AM_RANGE(0x04000000, 0x04003fff) AM_ROM
	AM_RANGE(0x08000000, 0x0801ffff) Same as 0x02 above. Mirrors every 128KB.
	AM_RANGE(0x0a000000, 0x0a000003) IRQs currently triggered
	AM_RANGE(0x0a000004, 0x0a000007) Raw status
	AM_RANGE(0x0a000008, 0x0a00000b) Which IRQs are enabled
	AM_RANGE(0x0a00000c, 0x0a00000f) Disable IRQs (write only)
	AM_RANGE(0x0a000010, 0x0a000013) IRQ service signal (write only)
	AM_RANGE(0x0a800000, 0x0a800003) Timer 0 period
	AM_RANGE(0x0a800004, 0x0a800007) Timer 0 current value
	AM_RANGE(0x0a800008, 0x0a80000b) Timer 0 control
	AM_RANGE(0x0a800010, 0x0a800013) Timer 1 period
	AM_RANGE(0x0a800014, 0x0a800017) Timer 1 current value
	AM_RANGE(0x0a800018, 0x0a80001b) Timer 1 control
	AM_RANGE(0x0a800020, 0x0a800023) Timer 2 period
	AM_RANGE(0x0a800024, 0x0a800027) Timer 2 current value
	AM_RANGE(0x0a800028, 0x0a80002b) Timer 2 control
	AM_RANGE(0x0b000000, 0x0b000003) Internal CPU Clock control
	AM_RANGE(0x0b800000, 0x0b800003) RTC control word
	AM_RANGE(0x0b800004, 0x0b800007) RTC Modify value (write only)
	AM_RANGE(0x0b800008, 0x0b80000b) RTC Time of day + day of week (read only)
	AM_RANGE(0x0b80000c, 0x0b80000f) RTC Date (read only)
	AM_RANGE(0x0d000000, 0x0b000003) LCD control word
	AM_RANGE(0x0d000100, 0x0b00017f) LCD buffer - 1 word per scanline	*/
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pockstat )
INPUT_PORTS_END


static MACHINE_RESET( pockstat )
{
}

static VIDEO_START( pockstat )
{
}

static VIDEO_UPDATE( pockstat )
{
	return 0;
}

static MACHINE_DRIVER_START( pockstat )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", ARM7, 7900000)	// ??
	MDRV_CPU_PROGRAM_MAP(pockstat_mem)

	MDRV_MACHINE_RESET(pockstat)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32, 32)
	MDRV_SCREEN_VISIBLE_AREA(0, 32-1, 0, 32-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(pockstat)
	MDRV_VIDEO_UPDATE(pockstat)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(pockstat)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( pockstat )
	ROM_REGION( 0x4000, "maincpu", 0 )
	ROM_LOAD( "kernel.bin", 0x0000, 0x4000, CRC(5fb47dd8) SHA1(6ae880493ddde880827d1e9f08e9cb2c38f9f2ec) )
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE    INPUT     INIT  CONFIG     COMPANY                             FULLNAME       FLAGS */
CONS( 1999, pockstat, 0,      0,      pockstat,  pockstat, 0,    pockstat,  "Sony Computer Entertainment Inc.", "Sony PocketStation", GAME_NOT_WORKING)
