/***************************************************************************

        Hawthorne Technologies TinyGiant HT68k

        29/11/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/68681.h"
#include "devices/flopdrv.h"
#include "machine/wd17xx.h"

static UINT16* ht68k_ram;

static ADDRESS_MAP_START(ht68k_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&ht68k_ram) // 512 KB RAM / ROM at boot
	//AM_RANGE(0x00080000, 0x000fffff) // Expansion
	//AM_RANGE(0x00d80000, 0x00d8ffff) // Printer
	AM_RANGE(0x00e00000, 0x00e00003) AM_MIRROR(0x7fffc) AM_DEVREADWRITE8("wd1770", wd17xx_r, wd17xx_w, 0xff) // FDC WD1770
	AM_RANGE(0x00e80000, 0x00e800ff) AM_MIRROR(0xff00) AM_DEVREADWRITE8( "duart68681", duart68681_r, duart68681_w, 0xff )
	AM_RANGE(0x00f00000, 0x00f07fff) AM_ROM AM_MIRROR(0xf8000) AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( ht68k )
INPUT_PORTS_END


static MACHINE_RESET(ht68k)
{
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)ht68k_ram,user1,0x8000);

	device_reset(cputag_get_cpu(machine, "maincpu"));
}

static VIDEO_START( ht68k )
{
}

static VIDEO_UPDATE( ht68k )
{
    return 0;
}


static void duart_irq_handler(const device_config *device, UINT8 vector)
{
	logerror("duart_irq_handler\n");
}

static void duart_tx(const device_config *device, int channel, UINT8 data)
{
}

static UINT8 duart_inp = 0x0e;

static UINT8 duart_input(const device_config *device)
{
	if (duart_inp != 0)
	{
		duart_inp = 0;
		return 0x0e;
	}
	else
	{
		duart_inp = 0x0e;
		return 0x00;
	}
}

static const duart68681_config ht68k_duart68681_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	NULL
};

static const floppy_config ht68k_floppy_config =
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

static MACHINE_DRIVER_START( ht68k )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(ht68k_mem)

    MDRV_MACHINE_RESET(ht68k)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ht68k)
    MDRV_VIDEO_UPDATE(ht68k)

	MDRV_DUART68681_ADD( "duart68681", XTAL_8MHz, ht68k_duart68681_config )
	MDRV_WD1770_ADD("wd1770", default_wd17xx_interface_2_drives )
	MDRV_FLOPPY_2_DRIVES_ADD(ht68k_floppy_config)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( ht68k )
    ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "ht68k-u4.bin", 0x0000, 0x4000, CRC(3fbcdd0a) SHA1(45fcbbf920dc1e9eada3b7b0a55f5720d08ffdd5))
	ROM_LOAD16_BYTE( "ht68k-u3.bin", 0x0001, 0x4000, CRC(1d85d101) SHA1(8ba01e1595b0b3c4fb128a4a50242f3588b89c43))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY                     FULLNAME                    FLAGS */
COMP( 1987, ht68k,  0,       0, 	 ht68k, 	ht68k, 	 0,  	  0,  	"Hawthorne Technologies",   "TinyGiant HT68k",		GAME_NOT_WORKING)

