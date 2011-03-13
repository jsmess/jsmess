/******************************************************************************
*
*    EA Car Computer
*        by Robbbert, March 2011.
*
*    Described in Electronics Australia magazine during 1982.
*
*    The only RAM is the 128 bytes that comes inside the CPU.
*
*    This computer is mounted in a car, and various sensors (fuel flow, etc)
*    are connected up. By pressing the appropriate buttons various statistics
*    may be obtained.
*
*    Memory Map
*    0000-03FF internal ram
*    4000-7FFF ROM
*    8000-BFFF 6821
*    C000-FFFF ROM (mirror)
*
*    The ROM was typed in from the dump in the magazine article, therefore
*    it is marked as BAD_DUMP.
*
******************************************************************************/
#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "eacc.lh"
#include "machine/6821pia.h"


class eacc_state : public driver_device
{
public:
	eacc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_pia(*this, "pia")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pia;
//	DECLARE_READ8_MEMBER( eacc_acia_status_r );
//	DECLARE_READ8_MEMBER( eacc_acia_data_r );
//	DECLARE_WRITE8_MEMBER( eacc_votrax_w );
//	DECLARE_WRITE8_MEMBER( eacc_kbd_put );
};




/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(eacc_mem, ADDRESS_SPACE_PROGRAM, 8, eacc_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xc7ff) // A11,A12,A13 not connected
	AM_RANGE(0x0000, 0x007f) AM_RAM // internal
	AM_RANGE(0xf800, 0xffff) AM_ROM AM_MIRROR(0x8000)
	AM_RANGE(0x8000, 0x8003) AM_MIRROR(0x7fc) AM_DEVREADWRITE_LEGACY("pia", pia6821_r, pia6821_w)
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/

static INPUT_PORTS_START(eacc)
	PORT_START("DSW1") /* not connected to cpu, each switch is connected directly to the output of a 4040 counter dividing the cpu m1? clock to feed the 6850 ACIA. Setting more than one switch on is a bad idea. see tnt_schematic.jpg */
	PORT_DIPNAME( 0xFF, 0x00, "Baud Rate" )	PORT_DIPLOCATION("SW1:1,2,3,4,5,6,7,8")
	PORT_DIPSETTING(    0x01, "75" )
	PORT_DIPSETTING(    0x02, "150" )
	PORT_DIPSETTING(    0x04, "300" )
	PORT_DIPSETTING(    0x08, "600" )
	PORT_DIPSETTING(    0x10, "1200" )
	PORT_DIPSETTING(    0x20, "2400" )
	PORT_DIPSETTING(    0x40, "4800" )
	PORT_DIPSETTING(    0x80, "9600" )
INPUT_PORTS_END

static MACHINE_RESET(eacc)
{
}

static TIMER_DEVICE_CALLBACK( eacc_multiplex )
{
	//eacc_state *state = timer.machine->driver_data<eacc_state>();
	//UINT8 status = votrax_status_r(state->m_votrax);
	//printf("%X ",status);
	//cpu_set_input_line(timer.machine->device("maincpu"), INPUT_LINE_IRQ0, status ? ASSERT_LINE : CLEAR_LINE);
}

static TIMER_DEVICE_CALLBACK( eacc_cb1 )
{
	//eacc_state *state = timer.machine->driver_data<eacc_state>();
	//UINT8 status = votrax_status_r(state->m_votrax);
	//printf("%X ",status);
	//cpu_set_input_line(timer.machine->device("maincpu"), INPUT_LINE_IRQ0, status ? ASSERT_LINE : CLEAR_LINE);
}

static const pia6821_interface eacc_mc6821_intf =
{
	DEVCB_NULL,//DEVCB_DRIVER_MEMBER(eacc_state, eacc_keyboard_r),	/* port A input */
	DEVCB_NULL,//DEVCB_DRIVER_MEMBER(eacc_state, eacc_cassette_r),	/* port B input */
	DEVCB_NULL,//DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_keydown_r),	/* CA1 input */
	DEVCB_NULL,//DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_rtc_pulse),	/* CB1 input */
	DEVCB_NULL,//DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_fn_key_r),	/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_NULL,//DEVCB_DRIVER_MEMBER(eacc_state, eacc_keyboard_w),	/* port A output */
	DEVCB_NULL,//DEVCB_DRIVER_MEMBER(eacc_state, eacc_cassette_w),	/* port B output */
	DEVCB_NULL,						/* CA2 output */
	DEVCB_NULL,//DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_screen_w),	/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE)		/* IRQB output */
};


/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_CONFIG_START( eacc, eacc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6802, XTAL_3_579545MHz)  /* Divided by 4 inside the m6802*/
	MCFG_CPU_PROGRAM_MAP(eacc_mem)

	MCFG_MACHINE_RESET(eacc)
	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_eacc)

	MCFG_PIA6821_ADD("pia", eacc_mc6821_intf)
	MCFG_TIMER_ADD_PERIODIC("eacc_multiplex", eacc_multiplex, attotime::from_hz(600) )
	MCFG_TIMER_ADD_PERIODIC("eacc_cb1", eacc_cb1, attotime::from_hz(30) )
MACHINE_CONFIG_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(eacc)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("eacc.bin", 0x4000, 0x0800, BAD_DUMP CRC(287a63c0) SHA1(f61b397d33ea40e5742e34d5f5468572125e8b39) )
ROM_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                            FLAGS */
COMP( 1980, eacc,       0,          0,      eacc,       eacc,   0,     "Electronics Australia", "EA Car Computer", GAME_NOT_WORKING | GAME_NO_SOUND_HW)

