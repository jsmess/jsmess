/******************************************************************************
 PeT mess@utanet.at September 2000
******************************************************************************/

#include "driver.h"

#include "includes/rriot.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"
#include "mk2.lh"

#define M6504_MEMORY_LAYOUT

/* usage:

   under the black keys are operations to be added as first sign
   black and white box are only changing the player

   for the computer to start as white
    switch to black (h enter)
	swap players (g enter)
*/
/*
chess champion mk II

MOS MPS 6504 2179
MOS MPS 6530 024 1879
 layout of 6530 dumped with my adapter
 0x1300-0x133f io
 0x1380-0x13bf ram
 0x1400-0x17ff rom

2x2111 ram (256x4?)
MOS MPS 6332 005 2179
74145 bcd to decimal encoder (10 low active select lines)
(7400)

4x 7 segment led display (each with dot)
4 single leds
21 keys
*/

/*
  83, 84 contains display variables
 */
// only lower 12 address bits on bus!
static ADDRESS_MAP_START(mk2_mem , ADDRESS_SPACE_PROGRAM, 8)
#ifdef M6504_MEMORY_LAYOUT
	ADDRESS_MAP_FLAGS( AMEF_ABITS(13) ) // m6504
	AM_RANGE( 0x0000, 0x01ff) AM_RAM // 2 2111, should be mirrored
	AM_RANGE( 0x0b00, 0x0b0f) AM_READWRITE( rriot_0_r, rriot_0_w )
	AM_RANGE( 0x0b80, 0x0bbf) AM_RAM // rriot ram
	AM_RANGE( 0x0c00, 0x0fff) AM_ROM // rriot rom
	AM_RANGE( 0x1000, 0x1fff) AM_ROM
#else
	AM_RANGE( 0x0000, 0x01ff) AM_RAM // 2 2111, should be mirrored
	AM_RANGE( 0x8009, 0x8009) AM_NOP // bit $8009 (ora #$80) causes false accesses
	AM_RANGE( 0x8b00, 0x8b0f) AM_READWRITE( rriot_0_r, rriot_0_w )
	AM_RANGE( 0x8b80, 0x8bbf) AM_RAM // rriot ram
	AM_RANGE( 0x8c00, 0x8fff) AM_ROM // rriot rom
	AM_RANGE( 0xf000, 0xffff) AM_ROM
#endif
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)

INPUT_PORTS_START( mk2 )
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

static UINT8 mk2_led[5];

static void update_leds(int dummy)
{
	int i;

	for (i=0; i<4; i++)
		output_set_digit_value(i, mk2_led[i]);
	output_set_led_value(0, mk2_led[4]&8?1:0);
	output_set_led_value(1, mk2_led[4]&0x20?1:0);
	output_set_led_value(2, mk2_led[4]&0x10?1:0);
	output_set_led_value(3, mk2_led[4]&0x10?0:1);
	
	mk2_led[0]= mk2_led[1]= mk2_led[2]= mk2_led[3]= mk2_led[4]= 0;
}

static MACHINE_START( mk2 )
{
	timer_pulse(TIME_IN_HZ(60), 0, update_leds);
	return 0;
}

static MACHINE_RESET( mk2 )
{
	rriot_reset(0);
}

static MACHINE_DRIVER_START( mk2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1000000)        /* 6504 */
	MDRV_CPU_PROGRAM_MAP(mk2_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( mk2 )
	MDRV_MACHINE_RESET( mk2 )

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_mk2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


ROM_START(mk2)
	ROM_REGION(0x10000,REGION_CPU1,0)
#ifdef M6504_MEMORY_LAYOUT
	ROM_LOAD("024_1879", 0x0c00, 0x0400, CRC(4f28c443) SHA1(e33f8b7f38e54d7a6e0f0763f2328cc12cb0eade))
	ROM_LOAD("005_2179", 0x1000, 0x1000, CRC(6f10991b) SHA1(90cdc5a15d9ad813ad20410f21081c6e3e481812)) // chess mate 7.5
#else
	ROM_LOAD("024_1879", 0x8c00, 0x0400, CRC(4f28c443) SHA1(e33f8b7f38e54d7a6e0f0763f2328cc12cb0eade))
	ROM_LOAD("005_2179", 0xf000, 0x1000, CRC(6f10991b) SHA1(90cdc5a15d9ad813ad20410f21081c6e3e481812)) // chess mate 7.5
#endif
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
   port a
   0..7 led output
   0..6 keyboard input

   port b
    0..5 outputs
	0 speaker out
	6 as chipselect used!?
	7 interrupt out?

 	c4, c5, keyboard polling
	c0, c1, c2, c3 led output

*/

static int mk2_read_a(int chip)
{
	int data=0xff;
	int help=input_port_1_r(0)|input_port_2_r(0); // looks like white and black keys are the same!

	switch (rriot_0_b_r(0)&0x7) {
	case 4:
		if (help&0x20) data&=~0x1; //F
		if (help&0x10) data&=~0x2; //E
		if (help&8) data&=~0x4; //D
		if (help&4) data&=~0x8; // C
		if (help&2) data&=~0x10; // B
		if (help&1) data&=~0x20; // A
#if 0
		if (input_port_3_r(0)&1) data&=~0x40; //?
#endif
		break;
	case 5:
#if 0
		if (input_port_3_r(0)&2) data&=~0x1; //?
		if (input_port_3_r(0)&4) data&=~0x2; //?
		if (input_port_3_r(0)&8) data&=~0x4; //?
#endif
		if (input_port_0_r(0)&4) data&=~0x8; // Enter
		if (input_port_0_r(0)&2) data&=~0x10; // Clear
		if (help&0x80) data&=~0x20; // H
		if (help&0x40) data&=~0x40; // G
		break;
	}
	return data;
}

static void mk2_write_a(int chip, int value)
{
	int temp=rriot_0_b_r(0);


	switch(temp&0x3) {
	case 0: case 1: case 2: case 3:
		mk2_led[temp&3]|=value;
	}
}

static int mk2_read_b(int chip)
{
	return 0xff&~0x40; // chip select mapped to pb6
}

static void mk2_write_b(int chip, int value)
{
	if (value&0x80)
		DAC_data_w(0,value&1?80:0);
	mk2_led[4]|=value;
}

static void mk2_irq(int chip, int level)
{
	cpunum_set_input_line(0, M6502_IRQ_LINE, level);
}

static RRIOT_CONFIG riot={
	1000000,
	{ mk2_read_a, mk2_write_a },
	{ mk2_read_b, mk2_write_b },
	mk2_irq
};

static DRIVER_INIT( mk2 )
{
	rriot_init(0,&riot);
}

/*    YEAR  NAME    PARENT	COMPAT	MACHINE INPUT   INIT    CONFIG    COMPANY   FULLNAME */
CONS( 1979,	mk2,	0,		0,		mk2,	mk2,	mk2,	NULL,	  "Quelle International",  "Chess Champion MK II", 0)
// second design sold (same computer/program?)
