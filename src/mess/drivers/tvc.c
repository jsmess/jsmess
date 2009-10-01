/***************************************************************************

        Videotone TVC 32/64 driver

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"

static UINT8 tvc_video_mode = 0;

static void tvc_set_mem_page(running_machine *machine, UINT8 data)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	switch(data & 0x18) {
		case 0x00 : // system ROM selected
				memory_install_readwrite8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1), SMH_NOP);
				memory_set_bankptr(space->machine, 1, memory_region(machine, "sys"));
				break;
		case 0x08 : // Cart ROM selected
				memory_install_readwrite8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1), SMH_NOP);
				memory_set_bankptr(space->machine, 1, memory_region(machine, "cart"));
				break;
		case 0x10 : // RAM selected
				memory_install_readwrite8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
				memory_set_bankptr(space->machine, 1, mess_ram);
				break;
	}
	// Bank 2 is always RAM
	memory_set_bankptr(space->machine, 2, mess_ram + 0x4000);
	if ((data & 0x20)==0) {
		// Video RAM
		memory_set_bankptr(space->machine, 3, mess_ram + 0x10000);
	} else {
		// System RAM page 3
		memory_set_bankptr(space->machine, 3, mess_ram + 0x8000);
	}
	switch(data & 0xc0) {
		case 0x00 : // Cart ROM selected
				memory_install_readwrite8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4), SMH_NOP);
				memory_set_bankptr(space->machine, 4, memory_region(machine, "cart"));
				break;
		case 0x40 : // System ROM selected
				memory_install_readwrite8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4), SMH_NOP);
				memory_set_bankptr(space->machine, 4, memory_region(machine, "sys"));
				break;
		case 0x80 : // RAM selected
				memory_install_readwrite8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4), SMH_BANK(4));
				memory_set_bankptr(space->machine, 4, mess_ram);
				break;
		case 0xc0 : // External ROM selected
				memory_install_readwrite8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4), SMH_NOP);
				memory_set_bankptr(space->machine, 4, memory_region(machine, "ext"));
				break;

	}
}

static WRITE8_HANDLER( tvc_bank_w )
{
	tvc_set_mem_page(space->machine, data);
}

static WRITE8_HANDLER( tvc_video_mode_w )
{
	tvc_video_mode = data & 0x03;
}

static UINT8 col[4]= {0,1,2,3};

static WRITE8_HANDLER( tvc_palette_w )
{
	//  0 I 0 G | 0 R 0 B
	//  0 0 0 0 | I R G B
	int i = ((data&0x40)>>3) | ((data&0x10)>>3) | (data&0x04) | (data&0x01);

	col[offset] = i;
}

static ADDRESS_MAP_START(tvc_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK(2)
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(3)
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK(4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tvc_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x02, 0x02) AM_WRITE(tvc_bank_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(tvc_video_mode_w)
	AM_RANGE(0x60, 0x64) AM_WRITE(tvc_palette_w)
	AM_RANGE(0x70, 0x70) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x71, 0x71) AM_DEVREADWRITE("crtc", mc6845_register_r , mc6845_register_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tvc )
INPUT_PORTS_END


static MACHINE_RESET(tvc)
{
	memset(mess_ram,0,(64+14)*1024);
	tvc_set_mem_page(machine, 0);
	tvc_video_mode = 0;
}

static VIDEO_START( tvc )
{
}

static VIDEO_UPDATE( tvc )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, "crtc");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( tvc_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	switch(tvc_video_mode) {
		case 0 :
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = mess_ram[ offset + 0x10000];
					*p = col[(data >> 7)]; p++;
					*p = col[(data >> 6)]; p++;
					*p = col[(data >> 5)]; p++;
					*p = col[(data >> 4)]; p++;
					*p = col[(data >> 3)]; p++;
					*p = col[(data >> 2)]; p++;
					*p = col[(data >> 1)]; p++;
					*p = col[(data >> 0)]; p++;
				}
				break;
		case 1 :
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = mess_ram[ offset + 0x10000];
					*p = col[BIT(data,7)*2 + BIT(data,3)]; p++;
					*p = col[BIT(data,7)*2 + BIT(data,3)]; p++;
					*p = col[BIT(data,6)*2 + BIT(data,2)]; p++;
					*p = col[BIT(data,6)*2 + BIT(data,2)]; p++;
					*p = col[BIT(data,5)*2 + BIT(data,1)]; p++;
					*p = col[BIT(data,5)*2 + BIT(data,1)]; p++;
					*p = col[BIT(data,4)*2 + BIT(data,0)]; p++;
					*p = col[BIT(data,4)*2 + BIT(data,0)]; p++;
				}
				break;
		default:
				for ( i = 0; i < x_count; i++ )
				{
					UINT16 offset = i  + (y * 64);
					UINT8 data = mess_ram[ offset + 0x10000];
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 4) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
					*p = col[(data >> 0) & 0xf]; p++;
				}
				break;

	}
}

static PALETTE_INIT( tvc )
{
	const static unsigned char tvc_palette[16][3] =
	{
		{ 0x00,0x00,0x00 },
		{ 0x00,0x00,0x7f },
		{ 0x00,0x7f,0x00 },
		{ 0x00,0x7f,0x7f },
		{ 0x7f,0x00,0x00 },
		{ 0x7f,0x00,0x7f },
		{ 0x7f,0x7f,0x00 },
		{ 0x7f,0x7f,0x7f },
		{ 0x00,0x00,0x00 },
		{ 0x00,0x00,0xff },
		{ 0x00,0xff,0x00 },
		{ 0x00,0xff,0xff },
		{ 0xff,0x00,0x00 },
		{ 0xff,0x00,0xff },
		{ 0xff,0xff,0x00 },
		{ 0xff,0xff,0xff }
	};
	int i;

	for(i = 0; i < 16; i++)
		palette_set_color_rgb(machine, i, tvc_palette[i][0], tvc_palette[i][1], tvc_palette[i][2]);
}

static const mc6845_interface tvc_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	tvc_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static MACHINE_DRIVER_START( tvc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, 3125000)
    MDRV_CPU_PROGRAM_MAP(tvc_mem)
    MDRV_CPU_IO_MAP(tvc_io)

    MDRV_MACHINE_RESET(tvc)

 /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 512 - 1, 0, 240 - 1)
	MDRV_PALETTE_LENGTH( 16 )
	MDRV_PALETTE_INIT(tvc)

	MDRV_MC6845_ADD("crtc", MC6845, 3125000, tvc_crtc6845_interface) // clk taken from schematics

    MDRV_VIDEO_START(tvc)
    MDRV_VIDEO_UPDATE(tvc)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(tvc)
	CONFIG_RAM_DEFAULT((64 + 16)* 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( tvc64 )
    ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc12_d4.64k", 0x0000, 0x2000, CRC(834ca9be) SHA1(c333318c1c6185aae2d3dfb86d55e3a4a3071a73))
	ROM_LOAD( "tvc12_d3.64k", 0x2000, 0x2000, CRC(71753d02) SHA1(d9a1905cf55c532b3380c83158fb5254ee503829))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc12_d7.64k", 0x2000, 0x2000, CRC(1cbbeac6) SHA1(54b29c9ca9942f04620fbf3edab3b8e3cd21c194))
ROM_END

ROM_START( tvc64p )
	ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc22_d6.64k", 0x0000, 0x2000, CRC(05ac3a34) SHA1(bdc7eda5fd53f806dca8c4929ee498e8e59eb787))
	ROM_LOAD( "tvc22_d4.64k", 0x2000, 0x2000, CRC(ba6ad589) SHA1(e5c8a6db506836a327d901387a8dc8c681a272db))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvc22_d7.64k", 0x2000, 0x2000, CRC(05e1c3a8) SHA1(abf119cf947ea32defd08b29a8a25d75f6bd4987))
/*

    ROM_LOAD( "tvcru_d4.bin", 0x0000, 0x2000, CRC(bac5dd4f) SHA1(665a1b8c80b6ad82090803621f0c73ef9243c7d4))
    ROM_LOAD( "tvcru_d6.bin", 0x2000, 0x2000, CRC(1e0fa0b8) SHA1(9bebb6c8f03f9641bd35c9fd45ffc13a48e5c572))
    ROM_LOAD( "tvcru_d7.bin", 0x2000, 0x2000, CRC(70cde756) SHA1(c49662af9f6653347ead641e85777c3463cc161b))
*/
ROM_END

ROM_START( tvc64pru )
	ROM_REGION( 0x4000, "sys", ROMREGION_ERASEFF )
	ROM_LOAD( "tvcru_d6.bin", 0x0000, 0x2000, CRC(1e0fa0b8) SHA1(9bebb6c8f03f9641bd35c9fd45ffc13a48e5c572))
	ROM_LOAD( "tvcru_d4.bin", 0x2000, 0x2000, CRC(bac5dd4f) SHA1(665a1b8c80b6ad82090803621f0c73ef9243c7d4))
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "ext", ROMREGION_ERASEFF )
	ROM_LOAD( "tvcru_d7.bin", 0x2000, 0x2000, CRC(70cde756) SHA1(c49662af9f6653347ead641e85777c3463cc161b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1985, tvc64,  0,   	 0, 	tvc, 	tvc, 	 0,  	  tvc,  	 "Videoton",   "TVC 64",		GAME_NOT_WORKING)
COMP( 1985, tvc64p, tvc64,   0, 	tvc, 	tvc, 	 0,  	  tvc,  	 "Videoton",   "TVC 64+",		GAME_NOT_WORKING)
COMP( 1985, tvc64pru,tvc64,   0, 	tvc, 	tvc, 	 0,  	  tvc,  	 "Videoton",   "TVC 64+ (Russian)",		GAME_NOT_WORKING)

