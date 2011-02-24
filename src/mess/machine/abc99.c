/*

Luxor ABC99

Keyboard PCB Layout
-------------------

|-----------------------------------------------------------------------|
|   CN1         CN2             CPU1        ROM1                    SW1 |
|                                                                       |
|   6MHz    CPU0        ROM0        GI                                  |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|-----------------------------------------------------------------------|

Notes:
    Relevant IC's shown.

    CPU0        - STMicro ET8035N-6 8035 CPU
    CPU1        - STMicro ET8035N-6 8035 CPU
    ROM0        - Texas Instruments TMS2516JL-16 2Kx8 ROM "107268-16"
    ROM1        - STMicro ET2716Q-1 2Kx8 ROM "107268-17"
    GI          - General Instruments 321239007 keyboard chip "4=7"
    CN1         - DB15 connector, Luxor ABC R8 (3 button mouse)
    CN2         - 12 pin PCB header, keyboard data cable
    SW1         - reset switch?

*/

/*

	TODO:

    - MCS-48 PC:01DC - Unimplemented opcode = 75 (ENT0 CLK, enable clock/3 output on T0)
	- EA
	- mouse
	- keys

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "abc99.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/devhelpr.h"
#include "sound/speaker.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define I8035_Z2_TAG		"z2"
#define I8035_Z5_TAG		"z5"
#define SPEAKER_TAG			"speaker"


enum
{
	LED_1 = 0,
	LED_2,
	LED_3,
	LED_4,
	LED_5,
	LED_6,
	LED_7,
	LED_8,
	LED_INS,
	LED_ALT,
	LED_CAPS_LOCK
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type ABC99 = abc99_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(abc99, "Luxor ABC 99")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abc99_device_config::device_config_complete()
{
	m_shortname = "abc99";
}


//-------------------------------------------------
//  ROM( abc99 )
//-------------------------------------------------

ROM_START( abc99 )
	ROM_REGION( 0x800, I8035_Z2_TAG, 0 )
	ROM_LOAD( "107268-17.z3", 0x000, 0x800, CRC(2f60cc35) SHA1(ebc6af9cd0a49a0d01698589370e628eebb6221c) )

	ROM_REGION( 0x800, I8035_Z5_TAG, 0 )
	ROM_LOAD( "107268-16.z6", 0x000, 0x800, CRC(785ec0c6) SHA1(0b261beae20dbc06fdfccc50b19ea48b5b6e22eb) )

	ROM_REGION( 0x1800, "unknown", 0)
	ROM_LOAD( "106819-09.bin", 0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) )
	ROM_LOAD( "107268-64.bin", 0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *abc99_device_config::rom_region() const
{
	return ROM_NAME( abc99 );
}


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z2_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_mem, ADDRESS_SPACE_PROGRAM, 8, abc99_device )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z2", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z2_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_io, ADDRESS_SPACE_IO, 8, abc99_device )
	AM_RANGE(0x30, 0x30) AM_READ_PORT("X0") AM_WRITENOP
	AM_RANGE(0x31, 0x31) AM_READ_PORT("X1") AM_WRITENOP
	AM_RANGE(0x32, 0x32) AM_READ_PORT("X2") AM_WRITENOP
	AM_RANGE(0x33, 0x33) AM_READ_PORT("X3") AM_WRITENOP
	AM_RANGE(0x34, 0x34) AM_READ_PORT("X4") AM_WRITENOP
	AM_RANGE(0x35, 0x35) AM_READ_PORT("X5") AM_WRITENOP
	AM_RANGE(0x36, 0x36) AM_READ_PORT("X6") AM_WRITENOP
	AM_RANGE(0x37, 0x37) AM_READ_PORT("X7") AM_WRITENOP
	AM_RANGE(0x38, 0x38) AM_READ_PORT("X8") AM_WRITENOP
	AM_RANGE(0x39, 0x39) AM_READ_PORT("X9") AM_WRITENOP
	AM_RANGE(0x3a, 0x3a) AM_READ_PORT("X10") AM_WRITENOP
	AM_RANGE(0x3b, 0x3b) AM_READ_PORT("X11") AM_WRITENOP
	AM_RANGE(0x3c, 0x3c) AM_READ_PORT("X12") AM_WRITENOP
	AM_RANGE(0x3d, 0x3d) AM_READ_PORT("X13") AM_WRITENOP
	AM_RANGE(0x3e, 0x3e) AM_READ_PORT("X14") AM_WRITENOP
	AM_RANGE(0x3f, 0x3f) AM_READ_PORT("X15") AM_WRITENOP
	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z2_bus_w)
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z2_p1_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z2_p2_r)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z2_t1_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_mem, ADDRESS_SPACE_PROGRAM, 8, abc99_device )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z5", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_io, ADDRESS_SPACE_IO, 8, abc99_device )
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z5_p1_r)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z5_p2_w)
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z5_t0_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z5_t1_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( abc99 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( abc99 )
	// keyboard CPU
	MCFG_CPU_ADD(I8035_Z2_TAG, I8035, XTAL_6MHz)
	MCFG_CPU_PROGRAM_MAP(abc99_z2_mem)
	MCFG_CPU_IO_MAP(abc99_z2_io)

	// mouse CPU
	MCFG_CPU_ADD(I8035_Z5_TAG, I8035, XTAL_6MHz)
	MCFG_CPU_PROGRAM_MAP(abc99_z5_mem)
	MCFG_CPU_IO_MAP(abc99_z5_io)

	// sound hardware
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor abc99_device_config::machine_config_additions() const
{
	return MACHINE_CONFIG_NAME( abc99 );
}


//-------------------------------------------------
//  INPUT_PORTS( abc99 )
//-------------------------------------------------

INPUT_PORTS_START( abc99 )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF13
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // TAB
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // RETURN
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 0 =
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PRINT
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ALT
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp CE
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // U
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp RETURN
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PRINT ??
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 9
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 3
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp .
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // left SHIFT
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // HELP
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 7
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 4
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 1
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 0
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // Z
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF15
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // DEL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // cursor B
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // cursor D
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // X
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF12
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // BS
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // uml U
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ' *
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // right SHIFT
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // Q
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // A

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF14
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // INS
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 6
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // cursor A
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // cursor C
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // CTRL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // CAPS LOCK

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF11
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // french E
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ring A
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // uml A
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // - _
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // W
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // S

	PORT_START("X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // STOP
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 8
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 5
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // kp 2
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // TAB left

	PORT_START("X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // + ?
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // P
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // uml O
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // . :
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // E
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // D

	PORT_START("X10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF10
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 0 =
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // O
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // L
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // , ;
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF1
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ESC

	PORT_START("X11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF9
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 9 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // I
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // K
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // N
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF2
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 1 !

	PORT_START("X12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 5 %
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // R
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // F
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // C
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // SPACE

	PORT_START("X13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF6
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 6 &
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // T
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // G
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // V
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF5
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 4 ¤

	PORT_START("X14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF7
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 7 /
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // Y
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // H
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // B
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF4
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 3 #

	PORT_START("X15")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF8
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PRINT ??
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // U
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) // J
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) // N
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // PF3
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) // 2 "

	PORT_START("DIP")
	PORT_DIPNAME( 0x07, 0x00, "Language" ) PORT_DIPLOCATION("S1:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x05, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x07, DEF_STR( Unknown ) )
	PORT_DIPNAME( 0x08, 0x08, "Keyboard Program" ) PORT_DIPLOCATION("S1:4")
	PORT_DIPSETTING(    0x00, "Internal (8048)" )
	PORT_DIPSETTING(    0x08, "External PROM" )

	PORT_START("MOUSEB")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START("MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1) PORT_CATEGORY(1)

	PORT_START("MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1) PORT_CATEGORY(1)
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

const input_port_token *abc99_device_config::input_ports() const
{
	return INPUT_PORTS_NAME( abc99 );
}
		


//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  serial_input -
//-------------------------------------------------

inline void abc99_device::serial_input()
{
	cpu_set_input_line(m_maincpu, INPUT_LINE_IRQ0, (m_si_en | m_si) ? CLEAR_LINE : ASSERT_LINE);
	
	cpu_set_input_line(m_mousecpu, INPUT_LINE_IRQ0, m_si ? CLEAR_LINE : ASSERT_LINE);
}


//-------------------------------------------------
//  serial_output -
//-------------------------------------------------

inline void abc99_device::serial_output()
{
	devcb_call_write_line(&m_out_txd_func, m_so_z2 & m_so_z5);
}


//-------------------------------------------------
//  serial_clock -
//-------------------------------------------------

inline void abc99_device::serial_clock()
{
	devcb_call_write_line(&m_out_clock_func, 1);
	devcb_call_write_line(&m_out_clock_func, 0);
}


//-------------------------------------------------
//  keydown -
//-------------------------------------------------

inline void abc99_device::key_down(int state)
{
	devcb_call_write_line(&m_out_keydown_func, state);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abc99_device - constructor
//-------------------------------------------------

abc99_device::abc99_device(running_machine &_machine, const abc99_device_config &config)
    : device_t(_machine, config),
	  m_maincpu(*this, I8035_Z2_TAG),
	  m_mousecpu(*this, I8035_Z5_TAG),
	  m_speaker(*this, SPEAKER_TAG),
	  m_si(1),
	  m_so_z2(1),
	  m_so_z5(1),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abc99_device::device_start()
{
	// allocate timers
	m_serial_timer = timer_alloc();
	m_serial_timer->adjust(MCS48_ALE_CLOCK(XTAL_6MHz));

	// resolve callbacks
    devcb_resolve_write_line(&m_out_txd_func, &m_config.m_out_txd_func, this);
    devcb_resolve_write_line(&m_out_clock_func, &m_config.m_out_clock_func, this);
    devcb_resolve_write_line(&m_out_keydown_func, &m_config.m_out_keydown_func, this);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void abc99_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	serial_clock();
}


//-------------------------------------------------
//  z2_bus_w -
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z2_bus_w )
{
	output_set_led_value(LED_1, BIT(data, 0));
	output_set_led_value(LED_2, BIT(data, 1));
	output_set_led_value(LED_3, BIT(data, 2));
	output_set_led_value(LED_4, BIT(data, 3));
	output_set_led_value(LED_5, BIT(data, 4));
	output_set_led_value(LED_6, BIT(data, 5));
	output_set_led_value(LED_7, BIT(data, 6));
	output_set_led_value(LED_8, BIT(data, 7));
}


//-------------------------------------------------
//  z2_p1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z2_p1_w )
{
	/*
	
		bit		description
		
		P10		serial output
		P11		KEY DOWN
		P12		transmit -> Z5 T1
		P13		INS led
		P14		ALT led
		P15		CAPS LOCK led
		P16		speaker output
		P17		Z8 enable
		
	*/

	// serial output
	m_so_z2 = BIT(data, 0);
	serial_output();

	// key down
	key_down(BIT(data, 1));

	// key LEDs
	output_set_led_value(LED_INS, BIT(data, 3));
	output_set_led_value(LED_ALT, BIT(data, 4));
	output_set_led_value(LED_CAPS_LOCK, BIT(data, 5));

	// master T1
	m_t1_z5 = BIT(data, 2);

	// speaker output
	speaker_level_w(m_speaker, !BIT(data, 6));
}


//-------------------------------------------------
//  z2_p2_r -
//-------------------------------------------------

READ8_MEMBER( abc99_device::z2_p2_r )
{
	/*
	
		bit		description
		
		P20		
		P21		
		P22		
		P23		
		P24		
		P25		DIP0
		P26		DIP1
		P27		DIP2
		
	*/
	
	astring tempstring;
	
	UINT8 data = input_port_read(machine, baseconfig().subtag(tempstring, "DIP")) << 5;

	return data;
}


//-------------------------------------------------
//  z2_t1_r -
//-------------------------------------------------

READ8_MEMBER( abc99_device::z2_t1_r )
{
	return m_t1_z2;
}


//-------------------------------------------------
//  z5_p1_r -
//-------------------------------------------------

READ8_MEMBER( abc99_device::z5_p1_r )
{
	/*
	
		bit		description
		
		P10		XA
		P11		XB
		P12		YA
		P13		YB
		P14		LB
		P15		MB
		P16		RB
		P17		input from host
		
	*/
	
	astring tempstring;
	
	UINT8 data = 0;

	// mouse buttons
	data |= input_port_read(machine, baseconfig().subtag(tempstring, "MOUSEB")) << 4;

	// serial input
	data |= m_si << 7;

	return data;
}


//-------------------------------------------------
//  z5_p2_w -
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z5_p2_w )
{
	/*
	
		bit		description
		
		P20		
		P21		
		P22		
		P23		
		P24		disable Z2 serial input
		P25		Z2 RESET
		P26		serial output
		P27		Z2 T1
		
	*/

	// disable keyboard CPU serial input
	m_si_en = BIT(data, 4);
	serial_input();

	// keyboard CPU reset
	cpu_set_input_line(m_maincpu, INPUT_LINE_RESET, BIT(data, 5) ? ASSERT_LINE : CLEAR_LINE);

	// serial output
	m_so_z5 = BIT(data, 6);
	serial_output();

	// keyboard CPU T1
	m_t1_z2 = BIT(data, 7);
}


//-------------------------------------------------
//  z5_t0_w -
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z5_t0_w )
{
	// clock output to Z2, not necessary to emulate
}


//-------------------------------------------------
//  z5_t1_r -
//-------------------------------------------------

READ8_MEMBER( abc99_device::z5_t1_r )
{
	return m_t1_z5;
}


//-------------------------------------------------
//  rxd_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abc99_device::rxd_w )
{
	m_si = state;

	serial_input();
}


//-------------------------------------------------
//  txd_r -
//-------------------------------------------------

READ_LINE_MEMBER( abc99_device::txd_r )
{
	return m_so_z2 & m_so_z5;
}


//-------------------------------------------------
//  reset_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abc99_device::reset_w )
{
	cpu_set_input_line(m_mousecpu, INPUT_LINE_RESET, state ? CLEAR_LINE : ASSERT_LINE);
}
