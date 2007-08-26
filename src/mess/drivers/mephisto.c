/******************************************************************************
 Mephisto 4 + 5 Chess Computer
 2007 Dirk V.
 
******************************************************************************/
/*


Keyboard is not working !!!!!!

CPU 65C02 P4
Clock 4.9152 MHz
NMI CLK 50 Hz
8 KByte  SRAM Sony CXK5864-15l

$0000-$1fff   S-RAM
$2000 LCD 4 Byte Shift Register writeonly right to left
every 2nd char xor�d by $FF

2c00-2c07 Keyboard (8to1 Multiplexer) 74HCT251
2*8 Matrix
Adr. 0x3407
==0 !=0
2c00 CL E5
2c01 POS F6
2c02 MEMO G7
2c03 INFO A1
2c04 LEV H8
2c05 ENT B2
2c06 >0 C3
2c07 <9 D4

$3400-$3407 LED 1-6, Buzzer, Keyboard select

$2400 // Chess Board
$2800 // Chess Board
$3000 // Chess Board 

$4000-7FFF Opening Modul HG550
$8000-$FFF ROM


*/




#include "driver.h"


#include "cpu/m6502/m6502.h"
//#include "sound/dac.h"
#include "mephisto.lh"

static UINT8 irq;
static UINT8 lcd_shift_reg[3];
static UINT8 lcd_shift_counter;
static UINT8 lcd_eor_val;
static UINT8 led_status;
static UINT8 led7;
/* static UINT8 key_array[15]; */

static WRITE8_HANDLER ( write_lcd ) 
{
  lcd_shift_reg[lcd_shift_counter]=data ^ lcd_eor_val;
  if (lcd_shift_counter==0) 
    {
      lcd_shift_counter=4;
      lcd_eor_val=lcd_eor_val ^ 0xff;
      logerror("LCD Shift: \n");
    }
  lcd_shift_counter--;
  
}

static READ8_HANDLER(read_keys)
{
  UINT8 data;
  // if ((led7==0))data=key_array[offset]; else data=key_array[offset+8];
  
  //data=readinputport(offset);
  
  if ((led7==0))data=readinputport(offset+1); else data=readinputport(offset+8+1);
  logerror("Keyboard Offset = %d Data = %d  led7 = %d\n",offset,data,led7);
  return data;

}

static READ8_HANDLER(read_board)
{
  return 0x00;
}

static WRITE8_HANDLER ( write_led ) 
{


// if (data==0){ led_status=led_status & (255-pow(2,offset)); else  led_status=led_status|pow(2,offset);}

  if (offset==0){ if (data==0) led_status=led_status & 254; else  led_status=led_status|1;}
  if (offset==1){ if (data==0) led_status=led_status & 253; else  led_status=led_status|2;}
  if (offset==2){ if (data==0) led_status=led_status & 251; else  led_status=led_status|4;}
  if (offset==3){ if (data==0) led_status=led_status & 247; else  led_status=led_status|8;}
  if (offset==4){ if (data==0) led_status=led_status & 239; else  led_status=led_status|16;}
  if (offset==5){ if (data==0) led_status=led_status & 223; else  led_status=led_status|32;}

  if (offset==7){ led7=data;}

  logerror("Offset = %d Data = %d\n",offset,data);
}


// only lower 12 address bits on bus!
static ADDRESS_MAP_START(mephisto_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM // 
	AM_RANGE( 0x2000, 0x2000) AM_WRITE( write_lcd)
	AM_RANGE( 0x2c00, 0x2c07) AM_READ( read_keys)
	// AM_RANGE( 0x2c00, 0x2c07) AM_RAM
	AM_RANGE( 0x3400, 0x3407) AM_WRITE( write_led) // Status LEDs+ buzzer
	// AM_RANGE( 0x3400, 0x3407) AM_RAM // Status LEDs+ buzzer
	AM_RANGE( 0x2400, 0x2407) AM_RAM // Chessboard
	AM_RANGE( 0x2800, 0x2800) AM_RAM // Chessboard
	AM_RANGE( 0x3800, 0x3800) AM_RAM // unknwon write access
	AM_RANGE( 0x3000, 0x3000) AM_READ ( read_board) // Chessboard
	AM_RANGE( 0x4000, 0x7fff) AM_ROM   // Opening Library
	AM_RANGE( 0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END



INPUT_PORTS_START( mephisto )
  PORT_START_TAG("keyboard_1")
  PORT_START  //Port $2c00
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_1)
  PORT_START //Port $2c01
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_2)
  PORT_START //Port $2c02
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_3)  
  PORT_START //Port $2c03
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("INFO") PORT_CODE(KEYCODE_4)
  PORT_START  //Port $2c04
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_5)
  PORT_START //Port $2c05
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
  PORT_START //Port $2c06
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)  
  PORT_START //Port $2c07
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
  PORT_START_TAG("keyboard_2")
  PORT_START  //Port $2c08
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
  PORT_START //Port $2c09
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
  PORT_START //Port $2c0a
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)  
  PORT_START //Port $2c0b
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
  PORT_START  //Port $2c0c
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
  PORT_START //Port $2c0d
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
  PORT_START //Port $2c0e
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)  
  PORT_START //Port $2c0f
  PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
INPUT_PORTS_END




static TIMER_CALLBACK( update_leds )
{
	int i;
	output_set_led_value(0, led_status&1?1:0);
	output_set_led_value(1, led_status&2?1:0);
	output_set_led_value(2, led_status&4?1:0);
	output_set_led_value(3, led_status&8?1:0);
	output_set_led_value(4, led_status&16?1:0);
	output_set_led_value(5, led_status&32?1:0);
	for (i=0; i<4; i++) output_set_digit_value(i, lcd_shift_reg[i]);
	//for (i=0;i<16;i++) key_array[i]=readinputport(i);
	
	

}




static TIMER_CALLBACK( update_nmi )
{
	irq=irq ^1;
	// cpunum_set_input_line(0, INPUT_LINE_NMI,irq ? ASSERT_LINE: CLEAR_LINE );
	cpunum_set_input_line(0, INPUT_LINE_NMI,PULSE_LINE);

}

static MACHINE_START( mephisto )
{
  lcd_shift_counter=3;
  lcd_eor_val=0;
  mame_timer_pulse(MAME_TIME_IN_HZ(60), 0, update_leds);
	mame_timer_pulse(MAME_TIME_IN_HZ(600), 0, update_nmi);

}


static MACHINE_RESET( mephisto )
{
	lcd_eor_val=0;
	lcd_shift_counter=3;
}


static MACHINE_DRIVER_START( mephisto )
	/* basic machine hardware */
	MDRV_CPU_ADD(M65C02,4915200)        /* 6502 */
	MDRV_CPU_PROGRAM_MAP(mephisto_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( mephisto )
	MDRV_MACHINE_RESET( mephisto )

    /* video hardware */
	 MDRV_DEFAULT_LAYOUT(layout_mephisto)

	/* sound hardware */
	// MDRV_SPEAKER_STANDARD_MONO("mono")

	//MDRV_SOUND_ADD(DAC, 0)
	// MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


ROM_START(mephisto)
	ROM_REGION(0x10000,REGION_CPU1,0)
  //ROM_LOAD("mephisto5.rom", 0x8000, 0x8000, CRC(89c3d9d2) SHA1(77cd6f8eeb03c713249db140d2541e3264328048))
	ROM_LOAD("mephisto4.rom", 0x8000, 0x8000, CRC(f68a4124) SHA1(d1d03a9aacc291d5cb720d2ee2a209eeba13a36c))
	// ROM_LOAD("hg550.rom", 0x4000, 0x4000, CRC(f68a4124) SHA1(d1d03a9aacc291d5cb720d2ee2a209eeba13a36c))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/




static DRIVER_INIT( mephisto )
{
	//rriot_init(0,&riot);
}

/*    YEAR  NAME    PARENT	COMPAT	MACHINE INPUT   INIT    CONFIG    COMPANY   FULLNAME */
CONS( 1983,	mephisto,	0,		0,		mephisto,	mephisto,	mephisto,	NULL,	  "Hegener & Glaser",  "Mephisto Schach Computer", 0)
// second design sold (same computer/program?)
