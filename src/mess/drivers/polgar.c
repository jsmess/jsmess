/******************************************************************************
 Mephisto 4 + 5 Chess Computer
 2007 Dirk V.

******************************************************************************/

/*


CPU 65C02 P4
Clock 4.9152 MHz
NMI CLK 600 Hz
IRQ Line is set to VSS
8 KByte  SRAM Sony CXK5864-15l

1-CD74HC4060E   14 Bit Counter
1-CD74HC166E
1-CD74HC251E
1-SN74HC138N TI
1-SN74HC139N TI
1-74HC14AP Toshiba
1-74HC02AP Toshiba
1-74HC00AP Toshiba
1-CD74HC259E


$0000-$1fff   S-RAM
$2000 LCD 4 Byte Shift Register writeonly right to left
every 2nd char xor'd by $FF

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

// lese Tastatur von 2c00 - 2c07, 
2c00 Trn
2c01 Info
2c02 Mem
2c03 Pos
2c04 Lev
2c05 Fct
2c06 Ent
2c07 CL




*/

#include "emu.h"
#include "cpu/m6502/m6502.h"
// #include"cpu/mcs51/mcs51.h"
// #include "sound/dac.h"
#include "sound/beep.h"

// #include "milano.lh"
#include "video/hd44780.h"
// #include "render.h"
// #include "rendlay.h"
// #include "uiinput.h"
#include "machine/mboard.h"

static UINT8 lcd_shift_counter;
static UINT8 led_status;
static UINT8 lcd_char=0;
// static UINT8 *milano_ram;
// static UINT8 led7;



static const hd44780_interface polgar_display =
{
    2,                  // number of lines
    16,                 // chars for line
    NULL                // custom display layout
};





static WRITE8_HANDLER ( write_lcd_i )
{

 hd44780_device * hd44780 = space->machine->device<hd44780_device>("hd44780");
 running_device *speaker = space->machine->device("beep");
// state->hd44780->data_write(offset, data);
//state->hd44780->control_write(0, data);

if ((data&1)==1) 
 hd44780->data_write(128, lcd_char);
 else
hd44780->control_write(128, lcd_char);


 if ((data&4)==4) beep_set_state(speaker,1);
 	else beep_set_state(speaker,0);
 	

	logerror("LCD Register = %d Data = %d\n",offset,data);
}

static WRITE8_HANDLER ( write_lcd )
{

 //hd44780_device * hd44780 = space->machine->device<hd44780_device>("hd44780");
// state->hd44780->data_write(offset, data);
//state->hd44780->control_write(0, data);
  lcd_char=data;
 //hd44780->data_write(128, data);
//hd44780->data_write(128, data_8);
}

static WRITE8_HANDLER ( write_led )
{
	UINT8 LED_offset=100;
	//data &= 0xff;
if (data==0xff) output_set_led_value(LED_offset+offset,1);
	else
		output_set_led_value(LED_offset+offset,0);
	//if (data==0)led_status &= 255-(1<<offset) ; else led_status|=1<<offset;
	// if (offset<6)output_set_led_value(LED_offset+offset, led_status&1<<offset?1:0);
	// if (offset==7) led7=data& 0x80 ? 0x00 :0xff;
	
	
//	logerror("LEDs  Offset = %d Data = %d\n",offset,data);
}

static READ8_HANDLER(read_keys)
{
	UINT8 data;
	static const char *const keynames[2][8] =
			{
				{ "KEY1_0", "KEY1_1", "KEY1_2", "KEY1_3", "KEY1_4", "KEY1_5", "KEY1_6", "KEY1_7" },
				{ "KEY2_0", "KEY2_1", "KEY2_2", "KEY2_3", "KEY2_4", "KEY2_5", "KEY2_6", "KEY2_7" }
			};

	data = 0xff;
/*
	if (((led_status & 0x80) == 0x00))
		data=input_port_read(space->machine, keynames[0][offset]);
	else
		data=input_port_read(space->machine, keynames[1][offset]);
*/
  data=input_port_read(space->machine, keynames[0][offset]);
	logerror("Keyboard Port = %s Data = %d\n  ", ((led_status & 0x80) == 0x00) ? keynames[0][offset] : keynames[1][offset], data);
	return data | 0x7f;
}



static VIDEO_START( polgar )
{

}

static VIDEO_UPDATE( polgar )
{
    hd44780_device * hd44780 = screen->machine->device<hd44780_device>("hd44780");

    return hd44780->video_update( bitmap, cliprect );
}

static PALETTE_INIT( polgar )
{
//    palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 0, MAKE_RGB(195, 201, 200));
    palette_set_color(machine, 1, MAKE_RGB(40, 30, 30));
}


static const gfx_layout polgar_charlayout =
{
    5, 8,                   /* 5 x 8 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes */
    { 3, 4, 5, 6, 7},
    { 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
    8*8                     /* 8 bytes */
};

static GFXDECODE_START( polgar )
    GFXDECODE_ENTRY( "hd44780", 0x0000, polgar_charlayout, 0, 1 )
GFXDECODE_END




static ADDRESS_MAP_START(polgar_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM // AM_BASE(&milano_ram)//
	AM_RANGE( 0x2400, 0x2407) AM_WRITE ( mboard_write_LED_8 )		// Chessboard
	AM_RANGE( 0x2800, 0x2800) AM_WRITE ( mboard_write_board_8)		// Chessboard
	AM_RANGE( 0x3000, 0x3000) AM_READ( mboard_read_board_8 )		// Chessboard	
  AM_RANGE( 0x3400, 0x3405) AM_WRITE( write_led)	// Function LEDs 3400 TRN 3401
  AM_RANGE( 0x2c00, 0x2c07) AM_READ( read_keys)	// CL Key
  AM_RANGE( 0x2004, 0x2004) AM_WRITE( write_lcd_i)	// LCD Instr. Reg + Beeper
  AM_RANGE( 0x2000, 0x2000) AM_WRITE( write_lcd)	        // LCD Char Reg.
	//AM_RANGE( 0x6000, 0x6000) AM_RAM
	//AM_RANGE( 0x7000, 0x7000) AM_RAM
 	AM_RANGE( 0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*
static ADDRESS_MAP_START(milano_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM // AM_BASE(&milano_ram)//
//	AM_RANGE( 0x2400, 0x2407) AM_WRITE ( write_LED_8 )		// Chessboard
//	AM_RANGE( 0x2800, 0x2800) AM_WRITE ( write_board_8)		// Chessboard
//	AM_RANGE( 0x3000, 0x3000) AM_READ( read_board_8 )		// Chessboard	
	  // AM_RANGE( 0x3400, 0x3405) AM_WRITE( write_leds)	// Function LEDs 3400 TRN 3401
//	  AM_RANGE( 0x2c00, 0x2c07) AM_READ( read_keys)	// CL Key
//  AM_RANGE( 0x2004, 0x2004) AM_WRITE( write_lcd_i)	// LCD Instr. Reg
//  AM_RANGE( 0x2000, 0x2000) AM_WRITE( write_lcd)	        // LCD Char Reg.
	//AM_RANGE( 0x6000, 0x6000) AM_RAM
	//AM_RANGE( 0x7000, 0x7000) AM_RAM
	AM_RANGE( 0x3000, 0xffff) AM_ROM
	// AM_RANGE( 0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END
*/

static INPUT_PORTS_START( board )
	PORT_START("LINE2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_START("LINE9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)

	PORT_START("LINE10")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_WHITE") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_BLACK") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x04,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x08,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD)

	PORT_START("B_BUTTONS") 
	PORT_BIT(0x01,  IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x02,  IP_ACTIVE_HIGH, IPT_KEYBOARD)

INPUT_PORTS_END

static INPUT_PORTS_START( polgar )
  PORT_INCLUDE( board )
	PORT_START("KEY1_0") //Port $2c00
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Trn") PORT_CODE(KEYCODE_F1)
	PORT_START("KEY1_1") //Port $2c01
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Info") PORT_CODE(KEYCODE_F2)
	PORT_START("KEY1_2") //Port $2c02
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Mem") PORT_CODE(KEYCODE_F3)
	PORT_START("KEY1_3") //Port $2c03
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pos") PORT_CODE(KEYCODE_F4)
	PORT_START("KEY1_4") //Port $2c04
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F5)
	PORT_START("KEY1_5") //Port $2c05
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCT") PORT_CODE(KEYCODE_F6)
	PORT_START("KEY1_6") //Port $2c06
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_F7)
	PORT_START("KEY1_7") //Port $2c07
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CL") PORT_CODE(KEYCODE_F8)
INPUT_PORTS_END

static TIMER_CALLBACK( update_nmi )
{
	// running_device *speaker = machine->device("beep");
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI,PULSE_LINE);
	// cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, CLEAR_LINE);
	// dac_data_w(0,led_status&64?128:0);
	// beep_set_state(speaker,led_status&64?1:0);
}


static MACHINE_START( polgar )
{
	lcd_shift_counter=3;
	// timer_pulse(machine, ATTOTIME_IN_HZ(60), NULL, 0, update_leds);
	timer_pulse(machine, ATTOTIME_IN_HZ(600), NULL, 0, update_nmi);
	timer_pulse(machine, ATTOTIME_IN_HZ(100), NULL, 0, mboard_update_artwork);
	mboard_savestate_register(machine);
}


static MACHINE_RESET( polgar )
{
	// lcd_shift_counter = 3;
        hd44780_device * hd44780 = machine->device<hd44780_device>("hd44780");
      	hd44780->control_write(128, 15);		/* Initialize LCD */
      	mboard_set_boarder_pieces();
	mboard_set_board();
}


static MACHINE_CONFIG_START( polgar, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M65C02,4915200)        /* 65C02 */
	//MDRV_CPU_ADD("maincpu",M65C02,10000000)        /* 65C02 */
	// MDRV_CPU_ADD("maincpu",I8032,120000000)        /* 65C02 */
	MDRV_CPU_PROGRAM_MAP(polgar_mem)
	MDRV_QUANTUM_TIME(HZ(60))
	MDRV_MACHINE_START( polgar )
	MDRV_MACHINE_RESET( polgar )

	/* video hardware */
  // MDRV_DEFAULT_LAYOUT(layout_milano)
  MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(100, 19)
	MDRV_SCREEN_VISIBLE_AREA(0, 100-1, 0, 19-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(polgar)

	MDRV_GFXDECODE(polgar)

	MDRV_HD44780_ADD("hd44780", polgar_display)

    MDRV_VIDEO_START(polgar)
    MDRV_VIDEO_UPDATE(polgar)



//	MDRV_DEFAULT_LAYOUT(layout_van16)
	 MDRV_DEFAULT_LAYOUT(layout_lcd)




	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END

/*

ROM_START(milano)    
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("milano.bin", 0x0000, 0x10000, CRC(8d02e1ef) SHA1(9972c75936613bd68cfd3fe62bd222e90e8b1083))
	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "m_chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

*/

ROM_START(polgar) //polgar
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("polgar.bin", 0x0000, 0x10000, CRC(88d55c0f) SHA1(e86d088ec3ac68deaf90f6b3b97e3e31b1515913))
	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "m_chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(b497af5c) SHA1(0bddebf91dac868ffbdd7f514253943635d7fe9a))
ROM_END


static DRIVER_INIT( polgar )
{
	lcd_shift_counter = 3;
}

/***************************************************************************

    Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY             FULLNAME                            FLAGS */
//CONS( 1983, mephisto,   0,      0,      mephisto,   mephisto,   mephisto, "Hegener & Glaser", "Mephisto Polgar Computer",         GAME_NOT_WORKING )
CONS( 1986, polgar,     0,      0,      polgar,     polgar,   polgar,   "Hegener & Glaser", "Mephisto Milano Schach Computer", 0 )
// second design sold (same computer/program?)
