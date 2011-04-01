/******************************************************************************
 Mephisto Polgar
 2010 Dirk V., Cowering

******************************************************************************/

/*

CPU 65C02 P4
Clock 4.9152 MHz


// lese Tastatur von 2c00 - 2c07,
2c00 Trn
2c01 Info
2c02 Mem
2c03 Pos
2c04 Lev
2c05 Fct
2c06 Ent
2c07 CL

$1ff0
Bit 0 LCD Command/Data  0/1
Bit 1 LCD Enable (signals Data is valid)
Bit 2 Beeper
Bit 3 Beeper
Bit 4  LED A-H
Bit 5  LED 1-8
Bit 6  LED 1-8
Bit 7  LED A-H


*/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/beep.h"
// #include "milano.lh"
#include "video/hd44780.h"
// #include "render.h"
// #include "rendlay.h"
// #include "uiinput.h"
#include "machine/mboard.h"
#include "rendlay.h"

class polgar_state : public driver_device
{
public:
	polgar_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_led_status;
	UINT8 m_lcd_char;
	UINT8 m_led7;
	UINT8 m_latch_data;
};

static const int value[4] = {0x80,0x81,0x00,0x01};


static const hd44780_interface polgar_display =
{
	2,	// number of lines
	16,	// chars for line
	NULL	// custom display layout
};

static UINT8 convert_imputmask(UINT8 input)
{
	input^=0xff;
	switch (input) {
		case 0x01:
			return 0x80;
		case 0x02:
			return 0x40;
		case 0x04:
			return 0x20;
		case 0x08:
			return 0x10;
		case 0x10:
			return 0x08;
		case 0x20:
			return 0x04;
		case 0x40:
			return 0x02;
		case 0x80:
			return 0x01;
		default:
			return 0x00;
		}
}

static int get_first_cleared_bit(UINT8 data)
{
	int i;

	for (i = 0; i < 8; i++)
		if (!BIT(data, i))
			return i;

	return NOT_VALID;
}

static WRITE8_HANDLER ( write_io )
{
	polgar_state *state = space->machine().driver_data<polgar_state>();
	int i;
	hd44780_device * hd44780 = space->machine().device<hd44780_device>("hd44780");
	device_t *speaker = space->machine().device("beep");

	if (BIT(data,1)) {
		if (BIT(data,0)) {
			hd44780->data_write(*space, 128, state->m_lcd_char);
		} else {
			hd44780->control_write(*space, 128, state->m_lcd_char);
		}
	}

	if (BIT(data,2) || BIT(data,3)) beep_set_state(speaker,1); else beep_set_state(speaker,0);

	if (BIT(data,7) && BIT(data, 4))
	{
		for (i=0;i<8;i++)
		output_set_led_value(i,!BIT(state->m_latch_data,i));
	}
	else if (BIT(data,6) && BIT(data,5))
	{
		for (i=0;i<8;i++)
		output_set_led_value(10+i,!BIT(state->m_latch_data,7-i));
	}
	else if (!data && (!strcmp(space->machine().system().name,"milano")))
		for (i=0;i<8;i++)
		{
			output_set_led_value(i,!BIT(state->m_latch_data,i));
			output_set_led_value(10+i,!BIT(state->m_latch_data,7-i));
		}
		

	//logerror("LCD Status  Data = %d\n",data);


}

static WRITE8_HANDLER ( write_lcd )
{
	polgar_state *state = space->machine().driver_data<polgar_state>();

	state->m_lcd_char=data;

	//logerror("LCD Data = %d [%c]\n",data,(data&0xff));

}

static WRITE8_HANDLER ( milano_write_led )
{
	UINT8 LED_offset=100;
	if (data==0xff)
		output_set_led_value(LED_offset+offset,1);
	else
		output_set_led_value(LED_offset+offset,0);

//	logerror("milano_write_led Offset = %d Data = %d\n",offset,data);
}

static WRITE8_HANDLER ( write_led )
{
	polgar_state *state = space->machine().driver_data<polgar_state>();
	UINT8 LED_offset=100;
	data &= 0x80;

	if (data==0)state->m_led_status &= 255-(1<<offset) ; else state->m_led_status|=1<<offset;
	if (offset<6)output_set_led_value(LED_offset+offset, state->m_led_status&1<<offset?1:0);
	if (offset==7) state->m_led7=data& 0x80 ? 0x00 :0xff;
	logerror("LEDs  Offset = %d Data = %d\n",offset,data);
}

#if 0
static WRITE8_HANDLER ( write_led )
{
	polgar_state *state = space->machine().driver_data<polgar_state>();
	UINT8 LED_offset=100;
	if (data==0xff)
		output_set_led_value(LED_offset+offset,1);
	else
		output_set_led_value(LED_offset+offset,0);
	if (data==0)state->m_led_status &= 255-(1<<offset) ; else state->m_led_status|=1<<offset;
	if (offset<6)output_set_led_value(LED_offset+offset, state->m_led_status&1<<offset?1:0);
	if (offset==7) state->m_led7=data& 0x80 ? 0x00 :0xff;


	logerror("LEDs  Offset = %d Data = %d\n",offset,data);
}
#endif

static WRITE8_HANDLER ( milano_write_board )
{
	polgar_state *state = space->machine().driver_data<polgar_state>();

	state->m_latch_data=data;
}

static READ8_HANDLER(milano_read_board)
{
	int line;
	polgar_state *state = space->machine().driver_data<polgar_state>();
	static const char *const board_lines[8] =
			{ "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };

	UINT8 data=0x00;
	UINT8 tmp=0xff;

	if (state->m_latch_data)
	{
		line=get_first_cleared_bit(state->m_latch_data);
		tmp=input_port_read(space->machine(),  board_lines[line]);
		
		if (tmp != 0xff)
			data=convert_imputmask(tmp);

	}

	return data;

}

static READ8_HANDLER(read_keys)
{
	//polgar_state *state = space->machine().driver_data<polgar_state>();
	UINT8 data;
	static const char *const keynames[2][8] =
			{
				{ "KEY1_0", "KEY1_1", "KEY1_2", "KEY1_3", "KEY1_4", "KEY1_5", "KEY1_6", "KEY1_7" },
				{ "KEY2_0", "KEY2_1", "KEY2_2", "KEY2_3", "KEY2_4", "KEY2_5", "KEY2_6", "KEY2_7" }
			};

	data = 0xff;
#if 0
	if (((state->m_led_status & 0x80) == 0x00))
		data=input_port_read(space->machine(), keynames[0][offset]);
	else
		data=input_port_read(space->machine(), keynames[1][offset]);
#endif
	data=input_port_read(space->machine(), keynames[0][offset]);

	// logerror("Keyboard Port = %s Data = %d\n  ", ((state->m_led_status & 0x80) == 0x00) ? keynames[0][offset] : keynames[1][offset], data);
	return data | 0x7f;
}



static VIDEO_START( polgar )
{

}

static SCREEN_UPDATE( polgar )
{
	hd44780_device * hd44780 = screen->machine().device<hd44780_device>("hd44780");
	return hd44780->video_update( bitmap, cliprect );
}

static PALETTE_INIT( polgar )
{
	//palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 0, MAKE_RGB(195, 201, 200));
	palette_set_color(machine, 1, MAKE_RGB(40, 30, 30));
}


static const gfx_layout polgar_charlayout =
{
	5, 8,	/* 5 x 8 characters */
	256,	/* 256 characters */
	1,	/* 1 bits per pixel */
	{ 0 },	/* no bitplanes */
	{ 3, 4, 5, 6, 7},
	{ 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8	/* 8 bytes */
};

static GFXDECODE_START( polgar )
	GFXDECODE_ENTRY( "hd44780", 0x0000, polgar_charlayout, 0, 1 )
GFXDECODE_END


static ADDRESS_MAP_START(polgar_mem , AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM
	AM_RANGE( 0x2400, 0x2407) AM_WRITE ( mboard_write_LED_8 )		// Chessboard
	AM_RANGE( 0x2800, 0x2800) AM_WRITE ( mboard_write_board_8)		// Chessboard
	AM_RANGE( 0x3000, 0x3000) AM_READ( mboard_read_board_8 )		// Chessboard
	AM_RANGE( 0x3400, 0x3405) AM_WRITE( write_led)	// Function LEDs 3400 TRN 3401
	AM_RANGE( 0x2c00, 0x2c07) AM_READ( read_keys)	// CL Key
	AM_RANGE( 0x2004, 0x2004) AM_WRITE( write_io)	// LCD Instr. Reg + Beeper
	AM_RANGE( 0x2000, 0x2000) AM_WRITE( write_lcd)	// LCD Char Reg.
	AM_RANGE( 0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(milano_mem , AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1f9f) AM_RAM
	AM_RANGE( 0x1fd0, 0x1fd0) AM_WRITE ( milano_write_board)	// Chessboard
	AM_RANGE( 0x1fe0, 0x1fe0) AM_READ( milano_read_board )		// Chessboard
	AM_RANGE( 0x1fe8, 0x1fed) AM_WRITE( milano_write_led )	// Function LEDs 3400 TRN 3401
	AM_RANGE( 0x1fd8, 0x1fdf) AM_READ( read_keys)	// CL Key
	AM_RANGE( 0x1ff0, 0x1ff0) AM_WRITE( write_io)	// IO control
	AM_RANGE( 0x1fc0, 0x1fc0) AM_WRITE( write_lcd)	// LCD Char Reg. (latched)
	AM_RANGE( 0x2000, 0xffff) AM_ROM
ADDRESS_MAP_END


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


static TIMER_DEVICE_CALLBACK( update_nmi )
{
	//polgar_state *state = timer.machine().driver_data<polgar_state>();
	// running_device *speaker = timer.machine().device("beep");
	cputag_set_input_line(timer.machine(), "maincpu", INPUT_LINE_NMI,PULSE_LINE);
	// cputag_set_input_line(timer.machine(), "maincpu", M6502_IRQ_LINE, CLEAR_LINE);
	// dac_data_w(0,state->m_led_status&64?128:0);
	// beep_set_state(speaker,state->m_led_status&64?1:0);
}

static MACHINE_START( polgar )
{
	mboard_savestate_register(machine);
}


static MACHINE_RESET( polgar )
{
	mboard_set_border_pieces();
	mboard_set_board();
}

static MACHINE_CONFIG_START( polgar, polgar_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M65C02,4915200)	/* 65C02 */
	MCFG_CPU_PROGRAM_MAP(polgar_mem)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_START( polgar )
	MCFG_MACHINE_RESET( polgar )

	/* video hardware */
	// MCFG_DEFAULT_LAYOUT(layout_milano)
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(100, 20)
	MCFG_SCREEN_VISIBLE_AREA(0, 100-1, 0, 20-1)
	MCFG_SCREEN_UPDATE(polgar)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(polgar)

	MCFG_GFXDECODE(polgar)

	MCFG_HD44780_ADD("hd44780", polgar_display)

	MCFG_VIDEO_START(polgar)


	//MCFG_DEFAULT_LAYOUT(layout_van16)
	MCFG_DEFAULT_LAYOUT(layout_lcd)


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	//MCFG_TIMER_ADD_PERIODIC("led_timer", update_leds, attotime::from_hz(60))
	MCFG_TIMER_ADD_PERIODIC("nmi_timer", update_nmi, attotime::from_hz(600))
	MCFG_TIMER_ADD_PERIODIC("artwork_timer", mboard_update_artwork, attotime::from_hz(100))
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( milano, polgar )
	/* basic machine hardware */
	// MCFG_IMPORT_FROM( polgar )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(milano_mem)

	//beep_set_frequency(0, 4000);
MACHINE_CONFIG_END

ROM_START(milano)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("milano.bin", 0x0000, 0x10000, CRC(0e9c8fe1) SHA1(e9176f42d86fe57e382185c703c7eff7e63ca711))
	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin", 0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END



ROM_START(polgar) //polgar
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("polgar.bin", 0x0000, 0x10000, CRC(88d55c0f) SHA1(e86d088ec3ac68deaf90f6b3b97e3e31b1515913))
	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin", 0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END


static DRIVER_INIT( polgar )
{
	polgar_state *state = machine.driver_data<polgar_state>();
	state->m_led_status=0;
}

/***************************************************************************

    Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY             FULLNAME                            FLAGS */
//CONS( 1983, mephisto,   0,      0,      mephisto,   mephisto,   mephisto, "Hegener & Glaser", "Mephisto Polgar Computer",         GAME_NOT_WORKING )
CONS( 1986, polgar,     0,      0,      polgar,     polgar,   polgar,   "Hegener & Glaser", "Mephisto Polgar Schach Computer", 0 )
CONS( 1986, milano,     polgar,      0,      milano,     polgar,   polgar,   "Hegener & Glaser", "Mephisto Milano Schach Computer", 0 )
// milano to be added to supercon.c driver - no current messdriv.c entry
// second design sold (same computer/program?)
