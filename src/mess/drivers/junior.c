/***************************************************************************
   
        Elektor Junior

        17/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"

static ADDRESS_MAP_START(junior_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_GLOBAL_MASK(0x1FFF)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_RAM // 1K RAM
	AM_RANGE(0x1000, 0x17ff) AM_ROM	// Printer ROM
	AM_RANGE(0x1a00, 0x1a7f) AM_RAM // 6532 RAM
	AM_RANGE(0x1a80, 0x1aff) AM_DEVREADWRITE("riot", riot6532_r, riot6532_w)
	AM_RANGE(0x1c00, 0x1fff) AM_ROM	// Monitor
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( junior )
INPUT_PORTS_END


static MACHINE_RESET(junior) 
{	
}

static VIDEO_START( junior )
{
}

static VIDEO_UPDATE( junior )
{
    return 0;
}

static UINT8 junior_riot_a_r(const device_config *device, UINT8 olddata)
{
	return 0;
}

static UINT8 junior_riot_b_r(const device_config *device, UINT8 olddata)
{
	return 0;
}


void junior_riot_a_w(const device_config *device, UINT8 data, UINT8 olddata)
{
}

void junior_riot_b_w(const device_config *device, UINT8 data, UINT8 olddata)
{
}


void junior_riot_irq(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}

static const riot6532_interface junior_riot_interface =
{
	junior_riot_a_r,
	junior_riot_b_r,
	junior_riot_a_w,
	junior_riot_b_w,
	junior_riot_irq
};


static MACHINE_DRIVER_START( junior )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6502, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(junior_mem)

    MDRV_MACHINE_RESET(junior)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(junior)
    MDRV_VIDEO_UPDATE(junior)
    
    MDRV_RIOT6532_ADD("riot", XTAL_1MHz, junior_riot_interface)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(junior)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( junior )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "printer.bin", 0x1000, 0x0800, CRC(b15bf48d) SHA1(0c8da5c0ce9d59cf1cc535cb47a73f147615f289))
    ROM_DEFAULT_BIOS("orig")
    ROM_SYSTEM_BIOS( 0, "orig", "Original (2708)" )
    ROMX_LOAD( "junior.ic2", 0x1c00, 0x0400, CRC(ee8aa69d) SHA1(a132a51603f1a841c354815e6d868b335ac84364), ROM_BIOS(1))
    ROM_SYSTEM_BIOS( 1, "2732", "Just monitor (2732) " )
    ROMX_LOAD( "junior27321a.ic2", 0x1c00, 0x0400, CRC(e22f24cc) SHA1(a6edb52a9eea5e99624c128065e748e5a3fb2e4c), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1980, junior,  0,       0, 	junior, 	junior, 	 0,  	  junior,  	 "Elektor Electronics",   "Junior",		GAME_NOT_WORKING)

