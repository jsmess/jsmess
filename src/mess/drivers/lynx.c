/******************************************************************************
 PeT peter.trauner@utanet.at 2000,2001

 info found in bastian schick's bll
 and in cc65 for lynx

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/lynx.h"

#include "devices/snapquik.h"

static QUICKLOAD_LOAD( lynx );

static ADDRESS_MAP_START( lynx_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfbff) AM_RAM AM_BASE(&lynx_mem_0000)
	AM_RANGE(0xfc00, 0xfcff) AM_RAM AM_BASE(&lynx_mem_fc00)
	AM_RANGE(0xfd00, 0xfdff) AM_RAM AM_BASE(&lynx_mem_fd00)
	AM_RANGE(0xfe00, 0xfff7) AM_READWRITE(SMH_BANK(3), SMH_RAM) AM_BASE(&lynx_mem_fe00) AM_SIZE(&lynx_mem_fe00_size)
	AM_RANGE(0xfff8, 0xfff8) AM_RAM
	AM_RANGE(0xfff9, 0xfff9) AM_READWRITE(lynx_memory_config_r, lynx_memory_config_w)
	AM_RANGE(0xfffa, 0xffff) AM_READWRITE(SMH_BANK(4), SMH_RAM) AM_BASE(&lynx_mem_fffa)
ADDRESS_MAP_END

static INPUT_PORTS_START( lynx )
	PORT_START("JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("A")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("B")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Opt 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Opt 1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )

	PORT_START("PAUSE")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_3)
	// power on and power off buttons

	PORT_START("ROTATION")
	PORT_CONFNAME ( 0x03, 0x03, "90 Degree Rotation" )
	PORT_CONFSETTING(	  0x00, DEF_STR( None ) )
	PORT_CONFSETTING(	  0x01, "Clockwise" )
	PORT_CONFSETTING(	  0x02, "Counterclockwise" )
	PORT_CONFSETTING(	  0x03, "Crcfile" )
INPUT_PORTS_END

static PALETTE_INIT( lynx )
{
    int i;

    for (i=0; i< 0x1000; i++)
	{
		palette_set_color_rgb(machine, i,
			((i >> 0) & 0x0f) * 0x11,
			((i >> 4) & 0x0f) * 0x11,
			((i >> 8) & 0x0f) * 0x11);
	}
}


static MACHINE_DRIVER_START( lynx )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M65SC02, 4000000)        /* vti core, integrated in vlsi, stz, but not bbr bbs */
	MDRV_CPU_PROGRAM_MAP(lynx_mem)
	MDRV_CPU_VBLANK_INT("screen", lynx_frame_int)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( lynx )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(LCD_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_SCREEN_SIZE(160, 102)*/
	MDRV_SCREEN_SIZE(160, 160)
	MDRV_SCREEN_VISIBLE_AREA(0, 160-1, 0, 102-1)
	MDRV_PALETTE_LENGTH(0x1000)
	MDRV_PALETTE_INIT( lynx )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("lynx", LYNX, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", lynx, "o", 0)
	
	MDRV_IMPORT_FROM(lynx_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lynx2 )
	MDRV_IMPORT_FROM( lynx )

	/* sound hardware */
	MDRV_DEVICE_REMOVE("mono")
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_DEVICE_REMOVE("lynx")
	MDRV_SOUND_ADD("lynx2", LYNX2, 0)
	MDRV_SOUND_ROUTE(0, "lspeaker", 0.50)
	MDRV_SOUND_ROUTE(1, "rspeaker", 0.50)
MACHINE_DRIVER_END


/* these 2 dumps are saved from an running machine,
   and therefor the rom byte at 0xff09 is not readable!
   (memory configuration)
   these 2 dumps differ only in this byte!
*/

ROM_START(lynx)
	ROM_REGION(0x200,"maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "default",   "rom save" )
	ROMX_LOAD( "lynx.bin",  0x00000, 0x200, CRC(e1ffecb6) SHA1(de60f2263851bbe10e5801ef8f6c357a4bc077e6), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "a", "alternate rom save" )
	ROMX_LOAD( "lynxa.bin", 0x00000, 0x200, CRC(0d973c9d) SHA1(e4ed47fae31693e016b081c6bda48da5b70d7ccb), ROM_BIOS(2))

	ROM_REGION(0x100,"gfx1", ROMREGION_ERASE00)

	ROM_REGION(0x100000, "user1", ROMREGION_ERASEFF)
ROM_END

ROM_START(lynx2)
	ROM_REGION(0x200,"maincpu", 0)
	ROM_LOAD("lynx2.bin", 0, 0x200, NO_DUMP)

	ROM_REGION(0x100,"gfx1", ROMREGION_ERASE00)

	ROM_REGION(0x100000, "user1", ROMREGION_ERASEFF)
ROM_END


static QUICKLOAD_LOAD( lynx )
{
	const device_config *cpu = cputag_get_cpu(image->machine, "maincpu");
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *data = NULL;
	UINT8 *rom = memory_region(image->machine, "maincpu");
	UINT8 header[10]; // 80 08 dw Start dw Len B S 9 3
	UINT16 start, length;
	int i;

	if (image_fread(image, header, sizeof(header)) != sizeof(header))
		return INIT_FAIL;

	/* Check the image */
	if (lynx_verify_cart((char*)header, LYNX_QUICKLOAD) == IMAGE_VERIFY_FAIL)
		return INIT_FAIL;

	start = header[3] | (header[2]<<8); //! big endian format in file format for little endian cpu
	length = header[5] | (header[4]<<8);
	length -= 10;

	data = malloc(length);

	if (image_fread(image, data, length) != length)
		return INIT_FAIL;

	for (i = 0; i < length; i++)
		memory_write_byte(space, start + i, data[i]);

	free (data);

	rom[0x1fc] = start & 0xff;
	rom[0x1fd] = start >> 8;
	memory_write_byte(space, 0x1fc, start & 0xff);
	memory_write_byte(space, 0x1fd, start >> 8);

	lynx_crc_keyword(devtag_get_device(image->machine, "quickload")); 

	cpu_set_reg(cpu, REG_GENPC, start);

	return INIT_PASS;
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY   FULLNAME      FLAGS */
CONS( 1989, lynx,   0,      0,      lynx,   lynx,   0,      0,  	 "Atari",  "Lynx",       GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )
CONS( 1991, lynx2,  lynx,   0,      lynx2,  lynx,   0,      0,  	 "Atari",  "Lynx II",    GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )
