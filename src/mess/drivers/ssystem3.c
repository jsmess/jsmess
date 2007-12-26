/******************************************************************************

  ssystem3.c

Driver file to handle emulation of the Chess Champion Super System III / Chess
Champion MK III
by PeT mess@utanet.at November 2000

Hardware descriptions:
- A 6502 CPU
- A 6522 VIA ($6000?) (PB6 and PB7 are tied)
- 2 x 2114  1kx4 SRAM to provide 1KB of RAM ($0000)
- 2xXXKB ROM (both connected to the same pins!,
  look into mess source mess/messroms/rddil24.c for notes)
  - signetics 7947e c19081e ss-3-lrom
  - signetics 7945e c19082 ss-3-hrom ($d000??)

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

static const struct via6522_interface config=
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

static TIMER_CALLBACK( ssystem3_pb6_toggle ) {
	static int toggle = 0;
	via_set_input_b( 0, toggle ? 0x40 : 0 );
	toggle ^= 1;
}

static MACHINE_RESET( ssystem3 )
{
	via_reset();
	timer_pulse( ATTOTIME_IN_HZ(4000000), NULL, 0, ssystem3_pb6_toggle );
}

static ADDRESS_MAP_START( ssystem3_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03ff) AM_RAM
//	AM_RANGE( 0x4000, 0x40ff) AM_NOP	/* lcd chip!? */
	AM_RANGE( 0x6000, 0x600f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE( 0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static INPUT_PORTS_START( ssystem3 )
	PORT_START
PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NEW GAME") PORT_CODE(KEYCODE_F3) // seams to be direct wired to reset
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_START
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black A    Black") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black B    Field") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black C    Time?") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black D    Time?") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black E    Time off?") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black F    LEVEL") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black G    Swap") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black H    White") PORT_CODE(KEYCODE_H)
	PORT_START
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White 8") PORT_CODE(KEYCODE_8)
#if 0
	PORT_START
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test 8") PORT_CODE(KEYCODE_8_PAD)
#endif
INPUT_PORTS_END



static MACHINE_DRIVER_START( ssystem3 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP(ssystem3_map, 0)
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
	ROM_LOAD("ss3lrom", 0xc000, 0x1000, CRC(9ea46ed3) SHA1(34eef85b356efbea6ddac1d1705b104fc8e2731a) )
	ROM_LOAD("ss3hrom", 0xf000, 0x1000, CRC(52741e0b) SHA1(2a7b950f9810c5a14a1b9d5e6b2bd93da621662e) )
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
