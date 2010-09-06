/***************************************************************************
   
        WaveMate Bullet

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"

static ADDRESS_MAP_START(bullet_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x001f) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0x0020, 0xbfff) AM_RAMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static UINT8 port19 = 0;

static READ8_HANDLER(port19_r) 
{
	return port19;
}

static WRITE8_HANDLER(port15_w) 
{
	memory_set_bankptr(space->machine, "bank1", messram_get_ptr(space->machine->device("messram")) + 0x0000);	
}

static ADDRESS_MAP_START( bullet_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE("wd179x", wd17xx_r, wd17xx_w)
	AM_RANGE(0x15, 0x15) AM_READ(port15_w)
	AM_RANGE(0x19, 0x19) AM_READ(port19_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( bullet )
INPUT_PORTS_END

INLINE running_device *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

static MACHINE_RESET(bullet) 
{
	floppy_mon_w(get_floppy_image(machine, 0), 1);
	floppy_mon_w(get_floppy_image(machine, 1), 1);
	floppy_mon_w(get_floppy_image(machine, 2), 1);
	floppy_mon_w(get_floppy_image(machine, 3), 1);

	floppy_drive_set_ready_state(get_floppy_image(machine, 0), 1, 1);
	floppy_drive_set_ready_state(get_floppy_image(machine, 1), 1, 1);	
	floppy_drive_set_ready_state(get_floppy_image(machine, 2), 1, 1);
	floppy_drive_set_ready_state(get_floppy_image(machine, 3), 1, 1);	
	
	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu"));
	memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + 0x0000);	
	memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + 0x0020);
	memory_set_bankptr(machine, "bank4", messram_get_ptr(machine->device("messram")) + 0xc000);
}

static VIDEO_START( bullet )
{
}

static VIDEO_UPDATE( bullet )
{
    return 0;
}

static const floppy_config bullet_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	"bullet_flop"
};

static WRITE_LINE_DEVICE_HANDLER( bullet_wd17xx_irq_w )
{
	if (state)
		port19 |= (1<<6);
	else
		port19 &=~(1<<6);
}
static WRITE_LINE_DEVICE_HANDLER( bullet_wd17xx_drq_w )
{
	if (state)
		port19 |= (1<<7);
	else
		port19 &=~(1<<7);
}

static const wd17xx_interface bullet_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(bullet_wd17xx_irq_w),
	DEVCB_LINE(bullet_wd17xx_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static MACHINE_CONFIG_START( bullet,driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bullet_mem)
    MDRV_CPU_IO_MAP(bullet_io)	

    MDRV_MACHINE_RESET(bullet)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bullet)
    MDRV_VIDEO_UPDATE(bullet)
	
	MDRV_WD179X_ADD("wd179x", bullet_wd17xx_interface )

	MDRV_FLOPPY_4_DRIVES_ADD(bullet_floppy_config)	
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( bullet )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sr70x.u8", 0x0000, 0x0020, CRC(d54b8a30) SHA1(65ff8753dd63c9dd1899bc9364a016225585d050))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 197?, bullet,  0,       0, 	bullet, 	bullet, 	 0,   "WaveMate",   "Bullet",		GAME_NOT_WORKING | GAME_NO_SOUND)

