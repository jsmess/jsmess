/******************************************************************************
*
*  Wavetek/Digelec model 804/EP804 (eprom programmer) driver
*  By balrog and Jonathan Gevaryahu AKA Lord Nightmare
*  Code adapted from zexall.c
*
*  DONE:
*  figure out z80 address space and hook up roms and rams (done)
*  figure out where 10937 vfd controller lives (port 0x44 bits 7 and 0)
*  figure out how keypresses are detected (/INT)
*  tentatively figure out how flow control from ACIA works (/NMI)?
*
*  TODO:
*  figure out the rest of the i/o map
*  find and dump 4.3 firmware and figure out how the banked ram works
*  actually hook up interrupts and nmi
*  hook up r6551 serial and attach terminal to it
*  correctly hook up 10937 vfd controller
*  hook up keypad and mode buttons
*  hook up leds on front panel
*  artwork
*
******************************************************************************/

#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"
#include "sound/speaker.h"
#include "machine/roc10937.h"


class digel804_state : public driver_device
{
public:
	digel804_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_terminal(*this, TERMINAL_TAG),
		  m_speaker(*this, "speaker")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_speaker;
	DECLARE_WRITE8_MEMBER( port_44_w );
	DECLARE_READ8_MEMBER( port_43_r );
	UINT8 vfd_data;
	UINT8 vfd_old;
	UINT8 vfd_count;
	UINT8 *m_main_ram;
};

static DRIVER_INIT( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_init(0,0,0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
}

static MACHINE_RESET( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_reset(0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
}

READ8_MEMBER( digel804_state::port_43_r ) // HACK to dump display contents to stderr
{
	fprintf(stderr,"%s\n",ROC10937_get_string(0));
	return 0xFF;
}

WRITE8_MEMBER( digel804_state::port_44_w )
{
	// latch vfd data on falling edge of clock only; this should really be part of the 10937 device, not here!
	if ((vfd_old&1) && ((data&1)==0))
	{
		vfd_data <<= 1;
		vfd_data |= (data&0x80)?1:0;
		vfd_count++;
		if (vfd_count == 8)
		{
			logerror("Digel804: Full byte written to port 44 vfd: %02X '%c'\n", vfd_data, vfd_data);
			ROC10937_newdata(0,vfd_data);
			vfd_data = 0;
			vfd_count = 0;
		}
	}
	vfd_old = data;
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem_804_1_4, AS_PROGRAM, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM // 3f in mapper = rom J3
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_BASE(m_main_ram) // 6f in mapper = RAM L3 (6164)
	AM_RANGE(0x6000, 0x7fff) AM_RAM AM_BASE(m_main_ram) // 77 in mapper = RAM M3 (6164)
	AM_RANGE(0x8000, 0x9fff) AM_RAM AM_BASE(m_main_ram) // 7b in mapper = RAM N3 (6164)
	AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(m_main_ram) // 7d in mapper = RAM O3 (6164)
	// c000-cfff is open bus in mapper, 7f
	AM_RANGE(0xd000, 0xd7ff) AM_RAM // 7e in mapper = RAM P3 (6116)
	AM_RANGE(0xd800, 0xf7ff) AM_ROM // 5f in mapper = rom K3
	// f800-ffff is open bus in mapper, 7f
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_mem_804_1_2, AS_PROGRAM, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM // 3f in mapper = rom J3
	AM_RANGE(0x2000, 0x3fff) AM_ROM // 5f in mapper = rom K3
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_BASE(m_main_ram) // 6f in mapper = RAM L3 (6164)
	AM_RANGE(0x6000, 0x7fff) AM_RAM AM_BASE(m_main_ram) // 77 in mapper = RAM M3 (6164)
	AM_RANGE(0x8000, 0x9fff) AM_RAM AM_BASE(m_main_ram) // 7b in mapper = RAM N3 (6164)
	AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(m_main_ram) // 7d in mapper = RAM O3 (6164)
	// c000-cfff is open bus in mapper, 7f
	AM_RANGE(0xd000, 0xd7ff) AM_RAM // 7e in mapper = RAM P3 (6116)
	// d800-ffff is open bus in mapper, 7f
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_io, AS_IO, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	//AM_RANGE(0x42, 0x42) AM_WRITE // gets 00 written to it
	AM_RANGE(0x43, 0x43) AM_READ(port_43_r) // unknown read port, possibly ACIA, currently hacked to dump display contents
	AM_RANGE(0x44, 0x44) AM_WRITE(port_44_w) // 10937 vfd controller clock on d0, data on d8; other bits unknown
	//AM_RANGE(0x45, 0x45) AM_WRITE // gets 01 written to it in a long chain. strongly suspect this is speaker beep but each write with bit 0 set generates a pulse
	//AM_RANGE(0x46, 0x46) AM_WRITE // gets 50 or 30 written to it
	//AM_RANGE(0x47, 0x47) AM_WRITE // gets 00 written to it
	//AM_RANGE(0x83, 0x83) AM_WRITE // may be acia?
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
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_6864MHz) /* Z80A, X1: 3.686Mhz */
	MCFG_CPU_PROGRAM_MAP(z80_mem_804_1_4)
	MCFG_CPU_IO_MAP(z80_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_RESET(digel804)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_CONFIG_END

/* TODO: make this copy the digel804 machine config and modify the program map! */
static MACHINE_CONFIG_START( ep804, digel804_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_6864MHz) /* Z80, X1: 3.686Mhz */
	MCFG_CPU_PROGRAM_MAP(z80_mem_804_1_2)
	MCFG_CPU_IO_MAP(z80_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_RESET(digel804)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER, 0)
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
	ROM_REGION(0x10000, "maincpu", 0)
	
	ROM_LOAD("1-04__76f1.27128.j6", 0x0000, 0x4000, CRC(61b50b61) SHA1(ad717fcbf3387b0a8fb0546025d3c527792eb3f0))
	// the second rom here is loaded bizarrely: the first 3/4 appears at e000-f7ff and the last 1/4 appears at d800-dfff
	ROM_LOAD("2-04__d6cc.2764a.k6", 0xe000, 0x1800, CRC(098cb008) SHA1(9f04e12489ab5f714d2fd4af8158969421557e75)) 
	ROM_CONTINUE(0xd800, 0x800)
	ROM_REGION(0x20, "proms", 0)
	ROM_LOAD("804-1-4.82s23a.j5", 0x0000, 0x0020, CRC(f961beb1) SHA1(f2ec89375e656eeabc30246d42741cf718fb0f91)) // Address mapper prom, 82s23/mmi6330/tbp18sa030 equivalent 32x8 open collector
ROM_END

ROM_START(ep804) // address mapper 804-1-2
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
