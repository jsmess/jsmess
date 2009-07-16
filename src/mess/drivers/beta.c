/***************************************************************************
   
        Beta Computer

        17/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"

static ADDRESS_MAP_START(beta_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x007f) AM_RAM // 128 bytes of RAM (6532)
	AM_RANGE(0x0080, 0x00ff) AM_DEVREADWRITE("riot", riot6532_r, riot6532_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM	// Monitor
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( beta )
INPUT_PORTS_END


static MACHINE_RESET(beta) 
{	
}

static VIDEO_START( beta )
{
}

static VIDEO_UPDATE( beta )
{
    return 0;
}

static UINT8 beta_riot_a_r(const device_config *device, UINT8 olddata)
{
	return 0;
}

static UINT8 beta_riot_b_r(const device_config *device, UINT8 olddata)
{
	return 0;
}


void beta_riot_a_w(const device_config *device, UINT8 data, UINT8 olddata)
{
}

void beta_riot_b_w(const device_config *device, UINT8 data, UINT8 olddata)
{
}


void beta_riot_irq(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}

static const riot6532_interface beta_riot_interface =
{
	beta_riot_a_r,
	beta_riot_b_r,
	beta_riot_a_w,
	beta_riot_b_w,
	beta_riot_irq
};



static MACHINE_DRIVER_START( beta )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6502, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(beta_mem)

    MDRV_MACHINE_RESET(beta)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(beta)
    MDRV_VIDEO_UPDATE(beta)

    MDRV_RIOT6532_ADD("riot", XTAL_1MHz, beta_riot_interface)    
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(beta)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( beta )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  	ROM_LOAD( "beta.rom", 0xf800, 0x0800, CRC(d42fdb17) SHA1(595225a0cd43dd76c46b2aff6c0f27d5991cc4f0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1984, beta,  0,       0, 	beta, 	beta, 	 0,  	  beta,  	 "Steven Bolt",   "Beta Computer",		GAME_NOT_WORKING)

