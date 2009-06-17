/***************************************************************************
   
        Nanos

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "machine/nec765.h"
#include "devices/basicdsk.h"

static const UINT8 *FNT;

static ADDRESS_MAP_START(nanos_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff ) AM_READWRITE(SMH_BANK(1), SMH_BANK(3))
	AM_RANGE( 0x1000, 0xffff ) AM_RAMBANK(2)
ADDRESS_MAP_END

static WRITE8_HANDLER(nanos_tc_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "nec765");
	nec765_set_tc_state(fdc, BIT(data,1));
}

static ADDRESS_MAP_START( nanos_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)	
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w)
	AM_RANGE(0x92, 0x92) AM_WRITE(nanos_tc_w)
	AM_RANGE(0x94, 0x94) AM_DEVREAD("nec765", nec765_status_r)
	AM_RANGE(0x95, 0x95) AM_DEVREADWRITE("nec765", nec765_data_r, nec765_data_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( nanos )
INPUT_PORTS_END


static MACHINE_RESET(nanos) 
{	
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_write8_handler(space, 0x0000, 0x0fff, 0, 0, SMH_BANK(3));		
	memory_install_write8_handler(space, 0x1000, 0xffff, 0, 0, SMH_BANK(2));	
	
	memory_set_bankptr(machine, 1, memory_region(machine, "maincpu"));
	memory_set_bankptr(machine, 2, mess_ram + 0x1000);
	memory_set_bankptr(machine, 3, mess_ram);
	
	floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), 1);
	
	floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), 1,1);

}

static VIDEO_START( nanos )
{
	FNT = memory_region(machine, "gfx1");
}

static VIDEO_UPDATE( nanos )
{
//	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

//	framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = mess_ram[0xf800+ x];

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0;

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=80;
	}
	return 0;
}

static READ8_DEVICE_HANDLER (nanos_port_a_r)
{	
//logerror("nanos_port_a_r\n");
	return 0x00;
}

static READ8_DEVICE_HANDLER (nanos_port_b_r)
{
	//logerror("nanos_port_b_r\n");
	return 0x00;
}

static WRITE8_DEVICE_HANDLER (nanos_port_a_w)
{
	//logerror("nanos_port_a_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (nanos_port_b_w)
{
//	logerror("nanos_port_b_w %02x\n",data);
	if (BIT(data,7)) {
		memory_set_bankptr(device->machine, 1, memory_region(device->machine, "maincpu"));
	} else {
		memory_set_bankptr(device->machine, 1, mess_ram);
	}
}

const z80pio_interface nanos_z80pio_intf =
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(nanos_port_a_r),
	DEVCB_HANDLER(nanos_port_b_r),
	DEVCB_HANDLER(nanos_port_a_w),
	DEVCB_HANDLER(nanos_port_b_w),
	DEVCB_NULL,
	DEVCB_NULL
};


static const nec765_interface nanos_nec765_interface =
{
	NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_NOT_CONNECTED
};


static MACHINE_DRIVER_START( nanos )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(nanos_mem)
    MDRV_CPU_IO_MAP(nanos_io)	

    MDRV_MACHINE_RESET(nanos)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,25*10-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(nanos)
    MDRV_VIDEO_UPDATE(nanos)
    
    MDRV_Z80PIO_ADD( "z80pio", nanos_z80pio_intf )
	/* NEC765 */
	MDRV_NEC765A_ADD("nec765", nanos_nec765_interface)    
   
MACHINE_DRIVER_END

static DEVICE_IMAGE_LOAD( nanos_floppy )
{
	int size;
	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 800*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;
			
	basicdsk_set_geometry (image, 80, 2, 5, 1024, 1, 0, FALSE);
	return INIT_PASS;
}

static void nanos_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(nanos_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(nanos)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(nanos_floppy_getinfo)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( nanos )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k7634_1.rom", 0x0000, 0x0800, CRC(8e34e6ac) SHA1(fd342f6effe991823c2a310737fbfcba213c4fe3))
	ROM_LOAD( "k7634_2.rom", 0x0800, 0x0180, CRC(4e01b02b) SHA1(8a279da886555c7470a1afcbb3a99693ea13c237))

	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "zg_nanos.rom", 0x0000, 0x0800, CRC(5682d3f9) SHA1(5b738972c815757821c050ee38b002654f8da163))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, nanos,  0,       0, 	nanos, 	nanos, 	 0,  	  nanos,  	 "Ingenieurhochschule fur Seefahrt Warnemunde/Wustrow",   "Nanos",		GAME_NOT_WORKING)

