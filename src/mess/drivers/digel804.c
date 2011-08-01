/******************************************************************************
*
*  Wavetek/Digelec model 804/EP804 (eprom programmer) driver
*  By balrog and Jonathan Gevaryahu AKA Lord Nightmare
*  Code adapted from zexall.c
*
*  DONE:
*  figure out z80 address space and hook up roms and rams (done)
*  figure out where 10937 vfd controller lives (port 0x44 bits 7 and 0, POR has not been found yet)
*  figure out how keypresses are detected (/INT, port 0x43, and port 0x46)
*  figure out how the banked ram works on fw4.x (writes to port 0x43 select a ram bank)
*  tentatively figure out how flow control from ACIA works (/NMI)?
*  hook up the speaker/beeper (port 0x45)
*  hook up vfd controller (done to stderr, no artwork)
*  hook up leds on front panel (done to stderr)
*  hook up r6551 serial
*
*  TODO:
*  figure out the rest of the i/o map
*  figure out why banked ram on 4.x causes glitches/sys errors; it works on 2.2
*  actually hook up interrupts and nmi
*  attach terminal to 6551 serial
*  correctly hook up 10937 vfd controller
*  hook up keypad and mode buttons
*  artwork
*
******************************************************************************/

#define ADDRESS_MAP_MODERN

#define VFD_SERIAL_VERBOSE 1
#undef VFD_VERBOSE
#undef LED_VERBOSE

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"
#include "sound/speaker.h"
#include "machine/roc10937.h"
#include "machine/6551.h"


class digel804_state : public driver_device
{
public:
	digel804_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_terminal(*this, TERMINAL_TAG),
		  m_speaker(*this, "speaker"),
		  m_acia(*this, "acia")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_speaker;
	required_device<device_t> m_acia;
	DECLARE_WRITE8_MEMBER( port_43_w );
	DECLARE_READ8_MEMBER( port_43_r );
	DECLARE_WRITE8_MEMBER( port_44_w );
	DECLARE_WRITE8_MEMBER( speaker_w );
	DECLARE_WRITE8_MEMBER( led_control_w );
	DECLARE_READ8_MEMBER( keypad_r );
	DECLARE_READ8_MEMBER( acia_rxd_r );
	DECLARE_WRITE8_MEMBER( acia_txd_w );
	DECLARE_READ8_MEMBER( acia_status_r );
	DECLARE_WRITE8_MEMBER( acia_reset_w );
	DECLARE_READ8_MEMBER( acia_command_r );
	DECLARE_WRITE8_MEMBER( acia_command_w );
	DECLARE_READ8_MEMBER( acia_control_r );
	DECLARE_WRITE8_MEMBER( acia_control_w );
	// return for port 43 (mode/status reg)
	UINT8 port43_rtn;
	// vfd helper stuff for port 44, should be unnecessary after 10937 gets a proper device
	UINT8 vfd_data;
	UINT8 vfd_old;
	UINT8 vfd_count;
	// current speaker state for port 45
	UINT8 speaker_state;
	// ram stuff for future banking
	UINT8 m_ram_bank;
	UINT8 *m_main_ram;
};

/*static void digel804_set_ram_bank( running_machine &machine )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	memory_set_bankptr( machine, "bankedram", machine.region("user_ram")->base() + ( state->m_ram_bank * 0x10000 ));
}*/

static DRIVER_INIT( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_init(0,0,0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
	state->speaker_state = 0;
	state->port43_rtn = 0xEE;//0xB6;
	state->m_ram_bank = 0;
	memory_set_bankptr( machine, "bankedram", machine.region("user_ram")->base() + ( state->m_ram_bank * 0x10000 ));
}

static MACHINE_RESET( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_reset(0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
}

READ8_MEMBER( digel804_state::port_43_r )
{
	/* Register 0x43: status/mode register read
	 bits 76543210
	      |||||||\- overload state (0 = not overloaded; 1 = overload detected, led on and power disconnected to ic)
	      ||||||\-- unknown, always 1? may be acia related
	      |||||\--- any key pressed on keypad (0 = one or more pressed, 1 = none pressed or unit is about to go to standby)
	      ||||\---- remote mode selected (0 = selected, 1 = not) \
	      |||\----- key mode selected (0 = selected, 1 = not)     > if all 3 of these are 1, unit is going to standby
	      ||\------ sim mode selected (0 = selected, 1 = not)    /
	      |\------- power failure status (1 = power has failed, 0 = ok)
	      \-------- chip insert detect state (1 = no chip or cmos chip which ammeter cannot detect; 0 = nmos or detectable chip inserted)
	 after power failure (in key mode):
	 0xEE 11101110 when no keypad key pressed
	 0xEA 11101010 when keypad key pressed
	 in key mode:
	 0xAE 10101110 when no keypad key pressed
	 0xAA 10101010 when keypad key pressed
	 in remote mode:
	 0xB6 10110110 when no keypad key pressed
	 0xB2 10110010 when keypad key pressed
	 in sim mode:
	 0x9E 10011110 when no keypad key pressed
	 0x9A 10011010 when keypad key pressed
	 in off mode (before z80 is powered down):
	 0xFE 11111110
	 
	*/
	// HACK to dump display contents to stderr
	fprintf(stderr,"%s\n",ROC10937_get_string(0));
	logerror("Digel804: returning %02X for port 43 status read\n", port43_rtn);
	return port43_rtn;
}

WRITE8_MEMBER( digel804_state::port_43_w )
{
	m_ram_bank = data&7;
	//digel804_set_ram_bank( machine() );
	memory_set_bankptr( machine(), "bankedram", machine().region("user_ram")->base() + ( m_ram_bank * 0x10000 ));
	logerror("Digel804: port 0x43 ram bank had %02x written to it!\n", data);
}

WRITE8_MEMBER( digel804_state::port_44_w )
{
	/* writes to 0x44 control the 10937 vfd chip and z80 power and related stuff
	 * bits:76543210
	 *      |||||||\- 10937 VFDC '/SCK' serial clock
	 *      ||||||\-- unknown, is sometimes written 1 and sometimes 0, purpose unclear. is NOT /RES for ACIA and is NOT apparently POR on the VFDC
	 *      |||||\--- z80 and system power control (0 = power on, 1 = power off/standby)
	 *      ||||\---- controls, somehow, the z80 /BUSRQ line (0 = idle/high, 1 = asserted/low)
	 *      |||\----- unknown, always 0?
	 *      ||\------ unknown, always 0?
	 *      |\------- unknown, is sometimes written 1 but usually 0, purpose unclear, but implied to have something to do with eprom socket power
	 *      \-------- 10937 VFDC 'DATA' serial data
	 
	 */
#ifdef VFD_SERIAL_VERBOSE
	logerror("Digel804: port 0x44 vfd control had %02x written to it!\n", data);
#endif
	// latch vfd data on falling edge of clock only; this should really be part of the 10937 device, not here!
	if ((vfd_old&1) && ((data&1)==0))
	{
		vfd_data <<= 1;
		vfd_data |= (data&0x80)?1:0;
		vfd_count++;
		if (vfd_count == 8)
		{
#ifdef VFD_VERBOSE
			logerror("Digel804: Full byte written to port 44 vfd: %02X '%c'\n", vfd_data, vfd_data);
#endif
			ROC10937_newdata(0,vfd_data);
			vfd_data = 0;
			vfd_count = 0;
		}
	}
	vfd_old = data;
}

WRITE8_MEMBER( digel804_state::speaker_w )
{
	// it APPEARS all writes here invert the speaker state, but not certain!
	if ((data != 0x00) && (data != 0x01) && (data != 0x03))
		logerror("Digel804: port 0x45 speaker control had %02x written to it!\n", data);
	speaker_state ^= 0xFF;
	speaker_level_w(m_speaker, speaker_state);
}

WRITE8_MEMBER( digel804_state::led_control_w )
{
	/* writes to 0x46 control the LEDS on the front panel
	 * bits:76543210
	 *      ||||\\\\- these four bits choose which of the 16 function leds is lit; the number is INVERTED first
	 *      |||\----- if this bit is 1, the function leds are disabled
	 *      ||\------ this bit controls the 'error' led; 1 = on
	 *      |\------- this bit controls the 'busy' led; 1 = on
	 *      \-------- this bit controls the 'input' led; 1 = on
	 */
#ifdef LED_VERBOSE
	 logerror("Digel804: port 0x46 LED control had %02x written to it!\n", data);
#endif
	 fprintf(stderr,"LEDS: ");
	 if (data&0x80) fprintf(stderr,"INPUT "); else fprintf(stderr,"----- ");
	 if (data&0x40) fprintf(stderr,"BUSY "); else fprintf(stderr,"---- ");
	 if (data&0x20) fprintf(stderr,"ERROR "); else fprintf(stderr,"----- ");
	 fprintf(stderr,"  function selected: ");
	 if (data&0x10) fprintf(stderr,"none\n"); else fprintf(stderr,"%01X\n", ~data&0xF);
}

READ8_MEMBER( digel804_state::keypad_r )
{
	/* reads E* for a keypad number 0-F
	 * reads F0 for enter
	 * reads F4 for next
	 * reads F8 for rept
	 * reads FC for clear
	 * F* takes precedence over E*
	 * higher numbers take precedence over lower ones
	 * this value auto-latches on a key press and remains through multiple reads
	*/
	logerror("Digel804: returning 0xF0 for port 46 keypad read\n");
	return 0xF0; // enter
}

/* ACIA Trampolines */
READ8_MEMBER( digel804_state::acia_rxd_r )
{
	return acia_6551_r(m_acia, 0);
}

WRITE8_MEMBER( digel804_state::acia_txd_w )
{
	acia_6551_w(m_acia, 0, data);
}

READ8_MEMBER( digel804_state::acia_status_r )
{
	return acia_6551_r(m_acia, 1);
}

WRITE8_MEMBER( digel804_state::acia_reset_w )
{
	acia_6551_w(m_acia, 1, data);
}

READ8_MEMBER( digel804_state::acia_command_r )
{
	return acia_6551_r(m_acia, 2);
}

WRITE8_MEMBER( digel804_state::acia_command_w )
{
	acia_6551_w(m_acia, 2, data);
}

READ8_MEMBER( digel804_state::acia_control_r )
{
	return acia_6551_r(m_acia, 3);
}

WRITE8_MEMBER( digel804_state::acia_control_w )
{
	acia_6551_w(m_acia, 3, data);
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem_804_1_4, AS_PROGRAM, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM // 3f in mapper = rom J3
	//AM_RANGE(0x4000, 0x5fff) AM_RAM AM_BASE(m_main_ram) // 6f in mapper = RAM L3 (6164)
	//AM_RANGE(0x6000, 0x7fff) AM_RAM AM_BASE(m_main_ram) // 77 in mapper = RAM M3 (6164)
	//AM_RANGE(0x8000, 0x9fff) AM_RAM AM_BASE(m_main_ram) // 7b in mapper = RAM N3 (6164)
	//AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(m_main_ram) // 7d in mapper = RAM O3 (6164)
	AM_RANGE(0x4000,0xbfff) AM_RAMBANK("bankedram")
	// c000-cfff is open bus in mapper, 7f
	AM_RANGE(0xd000, 0xd7ff) AM_RAM // 7e in mapper = RAM P3 (6116)
	AM_RANGE(0xd800, 0xf7ff) AM_ROM // 5f in mapper = rom K3
	// f800-ffff is open bus in mapper, 7f
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_mem_804_1_2, AS_PROGRAM, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM // 3f in mapper = rom J3
	AM_RANGE(0x2000, 0x3fff) AM_ROM // 5f in mapper = rom K3
	//AM_RANGE(0x4000, 0x5fff) AM_RAM AM_BASE(m_main_ram) // 6f in mapper = RAM L3 (6164)
	//AM_RANGE(0x6000, 0x7fff) AM_RAM AM_BASE(m_main_ram) // 77 in mapper = RAM M3 (6164)
	//AM_RANGE(0x8000, 0x9fff) AM_RAM AM_BASE(m_main_ram) // 7b in mapper = RAM N3 (6164)
	//AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(m_main_ram) // 7d in mapper = RAM O3 (6164)
	AM_RANGE(0x4000,0xbfff) AM_RAMBANK("bankedram")
	// c000-cfff is open bus in mapper, 7f
	//AM_RANGE(0xc000, 0xc7ff) AM_RAM // hack for now to test, since sometimes it writes to c3ff
	AM_RANGE(0xd000, 0xd7ff) AM_RAM // 7e in mapper = RAM P3 (6116)
	// d800-ffff is open bus in mapper, 7f
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_io, AS_IO, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	// io bits: x 1 x x x * * *
	// writes to 47, 4e, 57, 5e, 67, 6e, 77, 7e, c7, ce, d7, de, e7, ee, f7, fe all go to 47, same with reads
	//AM_RANGE(0x40, 0x40) AM_MIRROR(0xB8) AM_WRITE // W, unknown
	//AM_RANGE(0x41, 0x41) AM_MIRROR(0xB8) AM_WRITE // W, unknown
	//AM_RANGE(0x42, 0x42) AM_MIRROR(0xB8) AM_WRITE // W, gets 00 written to it
	AM_RANGE(0x43, 0x43) AM_MIRROR(0xB8) AM_READWRITE(port_43_r, port_43_w) // RW, mode and status register, also checks if keypad is pressed; write is unknown
	AM_RANGE(0x44, 0x44) AM_MIRROR(0xB8) AM_WRITE(port_44_w) // W, 10937 vfd controller and z80 power control reg
	AM_RANGE(0x45, 0x45) AM_MIRROR(0xB8) AM_WRITE(speaker_w) // W, speaker bit; every write inverts state, values written are 00, 01, 03
	AM_RANGE(0x46, 0x46) AM_MIRROR(0xB8) AM_READWRITE(keypad_r, led_control_w) // RW, reads keypad, writes controls the front panel leds.
	//AM_RANGE(0x47, 0x47) AM_MIRROR(0xB8) AM_WRITE // W gets 00 written to it
	// io bits: 1 0 x x x * * *
	// writes to 80, 88, 90, 98, a0, a8, b0, b8 all go to 80, same with reads
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x38) AM_WRITE(acia_txd_w) // (ACIA xmit reg)
	AM_RANGE(0x81, 0x81) AM_MIRROR(0x38) AM_READ(acia_rxd_r) // (ACIA rcv reg)
	AM_RANGE(0x82, 0x82) AM_MIRROR(0x38) AM_WRITE(acia_reset_w) // (ACIA reset reg)
	AM_RANGE(0x83, 0x83) AM_MIRROR(0x38) AM_READ(acia_status_r) // (ACIA status reg)
	AM_RANGE(0x84, 0x84) AM_MIRROR(0x38) AM_WRITE(acia_command_w) // (ACIA command reg)
	AM_RANGE(0x85, 0x85) AM_MIRROR(0x38) AM_READ(acia_command_r) // (ACIA command reg)
	AM_RANGE(0x86, 0x86) AM_MIRROR(0x38) AM_WRITE(acia_control_w) // (ACIA control reg)
	AM_RANGE(0x87, 0x87) AM_MIRROR(0x38) AM_READ(acia_control_r) // (ACIA control reg)*/
	//AM_RANGE(0x80,0x87) AM_MIRROR(0x38) AM_SHIFT(-1) AM_DEVREADWRITE_LEGACY("acia", acia_6551_r, acia_6551_w) // this doesn't work since we lack an AM_SHIFT command
	
ADDRESS_MAP_END



/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( digel804 )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/

static GENERIC_TERMINAL_INTERFACE( digel804_terminal_intf )
{
	DEVCB_NULL
};

static MACHINE_CONFIG_START( digel804, digel804_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_6864MHz/2) /* Z80A, X1: 3.686Mhz */
	MCFG_CPU_PROGRAM_MAP(z80_mem_804_1_4)
	MCFG_CPU_IO_MAP(z80_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_RESET(digel804)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* acia */
	MCFG_ACIA6551_ADD("acia")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_CONFIG_END

/* TODO: make this copy the digel804 machine config and modify the program map! */
static MACHINE_CONFIG_START( ep804, digel804_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_6864MHz/2) /* Z80, X1: 3.686Mhz */
	MCFG_CPU_PROGRAM_MAP(z80_mem_804_1_2)
	MCFG_CPU_IO_MAP(z80_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_RESET(digel804)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* acia */
	MCFG_ACIA6551_ADD("acia")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

/*
For the mapper PROM:

z80 a11 -> prom a0
z80 a12 -> prom a1
z80 a13 -> prom a2
z80 a14 -> prom a3
z80 a15 -> prom a4

prom d0 -> ram 6116 at p6
prom d1 -> ram 6164 at o6
prom d2 -> ram 6164 at n6
prom d3 -> ram 6164 at m6
prom d4 -> ram 6164 at l6
prom d5 -> rom at k6
prom d6 -> rom at j6
prom d7 -> N/C? (unused?)
*/

ROM_START(digel804) // address mapper 804-1-4
	ROM_REGION(0x80000, "user_ram", ROMREGION_ERASEFF) 
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("1-04__76f1.27128.j6", 0x0000, 0x4000, CRC(61b50b61) SHA1(ad717fcbf3387b0a8fb0546025d3c527792eb3f0))
	// the second rom here is loaded bizarrely: the first 3/4 appears at e000-f7ff and the last 1/4 appears at d800-dfff
	ROM_LOAD("2-04__d6cc.2764a.k6", 0xe000, 0x1800, CRC(098cb008) SHA1(9f04e12489ab5f714d2fd4af8158969421557e75)) 
	ROM_CONTINUE(0xd800, 0x800)
	ROM_REGION(0x20, "proms", 0)
	ROM_LOAD("804-1-4.82s23a.j5", 0x0000, 0x0020, CRC(f961beb1) SHA1(f2ec89375e656eeabc30246d42741cf718fb0f91)) // Address mapper prom, 82s23/mmi6330/tbp18sa030 equivalent 32x8 open collector
ROM_END

ROM_START(ep804) // address mapper 804-1-2
	ROM_REGION(0x80000, "user_ram", ROMREGION_ERASEFF) 
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_DEFAULT_BIOS("ep804_v1.6")
	ROM_SYSTEM_BIOS( 0, "ep804_v1.6", "Wavetek/Digelec EP804 FWv1.6") // hardware 1.1
	ROMX_LOAD("804-2__rev_1.6__07f7.hn482764g-4.j6", 0x0000, 0x2000, CRC(2D4C334C) SHA1(1BE70ED5F4F315A8D2E38A17327A049E00FA174E), ROM_BIOS(1))
	ROMX_LOAD("804-3__rev_1.6__265c.hn482764g-4.k6", 0x2000, 0x2000, CRC(9C14906B) SHA1(41996039E604011C7C0F757397F82B6479EE3F62), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "ep804_v1.4", "Wavetek/Digelec EP804 FWv1.4") // hardware 1.1
	ROMX_LOAD("804-2_rev_1.4__7a7e.hn482764g-4.j6", 0x0000, 0x2000, CRC(FDC0D2E3) SHA1(DA1BC1E8C4CB2A2D8CD2273F3E1A9F318AE8CB87), ROM_BIOS(2))
	ROMX_LOAD("804-3_rev_1.4__f240.2732.k6", 0x2000, 0x1000, CRC(29827E29) SHA1(4C7FADF81BCF32349A564D946F5D215DE50315C5), ROM_BIOS(2))
	ROMX_LOAD("804-3_rev_1.4__f240.2732.k6", 0x3000, 0x1000, CRC(29827E29) SHA1(4C7FADF81BCF32349A564D946F5D215DE50315C5), ROM_BIOS(2)) // load this twice
	ROM_SYSTEM_BIOS( 2, "ep804_v2.21", "Wavetek/Digelec EP804 FWv2.21") // hardware 1.5 NOTE: this may use the address mapper 804-1-3 which is not dumped!
	ROMX_LOAD("804-2_rev2.21__cs_ab50.hn482764g.j6", 0x0000, 0x2000, CRC(ffbc95f6) SHA1(b12aa97e23d546064f1d17aa9b90772017fec5ec), ROM_BIOS(3))
	ROMX_LOAD("804-3_rev2.21__cs_6b98.hn482764g.k6", 0x2000, 0x2000, CRC(a4acb9fe) SHA1(bbc7e3e2e6b3b1abe747380909dcddc985ef8d0d), ROM_BIOS(3))
	ROM_REGION(0x20, "proms", 0)
	ROM_LOAD("804-1-2.mmi_6330-in.j5", 0x0000, 0x0020, CRC(30DD4721) SHA1(E4B2F5756118BE4C8AB56C708DC4F42469C7E51B)) // Address mapper prom, 82s23/mmi6330/tbp18sa030 equivalent 32x8 open collector
ROM_END



/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                                                    FLAGS */
COMP( 1985, digel804,   0,          0,      digel804,   digel804, digel804,      "Digelec, Inc",   "Digelec 804 EPROM Programmer", GAME_NO_SOUND_HW )
COMP( 1982, ep804,   digel804,          0,      ep804,   digel804, digel804,      "Wavetek/Digelec, Inc",   "EP804 EPROM Programmer", GAME_NO_SOUND_HW )
