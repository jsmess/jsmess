/***************************************************************************
   
        DEC ZRT-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/ins8250.h"

static UINT8 *video_ram;

static ADDRESS_MAP_START(zrt80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM // Z25 - Main firmware
	AM_RANGE(0x1000, 0x1fff) AM_ROM // Z24 - Expansion
	AM_RANGE(0x4000, 0x43ff) AM_RAM	// Board RAM
	// Normaly video RAM is 0x800 but could be expanded up to 8K
	AM_RANGE(0xc000, 0xdfff) AM_RAM	 AM_BASE(&video_ram) // Video RAM
	
ADDRESS_MAP_END

static ADDRESS_MAP_START( zrt80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_DEVREADWRITE("ins8250", ins8250_r, ins8250_w )
	AM_RANGE(0x08, 0x08) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x09, 0x09) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
	AM_RANGE(0x18, 0x18) AM_READ_PORT("DIPSW2")
	AM_RANGE(0x20, 0x20) AM_READ_PORT("DIPSW3")
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( zrt80 )
	PORT_START("DIPSW1")
		PORT_DIPNAME( 0x01, 0x01, "Composite Sync" )
		PORT_DIPSETTING(    0x01, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x02, 0x02, "Vertical Sync" )
		PORT_DIPSETTING(    0x02, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x04, 0x00, "Video" )
		PORT_DIPSETTING(    0x04, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x08, 0x08, "Keypad" )
		PORT_DIPSETTING(    0x08, "Numeric" )
		PORT_DIPSETTING(    0x00, "Alternate Keyboard" )
		PORT_DIPNAME( 0x10, 0x10, "Horizontal Sync" )
		PORT_DIPSETTING(    0x10, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" ) 
		PORT_DIPNAME( 0x20, 0x20, "CPU" )
		PORT_DIPSETTING(    0x20, "Operating" )
		PORT_DIPSETTING(    0x00, "Reset" )
		PORT_DIPNAME( 0x40, 0x40, "Keyboard strobe" )
		PORT_DIPSETTING(    0x40, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x80, 0x00, "Beeper" )
		PORT_DIPSETTING(    0x80, "Silent" )
		PORT_DIPSETTING(    0x00, "Enable" ) 
	PORT_START("DIPSW2")
		PORT_DIPNAME( 0x0f, 0x05, "Baud rate" )
		PORT_DIPSETTING(    0x00, "50" )
		PORT_DIPSETTING(    0x01, "75" )
		PORT_DIPSETTING(    0x02, "110" )
		PORT_DIPSETTING(    0x03, "134.5" )
		PORT_DIPSETTING(    0x04, "150" )
		PORT_DIPSETTING(    0x05, "300" )
		PORT_DIPSETTING(    0x06, "600" )
		PORT_DIPSETTING(    0x07, "1200" )
		PORT_DIPSETTING(    0x08, "1800" )
		PORT_DIPSETTING(    0x09, "2000" )
		PORT_DIPSETTING(    0x0a, "2400" )
		PORT_DIPSETTING(    0x0b, "3600" )
		PORT_DIPSETTING(    0x0c, "4800" )
		PORT_DIPSETTING(    0x0d, "7200" )
		PORT_DIPSETTING(    0x0e, "9600" )
		PORT_DIPSETTING(    0x0f, "19200" )
		PORT_DIPNAME( 0x30, 0x20, "Parity" )
		PORT_DIPSETTING(    0x00, "Odd" )
		PORT_DIPSETTING(    0x10, "Even" )
		PORT_DIPSETTING(    0x20, "Marking" )
		PORT_DIPSETTING(    0x30, "Spacing" )
		PORT_DIPNAME( 0x40, 0x40, "Handshake" )
		PORT_DIPSETTING(    0x40, "CTS" )
		PORT_DIPSETTING(    0x00, "XON/XOFF" )
		PORT_DIPNAME( 0x80, 0x80, "Line Feed" )
		PORT_DIPSETTING(    0x80, "No LF on CR" )
		PORT_DIPSETTING(    0x00, "Auto" ) 
	PORT_START("DIPSW3")
		PORT_DIPNAME( 0x07, 0x07, "Video" )
		PORT_DIPSETTING(    0x00, "96 x 24 15750Hz, 50Hz" )
		PORT_DIPSETTING(    0x01, "80 x 48 15750Hz, 50Hz" )
		PORT_DIPSETTING(    0x02, "80 x 24 15750Hz, 50Hz" )		
		PORT_DIPSETTING(    0x03, "96 x 24 15750Hz, 60Hz" )
		PORT_DIPSETTING(    0x04, "80 x 48 18700Hz, 50Hz" )
		PORT_DIPSETTING(    0x05, "80 x 24 17540Hz, 60Hz" )
		PORT_DIPSETTING(    0x06, "80 x 48 15750Hz, 60Hz" )
		PORT_DIPSETTING(    0x07, "80 x 24 15750Hz, 60Hz" )
		PORT_DIPNAME( 0x18, 0x18, "Emulation" )
		PORT_DIPSETTING(    0x00, "Adds" )
		PORT_DIPSETTING(    0x08, "Beehive" )
		PORT_DIPSETTING(    0x10, "LSI ADM-3" )
		PORT_DIPSETTING(    0x18, "Heath H-19" )
		PORT_DIPNAME( 0x20, 0x20, "Mode" )
		PORT_DIPSETTING(    0x20, "Line" )
		PORT_DIPSETTING(    0x00, "Local" )
		PORT_DIPNAME( 0x40, 0x40, "Duplex" )
		PORT_DIPSETTING(    0x40, "Full" )
		PORT_DIPSETTING(    0x00, "Half" )
		PORT_DIPNAME( 0x80, 0x80, "Wraparound" )
		PORT_DIPSETTING(    0x80, "Disabled" )
		PORT_DIPSETTING(    0x00, "Enabled" ) 
INPUT_PORTS_END


static MACHINE_RESET(zrt80) 
{	
}

static VIDEO_START( zrt80 )
{
}

static VIDEO_UPDATE( zrt80 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( zrt80_update_row )
{
	UINT8 *charrom = memory_region(device->machine, "chargen");

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = video_ram[ma + column] & 0x7f;
		UINT16 addr = code << 4 | (ra & 0x0f);
		UINT8 data = charrom[addr & 0x7ff];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7) ? 1 : 0;
			if (input_port_read(device->machine, "DIPSW1") & 0x04) {
				color = color ? 0 : 1;
			}
				
			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static const mc6845_interface zrt80_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	zrt80_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static INS8250_INTERRUPT( zrt80_com_interrupt )
{
	logerror("com int\r\n");
}

static const ins8250_interface zrt80_com_interface =
{
	2457600,
	zrt80_com_interrupt,
	NULL,
	NULL,
	NULL
};
static MACHINE_DRIVER_START( zrt80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_2_4576MHz)
    MDRV_CPU_PROGRAM_MAP(zrt80_mem)
    MDRV_CPU_IO_MAP(zrt80_io)	

    MDRV_MACHINE_RESET(zrt80)
	
    /* video hardware */
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_20MHz / 8, zrt80_crtc6845_interface) 

    MDRV_VIDEO_START(zrt80)
    MDRV_VIDEO_UPDATE(zrt80)
    
    MDRV_INS8250_ADD( "ins8250", zrt80_com_interface )
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(zrt80)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( zrt80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "zrt80mon.z25", 0x0000, 0x1000, CRC(e6ea96dc) SHA1(e3075e30bb2b85f9288d0b8b8cdf1d2b4f7586fd))
	//z24 is 2nd chip, used as expansion
	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "zrt80chr.z30", 0x0000, 0x0800, CRC(4dbdc60f) SHA1(20e393f7207a8440029c8290cdf2f121d317a37e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1982, zrt80,  0,       0, 	zrt80, 	zrt80, 	 0,  	  zrt80,  	 "Digital Research Computers",   "ZRT-80",		GAME_NOT_WORKING)

