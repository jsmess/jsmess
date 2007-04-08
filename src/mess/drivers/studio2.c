/******************************************************************************
 PeT mess@utanet.at May 2000

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "image.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cartslot.h"
#include "includes/studio2.h"
#include "inputx.h"
#include "sound/beep.h"

#define ST2_HEADER_SIZE		256

extern  READ8_HANDLER( cdp1861_video_enable_r );

static UINT8 keylatch;

/* vidhrdw */

static unsigned char studio2_palette[] =
{
	0, 0, 0,
	255,255,255
};

static unsigned short studio2_colortable[1][2] = {
	{ 0, 1 },
};

static PALETTE_INIT( studio2 )
{
	palette_set_colors(machine, 0, studio2_palette, sizeof(studio2_palette) / 3);
	memcpy(colortable,studio2_colortable,sizeof(studio2_colortable));
}

/* Read/Write Handlers */

static WRITE8_HANDLER( keylatch_w )
{
	keylatch = data & 0x0f;
}

static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bankptr(1, memory_region(REGION_CPU1));
}

/* Memory Maps */

static ADDRESS_MAP_START( studio2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_RAM
	AM_RANGE(0x0a00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( studio2_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(cdp1861_video_enable_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(1) // rom mapped in at reset, switched to ram with out 4
	AM_RANGE(0x0400, 0x0fff) AM_RAM
	AM_RANGE(0x8000, 0x83ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(cdp1861_video_enable_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( studio2 )
	PORT_START
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_D)

	PORT_START
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 1") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 2") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 3") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 4") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 6") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 7") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 8") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 9") PORT_CODE(KEYCODE_3_PAD)
INPUT_PORTS_END

INPUT_PORTS_START( vip )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 MW") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A MR") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B TR") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F TW") PORT_CODE(KEYCODE_F)

	PORT_START
INPUT_PORTS_END

/* Graphics Layouts */

static gfx_layout studio2_charlayout =
{
        32,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
			0, 0, 0, 0,
			1, 1, 1, 1,
			2, 2, 2, 2,
			3, 3, 3, 3,
			4, 4, 4, 4,
			5, 5, 5, 5,
			6, 6, 6, 6,
			7, 7, 7, 7
        },
        /* y offsets */
        { 0 },
        1*8
};

/* Graphics Decode Information */

static gfx_decode studio2_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &studio2_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

/* Interrupt Generators */

static INTERRUPT_GEN( studio2_frame_int )
{
}

/* CDP1802 Configuration */

/* studio 2
   output q speaker (300 hz tone on/off)
   f1 dma_activ
   f3 on player 2 key pressed
   f4 on player 1 key pressed
   inp 1 video on
   out 2 read key value selects keys to put at f3/f4 */

/* vip
   out 1 turns video off
   out 2 set keyboard multiplexer (bit 0..3 selects key)
   out 4 switch ram at 0000
   inp 1 turn on video
   q sound
   f1 vertical blank line (low when displaying picture
   f2 tape input (high when tone read)
   f3 keyboard in
 */

static int studio2_in_ef(void)
{
	int a=0xff;
	if (studio2_get_vsync()) a-=1;

	if (readinputport(0)&(1<<keylatch)) a-=4;
	if (readinputport(1)&(1<<keylatch)) a-=8;

	return a;
}

static int vip_in_ef(void)
{
	int a=0xff;
	if (studio2_get_vsync()) a-=1;

	if (readinputport(0)&(1<<keylatch)) a-=4;

	return a;
}

static void studio2_out_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG studio2_config={
	studio2_video_dma,
	studio2_out_q,
	studio2_in_ef
};

static CDP1802_CONFIG vip_config={
	studio2_video_dma,
	studio2_out_q,
	vip_in_ef
};

/* Machine Initialization */

static MACHINE_RESET( studio2 )
{
}

static MACHINE_RESET( vip )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1)+0x8000);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( studio2 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", CDP1802, 1780000/8)
	MDRV_CPU_PROGRAM_MAP(studio2_map, 0)
	MDRV_CPU_IO_MAP(studio2_io_map, 0)
	MDRV_CPU_VBLANK_INT(studio2_frame_int, 1)
	MDRV_CPU_CONFIG(studio2_config)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( studio2 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*4, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 64*4-1, 0, 128-1)
	MDRV_GFXDECODE( studio2_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (studio2_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (studio2_colortable) / sizeof(studio2_colortable[0][0]))
	MDRV_PALETTE_INIT( studio2 )

	MDRV_VIDEO_START( studio2 )
	MDRV_VIDEO_UPDATE( studio2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vip )
	MDRV_IMPORT_FROM( studio2 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(vip_map, 0)
	MDRV_CPU_IO_MAP(vip_io_map, 0)
	MDRV_CPU_CONFIG( vip_config )
	MDRV_MACHINE_RESET( vip )
MACHINE_DRIVER_END

/* ROMs */

ROM_START(studio2)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("studio2.rom", 0x0000, 0x800, CRC(a494b339) SHA1(f2650dacc9daab06b9fdf0e7748e977b2907010c))
//	ROM_CART_LOAD(0, "st2,bin", 0x0400, 0xfc00, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL )
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vip)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("monitor.rom", 0x8000, 0x200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06))
	ROM_LOAD("chip8.rom", 0x8200, 0x200, CRC(3e0f50f0) SHA1(4a9759035f99d125859cdf6ad71b8728637dcf4f))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

/* System Configuration */

DEVICE_LOAD( studio2_cart ) {
	UINT8	*ptr = NULL;
	UINT8	header[ST2_HEADER_SIZE];
	int	filesize;

	filesize = image_length( image );
	if ( filesize <= ST2_HEADER_SIZE ) {
		logerror( "Error loading cartridge: Invalid ROM file: %s.\n", image_filename( image ) );
		return INIT_FAIL;
	}

	/* read ST2 header */
	if ( image_fread(image, header, ST2_HEADER_SIZE ) != ST2_HEADER_SIZE ) {
		logerror( "Error loading cartridge: Unable to read header from file: %s.\n", image_filename( image ) );
		return INIT_FAIL;
	}
	filesize -= ST2_HEADER_SIZE;
	/* Read ST2 cartridge contents */
	ptr = ((UINT8 *)memory_region( REGION_CPU1 ) ) + 0x0400;
	if ( image_fread(image, ptr, filesize ) != filesize ) {
		logerror( "Error loading cartridge: Unable to read contents from file: %s.\n", image_filename( image ) );
		return INIT_FAIL;
	}
	return INIT_PASS;
}

static void studio2_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_studio2_cart;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "st2");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START(studio2)
	/* maybe quickloader */
	/* tape */
	/* cartridges at 0x400-0x7ff ? */
	CONFIG_DEVICE(studio2_cartslot_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static void setup_beep(int dummy) {
	beep_set_state( 0, 0 );
	beep_set_frequency( 0, 300 );
}

static DRIVER_INIT( studio2 )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	for (i=0; i<256; i++)
		gfx[i]=i;
	timer_set(0.0, 0, setup_beep);
}

static DRIVER_INIT( vip )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	for (i=0; i<256; i++)
		gfx[i]=i;
	memory_region(REGION_CPU1)[0x8022] = 0x3e; //bn3, default monitor
	timer_set(0.0, 0, setup_beep);
}

/* Game Drivers */

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG      COMPANY   FULLNAME */
// rca cosmac elf development board (2 7segment leds, some switches/keys)
// rca cosmac elf2 16 key keyblock
CONS(1977,	vip,		0,		0,		vip,		vip,		vip,		studio2,	"RCA",		"COSMAC VIP", GAME_NOT_WORKING )
CONS(1976,	studio2,	0,		0,		studio2,	studio2,	studio2,	studio2,	"RCA",		"Studio II", 0 )
// hanimex mpt-02
// colour studio 2 (m1200) with little color capability
