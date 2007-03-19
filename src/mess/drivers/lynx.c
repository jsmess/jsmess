/******************************************************************************
 PeT mess@utanet.at 2000,2001

 info found in bastian schick's bll
 and in cc65 for lynx

******************************************************************************/

#include <zlib.h>

#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "includes/lynx.h"
#include "video/generic.h"
#include "hash.h"

static int rotate=0;
int lynx_rotate;
static int lynx_line_y;
UINT32 lynx_palette[0x10];

static ADDRESS_MAP_START( lynx_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfbff) AM_RAM AM_BASE(&lynx_mem_0000)
	AM_RANGE(0xfc00, 0xfcff) AM_RAM AM_BASE(&lynx_mem_fc00)
	AM_RANGE(0xfd00, 0xfdff) AM_RAM AM_BASE(&lynx_mem_fd00)
	AM_RANGE(0xfe00, 0xfff7) AM_READWRITE( MRA8_BANK3, MWA8_RAM ) AM_BASE(&lynx_mem_fe00) AM_SIZE(&lynx_mem_fe00_size)
	AM_RANGE(0xfff8, 0xfff8) AM_RAM
	AM_RANGE(0xfff9, 0xfff9) AM_READWRITE( lynx_memory_config_r, lynx_memory_config_w )
	AM_RANGE(0xfffa, 0xffff) AM_READWRITE( MRA8_BANK4, MWA8_RAM ) AM_BASE(&lynx_mem_fffa)
ADDRESS_MAP_END

INPUT_PORTS_START( lynx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("A")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("B")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Opt 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Opt 1") PORT_CODE(KEYCODE_1)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_3)
	// power on and power off buttons
	PORT_START
	PORT_DIPNAME ( 0x03, 3, "90 Degree Rotation")
	PORT_DIPSETTING(	2, "Counterclockwise" )
	PORT_DIPSETTING(	1, "Clockwise" )
	PORT_DIPSETTING(	0, DEF_STR( None ) )
	PORT_DIPSETTING(	3, "Crcfile" )
INPUT_PORTS_END

static INTERRUPT_GEN( lynx_frame_int )
{
    lynx_rotate=rotate;
    if ((readinputport(2)&3)!=3)
		lynx_rotate=readinputport(2)&3;
}

static UINT8 lynx_read_vram(UINT16 address)
{
	UINT8 result = 0x00;
	if (address <= 0xfbff)
		result = lynx_mem_0000[address - 0x0000];
	else if (address <= 0xfcff)
		result = lynx_mem_fc00[address - 0xfc00];
	else if (address <= 0xfdff)
		result = lynx_mem_fd00[address - 0xfd00];
	else if (address <= 0xfff7)
		result = lynx_mem_fe00[address - 0xfe00];
	else if (address >= 0xfffa)
		result = lynx_mem_fffa[address - 0xfffa];
	return result;
}

/*
DISPCTL EQU $FD92       ; set to $D by INITMIKEY

; B7..B4        0
; B3    1 EQU color
; B2    1 EQU 4 bit mode
; B1    1 EQU flip screen
; B0    1 EQU video DMA enabled
*/
void lynx_draw_lines(int newline)
{
	static int height=-1, width=-1;
	int h,w;
	int x, yend;
	UINT16 j; // clipping needed!
	UINT8 byte;
	UINT16 *line;

	if (video_skip_this_frame()) newline=-1;

	if (newline==-1)
		yend = 102;
	else
		yend = newline;

	if (yend > 102)
		yend=102;

	if (yend==lynx_line_y)
	{
		if (newline==-1)
			lynx_line_y=0;
		return;
	}

	j=(mikey.data[0x94]|(mikey.data[0x95]<<8))+lynx_line_y*160/2;
	if (mikey.data[0x92]&2)
		j-=160*102/2-1;

	if (lynx_rotate&3)
	{
		/* rotation */
		h=160; w=102;
		if ( ((lynx_rotate==1)&&(mikey.data[0x92]&2))
				||( (lynx_rotate==2)&&!(mikey.data[0x92]&2)) )
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				line = BITMAP_ADDR16(tmpbitmap, lynx_line_y, 0);
				for (x=160-2;x>=0;j++,x-=2)
				{
					byte = lynx_read_vram(j);
					line[x + 1] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 0] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				line = BITMAP_ADDR16(tmpbitmap, 102-1-lynx_line_y, 0);
				for (x=0;x<160;j++,x+=2)
				{
					byte = lynx_read_vram(j);
					line[x + 0] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 1] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	else
	{
		w=160; h=102;
		if ( mikey.data[0x92]&2)
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				line = BITMAP_ADDR16(tmpbitmap, 102-1-lynx_line_y, 0);
				for (x=160-2;x>=0;j++,x-=2)
				{
					byte = lynx_read_vram(j);
					line[x + 1] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 0] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				line = BITMAP_ADDR16(tmpbitmap, lynx_line_y, 0);
				for (x=0;x<160;j++,x+=2)
				{
					byte = lynx_read_vram(j);
					line[x + 0] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 1] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	if (newline==-1)
	{
		lynx_line_y=0;
		if ((w!=width)||(h!=height))
		{
			width=w;
			height=h;
			video_screen_set_visarea(0, 0, width-1, 0, height-1);
		}
	}
}

static PALETTE_INIT( lynx )
{
    int i;

    for (i=0; i< 0x1000; i++)
	{
		palette_set_color(machine, i,
			((i >> 0) & 0x0f) * 0x11,
			((i >> 4) & 0x0f) * 0x11,
			((i >> 8) & 0x0f) * 0x11);
	}
}

struct CustomSound_interface lynx_sound_interface =
{
	lynx_custom_start
};

struct CustomSound_interface lynx2_sound_interface =
{
	lynx2_custom_start
};


static MACHINE_DRIVER_START( lynx )
	/* basic machine hardware */
	MDRV_CPU_ADD(M65SC02, 4000000)        /* vti core, integrated in vlsi, stz, but not bbr bbs */
	MDRV_CPU_PROGRAM_MAP(lynx_mem, 0)
	MDRV_CPU_VBLANK_INT(lynx_frame_int, 1)
	MDRV_SCREEN_REFRESH_RATE(LCD_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( lynx )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_SCREEN_SIZE(160, 102)*/
	MDRV_SCREEN_SIZE(160, 160)
	MDRV_SCREEN_VISIBLE_AREA(0, 160-1, 0, 102-1)
	MDRV_PALETTE_LENGTH(0x1000)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT( lynx )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(lynx_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lynx2 )
	MDRV_IMPORT_FROM( lynx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(lynx2_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END


/* these 2 dumps are saved from an running machine,
   and therefor the rom byte at 0xff09 is not readable!
   (memory configuration)
   these 2 dumps differ only in this byte!
*/
ROM_START(lynx)
	ROM_REGION(0x200,REGION_CPU1, 0)
	ROM_LOAD("lynx.bin", 0, 0x200, CRC(e1ffecb6) SHA1(de60f2263851bbe10e5801ef8f6c357a4bc077e6))
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynxa)
	ROM_REGION(0x200,REGION_CPU1, 0)
	ROM_LOAD("lynxa.bin", 0, 0x200, CRC(0d973c9d) SHA1(e4ed47fae31693e016b081c6bda48da5b70d7ccb))
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynx2)
	ROM_REGION(0x200,REGION_CPU1, 0)
	ROM_LOAD("lynx2.bin", 0, 0x200, NO_DUMP)
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END



void lynx_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 64)
		return;
	hash_compute(dest, &data[64], length - 64, functions);
}



static int lynx_verify_cart (char *header)
{

	logerror("Trying Header Compare\n");

	if (strncmp("LYNX",&header[0],4) && strncmp("BS9",&header[6],3)) {
		logerror("Not an valid Lynx image\n");
		return IMAGE_VERIFY_FAIL;
	}
	logerror("returning ID_OK\n");
	return IMAGE_VERIFY_PASS;
}

static void lynx_crc_keyword(mess_image *image)
{
    const char *info;

    info = image_extrainfo(image);
    rotate = 0;

    if (info)
	{
		if(strcmp(info, "ROTATE90DEGREE")==0)
			rotate = 1;
		else if (strcmp(info, "ROTATE270DEGREE")==0)
			rotate = 2;
    }
}

static int device_load_lynx_cart(mess_image *image)
{
	UINT8 *rom = memory_region(REGION_USER1);
	int size;
	UINT8 header[0x40];
/* 64 byte header
   LYNX
   intelword lower counter size
   0 0 1 0
   32 chars name
   22 chars manufacturer
*/

	size = image_length(image);
	if (image_fread(image, header, 0x40)!=0x40)
	{
		logerror("%s load error\n", image_filename(image));
		return 1;
	}

	/* Check the image */
	if (lynx_verify_cart((char*)header) == IMAGE_VERIFY_FAIL)
	{
		return INIT_FAIL;
	}

	size-=0x40;
	lynx_granularity=header[4]|(header[5]<<8);

	logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
			  header+10,size/1024,lynx_granularity, header+42);

	if (image_fread(image, rom, size) != size)
	{
		logerror("%s load error\n", image_filename(image));
		return 1;
	}

	lynx_crc_keyword(image);

	return 0;
}

static QUICKLOAD_LOAD( lynx )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 header[10]; // 80 08 dw Start dw Len B S 9 3
	// maybe the first 2 bytes must be used to identify the endianess of the file
	UINT16 start;

	if (image_fread(image, header, sizeof(header)) != sizeof(header))
		return INIT_FAIL;

	quickload_size -= sizeof(header);
	start = header[3] | (header[2]<<8); //! big endian format in file format for little endian cpu

	if (image_fread(image, rom+start, quickload_size) != quickload_size)
		return INIT_FAIL;

	rom[0xfffc+0x200] = start&0xff;
	rom[0xfffd+0x200] = start>>8;

	lynx_crc_keyword(image_from_devtype_and_index(IO_QUICKLOAD, 0));
	return 0;
}

static void lynx_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_lynx_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = lynx_partialhash; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "lnx"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void lynx_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "o"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_lynx; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(lynx)
	CONFIG_DEVICE(lynx_cartslot_getinfo)
	CONFIG_DEVICE(lynx_quickload_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT	MACHINE	INPUT	INIT	CONFIG	MONITOR	COMPANY   FULLNAME */
CONS( 1989, lynx,	  0, 		0,		lynx,	lynx,	0,		lynx,	"Atari",  "Lynx", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONS( 1989, lynxa,	  lynx, 	0,		lynx,	lynx,	0,		lynx,	"Atari",  "Lynx (alternate rom save!)", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONS( 1991, lynx2,	  lynx, 	0,		lynx2,	lynx,	0,		lynx,	"Atari",  "Lynx II", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
