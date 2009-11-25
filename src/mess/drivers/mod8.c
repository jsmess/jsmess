/***************************************************************************

		Microsystems International Limited MOD-8

        18/11/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8008/i8008.h"

static UINT16 tty_data;
static UINT8 tty_key_data;
static int tty_cnt  = 0;
static WRITE8_HANDLER(out_w)
{
	tty_data >>= 1;
	tty_data |= (data & 0x01) ? 0x8000 : 0;
	tty_cnt++;
	if (tty_cnt==10) {
		logerror("%c",(tty_data >> 7) & 0x7f);
		tty_cnt = 0;
	}
}

static WRITE8_HANDLER(tty_w)
{
	tty_data = 0;
	tty_cnt = 0;
}

static READ8_HANDLER(tty_r)
{
	UINT8 d = tty_key_data & 0x01;
	tty_key_data >>= 1;
	return d;
}
static ADDRESS_MAP_START(mod8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000,0x6ff) AM_ROM
	AM_RANGE(0x700,0xfff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mod8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00,0x00) AM_READ(tty_r)
	AM_RANGE(0x0a,0x0a) AM_WRITE(out_w)
	AM_RANGE(0x0b,0x0b) AM_WRITE(tty_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mod8 )
INPUT_PORTS_END

static IRQ_CALLBACK ( mod8_irq_callback )
{
	return 0xC0; // LAA - NOP equivalent
}

static MACHINE_RESET(mod8)
{
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), mod8_irq_callback);
}

static VIDEO_START( mod8 )
{
}

static VIDEO_UPDATE( mod8 )
{
    return 0;
}

static MACHINE_DRIVER_START( mod8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8008, 800000)
    MDRV_CPU_PROGRAM_MAP(mod8_mem)
    MDRV_CPU_IO_MAP(mod8_io)

    MDRV_MACHINE_RESET(mod8)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mod8)
    MDRV_VIDEO_UPDATE(mod8)
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( mod8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon8.001", 0x0000, 0x0100, CRC(b82ac6b8) SHA1(fbea5a6dd4c779ca1671d84089f857a3f548ffcb))
	ROM_LOAD( "mon8.002", 0x0100, 0x0100, CRC(8b82bc3c) SHA1(66222511527b27e56a5a1f9656d424d407eac7d3))
	ROM_LOAD( "mon8.003", 0x0200, 0x0100, CRC(679ae913) SHA1(22423efcb9051c9812fcbac9a27af70415d0dd81))
	ROM_LOAD( "mon8.004", 0x0300, 0x0100, CRC(2a4e580f) SHA1(8b0cb9660fde3cacd299faaa31724e4f3262d77f))
	ROM_LOAD( "mon8.005", 0x0400, 0x0100, CRC(e281bb1a) SHA1(cc7c2746e075512dbf5eed88ae3aea009558dbd0))
	ROM_LOAD( "mon8.006", 0x0500, 0x0100, CRC(b7e2f585) SHA1(5408adabc3df6e6ea8dcfb2327b2883b435ab85e))
	ROM_LOAD( "mon8.007", 0x0600, 0x0100, CRC(49a5c626) SHA1(66c1865db9151818d3b20ec3c68dd793cb98a221))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1974, mod8,   0,       0, 		mod8, 	mod8, 	 0,  	  0,  	"Microsystems International Ltd.",   "MOD-8",		GAME_NOT_WORKING)

