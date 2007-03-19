/******************************************************************************
 PeT mess@utanet.at November 2000
******************************************************************************/


/* 
chess champion super system III
-------------------------------
(germany chess champion mk III)

6502
6522 ($6000?)
2x2114 (1kx4 sram) ($0000)
signetics 7947e c19081e ss-3-lrom
signetics 7945e c19082 ss-3-hrom ($d000??)
(both connected to the same pins!,
look into mess source mess/messroms/rddil24.c for notes)

optional printer (special serial connection)
optional board display (special serial connection)
internal expansion/cartridge port
 (special power saving pack)
*/

#include "driver.h"

#include "includes/ssystem3.h"
#include "machine/6522via.h"
#include "cpu/m6502/m6502.h"



/*
  port b
   bit 0: ??

    hi speed serial 1 
   bit 1: output data
   bit 2: output clock (hi data is taken)

	bit 6: input clocks!?

 */

struct via6522_interface config=
{
	0,//read8_handler in_a_func;
	0,//read8_handler in_b_func;
	0,//read8_handler in_ca1_func;
	0,//read8_handler in_cb1_func;
	0,//read8_handler in_ca2_func;
	0,//read8_handler in_cb2_func;
	0,//write8_handler out_a_func;
	0,//write8_handler out_b_func;
	0,//write8_handler out_ca2_func;
	0,//write8_handler out_cb2_func;
	0,//void (*irq_func)(int state);
};

static DRIVER_INIT( ssystem3 )
{
	via_config(0,&config);
}

static INTERRUPT_GEN( ssystem3_frame_int )
{
	static int toggle=0;
	via_set_input_b(0,toggle?0x40:0);
	toggle^=0;
}

static MACHINE_RESET( ssystem3 )
{
	via_reset();
}

static ADDRESS_MAP_START( ssystem3_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03ff) AM_RAM
//	AM_RANGE( 0x4000, 0x40ff) AM_NOP	/* lcd chip!? */
	AM_RANGE( 0x6000, 0x600f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE( 0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)

INPUT_PORTS_START( ssystem3 )
	PORT_START
DIPS_HELPER( 0x001, "NEW GAME", KEYCODE_F3, CODE_NONE) // seams to be direct wired to reset
	DIPS_HELPER( 0x002, "CLEAR", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x004, "ENTER", KEYCODE_ENTER, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "Black A    Black", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x002, "Black B    Field", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x004, "Black C    Time?", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x008, "Black D    Time?", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x010, "Black E    Time off?", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x020, "Black F    LEVEL", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x040, "Black G    Swap", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x080, "Black H    White", KEYCODE_H, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "White 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x002, "White 2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x004, "White 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x008, "White 4", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x010, "White 5", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x020, "White 6", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x040, "White 7", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x080, "White 8", KEYCODE_8, CODE_NONE)
#if 0
	PORT_START
	DIPS_HELPER( 0x001, "Test 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x002, "Test 2", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "Test 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x008, "Test 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "Test 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x020, "Test 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "Test 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x080, "Test 8", KEYCODE_8_PAD, CODE_NONE)
#endif
INPUT_PORTS_END



static MACHINE_DRIVER_START( ssystem3 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP(ssystem3_map, 0)
	MDRV_CPU_VBLANK_INT(ssystem3_frame_int, 1)
	MDRV_SCREEN_REFRESH_RATE(LCD_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( ssystem3 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(728, 437)
	MDRV_SCREEN_VISIBLE_AREA(0, 728-1, 0, 437-1)
	MDRV_PALETTE_LENGTH(242 + 32768)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT( ssystem3 )

	MDRV_VIDEO_START( ssystem3 )
	MDRV_VIDEO_UPDATE( ssystem3 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


ROM_START(ssystem3)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("ss3lrom", 0xc000, 0x1000, CRC(9ea46ed3))
	ROM_LOAD("ss3hrom", 0xf000, 0x1000, CRC(52741e0b))
	ROM_RELOAD(0xd000, 0x1000)
/* 0xd450 reset,irq,nmi

   d7c7 outputs 2e..32 to serial port 1
 */

ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT		INIT		CONFIG		COMPANY		FULLNAME */
CONS( 1979,	ssystem3, 0, 		0,		ssystem3, ssystem3,	ssystem3,	NULL,		"NOVAG Industries Ltd.",  "Chess Champion Super System III", 0) 
//chess champion MK III in germany
