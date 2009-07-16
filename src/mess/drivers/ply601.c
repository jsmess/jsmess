/***************************************************************************

        Plydin-601

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "video/mc6845.h"

static UINT8 rom_page;

static UINT32 vdisk_addr = 0;

static READ8_HANDLER (rom_page_r)
{
	return rom_page;
}

static WRITE8_HANDLER (rom_page_w)
{
	rom_page =data;
	if (data & 8) {
	    int chip = (data >> 4) % 5;
	    int page = data & 7;	    
	    memory_set_bankptr(space->machine, 2, memory_region(space->machine, "romdisk") + chip*0x10000 + page * 0x2000);
	} else {
    	memory_set_bankptr(space->machine, 2, mess_ram + 0xc000);
	}	
} 


static WRITE8_HANDLER (vdisk_page_w)
{
	vdisk_addr = (vdisk_addr & 0x0ffff) | ((data & 0x0f)<<16); 
}

static WRITE8_HANDLER (vdisk_h_w)
{
	vdisk_addr = (vdisk_addr & 0xf00ff) | (data<<8); 
}

static WRITE8_HANDLER (vdisk_l_w)
{
	vdisk_addr = (vdisk_addr & 0xfff00) | data; 
}

static WRITE8_HANDLER (vdisk_data_w)
{
	mess_ram[0x10000 + (vdisk_addr & 0x7ffff)] = data;
	vdisk_addr++;
	vdisk_addr&=0x7ffff;
}

static READ8_HANDLER (vdisk_data_r)
{
	UINT8 retVal = mess_ram[0x10000 + (vdisk_addr & 0x7ffff)];
	vdisk_addr++;
	vdisk_addr&=0x7ffff;
	return retVal;
}
    
static ADDRESS_MAP_START(ply601_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xbfff ) AM_RAMBANK(1)
	AM_RANGE( 0xc000, 0xdfff ) AM_RAMBANK(2)
	AM_RANGE( 0xe000, 0xe5ff ) AM_RAMBANK(3)
	AM_RANGE( 0xe600, 0xe600 ) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE( 0xe601, 0xe601 ) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
	AM_RANGE( 0xe604, 0xe604 ) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE( 0xe605, 0xe605 ) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
	AM_RANGE( 0xe680, 0xe680 ) AM_WRITE(vdisk_page_w)
	AM_RANGE( 0xe681, 0xe681 ) AM_WRITE(vdisk_h_w)
	AM_RANGE( 0xe682, 0xe682 ) AM_WRITE(vdisk_l_w)
	AM_RANGE( 0xe683, 0xe683 ) AM_READWRITE(vdisk_data_r,vdisk_data_w)
	AM_RANGE( 0xe6f0, 0xe6f0 ) AM_READWRITE(rom_page_r, rom_page_w)
	AM_RANGE( 0xe700, 0xefff ) AM_RAMBANK(4)
	AM_RANGE( 0xf000, 0xffff ) AM_READWRITE(SMH_BANK(5), SMH_BANK(6))
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ply601 )
INPUT_PORTS_END


static MACHINE_RESET(ply601)
{
	memory_set_bankptr(machine, 1, mess_ram + 0x0000);
	memory_set_bankptr(machine, 2, mess_ram + 0xc000);	
	memory_set_bankptr(machine, 3, mess_ram + 0xe000);	
	memory_set_bankptr(machine, 4, mess_ram + 0xe700);	
	memory_set_bankptr(machine, 5, memory_region(machine, "maincpu") + 0xf000);	
	memory_set_bankptr(machine, 6, mess_ram + 0xf000);	
	device_reset(cputag_get_cpu(machine, "maincpu"));
}

static VIDEO_START( ply601 )
{
}

static VIDEO_UPDATE( ply601 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( ply601_update_row )
{
	UINT8 *charrom = memory_region(device->machine, "gfx1");

	int column, bit;
	UINT8 data;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = mess_ram[(((ma + column) & 0x0fff) + 0xf000)];		
		code = ((code << 1) | (code >> 7)) & 0xff;
		data = charrom[((code << 3) | (ra & 0x07)) & 0x7ff];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7) ? 1 : 0;
				
			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static const mc6845_interface ply601_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	ply601_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static DRIVER_INIT(ply601)
{
	memset(mess_ram, 0, 64 * 1024);
}

static MACHINE_DRIVER_START( ply601 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6800, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(ply601_mem)

    MDRV_MACHINE_RESET(ply601)

    /* video hardware */ 
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_1MHz, ply601_crtc6845_interface) // clk taken from schematics

	MDRV_VIDEO_START( ply601 )
	MDRV_VIDEO_UPDATE( ply601 )	
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(ply601)
	CONFIG_RAM_DEFAULT((64 +512) * 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( ply601 )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "bios.rom",   0xf000, 0x1000, CRC(41fe4c4b) SHA1(d8ca92aea0eb283e8d7779cb976bcdfa03e81aea))
  ROM_REGION(0x0800, "gfx1",0)
  ROM_LOAD( "video.rom",  0x0000, 0x0800, CRC(1c23ba43) SHA1(eb1cfc139858abd0aedbbf3d523f8ba55d27a11d))
  ROM_REGION(0x50000, "romdisk",ROMREGION_ERASEFF)
  ROM_LOAD( "rom0.rom", 0x00000, 0x10000, CRC(60103920) SHA1(ee5b4ee5b513c4a0204da751e53d63b8c6c0aab9))
  ROM_LOAD( "rom1.rom", 0x10000, 0x10000, CRC(cb4a9b22) SHA1(dd09e4ba35b8d1a6f60e6e262aaf2f156367e385))
  ROM_LOAD( "rom2.rom", 0x20000, 0x08000, CRC(0b7684bf) SHA1(c02ad1f2a6f484cd9d178d8b060c21c0d4e53442))
  ROM_COPY("romdisk", 0x20000, 0x28000, 0x08000)
  ROM_LOAD( "rom3.rom", 0x30000, 0x08000, CRC(e4a86dfa) SHA1(96e6bb9ffd66f81fca63bf7491fbba81c4ff1fd2))
  ROM_COPY("romdisk", 0x30000, 0x38000, 0x08000)
  ROM_LOAD( "rom4.rom", 0x40000, 0x08000, CRC(d88ac21d) SHA1(022db11fdcf8db81ce9efd9cd9fa50ebca88e79e))
  ROM_COPY("romdisk", 0x40000, 0x48000, 0x08000)
ROM_END

ROM_START( ply601a )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "bios_a.rom", 0xf000, 0x1000, CRC(e018b11e) SHA1(884d59abd5fa5af1295d1b5a53693facc7945b63))
  ROM_REGION(0x0800, "gfx1",0)
  ROM_LOAD( "video.rom",  0x0000, 0x0800, CRC(1c23ba43) SHA1(eb1cfc139858abd0aedbbf3d523f8ba55d27a11d))
  ROM_REGION(0x50000, "romdisk",ROMREGION_ERASEFF)
  ROM_LOAD( "rom0.rom", 0x00000, 0x10000, CRC(60103920) SHA1(ee5b4ee5b513c4a0204da751e53d63b8c6c0aab9))
  ROM_LOAD( "rom1.rom", 0x10000, 0x10000, CRC(cb4a9b22) SHA1(dd09e4ba35b8d1a6f60e6e262aaf2f156367e385))
  ROM_LOAD( "rom2.rom", 0x20000, 0x08000, CRC(0b7684bf) SHA1(c02ad1f2a6f484cd9d178d8b060c21c0d4e53442))
  ROM_COPY("romdisk", 0x20000, 0x28000, 0x08000)
  ROM_LOAD( "rom3.rom", 0x30000, 0x08000, CRC(e4a86dfa) SHA1(96e6bb9ffd66f81fca63bf7491fbba81c4ff1fd2))
  ROM_COPY("romdisk", 0x30000, 0x38000, 0x08000)
  ROM_LOAD( "rom4.rom", 0x40000, 0x08000, CRC(d88ac21d) SHA1(022db11fdcf8db81ce9efd9cd9fa50ebca88e79e))
  ROM_COPY("romdisk", 0x40000, 0x48000, 0x08000)
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, ply601,  0,       0, 	ply601, 	ply601, ply601,  	  ply601,  	 "Mikroelektronika",   "Plydin-601",		GAME_NOT_WORKING)
COMP( ????, ply601a, ply601,  0, 	ply601, 	ply601, ply601,  	  ply601,  	 "Mikroelektronika",   "Plydin-601A",		GAME_NOT_WORKING)

