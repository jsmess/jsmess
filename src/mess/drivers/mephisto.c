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


*/

/*  Mephisto 4 Turbo Kit 18mhz - (mm4tk)
	This is a replacement rom combining the turbo kit initial rom with the original MM IV.
	The Turbo Kit powers up to it's tiny rom, copies itself to ram, banks in normal rom,
	copies that to faster SRAM, then patches the checksum and the LED blink delays.
	If someone else wants to code up the power up banking, feel free

	There is an undumped MM V Turbo Kit, which will be the exact same except for location of
	the patches. The mm5tk just needs the normal mm5 ROM swapped out for that one to
	blinks the LEDs a little slower.

	-- Cowering (2011)
*/


#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/beep.h"
//#include "mephisto.lh"

#include "machine/mboard.h"


class mephisto_state : public driver_device
{
public:
	mephisto_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_lcd_shift_counter;
	UINT8 m_led_status;
	UINT8 *m_ram;
	UINT8 m_led7;
	UINT8 m_allowNMI;
};


static WRITE8_HANDLER ( write_lcd )
{
	mephisto_state *state = space->machine().driver_data<mephisto_state>();
	if (state->m_led7 == 0) output_set_digit_value(state->m_lcd_shift_counter,data);	// 0x109 MM IV // 0x040 MM V

	//output_set_digit_value(state->m_lcd_shift_counter,data ^ state->m_ram[0x165]);    // 0x109 MM IV // 0x040 MM V
	state->m_lcd_shift_counter--;
	state->m_lcd_shift_counter &= 3;
}

static WRITE8_HANDLER ( mephisto_NMI )
{

	mephisto_state *state = space->machine().driver_data<mephisto_state>();
	state->m_allowNMI = 1;
}

static READ8_HANDLER(read_keys)
{
	mephisto_state *state = space->machine().driver_data<mephisto_state>();
	UINT8 data;
	static const char *const keynames[2][8] =
			{
				{ "KEY1_0", "KEY1_1", "KEY1_2", "KEY1_3", "KEY1_4", "KEY1_5", "KEY1_6", "KEY1_7" },
				{ "KEY2_0", "KEY2_1", "KEY2_2", "KEY2_3", "KEY2_4", "KEY2_5", "KEY2_6", "KEY2_7" }
			};

	data = 0xff;
	if (((state->m_led_status & 0x80) == 0x00))
		data=input_port_read(space->machine(), keynames[0][offset]);
	else
		data=input_port_read(space->machine(), keynames[1][offset]);

	logerror("Keyboard Port = %s Data = %d\n  ", ((state->m_led_status & 0x80) == 0x00) ? keynames[0][offset] : keynames[1][offset], data);
	return data | 0x7f;
}

static WRITE8_HANDLER ( write_led )
{
	mephisto_state *state = space->machine().driver_data<mephisto_state>();
	UINT8 LED_offset=100;
	data &= 0x80;

	if (data==0)state->m_led_status &= 255-(1<<offset) ; else state->m_led_status|=1<<offset;
	if (offset<6)output_set_led_value(LED_offset+offset, state->m_led_status&1<<offset?1:0);
	if (offset==7) state->m_led7=data& 0x80 ? 0x00 :0xff;
	logerror("LEDs  Offset = %d Data = %d\n",offset,data);
}

static WRITE8_HANDLER ( write_led_mm2 )
{
	mephisto_state *state = space->machine().driver_data<mephisto_state>();

	UINT8 LED_offset=100;
	data &= 0x80;

	if (data==0)
		state->m_led_status &= 255-(1<<offset);
	else
		state->m_led_status|=1<<offset;

	if (offset<6)
		output_set_led_value(LED_offset+offset, state->m_led_status&1<<offset?1:0);

	if (offset==7)
		state->m_led7= data & 0x80 ? 0xff :0x00;	//MM2

}

static ADDRESS_MAP_START(rebel5_mem , AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM						// AM_BASE_MEMBER(mephisto_state, m_ram)//
	AM_RANGE( 0x5000, 0x5000) AM_WRITE( write_lcd )
	AM_RANGE( 0x3000, 0x3007) AM_READ( read_keys )			// Rebel 5.0
	AM_RANGE( 0x2000, 0x2007) AM_WRITE( write_led )			// Status LEDs+ buzzer
	AM_RANGE( 0x3000, 0x4000) AM_READ( mboard_read_board_8 )		// Chessboard
	AM_RANGE( 0x6000, 0x6000) AM_WRITE ( mboard_write_LED_8)		// Chessboard
	AM_RANGE( 0x7000, 0x7000) AM_WRITE ( mboard_write_board_8 )	// Chessboard
	AM_RANGE( 0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(mephisto_mem , AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_RAM AM_BASE_MEMBER(mephisto_state, m_ram )//
	AM_RANGE( 0x2000, 0x2000) AM_WRITE( write_lcd )
	AM_RANGE( 0x2c00, 0x2c07) AM_READ( read_keys )
	AM_RANGE( 0x3400, 0x3407) AM_WRITE( write_led )			// Status LEDs+ buzzer
	AM_RANGE( 0x2400, 0x2407) AM_WRITE ( mboard_write_LED_8 )		// Chessboard
	AM_RANGE( 0x2800, 0x2800) AM_WRITE ( mboard_write_board_8)		// Chessboard
	AM_RANGE( 0x3800, 0x3800) AM_WRITE ( mephisto_NMI )			// NMI enable
	AM_RANGE( 0x3000, 0x3000) AM_READ( mboard_read_board_8 )		// Chessboard
	AM_RANGE( 0x4000, 0x7fff) AM_ROM						// Opening Library
	AM_RANGE( 0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(mm2_mem , AS_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x0fff) AM_RAM AM_BASE_MEMBER(mephisto_state, m_ram )
	AM_RANGE( 0x2800, 0x2800) AM_WRITE( write_lcd )
	AM_RANGE( 0x1800, 0x1807) AM_READ( read_keys )
	AM_RANGE( 0x1000, 0x1007) AM_WRITE( write_led_mm2 )		//Status LEDs

	AM_RANGE( 0x3000, 0x3000) AM_WRITE ( mboard_write_LED_8 )		//Chessboard
	AM_RANGE( 0x3800, 0x3800) AM_WRITE ( mboard_write_board_8)		//Chessboard
	AM_RANGE( 0x2000, 0x2000) AM_READ( mboard_read_board_8 )		//Chessboard

	AM_RANGE( 0x4000, 0x7fff) AM_ROM						// Opening Library ?
	AM_RANGE( 0x8000, 0xffff) AM_ROM
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

static INPUT_PORTS_START( mephisto )
	PORT_START("KEY1_0") //Port $2c00
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLEAR") PORT_CODE(KEYCODE_F1)
	PORT_START("KEY1_1") //Port $2c01
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2)
	PORT_START("KEY1_2") //Port $2c02
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F3)
	PORT_START("KEY1_3") //Port $2c03
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INFO") PORT_CODE(KEYCODE_F4)
	PORT_START("KEY1_4") //Port $2c04
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F5)
	PORT_START("KEY1_5") //Port $2c05
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
	PORT_START("KEY1_6") //Port $2c06
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_START("KEY1_7") //Port $2c07
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)

	PORT_START("KEY2_0") //Port $2c08
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E 5") PORT_CODE(KEYCODE_E) PORT_CODE(KEYCODE_5)
	PORT_START("KEY2_1") //Port $2c09
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F 6") PORT_CODE(KEYCODE_F) PORT_CODE(KEYCODE_6)
	PORT_START("KEY2_2") //Port $2c0a
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G 7") PORT_CODE(KEYCODE_G) PORT_CODE(KEYCODE_7)
	PORT_START("KEY2_3") //Port $2c0b
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A 1") PORT_CODE(KEYCODE_A) PORT_CODE(KEYCODE_1)
	PORT_START("KEY2_4") //Port $2c0c
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H 8") PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_8)
	PORT_START("KEY2_5") //Port $2c0d
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B 2") PORT_CODE(KEYCODE_B) PORT_CODE(KEYCODE_2)
	PORT_START("KEY2_6") //Port $2c0e
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C 3") PORT_CODE(KEYCODE_C) PORT_CODE(KEYCODE_3)
	PORT_START("KEY2_7") //Port $2c0f
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D 4") PORT_CODE(KEYCODE_D) PORT_CODE(KEYCODE_4)

	PORT_INCLUDE( board )

INPUT_PORTS_END


static TIMER_DEVICE_CALLBACK( update_nmi )
{
	mephisto_state *state = timer.machine().driver_data<mephisto_state>();
	device_t *speaker = timer.machine().device("beep");
	if (state->m_allowNMI) {
		state->m_allowNMI = 0;
		cputag_set_input_line(timer.machine(), "maincpu", INPUT_LINE_NMI,PULSE_LINE);
	}
	beep_set_state(speaker,state->m_led_status&64?1:0);
}

static TIMER_DEVICE_CALLBACK( update_irq )		//only mm2
{
	mephisto_state *state = timer.machine().driver_data<mephisto_state>();
	device_t *speaker = timer.machine().device("beep");

	cputag_set_input_line(timer.machine(), "maincpu", M6502_IRQ_LINE, ASSERT_LINE);
	cputag_set_input_line(timer.machine(), "maincpu", M6502_IRQ_LINE, CLEAR_LINE);

	beep_set_state(speaker,state->m_led_status&64?1:0);
}
static MACHINE_START( mephisto )
{
	mephisto_state *state = machine.driver_data<mephisto_state>();
	state->m_lcd_shift_counter = 3;
	state->m_allowNMI = 1;
	mboard_savestate_register(machine);
}

static MACHINE_START( mm2 )
{
	mephisto_state *state = machine.driver_data<mephisto_state>();
	state->m_lcd_shift_counter = 3;
	state->m_led7=0xff;

	mboard_savestate_register(machine);
}


static MACHINE_RESET( mephisto )
{
	mephisto_state *state = machine.driver_data<mephisto_state>();
	state->m_lcd_shift_counter = 3;
	state->m_allowNMI = 1;
	mboard_set_border_pieces();
	mboard_set_board();

/* adjust artwork depending on current emulation*/

	if (!strcmp(machine.system().name,"mm2") )
		output_set_value("MM",1);
	else if (!strcmp(machine.system().name,"mm4") )
		output_set_value("MM",2);
	else if (!strcmp(machine.system().name,"mm4tk") )
		output_set_value("MM",5);
	else if (!strcmp(machine.system().name,"mm5tk") )
		output_set_value("MM",5);
	else if (!strcmp(machine.system().name,"mm5") )
		output_set_value("MM",3);
	else if (!strcmp(machine.system().name,"mm50") )
		output_set_value("MM",3);
	else if (!strcmp(machine.system().name,"rebel5") )
		output_set_value("MM",4);
}


static MACHINE_CONFIG_START( mephisto, mephisto_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M65C02,4915200)	/* 65C02 */
	MCFG_CPU_PROGRAM_MAP(mephisto_mem)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_START( mephisto )
	MCFG_MACHINE_RESET( mephisto )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_TIMER_ADD_PERIODIC("nmi_timer", update_nmi, attotime::from_hz(600))
	MCFG_TIMER_ADD_PERIODIC("artwork_timer", mboard_update_artwork, attotime::from_hz(100))
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( rebel5, mephisto )
	/* basic machine hardware */
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(rebel5_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mm2, mephisto )
	MCFG_CPU_REPLACE("maincpu", M65C02, 3700000)
	MCFG_CPU_PROGRAM_MAP(mm2_mem)
	MCFG_MACHINE_START( mm2 )

	MCFG_DEVICE_REMOVE("nmi_timer")
	MCFG_TIMER_ADD_PERIODIC("irq_timer", update_irq, attotime::from_hz(450))
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mm4tk, mephisto )
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_REPLACE("maincpu", M65C02, 18000000)
	MCFG_CPU_PROGRAM_MAP(mephisto_mem)
MACHINE_CONFIG_END

ROM_START(rebel5)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("rebel5.rom", 0x8000, 0x8000, CRC(8d02e1ef) SHA1(9972c75936613bd68cfd3fe62bd222e90e8b1083))
ROM_END

ROM_START(mm2)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mm2_1.bin", 0x8000, 0x4000, CRC(e2daac82) SHA1(c9fa59ca92362f8ee770733073bfa2ab8c7904ad))
	ROM_LOAD("mm2_2.bin", 0xc000, 0x4000, CRC(5e296939) SHA1(badd2a377259cf738cd076d8fb245c3dc284c24d))
ROM_END

ROM_START(mm4)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mephisto4.rom", 0x8000, 0x8000, CRC(f68a4124) SHA1(d1d03a9aacc291d5cb720d2ee2a209eeba13a36c))
	ROM_SYSTEM_BIOS( 0, "none", "No Opening Library" )
	ROM_SYSTEM_BIOS( 1, "hg440", "HG440 Opening Library" )
	ROMX_LOAD( "hg440.rom", 0x4000, 0x4000, CRC(81ffcdfd) SHA1(b0f7bcc11d1e821daf92cde31e3446c8be0bbe19), ROM_BIOS(2))
ROM_END

ROM_START(mm4tk)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mm4tk.rom", 0x8000, 0x8000, CRC(51cb36a4) SHA1(9e184b4e85bb721e794b88d8657ae8d2ff5a24af))
	ROM_SYSTEM_BIOS( 0, "none", "No Opening Library" )
	ROM_SYSTEM_BIOS( 1, "hg440", "HG440 Opening Library" )
	ROMX_LOAD( "hg440.rom", 0x4000, 0x4000, CRC(81ffcdfd) SHA1(b0f7bcc11d1e821daf92cde31e3446c8be0bbe19), ROM_BIOS(0))
ROM_END

ROM_START(mm5tk)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mephisto5.rom", 0x8000, 0x8000, BAD_DUMP CRC(89c3d9d2) SHA1(77cd6f8eeb03c713249db140d2541e3264328048))
	ROM_SYSTEM_BIOS( 0, "none", "No Opening Library" )
	ROM_SYSTEM_BIOS( 1, "hg550", "HG550 Opening Library" )
	ROMX_LOAD("hg550.rom", 0x4000, 0x4000, CRC(0359f13d) SHA1(833cef8302ad8d283d3f95b1d325353c7e3b8614),ROM_BIOS(0))
ROM_END

ROM_START(mm5)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mephisto5.rom", 0x8000, 0x8000, CRC(89c3d9d2) SHA1(77cd6f8eeb03c713249db140d2541e3264328048))
	ROM_SYSTEM_BIOS( 0, "none", "No Opening Library" )
	ROM_SYSTEM_BIOS( 1, "hg550", "HG550 Opening Library" )
	ROMX_LOAD("hg550.rom", 0x4000, 0x4000, CRC(0359f13d) SHA1(833cef8302ad8d283d3f95b1d325353c7e3b8614),ROM_BIOS(2))
ROM_END

ROM_START(mm50)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mm50.rom", 0x8000, 0x8000, CRC(fcfa7e6e) SHA1(afeac3a8c957ba58cefaa27b11df974f6f2066da))
	ROM_SYSTEM_BIOS( 0, "none", "No Opening Library" )
	ROM_SYSTEM_BIOS( 1, "hg550", "HG550 Opening Library" )
	ROMX_LOAD("hg550.rom", 0x4000, 0x4000, CRC(0359f13d) SHA1(833cef8302ad8d283d3f95b1d325353c7e3b8614),ROM_BIOS(2))
ROM_END


static DRIVER_INIT( mephisto )
{
	mephisto_state *state = machine.driver_data<mephisto_state>();
	state->m_lcd_shift_counter = 3;
}

/***************************************************************************

    Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY             FULLNAME                            FLAGS */

CONS( 1984, mm2,        mm4,	0,      mm2,        mephisto,   mephisto,   "Hegener & Glaser", "Mephisto MM2 Schach Computer",     GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1986, rebel5,     mm4,	0,      rebel5,     mephisto,   mephisto,   "Hegener & Glaser", "Mephisto Rebel 5 Schach Computer", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1987, mm4,        0,      0,      mephisto,   mephisto,   mephisto,   "Hegener & Glaser", "Mephisto 4 Schach Computer",       GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1987, mm4tk,      mm4,    0,      mm4tk,      mephisto,   mephisto,   "Hegener & Glaser", "Mephisto 4 Schach Computer Turbo Kit + HG440",       GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1990, mm5,        mm4,	0,      mephisto,   mephisto,   mephisto,   "Hegener & Glaser", "Mephisto 5.1 Schach Computer",     GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1990, mm50,       mm4,	0,      mephisto,   mephisto,   mephisto,   "Hegener & Glaser", "Mephisto 5.0 Schach Computer",     GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
CONS( 1990, mm5tk,      mm4,    0,      mm4tk,      mephisto,   mephisto,   "Hegener & Glaser", "Mephisto 5.1 Schach Computer Turbo Kit + HG550",       GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )


