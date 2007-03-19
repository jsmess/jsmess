/******************************************************************************
 PeT mess@utanet.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include <assert.h>
#include "driver.h"
#include "image.h"
#include "cpu/s2650/s2650.h"
#include "includes/arcadia.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( arcadia_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff) AM_ROM
	AM_RANGE( 0x1800, 0x1aff) AM_READWRITE( arcadia_video_r, arcadia_video_w )
	AM_RANGE( 0x2000, 0x2fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( arcadia_io, ADDRESS_SPACE_IO, 8)
//{ S2650_CTRL_PORT,S2650_CTRL_PORT, },
//{ S2650_DATA_PORT,S2650_DATA_PORT, },
	AM_RANGE( S2650_SENSE_PORT,S2650_SENSE_PORT) AM_READ( arcadia_vsync_r)
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)

INPUT_PORTS_START( arcadia )
	PORT_START
	DIPS_HELPER( 0x01, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x02, "Option", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x04, DEF_STR(Difficulty), KEYCODE_F5, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x02, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x01, "Player 1/Left Clear", KEYCODE_BACKSPACE, KEYCODE_Z)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x04, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x02, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x01, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x02, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x01, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_C)
	PORT_START
	PORT_BIT ( 0xff, 0xf0,	 IPT_UNUSED ) // used in palladium
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x04, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xff, 0xf0,	 IPT_UNUSED ) // used in palladium
#if 0
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_X) PORT_SENSITIVITY(1) PORT_KEYDELTA(2000) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_Y) PORT_SENSITIVITY(1) PORT_KEYDELTA(2000) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_DEL) PORT_CODE_INC(KEYCODE_PGDN) PORT_CODE_DEC(JOYCODE_2_LEFT) PORT_CODE_INC(JOYCODE_2_RIGHT) PORT_PLAYER(2) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_END) PORT_CODE_DEC(JOYCODE_2_UP) PORT_CODE_INC(JOYCODE_2_DOWN) PORT_PLAYER(2) PORT_RESET
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

INPUT_PORTS_START( vcg )
	PORT_START
	DIPS_HELPER( 0x01, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x02, "Selector A", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x04, "Selector B", KEYCODE_F5, CODE_NONE)
	PORT_START
PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED ) // some bits must be high
	DIPS_HELPER( 0x08, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x02, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x01, "Player 1/Left Clear", KEYCODE_BACKSPACE, KEYCODE_Z)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x04, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x02, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x01, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x02, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x01, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_C)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x01, "Player 1/Left x1", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/Left x2", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left x3", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/Left x4", KEYCODE_G, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x04, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x01, "Player 2/Right x1", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right x2", KEYCODE_SLASH_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right x3", KEYCODE_MINUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 2/Right x4", KEYCODE_PLUS_PAD, CODE_NONE)
#if 0
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_X) PORT_SENSITIVITY(1) PORT_KEYDELTA(2000) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_Y) PORT_SENSITIVITY(1) PORT_KEYDELTA(2000) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_DEL) PORT_CODE_INC(KEYCODE_PGDN) PORT_CODE_DEC(JOYCODE_2_LEFT) PORT_CODE_INC(JOYCODE_2_RIGHT) PORT_PLAYER(2) PORT_RESET
    PORT_START
PORT_BIT(0x1ff,0x10,IPT_AD_STICK_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_MINMAX(0,0x1f) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_END) PORT_CODE_DEC(JOYCODE_2_UP) PORT_CODE_INC(JOYCODE_2_DOWN) PORT_PLAYER(2) PORT_RESET
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

static gfx_layout arcadia_charlayout =
{
        8,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
	    0,
	    1,
	    2,
	    3,
	    4,
	    5,
	    6,
	    7,
        },
        /* y offsets */
        { 0 },
        1*8
};

static gfx_decode arcadia_gfxdecodeinfo[] = {
    { REGION_GFX1, 0x0000, &arcadia_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static unsigned char arcadia_palette[] =
{
	255, 255, 255,	// white
	255, 255, 0,	// yellow
	0, 255, 255,	// cyan
	0, 255, 0,		// green
	255, 0, 255,	// magenta
	255, 0, 0,		// red
	0, 0, 255,		// blue
	0, 0, 0			// black
};

static unsigned short arcadia_colortable[1][2] = {
	{ 0, 1 },
};

static PALETTE_INIT( arcadia )
{
	palette_set_colors(machine, 0, arcadia_palette, sizeof (arcadia_palette)/3);
    memcpy(colortable, arcadia_colortable,sizeof(arcadia_colortable));
}

static MACHINE_DRIVER_START( arcadia )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", S2650, 3580000/3)        /* 1.796 Mhz */
	MDRV_CPU_PROGRAM_MAP(arcadia_mem, 0)
	MDRV_CPU_IO_MAP(arcadia_io, 0)
	MDRV_CPU_PERIODIC_INT(arcadia_video_line, TIME_IN_HZ(262*60))
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128+2*XPOS, 262)
	MDRV_SCREEN_VISIBLE_AREA(0, 2*XPOS+128-1, 0, 262-1)
	MDRV_GFXDECODE( arcadia_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (arcadia_palette) / sizeof (arcadia_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof (arcadia_colortable) / sizeof(arcadia_colortable[0][0]))
	MDRV_PALETTE_INIT( arcadia )

	MDRV_VIDEO_START( arcadia )
	MDRV_VIDEO_UPDATE( arcadia )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(arcadia_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(arcadia)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vcg)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

static int device_load_arcadia_cart(mess_image *image)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;

	memset(rom, 0, 0x8000);
	size = image_length(image);

	if (size > memory_region_length(REGION_CPU1))
		size = memory_region_length(REGION_CPU1);

	if (image_fread(image, rom, size) != size)
		return INIT_FAIL;

	if (size > 0x1000)
		memmove(rom + 0x2000, rom + 0x1000, size - 0x1000);
        if (size > 0x2000)
                memmove(rom + 0x4000, rom + 0x3000, size - 0x2000);

#if 1
	// golf cartridge support
	// 4kbyte at 0x0000
	// 2kbyte at 0x4000
        if (size<=0x2000) memcpy (rom+0x4000, rom+0x2000, 0x1000);
#else
	/* this is a testpatch for the golf cartridge
	   so it could be burned in a arcadia 2001 cartridge
	   activate it and use debugger to save patched version */
	// not enough yet (some pointers stored as data?)
	struct { UINT16 address; UINT8 old; UINT8 neu; }
	patch[]= {
		{ 0x0077,0x40,0x20 },
		{ 0x011e,0x40,0x20 },
		{ 0x0348,0x40,0x20 },
		{ 0x03be,0x40,0x20 },
		{ 0x04ce,0x40,0x20 },
		{ 0x04da,0x40,0x20 },
		{ 0x0562,0x42,0x22 },
		{ 0x0617,0x40,0x20 },
		{ 0x0822,0x40,0x20 },
		{ 0x095e,0x42,0x22 },
		{ 0x09d3,0x42,0x22 },
		{ 0x0bb0,0x42,0x22 },
		{ 0x0efb,0x40,0x20 },
		{ 0x0ec1,0x43,0x23 },
		{ 0x0f00,0x40,0x20 },
		{ 0x0f12,0x40,0x20 },
		{ 0x0ff5,0x43,0x23 },
		{ 0x0ff7,0x41,0x21 },
		{ 0x0ff9,0x40,0x20 },
		{ 0x0ffb,0x41,0x21 },
		{ 0x20ec,0x42,0x22 }
	};
	for (int i=0; i<ARRAY_LENGTH(patch); i++) {
	    assert(rom[patch[i].address]==patch[i].old);
	    rom[patch[i].address]=patch[i].neu;
	}
#endif
	return INIT_PASS;
}

static void arcadia_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_arcadia_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(arcadia)
	CONFIG_DEVICE(arcadia_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static DRIVER_INIT( arcadia )
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
#if 0
	// this is here to allow developement of some simple testroutines
	// for a real console
	{
	    UINT8 *rom=memory_region(REGION_CPU1);
	    /* this is a simple routine to display all rom characters
	       on the display for a snapshot */
	    UINT8 prog[]={ // address 0 of course
		0x20, // eorz, 0
		0x1b, 0x01, // bctr,a $0004
		0x17, // retc a
		0x76, 0x20, // ppsu ii

		// fill screen
		0x04, 0x00, // lodi,0 0
		0x04|1, 0x00, // lodi,1 0
		0xcc|1, 0x78, 0x10, //a: stra,0 $1800,r1
		0x75,9, //cpsl wc|c
		0x84,0x41, // addi,0 0x41
		0x75,9, //cpsl wc|c
		0x84|1, 0x01, // addi,1 1
		0xe4|1, 0x40, // comi,1 40
		0x98, 0x80-15, // bcfr,0 a

		0x04, 0xff, // lodi,0 7
		0xcc, 0x18, 0xfc, // stra,0 $19f8
		0x04, 0x00, // lodi,0 7
		0xcc, 0x18, 0xfd, // stra,0 $18fd
		0x04, 0x07, // lodi,0 7
		0xcc, 0x19, 0xf9, // stra,0 $19f9

		0x04, 0x00, // lodi,0 7
		0xcc, 0x19, 0xbe, // stra,0 $19bf
		0x04, 0x00, // lodi,0 7
		0xcc, 0x19, 0xbf, // stra,0 $19bf

		//loop: 0x0021
		// print keyboards
		0x04|1, 0x00, //y:lodi,1 0
		0x0c|1, 0x79, 0x00, //x: ldra,0 1900,r1
		0x44|0, 0x0f, //andi,0 0f
		0x64|0, 0x10, //ori,0  10
		0xcc|1, 0x78, 0x01, //stra,0 1840,r1
		0x75,9, //cpsl wc|c
		0x84|1, 0x01, //addi,1 1
		0xe4|1, 0x09, //comi,1 9
		0x98, 0x80-18, //bcfr,0 x

		// cycle colors
		0x0c|1, 0x19, 0x00, //ldra,1 1900
		0x44|1, 0xf, //andi,0 0f
		0xe4|1, 1, //comi,1 1
		0x98, +10, //bcfr,0 c
		0x0c, 0x19, 0xbf,//ldra,0 19f9,0
		0x84, 1, //addi,0 1
		0xcc, 0x19, 0xbf, //stra,0 19f9,0
		0x18|3, 12, // bctr,a
		0xe4|1, 2, //c:comi,1 2
		0x98, +10, //bcfr,0 d
		0x0c, 0x19, 0xbf, //ldra,0 19f9,0
		0x84, 8, //addi,0 8
		0xcc, 0x19, 0xbf, //stra,0 19f9,0
		0x18|3, 12, // bctr,a

		// cycle colors
		0xe4|1, 4, //comi,1 4
		0x98, +10, //bcfr,0 c
		0x0c, 0x19, 0xbe,//ldra,0 19f9,0
		0x84, 1, //addi,0 1
		0xcc, 0x19, 0xbe, //stra,0 19f9,0
		0x18|3, 12, // bctr,a
		0xe4|1, 8, //c:comi,1 2
		0x98, +8+9, //bcfr,0 d
		0x0c, 0x19, 0xbe, //ldra,0 19f9,0
		0x84, 8, //addi,0 8
		0xcc, 0x19, 0xbe, //stra,0 19f9,0

		0x0c, 0x19, 0x00, //b: ldra,0 1900
		0x44|0, 0xf, //andi,0 0f
		0xe4, 0, //comi,0 0
		0x98, 0x80-9, //bcfr,0 b

		0x0c, 0x19, 0xbe, //ldra,0 19bf
		0xcc, 0x19, 0xf8, //stra,0 19f8
		0x0c, 0x19, 0xbf, //ldra,0 19bf
		0xcc, 0x19, 0xf9, //stra,0 19f8

		0x0c, 0x19, 0xbe, //ldra,0 17ff
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0d, //stra,0 180f
		0x0c, 0x19, 0xbe, //x: ldra,0 19bf
		0x50, 0x50, 0x50, 0x50, //shr,0 4
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0c, //stra,0 180e

		0x0c, 0x19, 0xbf, //ldra,0 17ff
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0f, //stra,0 180f
		0x0c, 0x19, 0xbf, //x: ldra,0 19bf
		0x50, 0x50, 0x50, 0x50, //shr,0 4
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0e, //stra,0 180e

		0x0c, 0x18, 0x00, //ldra,0 1800
		0x84, 1, //addi,0 1
		0xcc, 0x18, 0x00, //stra,0 1800

//		0x1b, 0x80-20-29-26-9-8-2 // bctr,a y
		0x1c|3, 0, 0x32, // bcta,3 loop

		// calling too many subdirectories causes cpu to reset!
		// bxa causes trap
	    };
#if 1
	    FILE *f;
	    f=fopen("chartest.bin","wb");
	    fwrite(prog, ARRAY_LENGTH(prog), sizeof(prog[0]), f);
	    fclose(f);
#endif
	    for (i=0; i<ARRAY_LENGTH(prog); i++) rom[i]=prog[i];

	}
#endif
}

/*    YEAR	NAME		PARENT		COMPAT	MACHINE   INPUT     INIT		COMPANY		FULLNAME */
// marketed from several firms/names

CONS(1982,	arcadia,	0,			0,		arcadia,  arcadia,  arcadia,	arcadia,	"Emerson",		"Arcadia 2001", GAME_IMPERFECT_SOUND )
// schmid tvg 2000 (developer? PAL)

// different cartridge connector
// hanimex mpt 03 model

// different cartridge connector (same size as mpt03, but different pinout!)
// 16 keys instead of 12
CONS(198?, vcg,		arcadia,	0,		arcadia,  vcg,		arcadia,	arcadia,	"Palladium",		"VIDEO - COMPUTER - GAME", GAME_IMPERFECT_SOUND )
