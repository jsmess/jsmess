/******************************************************************************
 Contributors:

	Marat Fayzullin (MG source)
	Charles Mac Donald
	Mathis Rosenhauer
	Brad Oliver
	Michael Luong

 To do:

 - PSG control for Game Gear (needs custom SN76489 with stereo output for each channel)
 - SIO interface for Game Gear (needs netplay, I guess)
 - SMS lightgun support
 - LCD persistence emulation for GG
 - SMS 3D glass support

 The Game Gear SIO and PSG hardware are not emulated but have some
 placeholders in 'machine/sms.c'

 Changes:
	Apr 02 - Added raster palette effects for SMS & GG (ML)
				 - Added sprite collision (ML)
				 - Added zoomed sprites (ML)
	May 02 - Fixed paging bug (ML)
				 - Fixed sprite and tile priority bug (ML)
				 - Fixed bug #66 (ML)
				 - Fixed bug #78 (ML)
				 - try to implement LCD persistence emulation for GG (ML)
	Jun 10, 02 - Added bios emulation (ML)
	Jun 12, 02 - Added PAL & NTSC systems (ML)
	Jun 25, 02 - Added border emulation (ML)
	Jun 27, 02 - Version bits for Game Gear (bits 6 of port 00) (ML)
	Nov-Dec, 05 - Numerous cleanups, fixes, updates (WP)
	Mar, 07 - More cleanups, fixes, mapper additions, etc (WP)

SMS Store Unit memory map for the second CPU:

0000-3FFF - BIOS
4000-47FF - RAM
8000      - System Control Register (R/W)
            Reading:
            bit7      - ready (0 = ready, 1 = not ready)
            bit6-bit5 - unknown
            bit4-bit3 - timer selection bit switches
            bit2-bit0 - unknown
            Writing:
            bit7-bit4 - unknown, maybe led of selected game to set?
            bit3      - unknown, 1 seems to be written all the time
            bit2      - unknown, 1 seems to be written all the time
            bit1      - reset signal for sms cpu, 0 = reset low, 1 = reset high
            bit0      - which cpu receives interrupt signals, 0 = sms cpu, 1 = controlling cpu
C000      - Card/Cartridge selction register (W)
            bit7-bit4 - slot to select
            bit3      - slot type (0 = cartridge, 1 = card ?)
            bit2-bit0 - unknown
C400      - ???? (used once)
D800      - Selection buttons #1, 1-8 (R)
DC00      - Selection buttons #2, 9-16 (R)

 ******************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "sound/2413intf.h"
#include "video/generic.h"
#include "includes/sms.h"
#include "video/smsvdp.h"
#include "devices/cartslot.h"

#define MASTER_CLOCK_NTSC	53693175
#define MASTER_CLOCK_PAL	53203400	/* This might be a tiny bit too low */

static ADDRESS_MAP_START( sms_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03FF) AM_ROMBANK(1)					/* First 0x0400 part always points to first page */
	AM_RANGE(0x0400, 0x3FFF) AM_ROMBANK(2)					/* switchable rom bank */
	AM_RANGE(0x4000, 0x7FFF) AM_ROMBANK(3)					/* switchable rom bank */
	AM_RANGE(0x8000, 0x9FFF) AM_READWRITE(MRA8_BANK4, sms_cartram_w)	/* ROM bank / on-cart RAM */
	AM_RANGE(0xA000, 0xBFFF) AM_READWRITE(MRA8_BANK5, sms_cartram2_w)	/* ROM bank / on-cart RAM */
	AM_RANGE(0xC000, 0xDFFB) AM_MIRROR(0x2000) AM_RAM			/* RAM (mirror at 0xE000) */
	AM_RANGE(0xDFFC, 0xDFFF) AM_RAM						/* RAM "underneath" frame registers */
	AM_RANGE(0xFFFC, 0xFFFF) AM_READWRITE(sms_mapper_r, sms_mapper_w)	/* Bankswitch control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sms_store_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3FFF) AM_ROM						/* BIOS */
	AM_RANGE(0x4000, 0x47FF) AM_RAM						/* RAM */
	AM_RANGE(0x6000, 0x7FFF) AM_ROMBANK(10)					/* Cartridge/card peek area */
	AM_RANGE(0x8000, 0x8000) AM_READWRITE(sms_store_control_r, sms_store_control_w)	/* Control */
	AM_RANGE(0xC000, 0xC000) AM_READWRITE(sms_store_cart_select_r, sms_store_cart_select_w) 	/* cartridge/card slot selector */
	AM_RANGE(0xD800, 0xD800) AM_READ(sms_store_select1)			/* Game selector port #1 */
	AM_RANGE(0xDC00, 0xDC00) AM_READ(sms_store_select2)			/* Game selector port #2 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sms_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x3e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x3e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_count_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0) AM_MIRROR(0x1e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xC1, 0xC1) AM_MIRROR(0x1e) AM_READ(sms_version_r)
	AM_RANGE(0xE0, 0xE0) AM_MIRROR(0x0e) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xE1, 0xE1) AM_MIRROR(0x0e) AM_READ(sms_version_r)
	AM_RANGE(0xF0, 0xF0)				 AM_READWRITE(sms_input_port_0_r, sms_YM2413_register_port_0_w)
	AM_RANGE(0xF1, 0xF1)				 AM_READWRITE(sms_version_r, sms_YM2413_data_port_0_w)
	AM_RANGE(0xF2, 0xF2)				 AM_READWRITE(sms_fm_detect_r, sms_fm_detect_w)
	AM_RANGE(0xF3, 0xF3)				 AM_READ(sms_version_r)
	AM_RANGE(0xF4, 0xF4) AM_MIRROR(0x02) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF5, 0xF5) AM_MIRROR(0x02) AM_READ(sms_version_r)
	AM_RANGE(0xF8, 0xF8) AM_MIRROR(0x06) AM_READ(sms_input_port_0_r)
	AM_RANGE(0xF9, 0xF9) AM_MIRROR(0x06) AM_READ(sms_version_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gg_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00)				 AM_READ(gg_input_port_2_r)
	AM_RANGE(0x01, 0x05)				 AM_READWRITE(gg_sio_r, gg_sio_w)
	AM_RANGE(0x06, 0x06)				 AM_READWRITE(gg_psg_r, gg_psg_w)
	AM_RANGE(0x07, 0x07) 				 AM_WRITE(sms_version_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x06) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x09, 0x09) AM_MIRROR(0x06) AM_WRITE(sms_version_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e) AM_WRITE(sms_version_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x1e) AM_WRITE(sms_bios_w) 
	AM_RANGE(0x21, 0x21) AM_MIRROR(0x1e) AM_WRITE(sms_version_w)
	AM_RANGE(0x40, 0x7F)                 AM_READWRITE(sms_count_r, SN76496_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_data_r, sms_vdp_data_w)
	AM_RANGE(0x80, 0x81) AM_MIRROR(0x3e) AM_READWRITE(sms_vdp_ctrl_r, sms_vdp_ctrl_w)
	AM_RANGE(0xC0, 0xC0)				 AM_READ(input_port_0_r)
	AM_RANGE(0xC1, 0xC1)				 AM_READ(input_port_1_r)
	AM_RANGE(0xDC, 0xDC)				 AM_READ(input_port_0_r)
	AM_RANGE(0xDD, 0xDD)				 AM_READ(input_port_1_r)
ADDRESS_MAP_END


INPUT_PORTS_START( sms )

	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(10) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(10) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(10) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(10) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(20) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(20) PORT_PLAYER(2) PORT_8WAY

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(20) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(20) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* Software Reset bit */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START ) /* Game Gear START */

	PORT_START	/* IN3 - Light phaser X - player 1 */
//	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR( X, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_CATEGORY(11) PORT_PLAYER(1)

	PORT_START	/* IN4 - Light phaser Y - player 1 */
//	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR( Y, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_CATEGORY(11) PORT_PLAYER(1)

	PORT_START	/* IN5 - Light phaser X - player 2 */
//	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR( X, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEUDELTA(15) PORT_CATEGORY(21) PORT_PLAYER(2)

	PORT_START	/* IN6 - Light phaser Y - player 2 */
//	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR( Y, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(25) PORT_CATEGORY(21) PORT_PLAYER(2)

	PORT_START	/* IN7 - Rapid Fire Unit */
	PORT_DIPNAME( 0x03, 0x00, "Rapid Fire Unit - Player 1" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x01, "Button A" )
	PORT_DIPSETTING( 0x02, "Button B" )
	PORT_DIPSETTING( 0x03, "Button A+B" )
	PORT_DIPNAME( 0x0c, 0x00, "Rapid Fire Unit - Player 2" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x04, "Button A" )
	PORT_DIPSETTING( 0x08, "Button B" )
	PORT_DIPSETTING( 0x0C, "Button A+B" )

	PORT_START	/* IN8 - Paddle player 1 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(20) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(12) PORT_PLAYER(1)

	PORT_START	/* IN9 - Paddle player 2 */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(20) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(22) PORT_PLAYER(2)

	PORT_START	/* IN10 - Light Phaser and Paddle Control buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(11) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(12) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(13) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(13) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(22) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(23) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(23) PORT_PLAYER(2)

	PORT_START	/* IN11 - Controller selection */
	PORT_CATEGORY_CLASS( 0x0F, 0x00, "Player 1 Controller" )
	PORT_CATEGORY_ITEM( 0x00, DEF_STR( Joystick ), 10 )
//	PORT_CATEGORY_ITEM( 0x01, "Light Phaser", 11 )
	PORT_CATEGORY_ITEM( 0x02, "Sega Paddle Control", 12 )
	PORT_CATEGORY_ITEM( 0x03, "Sega Sports Pad", 13 )
	PORT_CATEGORY_CLASS( 0xF0, 0x00, "Player 2 Controller" )
	PORT_CATEGORY_ITEM( 0x00, DEF_STR( Joystick ), 20 )
//	PORT_CATEGORY_ITEM( 0x10, "Light Phaser", 21 )
	PORT_CATEGORY_ITEM( 0x20, "Sega Paddle Control", 22 )
	PORT_CATEGORY_ITEM( 0x30, "Sega Sports Pad", 23 )

	PORT_START	/* IN12 - Player 1 Sports Pad X axis */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(40) PORT_RESET PORT_REVERSE PORT_CATEGORY(13) PORT_PLAYER(1)

	PORT_START	/* IN13 - Player 1 Sports Pad Y axis */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(40) PORT_RESET PORT_REVERSE PORT_CATEGORY(13) PORT_PLAYER(1)

	PORT_START	/* IN14 - Player 2 Sports Pad X axis */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(40) PORT_RESET PORT_REVERSE PORT_CATEGORY(23) PORT_PLAYER(2)

	PORT_START	/* IN15 - Player 2 Sports Pad Y axis */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(40) PORT_RESET PORT_REVERSE PORT_CATEGORY(23) PORT_PLAYER(2)

INPUT_PORTS_END

static PALETTE_INIT( sms ) {
	int i;
	for( i = 0; i < 64; i++ ) {
		int r = i & 0x03;
		int g = ( i & 0x0C ) >> 2;
		int b = ( i & 0x30 ) >> 4;
		palette_set_color_rgb(machine, i, r << 6, g << 6, b << 6 );
	}
	/* TMS9918 palette */
	palette_set_color_rgb(machine, 64+ 0,   0,   0,   0 );
	palette_set_color_rgb(machine, 64+ 1,   0,   0,   0 );
	palette_set_color_rgb(machine, 64+ 2,  33, 200,  66 );
	palette_set_color_rgb(machine, 64+ 3,  94, 220, 120 );
	palette_set_color_rgb(machine, 64+ 4,  84,  85, 237 );
	palette_set_color_rgb(machine, 64+ 5, 125, 118, 252 );
	palette_set_color_rgb(machine, 64+ 6, 212,  82,  77 );
	palette_set_color_rgb(machine, 64+ 7,  66, 235, 245 );
	palette_set_color_rgb(machine, 64+ 8, 252,  85,  84 );
	palette_set_color_rgb(machine, 64+ 9, 255, 121, 120 );
	palette_set_color_rgb(machine, 64+10, 212, 193,  84 );
	palette_set_color_rgb(machine, 64+11, 230, 206, 128 );
	palette_set_color_rgb(machine, 64+12,  33, 176,  59 );
	palette_set_color_rgb(machine, 64+13, 201,  91, 186 );
	palette_set_color_rgb(machine, 64+14, 204, 204, 204 );
	palette_set_color_rgb(machine, 64+15, 255, 255, 255 );
}

static PALETTE_INIT( gamegear ) {
	int i;
	for( i = 0; i < 4096; i++ ) {
		int r = i & 0x000F;
		int g = ( i & 0x00F0 ) >> 4;
		int b = ( i & 0x0F00 ) >> 8;
		palette_set_color_rgb(machine, i, r << 4, g << 4, b << 4 );
	}
}

const smsvdp_configuration config_315_5124 = { MODEL_315_5124, sms_int_callback };
const smsvdp_configuration config_315_5246 = { MODEL_315_5246, sms_int_callback };
const smsvdp_configuration config_315_5378 = { MODEL_315_5378, sms_int_callback };
const smsvdp_configuration config_store = { MODEL_315_5124, sms_store_int_callback };

VIDEO_START(sega_315_5124) {
	smsvdp_video_init( &config_315_5124 );
}

VIDEO_START(sega_315_5246) {
	smsvdp_video_init( &config_315_5246 );
}

VIDEO_START(sega_315_5378) {
	smsvdp_video_init( &config_315_5378 );
}

VIDEO_START(sega_store_315_5124) {
	smsvdp_video_init( &config_store );
}

static MACHINE_DRIVER_START(sms1ntsc)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK_NTSC/15)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(sms_io, 0)

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(sms)
	MDRV_MACHINE_RESET(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS(MASTER_CLOCK_NTSC/10, SMS_X_PIXELS, LBORDER_START + LBORDER_X_PIXELS - 2, LBORDER_START + LBORDER_X_PIXELS + 256 + 10, NTSC_Y_PIXELS, TBORDER_START + NTSC_224_TBORDER_Y_PIXELS, TBORDER_START + NTSC_224_TBORDER_Y_PIXELS + 224)

	MDRV_PALETTE_LENGTH(64+16)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(sms)

	MDRV_VIDEO_START(sega_315_5124)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SMSIII, MASTER_CLOCK_NTSC/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(sms2ntsc)
	MDRV_IMPORT_FROM(sms1ntsc)
	MDRV_VIDEO_START(sega_315_5246)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smssdisp)
	MDRV_IMPORT_FROM(sms1ntsc)

	MDRV_VIDEO_START(sega_store_315_5124)

	MDRV_CPU_ADD_TAG("control", Z80, MASTER_CLOCK_NTSC/15)
	MDRV_CPU_PROGRAM_MAP(sms_store_mem, 0)
	/* Both CPUs seem to communicate with the VDP etc? */
	MDRV_CPU_IO_MAP(sms_io, 0)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(sms1pal)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK_PAL/15)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(sms_io, 0)

	MDRV_INTERLEAVE(1)
        
	MDRV_MACHINE_START(sms)
	MDRV_MACHINE_RESET(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS(MASTER_CLOCK_PAL/10, SMS_X_PIXELS, LBORDER_START + LBORDER_X_PIXELS - 2, LBORDER_START + LBORDER_X_PIXELS + 256 + 10, PAL_Y_PIXELS, TBORDER_START + PAL_240_TBORDER_Y_PIXELS, TBORDER_START + PAL_240_TBORDER_Y_PIXELS + 240)

	MDRV_PALETTE_LENGTH(64+16)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(sms)
 
	MDRV_VIDEO_START(sega_315_5124)
	MDRV_VIDEO_UPDATE(sms)
 
	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SMSIII, MASTER_CLOCK_PAL/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(sms2pal)
	MDRV_IMPORT_FROM(sms1pal)
	MDRV_VIDEO_START(sega_315_5246)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsfm)
	MDRV_IMPORT_FROM(sms1ntsc)

	MDRV_SOUND_ADD(YM2413, MASTER_CLOCK_NTSC/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(sms2fm)
	MDRV_IMPORT_FROM(sms2ntsc)

	MDRV_SOUND_ADD(YM2413, MASTER_CLOCK_NTSC/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(gamegear)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK_NTSC/15)
	MDRV_CPU_PROGRAM_MAP(sms_mem, 0)
	MDRV_CPU_IO_MAP(gg_io, 0)

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(sms)
	MDRV_MACHINE_RESET(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS(MASTER_CLOCK_NTSC/10, SMS_X_PIXELS, LBORDER_START + LBORDER_X_PIXELS + 6*8, LBORDER_START + LBORDER_X_PIXELS + 26*8, NTSC_Y_PIXELS, TBORDER_START + NTSC_192_TBORDER_Y_PIXELS + 3*8, TBORDER_START + NTSC_192_TBORDER_Y_PIXELS + 21*8 )
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(gamegear)

	MDRV_VIDEO_START(sega_315_5378)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(GAMEGEAR, MASTER_CLOCK_NTSC/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START(sms1)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_SYSTEM_BIOS( 0, "bios13", "US/European BIOS v1.3 (1986)" )
	ROMX_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "hangonsh", "US/European BIOS v2.4 with Hang On and Safari Hunt (1988)" )
	ROMX_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "hangon", "US/European BIOS v3.4 with Hang On (1988)" )
	ROMX_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "missiled", "US/European BIOS v4.4 with Missile Defense 3D (1988)" )
	ROMX_LOAD("missiled.rom", 0x0000, 0x20000, CRC(E79BB689) SHA1(aa92ae576ca670b00855e278378d89e9f85e0351), ROM_BIOS(4))
	ROM_SYSTEM_BIOS( 4, "proto", "US Master System Prototype BIOS" )
	ROMX_LOAD("m404prot.rom", 0x0000, 0x2000, CRC(1a15dfcc) SHA1(4a06c8e66261611dce0305217c42138b71331701), ROM_BIOS(5))
ROM_END

ROM_START(sms)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_SYSTEM_BIOS( 0, "alexkidd", "US/European BIOS with Alex Kidd in Miracle World (1990)" )
	ROMX_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051), ROM_BIOS(1))
ROM_END

ROM_START(smssdisp)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x4000, REGION_USER1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x4000, REGION_CPU2, 0)
	ROM_LOAD("smssdisp.rom", 0x0000, 0x4000, CRC(ee2c29ba) SHA1(fc465122134d95363112eb51b9ab71db3576cefd))
ROM_END

ROM_START(sms1pal)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x20000, REGION_USER1, 0)
    ROM_SYSTEM_BIOS( 0, "bios13", "US/European BIOS v1.3 (1986)" )
	ROMX_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54) SHA1(c315672807d8ddb8d91443729405c766dd95cae7), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "hangonsh", "US/European BIOS v2.4 with Hang On and Safari Hunt (1988)" )
	ROMX_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385) SHA1(9e179392cd416af14024d8f31c981d9ee9a64517), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "hangon", "Sega Master System - US/European BIOS v3.4 with Hang On (1988)" )
	ROMX_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6) SHA1(51fd6d7990f62cd9d18c9ecfc62ed7936169107e), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "missiled", "US/European BIOS v4.4 with Missile Defense 3D (1988)" )
	ROMX_LOAD("missiled.rom", 0x0000, 0x20000, CRC(E79BB689) SHA1(aa92ae576ca670b00855e278378d89e9f85e0351), ROM_BIOS(4))
ROM_END

ROM_START(smspal)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROM_SYSTEM_BIOS( 0, "alexkidd", "US/European BIOS with Alex Kidd in Miracle World (1990)" )
	ROMX_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA) SHA1(3af7b66248d34eb26da40c92bf2fa4c73a46a051), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "sonic", "European/Brazilian BIOS with Sonic the Hedgehog (1991)" )
	ROMX_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B) SHA1(6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df), ROM_BIOS(2))
ROM_END

ROM_START(sg1000m3)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
ROM_END

ROM_START(smsj)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x4000, REGION_USER1, 0)
    ROM_SYSTEM_BIOS( 0, "jbios21", "Japanese BIOS v2.1 (1987)" )
	ROMX_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13) SHA1(a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2), ROM_BIOS(1))
ROM_END

ROM_START(sms2kr)
	ROM_REGION(0x4000, REGION_CPU1, 0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_SYSTEM_BIOS( 0, "akbioskr", "Samsung Gam*Boy II with Alex Kidd in Miracle World (1990)" )
	ROMX_LOAD("akbioskr.rom", 0x000, 0x20000, CRC(9c5bad91) SHA1(2feafd8f1c40fdf1bd5668f8c5c02e5560945b17), ROM_BIOS(1))
ROM_END

ROM_START(gamegear)
	ROM_REGION(0x4000, REGION_CPU1,0)
	ROM_FILL(0x0000,0x4000,0xFF)
	ROM_REGION(0x0400, REGION_USER1, 0)
	ROM_SYSTEM_BIOS( 0, "none", "No BIOS" ) /* gamegear */
	ROM_SYSTEM_BIOS( 1, "majesco", "Majesco BIOS" ) /* gamg */
	ROMX_LOAD("majbios.rom", 0x0000, 0x0400, CRC(0EBEA9D4) SHA1(914aa165e3d879f060be77870d345b60cfeb4ede), ROM_BIOS(2))
ROM_END

#define rom_gamegeaj rom_gamegear

static void sms_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:				info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:		info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:				info->init = device_init_sms_cart; break;
		case DEVINFO_PTR_LOAD:				info->load = device_load_sms_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "sms"); break;

		default:					cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(sms)
	CONFIG_DEVICE(sms_cartslot_getinfo)
SYSTEM_CONFIG_END

static void sg1000_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info) {
	/* cartslot */
	switch(state) {
		case DEVINFO_INT_MUST_BE_LOADED:		info->i = 1; break;
		default:					sms_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(sg1000)
	CONFIG_DEVICE(sg1000_cartslot_getinfo)
SYSTEM_CONFIG_END

static void smssdisp_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info) {
	switch(state) {
		case DEVINFO_INT_COUNT:				info->i = 5; break;
		case DEVINFO_INT_MUST_BE_LOADED:	info->i = 1; break;
		default:					sms_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(smssdisp)
	CONFIG_DEVICE(smssdisp_cartslot_getinfo)
SYSTEM_CONFIG_END

static void gamegear_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_INT_COUNT:				info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:		info->i = 1; break;
		case DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "gg"); break;

		default:					sms_cartslot_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(gamegear)
	CONFIG_DEVICE(gamegear_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

  US
   - Sega Master System I (sms1)
     - prototype bios - 1986
     - without built-in game v1.3 - 1986
     - built-in Hang On/Safari Hunt v2.4 - 1988
     - built-in Hang On v3.4 - 1988
     - built-in Missile Defense 3-D v4.4 - 1988
     - built-in Hang On/Astro Warrior ????
   - Sega Master System II (sms/sms2)
     - built-in Alex Kidd in Miracle World - 1990

  JP
   - Sega SG-1000 Mark III (smsm3)
     - no bios
   - Sega Master System (I) (smsj)
     - without built-in game v2.1 - 1987

  KR
   - Sega Master System II (sms2kr)
     - built-in Alex Kidd in Miracle World (Korean)

  EU
   - Sega Master System I (sms1pal)
     - without built-in game v1.3 - 1986
     - built-in Hang On/Safari Hunt v2.4 - 1988
     - built-in Hang On v3.4 - 1988
     - built-in Missile Defense 3-D v4.4 - 1988
     - built-in Hang On/Astro Warrior ????
   - Sega Master System II (sms2pal)
     - built-in Alex Kidd in Miracle World - 1990
     - built-in Sonic the Hedgehog - 1991

  BR
   - Sega Master System I - 1987
   - Sega Master System II???
   - Sega Master System III - Tec Toy, 1987
   - Sega Master System Compact - Tec Toy, 1992
   - Sega Master System Girl - Tec Toy, 1992

***************************************************************************/

/*		YEAR	NAME		PARENT		COMPATIBLE	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
CONSB(	1990,      sms,   0,    sms2, 0, sms2ntsc, sms, 0,    sms, "Sega", "Sega Master System II", FLAG_BIOS_FULL )
CONSB(  1986,     sms1, sms,    sms1, 0, sms1ntsc, sms, 0,    sms, "Sega", "Sega Master System I", FLAG_BIOS_FULL )

CONSB(	1986,  sms1pal, sms, sms1pal, 0,  sms1pal, sms, 0,    sms, "Sega", "Sega Master System I (PAL)" , FLAG_BIOS_FULL )
CONSB(  1990,   smspal, sms, sms2pal, 0,  sms2pal, sms, 0,    sms, "Sega", "Sega Master System II (PAL)", FLAG_BIOS_FULL )
CONS(   1984, sg1000m3, sms,          0,    smsfm, sms, 0, sg1000, "Sega", "Sega SG-1000 Mark III" , FLAG_REGION_JAPAN | FLAG_FM )
CONSB(	1987,     smsj, sms,    smsj, 0,    smsfm, sms, 0,    sms, "Sega", "Sega Master System (Japan)" , FLAG_REGION_JAPAN | FLAG_BIOS_2000 | FLAG_FM )
CONSB(  1990,   sms2kr, sms,  sms2kr, 0,   sms2fm, sms, 0,    sms, "Samsung", "Samsung Gam*Boy II (Korea)", FLAG_REGION_JAPAN | FLAG_BIOS_FULL | FLAG_FM )

CONSB(  1986, smssdisp, sms, sms2, 0, smssdisp, sms, 0, smssdisp, "Sega", "Sega Master System Store Display", GAME_NOT_WORKING )

CONSB(	1990, gamegear,        0, gamegear, sms, gamegear, sms, 0, gamegear, "Sega", "Sega Game Gear - European/American" , FLAG_GAMEGEAR )
CONSB(	1990, gamegeaj, gamegear, gamegear,   0, gamegear, sms, 0, gamegear, "Sega", "Sega Game Gear - Japanese" , FLAG_REGION_JAPAN | FLAG_GAMEGEAR | FLAG_BIOS_0400 )

