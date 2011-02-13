/******************************************************************************
*
*  Pacific Educational Systems 'PES' Speech box
*  Part number VPU-1 V1
*  By Kevin 'kevtris' Horton and Jonathan Gevaryahu AKA Lord Nightmare
*
*  RE work done by Kevin 'kevtris' Horton and Jonathan Gevaryahu
*
*  DONE:
*  compiles correctly
*  rom loads correctly
*  interface with tms5220 is done
*  rts and cts bits are stored in struct
*
*  TODO:
*  figure out how to attach serial/terminal to the 80c31
*
***********************************************************************
This is almost as simple as hardware gets from the digital side:
Hardware consists of:
10.245Mhz xtal
8031 cpu
27c64 rom (holds less than 256 bytes of code)
unpopulated 6164 sram, which isn't used
TSP5220C speech chip (aka tms5220c)
mc145406 RS232 driver/reciever (+-24v to 0-5v converter for serial i/o)
74hc573b1 octal tri-state D-latch (part of bus interface for ROM)
74hc74b1 dual d-flipflop with set/reset, positive edge trigger (?)
74hc02b1 quad 2-input NOR gate (wired up to decode address 0, and data 0 and 1 to produce /RS and /WS)
mc14520b dual 4-bit binary up counter (used as a chopper for the analog filter)
Big messy analog section to allow voice output to be filtered extensively by a 4th order filter

Address map:
80C31 ADDR:
  0000-1FFF: ROM
  2000-3FFF: open bus (would be ram)
  4000-ffff: open bus
80C31 IO:
  00 W: d0 and d1 are the /RS and /WS bits
  port 1.x: tms5220 bus
  port 2.x: unused
  port 3.0: RxD serial recieve
  port 3.1: TxD serial send
  port 3.2: read, from /INT on tms5220c
  port 3.3: read, from /READY on tms5220c
  port 3.4: read, from the serial RTS line
  port 3.5: write, to the serial CTS line
  port 3.6: write, /WR (general) and /WE (pin 27) for unpopulated 6164 SRAM
  port 3.7: write, /RD (general) and /OE (pin 22) for unpopulated 6164 SRAM

*/

#define ADDRESS_MAP_MODERN
#define CPU_CLOCK		XTAL_10_245MHz

/* Core includes */
#include "emu.h"
#include "includes/pes.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/tms5220.h"
#include "machine/terminal.h"

/* Devices */
static WRITE8_DEVICE_HANDLER( pes_kbd_put )
{
	//duart68681_rx_data(device->machine->device("duart68681"), 1, data);
	// nothing, yet
	// todo: attach to serial
}

static GENERIC_TERMINAL_INTERFACE( pes_terminal_intf )
{
	DEVCB_HANDLER(pes_kbd_put)
};

/* Handlers */
WRITE8_MEMBER( pes_state::rsws_w )
{
	pes_state *state = machine->driver_data<pes_state>();
	m_wsstate = data&0x1; // /ws is bit 0
	m_rsstate = (data&0x2)>>1; // /rs is bit 1
	logerror("port0 write: RSWS states updated: /RS: %d, /WS: %d\n", m_rsstate, m_wsstate);
	tms5220_rsq_w(state->m_speech, m_rsstate);
	tms5220_wsq_w(state->m_speech, m_wsstate);
}

WRITE8_MEMBER( pes_state::port1_w )
{
	pes_state *state = machine->driver_data<pes_state>();
	logerror("port1 write: tms5220 data written: %02X\n", data);
	tms5220_data_w(state->m_speech, 0, data);
	
}

WRITE8_MEMBER( pes_state::port3_w )
{
	m_serial_cts = (data&0x20)>>5;
	logerror("port3 write: control data written: %02X; CTS: %d\n", data, m_serial_cts);
	// todo: poke serial handler
}


READ8_MEMBER( pes_state::port1_r )
{
	UINT8 temp = 0xFF;
	pes_state *state = machine->driver_data<pes_state>();
	temp = tms5220_status_r(state->m_speech, 0);
	logerror("port1 read: tms5220 data read: 0x%02X\n", temp);
	return temp;
}

READ8_MEMBER( pes_state::port3_r )
{
	UINT8 temp = 0xE3; // todo: actually return last written state?
	pes_state *state = machine->driver_data<pes_state>();
	temp |= (m_serial_rts<<4);
	temp |= (tms5220_readyq_r(state->m_speech)<<3);
	temp |= (tms5220_intq_r(state->m_speech)<<2);
	logerror("port3 read: returning 0x%02X\n", temp);
	return temp;
}


/* Reset */
void pes_state::machine_reset()
{
	m_wsstate = 1;
	m_rsstate = 1;
	m_serial_rts = 1;
	m_serial_cts = 1;
	devtag_reset(machine, "tms5220");
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(i80c31_mem, ADDRESS_SPACE_PROGRAM, 8, pes_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM /* 27C64 ROM */
	// AM_RANGE(0x2000, 0x3fff) AM_RAM /* 6164 8k SRAM, not populated */
ADDRESS_MAP_END

static ADDRESS_MAP_START(i80c31_io, ADDRESS_SPACE_IO, 8, pes_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0, 0x0) AM_WRITE(rsws_w) /* /WS(0) and /RS(1) */
	AM_RANGE(0x1, 0x1) AM_READWRITE(port1_r, port1_w) /* tms5220 reads and writes */
	AM_RANGE(0x3, 0x3) AM_READWRITE(port3_r, port3_w) /* writes and reads from port 3, see top of file */
ADDRESS_MAP_END

/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( pes )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static MACHINE_CONFIG_START( pes, pes_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I80C31, CPU_CLOCK)
    MCFG_CPU_PROGRAM_MAP(i80c31_mem)
    MCFG_CPU_IO_MAP(i80c31_io)
	
    /* sound hardware */
    MCFG_SPEAKER_STANDARD_MONO("mono")
    MCFG_SOUND_ADD("tms5220", TMS5220C, 640000) /* 720Khz clock, 10khz output */
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,pes_terminal_intf)
MACHINE_CONFIG_END

/******************************************************************************
 ROM Definitions
******************************************************************************/
ROM_START( pes )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_DEFAULT_BIOS("kevbios")
	ROM_SYSTEM_BIOS( 0, "orig", "PES box with original firmware v2.5")
	ROMX_LOAD( "vpu_2-5.bin",   0x0000, 0x2000, CRC(B27CFDF7) SHA1(C52ACF9C080823DE5EF26AC55ABE168AD53A7D38), ROM_BIOS(1)) // original firmware, rather buggy, 4800bps serial, buggy RTS/CTS flow control, no buffer
	ROM_SYSTEM_BIOS( 1, "kevbios", "PES box with kevtris' rewritten firmware")
	ROMX_LOAD( "pes.bin",   0x0000, 0x2000, CRC(22C1C4EC) SHA1(042E139CD0CF6FFAFCD88904F1636C6FA1B38F25), ROM_BIOS(2)) // rewritten firmware by kevtris, 4800bps serial, RTS/CTS plus XON/XOFF flow control, 64 byte buffer
ROM_END

/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME	PARENT	COMPAT	MACHINE		INPUT	INIT	COMPANY     FULLNAME            FLAGS */
COMP( 1987, pes,	0,		0,		pes,		pes,	0,		"Pacific Educational Systems",	"VPU-01 Speech box",	GAME_NOT_WORKING )
