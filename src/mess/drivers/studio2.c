/*

Mustang 9016 Telespiel Computer
1978

PCB Layout
|----------------------------------------------------------------------------------------------------------|
|7805                              |----------------------------|                          CD4069  MC14001 |
|                                  |----------------------------|                                          |
|                                         (ROM cart slot)                                                  |
|                                                                                                          |
|       ROM.IC13  ROM.IC14      CDP1822  CDP1822 CDP1822 CDP1822                            |-----------|  |
|                                                                                           | Output    |  |
|                                                                                           |TV Module? |  |
| ROM.IC12                                                        CDP1802  CDP1864          |           |  |
|                 CDP1822                                                                   |           |  |
|                          CD4019 CDP1858 CD4081 CD4069                                     |           |  |
|                                                                                           |           |  |
|                                                        CD4515                             |           |  |
|                                                                          1.750MHz         |-----------|  |
|                                                                                              4.3236MHz   |
|                                                                                                          |
|                                                                                                          |
|                                                                                                          |
|----------------------------------------------------------------------------------------------------------|
Notes:
      CDP1802 - RCA CDP1802 Microprocessor
      CDP1864 - RCA CDP1864 PAL Video Display Controller (VDC)
      CDP1822 - RCA CDP1822 256 x4 RAM (= Mitsubishi M58721P)
      ROM.ICx - RCA CDP1833 1k x8 MASKROM. All ROMs are marked 'PROGRAM COPYRIGHT (C) RCA CORP. 1978'
      CD4019  - 4019 Quad AND-OR Select Gate (4000-series CMOS TTL logic IC)
      CDP1858 - Latch/Decoder - 4-bit
      CD4081  - 4081 Quad 2-Input AND Buffered B Series Gate (4000-series CMOS TTL logic IC)
      CD4069  - 4069 Hex Buffer, Inverter (4000-series CMOS TTL logic IC)
      CD4515  - 4515 4-Bit Latched/4-to-16 Line Decoders (4000-series CMOS TTL logic IC)
      7805    - Voltage regulator, input 10V-35V, output +5V
*/

/*

	TODO:

	- cdp1861/cdp1864 video
	- discrete sound
	- ram mirroring (mapped when A9=0 and ROM not present)
	- load st2 carts using header information
	- redump studio2 bios as 2 roms

*/

#include "driver.h"
#include "image.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1861.h"
#include "video/cdp1864.h"
#include "devices/cartslot.h"
#include "inputx.h"
#include "sound/beep.h"
#include "sound/discrete.h"

static UINT8 keylatch;
extern int cdp1861_efx;

/* Read/Write Handlers */

static WRITE8_HANDLER( keylatch_w )
{
	keylatch = data & 0x0f;
}

/* Memory Maps */

static ADDRESS_MAP_START( studio2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_RAM // AM_MIRROR(A9=0)
	AM_RANGE(0x0a00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( studio2_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(cdp1861_dispon_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
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

/* CDP1802 Configuration */

/* studio 2
   output q speaker (300 hz tone on/off)
   f1 dma_activ
   f3 on player 2 key pressed
   f4 on player 1 key pressed
   inp 1 video on
   out 2 read key value selects keys to put at f3/f4 */

static UINT8 studio2_ef(void)
{
	int ef = 0x0f;

	if (cdp1861_efx) ef -= EF1;

	if (readinputport(0) & (1 << keylatch)) ef -= EF3;
	if (readinputport(1) & (1 << keylatch)) ef -= EF4;

	return ef;
}

static void studio2_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG studio2_config = 
{
	NULL,
	cdp1861_dma_w,
	studio2_q,
	studio2_ef,
	cdp1861_sc
};

static CDP1802_CONFIG mpt02_config = 
{
	NULL,
	cdp1864_dma_w,
	studio2_q,
	studio2_ef,
	cdp1864_sc
};

/* Machine Initialization */

static MACHINE_RESET( studio2 )
{
	machine_reset_cdp1861(machine);

	cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
}

static MACHINE_RESET( mpt02 )
{
	machine_reset_cdp1864(machine);

	cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( studio2 )

	// basic machine hardware

	MDRV_CPU_ADD_TAG("main", CDP1802, 3579545.0/2)
	MDRV_CPU_PROGRAM_MAP(studio2_map, 0)
	MDRV_CPU_IO_MAP(studio2_io_map, 0)
	MDRV_CPU_CONFIG(studio2_config)
	MDRV_MACHINE_RESET(studio2)

    // video hardware

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(3579545.0/2, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(cdp1861)
	MDRV_VIDEO_UPDATE(cdp1861)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mpt02 )

	// basic machine hardware

	MDRV_CPU_ADD_TAG("main", CDP1802, CDP1864_CLK_FREQ)
	MDRV_CPU_PROGRAM_MAP(studio2_map, 0)
	MDRV_CPU_IO_MAP(studio2_io_map, 0)
	MDRV_CPU_CONFIG(mpt02_config)
	MDRV_MACHINE_RESET(mpt02)

    // video hardware

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1864_CLK_FREQ, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(cdp1861)
	MDRV_VIDEO_UPDATE(cdp1861)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( studio2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "studio2.rom", 0x0000, 0x0800, BAD_DUMP CRC(a494b339) SHA1(f2650dacc9daab06b9fdf0e7748e977b2907010c) ) // should be 2 roms
ROM_END

ROM_START( m9016tc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "86676.ic13",  0x0000, 0x0400, NO_DUMP )
	ROM_LOAD( "86677b.ic14", 0x0400, 0x0400, NO_DUMP )
	ROM_LOAD( "87201.ic12",  0x0800, 0x0400, NO_DUMP )
ROM_END

/* System Configuration */

#define ST2_HEADER_SIZE		256

DEVICE_LOAD( studio2_cart )
{
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

static void studio2_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
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

SYSTEM_CONFIG_START( studio2 )
	CONFIG_DEVICE( studio2_cartslot_getinfo )
SYSTEM_CONFIG_END

/* Driver Initialization */

static void setup_beep(int dummy)
{
	beep_set_state( 0, 0 );
	beep_set_frequency( 0, 300 );
}

static DRIVER_INIT( studio2 )
{
	timer_set(0.0, 0, setup_beep);
}

/* Game Drivers */

/*

	PAL clones of RCA Studio II using CDP1864C (color display):

	"Mustang", "9016 Telespiel Computer (Germany)"
	"Soundic", "MPT-02 Victory Home TV Programmer (Austria)"
	"Hanimex", "MPT-02 (France)"
	"Sheen",   "1200 Micro Computer (Australia)"

*/

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG      COMPANY   FULLNAME */
CONS( 1977,	studio2,	0,		0,		studio2,	studio2,	studio2,	studio2,	"RCA",		"Studio II", GAME_SUPPORTS_SAVE )
CONS( 1978,	m9016tc,	studio2,0,		mpt02,		studio2,	0,			studio2,	"Mustang",	"9016 Telespiel Computer (Germany)", GAME_SUPPORTS_SAVE )
