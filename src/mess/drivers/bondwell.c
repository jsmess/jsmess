/***************************************************************************
   
        Bondwell 12/14

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/nec765.h"
#include "devices/basicdsk.h"
#include "machine/6821pia.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"

static UINT8 *bw_videoram;

static ADDRESS_MAP_START(bw12_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE(&bw_videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw12_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
	AM_RANGE(0x20, 0x20) AM_DEVREAD("nec765", nec765_status_r)
	AM_RANGE(0x21, 0x21) AM_DEVREADWRITE("nec765", nec765_data_r, nec765_data_w)
	AM_RANGE(0x3c, 0x3f) AM_DEVREADWRITE("pia6821", pia6821_r, pia6821_w)
	AM_RANGE(0x60, 0x63) AM_DEVREADWRITE("z80ctc", z80ctc_r, z80ctc_w)
	AM_RANGE(0x40, 0x40) AM_DEVREADWRITE("z80sio", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x41, 0x41) AM_DEVREADWRITE("z80sio", z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x42, 0x42) AM_DEVREADWRITE("z80sio", z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x43, 0x43) AM_DEVREADWRITE("z80sio", z80sio_c_r, z80sio_c_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( bw12 )
INPUT_PORTS_END

static MACHINE_RESET(bw12) 
{	
}

static VIDEO_START( bw12 )
{
}

static VIDEO_UPDATE( bw12 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;	
}

static MC6845_UPDATE_ROW( bw12_update_row )
{
	UINT8 *charrom = memory_region(device->machine, "gfx1");

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = bw_videoram[((ma + column) & 0x7ff)];
		UINT16 addr = code << 4 | (ra & 0x0f);
		UINT8 data = charrom[addr & 0x7ff];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7) ? 1 : 0;
				
			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static const mc6845_interface bw12_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	bw12_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static const nec765_interface bw12_nec765_interface =
{
	NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_NOT_CONNECTED
};

static READ8_DEVICE_HANDLER(bw12_pia_a_r)
{
	return 0xff;
}

static const pia6821_interface bw12_pia_config =
{
	DEVCB_HANDLER(bw12_pia_a_r), DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL
};

static void bw12_ctc_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", 0, state);
}

static const z80ctc_interface bw12_ctc_intf =
{
	0,				/* timer disablers */
	bw12_ctc_interrupt,			/* interrupt callback */
	NULL,			/* ZC/TO0 callback */
	NULL,			/* ZC/TO1 callback */
	NULL			/* ZC/TO2 callback */
};


const z80sio_interface bw12_z80sio_intf =
{
	0,	/* interrupt handler */
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};
static MACHINE_DRIVER_START( bw12 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bw12_mem)
    MDRV_CPU_IO_MAP(bw12_io)	

    MDRV_MACHINE_RESET(bw12)
	
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_12_288MHz / 8, bw12_crtc6845_interface)

    MDRV_VIDEO_START(bw12)
    MDRV_VIDEO_UPDATE(bw12)

	/* NEC765 */
	MDRV_NEC765A_ADD("nec765", bw12_nec765_interface)    
	
	MDRV_PIA6821_ADD("pia6821", bw12_pia_config)
	
	MDRV_Z80CTC_ADD( "z80ctc", XTAL_4MHz, bw12_ctc_intf ) 
	
	MDRV_Z80SIO_ADD( "z80sio", XTAL_4MHz, bw12_z80sio_intf )   
MACHINE_DRIVER_END

static DEVICE_IMAGE_LOAD( bw12_floppy )
{
	basicdsk_set_geometry (image, 40, 2, 8, 512, 1, 0, FALSE);
	return INIT_PASS;
}

static void bw12_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(bw12_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(bw12)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(bw12_floppy_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(bw14)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE(bw12_floppy_getinfo)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( bw12 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bw14boot.bin", 0x0000, 0x1000, CRC(782fe341) SHA1(eefe5ad6b1ef77a1caf0af743b74de5fa1c4c19d))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD( "bw14char.bin", 0x0000, 0x1000, CRC(f9dd68b5) SHA1(50132b759a6d84c22c387c39c0f57535cd380411))
ROM_END

#define rom_bw14 rom_bw12
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1984, bw12,  0,       0, 	bw12, 	bw12, 	 0,  	  bw12,  	 "Bondwell",   "BW 12",		GAME_NOT_WORKING)
COMP( 1984, bw14,  bw12,    0, 	bw12, 	bw12, 	 0,  	  bw14,  	 "Bondwell",   "BW 14",		GAME_NOT_WORKING)
