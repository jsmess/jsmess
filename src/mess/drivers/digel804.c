/******************************************************************************
*
*  Wavetek/Digelec model 804/EP804 (eprom programmer) driver
*  By balrog and Jonathan Gevaryahu AKA Lord Nightmare
*  Code adapted from zexall.c
*
*  DONE:
*  figure out z80 address space and hook up roms and rams (done)
*  figure out where 10937 vfd controller lives (port 0x44 bits 7 and 0, POR has not been found yet)
*  figure out how keypresses are detected (/INT?, port 0x43, and port 0x46)
*  figure out how the banked ram works (writes to port 0x43 select a ram bank, fw2.2 supports 64k and fw4.1 supports 256k)
*  tentatively figure out how flow control from ACIA works (/NMI)?
*  hook up the speaker/beeper (port 0x45)
*  hook up vfd controller (done to stderr, no artwork)
*  hook up leds on front panel (done to stderr for now)
*  hook up r6551 serial and attach terminal for sending to ep804
*  /nmi is emulated correctly (i.e. its tied to +5v. that was easy!)
*
*  TODO:
*  figure out the rest of the i/o map
*  figure out why banked ram on 4.x causes glitches/sys errors; it works on 2.2
*  actually hook up interrupts: they fire on keypresses and 6551/ACIA receives
*  attach terminal to 6551/ACIA serial for recieving from ep804
*  correctly hook up 10937 vfd controller: por is not hooked up
*  hook up keypad via 74C923 and mode buttons via logic gate mess
*  artwork
*
******************************************************************************/

// port 40 read reads eprom socket pins 11-13, 15-19 (i.e. pin D0 to pin D7)

// port 40 write writes eprom socket pins 11-13, 15-19 (i.e. pin D0 to pin D7)

// port 41 write controls eprom socket pins 10 to 3 (d0 to d8) and SIM pins 7, 20/11, 8/12, 6/27, 23, and 21 (d0 to d5)

// port 42 write controls eprom socket pins 25(d0), 2(d4), 27(d6)

// port 43 read is status/mode
#undef PORT43_R_VERBOSE
// port 43 write is ram banking and ctl1-7; it also clears overload state
#undef PORT43_W_VERBOSE
// port 44 write is vfd serial, as well as power and z80 control and various enables; it also clears powerfail state
#define PORT44_W_VERBOSE 1
// port 45 write is speaker control
#undef PORT45_W_VERBOSE
// port 46 read is keypad

// port 46 write is LED control
#undef PORT46_W_VERBOSE

// port 47 write is tim0-tim7

// collated serial data sent to vfd
#undef VFD_VERBOSE


/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"
#include "sound/speaker.h"
#include "machine/roc10937.h"
#include "machine/6551acia.h"


class digel804_state : public driver_device
{
public:
	digel804_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_terminal(*this, TERMINAL_TAG),
		  m_speaker(*this, "speaker"),
		  m_acia(*this, "acia")
		//, m_main_ram(*this, "main_ram")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_speaker;
	required_device<acia6551_device> m_acia;
	DECLARE_READ8_MEMBER( ip40 );
	DECLARE_WRITE8_MEMBER( op40 );
	DECLARE_WRITE8_MEMBER( op41 );
	DECLARE_WRITE8_MEMBER( op42 );
	DECLARE_READ8_MEMBER( ip43 );
	DECLARE_WRITE8_MEMBER( op43 );
	DECLARE_WRITE8_MEMBER( op44 );
	DECLARE_WRITE8_MEMBER( op45 );
	DECLARE_READ8_MEMBER( ip46 );
	DECLARE_WRITE8_MEMBER( op46 );
	DECLARE_WRITE8_MEMBER( op47 );
	DECLARE_READ8_MEMBER( acia_rxd_r );
	DECLARE_WRITE8_MEMBER( acia_txd_w );
	DECLARE_READ8_MEMBER( acia_status_r );
	DECLARE_WRITE8_MEMBER( acia_reset_w );
	DECLARE_READ8_MEMBER( acia_command_r );
	DECLARE_WRITE8_MEMBER( acia_command_w );
	DECLARE_READ8_MEMBER( acia_control_r );
	DECLARE_WRITE8_MEMBER( acia_control_w );
	UINT8 digel804_convert_col_to_bin( UINT8 col, UINT8 row );
	// vfd helper stuff for port 44, should be unnecessary after 10937 gets a proper device
	UINT8 vfd_data;
	UINT8 vfd_old;
	UINT8 vfd_count;
	// current speaker state for port 45
	UINT8 m_speaker_state;
	// ram stuff for banking
	UINT8 m_ram_bank;
	//required_shared_ptr<UINT8> m_main_ram;
	// states
	UINT8 m_kbd;
	UINT8 m_kbd_row;
	UINT8 m_acia_intq;
	UINT8 m_overload_state;
	UINT8 m_debug_x5_state;
	UINT8 m_key_intq;
	UINT8 m_remote_mode;
	UINT8 m_key_mode;
	UINT8 m_sim_mode;
	UINT8 m_powerfail_state;
	UINT8 m_chipinsert_state;
};

/*
 74c923 handler by Robbbert copied more or less from tec1.c
*/
static TIMER_CALLBACK( ep804_kbd_callback )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };
	digel804_state *state = machine.driver_data<digel804_state>();

	machine.scheduler().timer_set(attotime::from_hz(10000), FUNC(ep804_kbd_callback));
    // 74C923 4 by 5 key encoder.
    // if previous key is still held, bail out
	if (input_port_read(machine, keynames[state->m_kbd_row]))
		if (state->digel804_convert_col_to_bin(input_port_read(machine, keynames[state->m_kbd_row]), state->m_kbd_row) == state->m_kbd)
			return;

	state->m_kbd_row++;
	state->m_kbd_row &= 3;

	/* see if a key pressed */
	if (input_port_read(machine, keynames[state->m_kbd_row]))
	{
		fprintf(stderr,"DEBUG: key was pressed!\n");
		state->m_kbd = state->digel804_convert_col_to_bin(input_port_read(machine, keynames[state->m_kbd_row]), state->m_kbd_row);
		state->m_key_intq = 0;
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);//ASSERT_LINE);
	}
	else
	{
		state->m_key_intq = 1;
		//if (state->m_acia_intq == 1) cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
	}
}

static DRIVER_INIT( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_init(0,0,0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
	state->m_speaker_state = 0;
	//state->port43_rtn = 0xEE;//0xB6;
	state->m_kbd = 0;
	state->m_kbd_row = 0;
	state->m_acia_intq = 1; // /INT source 1
	state->m_overload_state = 0; // OVLD
	state->m_debug_x5_state = 1; // "/DEBUG"
	state->m_key_intq = 1; // /INT source 2
	state->m_remote_mode = 1; // /REM
	state->m_key_mode = 0; // /KEY
	state->m_sim_mode = 1; // /SIM
	state->m_powerfail_state = 1; // "O11"
	state->m_chipinsert_state = 0; // PIN
	state->m_ram_bank = 0;
	memory_set_bankptr( machine, "bankedram", machine.region("user_ram")->base() + ( state->m_ram_bank * 0x10000 ));
	machine.scheduler().timer_set(attotime::from_hz(10000), FUNC(ep804_kbd_callback));
}

static MACHINE_RESET( digel804 )
{
	digel804_state *state = machine.driver_data<digel804_state>();
	ROC10937_reset(0); // TODO: replace this with a proper device
	state->vfd_old = state->vfd_data = state->vfd_count = 0;
}

UINT8 digel804_state::digel804_convert_col_to_bin( UINT8 col, UINT8 row )
{
	UINT8 data = row;

	if (BIT(col, 1))
		data |= 4;
	else
	if (BIT(col, 2))
		data |= 8;
	else
	if (BIT(col, 3))
		data |= 12;
	else
	if (BIT(col, 4))
		data |= 16;

	return data;
}

READ8_MEMBER( digel804_state::ip40 ) // eprom data bus read
{
	// TODO: would be nice to have a 'fake eprom' here
	return 0xFF;
}

WRITE8_MEMBER( digel804_state::op40 ) // eprom data bus write
{
	// TODO: would be nice to have a 'fake eprom' here
	logerror("Digel804: port 40, eprom databus had %02X written to it!\n", data);
}

WRITE8_MEMBER( digel804_state::op41 ) // eprom address low write
{
	// TODO: would be nice to have a 'fake eprom' here
	logerror("Digel804: port 41, eprom address low had %02X written to it!\n", data);
}

WRITE8_MEMBER( digel804_state::op42 ) // eprom address hi and control write
{
	// TODO: would be nice to have a 'fake eprom' here
	logerror("Digel804: port 42, eprom address hi/control had %02X written to it!\n", data);
}

READ8_MEMBER( digel804_state::ip43 )
{
	/* Register 0x43: status/mode register read
     bits 76543210
          |||||||\- overload state (0 = not overloaded; 1 = overload detected, led on and power disconnected to ic, writing to R43 sets this to ok)
          ||||||\-- debug jumper X5 on the board; reads as 1 unless jumper is present
          |||||\--- /INT status (any key pressed on keypad (0 = one or more pressed, 1 = none pressed) OR ACIA has thrown an int)
          ||||\---- remote mode selected (0 = selected, 1 = not) \
          |||\----- key mode selected (0 = selected, 1 = not)     > if all 3 of these are 1, unit is going to standby
          ||\------ sim mode selected (0 = selected, 1 = not)    /
          |\------- power failure status (1 = power has failed, 0 = ok; writes to R44 set this to ok)
          \-------- chip insert detect state 'PIN' (1 = no chip or cmos chip which ammeter cannot detect; 0 = nmos or detectable chip inserted)
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
#ifdef PORT43_R_VERBOSE
	logerror("Digel804: returning %02X for port 43 status read\n", port43_rtn);
#endif
	return ((m_overload_state<<7)
		|(m_debug_x5_state<<6)
		|((m_key_intq&m_acia_intq)<<5)
		|(m_remote_mode<<4)
		|(m_key_mode<<3)
		|(m_sim_mode<<2)
		|(m_powerfail_state<<1)
		|(m_chipinsert_state));

}

WRITE8_MEMBER( digel804_state::op43 )
{
	/* writes to 0x43 control the ram banking on firmware which supports it
     * bits:76543210
     *      |||||\\\- select ram bank for 4000-bfff area based on these bits
     *      \\\\\---- unknown, always 0?

     * writes to 0x43 ALSO control

     * all writes to port 43 will reset the overload state unless the ammeter detects the overload is ongoing
     */
	m_overload_state = 0; // writes to port 43 clear overload state
#ifdef PORT43_W_VERBOSE
	logerror("Digel804: port 0x43 ram bank had %02x written to it!\n", data);
#endif
	m_ram_bank = data&7;
	if ((data&0xF8)!=0)
		logerror("Digel804: port 0x43 ram bank had unexpected data %02x written to it!\n", data);
	memory_set_bankptr( machine(), "bankedram", machine().region("user_ram")->base() + ( m_ram_bank * 0x10000 ));

}

WRITE8_MEMBER( digel804_state::op44 ) // state write
{
	/* writes to 0x44 control the 10937 vfd chip, z80 power/busrq, eprom driving and some eprom power ctl lines
     * bits:76543210
     *      |||||||\- 10937 VFDC '/SCK' serial clock '/CDIS'
     *      ||||||\-- controls /KEYEN
     *      |||||\--- z80 and system power control (0 = power on, 1 = power off/standby), also controls '/MEMEN'
     *      ||||\---- controls the z80 /BUSRQ line (0 = idle/high, 1 = asserted/low) '/BRQ'
     *      |||\----- when 1, enables the output drivers of op40 to the rom data pins
     *      ||\------ controls 'CTL8'
     *      |\------- controls 'CTL9'
     *      \-------- 10937 VFDC 'DATA' serial data /DDIS

     * all writes to port 44 will reset the powerfail state
     */
	m_powerfail_state = 0; // writes to port 44 clear powerfail state
#ifdef PORT44_W_VERBOSE
	logerror("Digel804: port 0x44 vfd/state control had %02x written to it!\n", data);
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

WRITE8_MEMBER( digel804_state::op45 ) // speaker write
{
	// all writes to here invert the speaker state, verified from schematics
#ifdef PORT45_W_VERBOSE
	logerror("Digel804: port 0x45 speaker had %02x written to it!\n", data);
#endif
	m_speaker_state ^= 0xFF;
	speaker_level_w(m_speaker, m_speaker_state);
}

READ8_MEMBER( digel804_state::ip46 ) // keypad read
{
	/* reads E* for a keypad number 0-F
     * reads F0 for enter
     * reads F4 for next
     * reads F8 for rept
     * reads FC for clear
     * F* takes precedence over E*
     * higher numbers take precedence over lower ones
     * this value auto-latches on a key press and remains through multiple reads
     * this is done by a 74C923 integrated circuit
    */
#ifdef PORT46_R_VERBOSE
	logerror("Digel804: returning %02X for port 46 keypad read\n", m_kbd);
#endif
	return m_kbd;
}

WRITE8_MEMBER( digel804_state::op46 )
{
	/* writes to 0x46 control the LEDS on the front panel
     * bits:76543210
     *      ||||\\\\- these four bits choose which of the 16 function leds is lit; the number is INVERTED first
     *      |||\----- if this bit is 1, the function leds are disabled
     *      ||\------ this bit controls the 'error' led; 1 = on
     *      |\------- this bit controls the 'busy' led; 1 = on
     *      \-------- this bit controls the 'input' led; 1 = on
     */
#ifdef PORT46_W_VERBOSE
	 logerror("Digel804: port 0x46 LED control had %02x written to it!\n", data);
#endif
	 fprintf(stderr,"LEDS: ");
	 if (data&0x80) fprintf(stderr,"INPUT "); else fprintf(stderr,"----- ");
	 if (data&0x40) fprintf(stderr,"BUSY "); else fprintf(stderr,"---- ");
	 if (data&0x20) fprintf(stderr,"ERROR "); else fprintf(stderr,"----- ");
	 fprintf(stderr,"  function selected: ");
	 if (data&0x10) fprintf(stderr,"none\n"); else fprintf(stderr,"%01X\n", ~data&0xF);
}

WRITE8_MEMBER( digel804_state::op47 ) // eprom timing/power and control write
{
	// TODO: would be nice to have a 'fake eprom' here
	logerror("Digel804: port 47, eprom timing/power and control had %02X written to it!\n", data);
}


/* ACIA Trampolines */
READ8_MEMBER( digel804_state::acia_rxd_r )
{
	return m_acia->read(space, 0);
}

WRITE8_MEMBER( digel804_state::acia_txd_w )
{
	m_acia->write(space, 0, data);
}

READ8_MEMBER( digel804_state::acia_status_r )
{
	return m_acia->read(space, 1);
}

WRITE8_MEMBER( digel804_state::acia_reset_w )
{
	m_acia->write(space, 1, data);
}

READ8_MEMBER( digel804_state::acia_command_r )
{
	return m_acia->read(space, 2);
}

WRITE8_MEMBER( digel804_state::acia_command_w )
{
	m_acia->write(space, 2, data);
}

READ8_MEMBER( digel804_state::acia_control_r )
{
	return m_acia->read(space, 3);
}

WRITE8_MEMBER( digel804_state::acia_control_w )
{
	m_acia->write(space, 3, data);
}


/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem_804_1_4, AS_PROGRAM, 8, digel804_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM // 3f in mapper = rom J3
	//AM_RANGE(0x4000, 0x5fff) AM_RAM AM_SHARE("main_ram") // 6f in mapper = RAM L3 (6164)
	//AM_RANGE(0x6000, 0x7fff) AM_RAM AM_SHARE("main_ram") // 77 in mapper = RAM M3 (6164)
	//AM_RANGE(0x8000, 0x9fff) AM_RAM AM_SHARE("main_ram") // 7b in mapper = RAM N3 (6164)
	//AM_RANGE(0xa000, 0xbfff) AM_RAM AM_SHARE("main_ram") // 7d in mapper = RAM O3 (6164)
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
	//AM_RANGE(0x4000, 0x5fff) AM_RAM AM_SHARE("main_ram") // 6f in mapper = RAM L3 (6164)
	//AM_RANGE(0x6000, 0x7fff) AM_RAM AM_SHARE("main_ram") // 77 in mapper = RAM M3 (6164)
	//AM_RANGE(0x8000, 0x9fff) AM_RAM AM_SHARE("main_ram") // 7b in mapper = RAM N3 (6164)
	//AM_RANGE(0xa000, 0xbfff) AM_RAM AM_SHARE("main_ram") // 7d in mapper = RAM O3 (6164)
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
	AM_RANGE(0x40, 0x40) AM_MIRROR(0xB8) AM_READWRITE(ip40, op40) // RW, eprom socket data bus input/output value
	AM_RANGE(0x41, 0x41) AM_MIRROR(0xB8) AM_WRITE(op41) // W, eprom socket address low out
	AM_RANGE(0x42, 0x42) AM_MIRROR(0xB8) AM_WRITE(op42) // W, eprom socket address hi/control out
	AM_RANGE(0x43, 0x43) AM_MIRROR(0xB8) AM_READWRITE(ip43, op43) // RW, mode and status register, also checks if keypad is pressed; write is unknown
	AM_RANGE(0x44, 0x44) AM_MIRROR(0xB8) AM_WRITE(op44) // W, 10937 vfd controller, z80 power and state control reg
	AM_RANGE(0x45, 0x45) AM_MIRROR(0xB8) AM_WRITE(op45) // W, speaker bit; every write inverts state
	AM_RANGE(0x46, 0x46) AM_MIRROR(0xB8) AM_READWRITE(ip46, op46) // RW, reads keypad, writes controls the front panel leds.
	AM_RANGE(0x47, 0x47) AM_MIRROR(0xB8) AM_WRITE(op47) // W eprom socket power and timing control
	// io bits: 1 0 x x x * * *
	// writes to 80, 88, 90, 98, a0, a8, b0, b8 all go to 80, same with reads
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x38) AM_WRITE(acia_txd_w) // (ACIA xmit reg)
	AM_RANGE(0x81, 0x81) AM_MIRROR(0x38) AM_READ(acia_rxd_r) // (ACIA rcv reg)
	AM_RANGE(0x82, 0x82) AM_MIRROR(0x38) AM_WRITE(acia_reset_w) // (ACIA reset reg)
	AM_RANGE(0x83, 0x83) AM_MIRROR(0x38) AM_READ(acia_status_r) // (ACIA status reg)
	AM_RANGE(0x84, 0x84) AM_MIRROR(0x38) AM_WRITE(acia_command_w) // (ACIA command reg)
	AM_RANGE(0x85, 0x85) AM_MIRROR(0x38) AM_READ(acia_command_r) // (ACIA command reg)
	AM_RANGE(0x86, 0x86) AM_MIRROR(0x38) AM_WRITE(acia_control_w) // (ACIA control reg)
	AM_RANGE(0x87, 0x87) AM_MIRROR(0x38) AM_READ(acia_control_r) // (ACIA control reg)
	//AM_RANGE(0x80,0x87) AM_MIRROR(0x38) AM_SHIFT(-1) AM_DEVREADWRITE("acia", acia6551_device, read, write) // this doesn't work since we lack an AM_SHIFT command

ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( digel804 )
	PORT_START("LINE0") /* KEY ROW 0, 'X1' */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)	PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)	PORT_CHAR('2')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)	PORT_CHAR('3')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTR/INCR") PORT_CODE(KEYCODE_ENTER)	PORT_CHAR('^')

	PORT_START("LINE1") /* KEY ROW 1, 'X2' */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)	PORT_CHAR('4')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)	PORT_CHAR('6')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NEXT/DECR") PORT_CODE(KEYCODE_DOWN)	PORT_CHAR('V')

	PORT_START("LINE2") /* KEY ROW 2, 'X3' */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)	PORT_CHAR('8')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)	PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)	PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("REPT") PORT_CODE(KEYCODE_X)	PORT_CHAR('X')

	PORT_START("LINE3") /* KEY ROW 3, 'X4' */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CLR") PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')

	PORT_START("MODE") // TODO
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static WRITE8_DEVICE_HANDLER( digel804_serial_put )
{
	digel804_state *state = device->machine().driver_data<digel804_state>();
	state->m_acia->receive_character(data);
}

static GENERIC_TERMINAL_INTERFACE( digel804_terminal_intf )
{
	DEVCB_HANDLER(digel804_serial_put)
};

static MACHINE_CONFIG_START( digel804, digel804_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_6864MHz/2) /* Z80A, X1: 3.686Mhz */
	MCFG_CPU_PROGRAM_MAP(z80_mem_804_1_4)
	MCFG_CPU_IO_MAP(z80_io)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))
	MCFG_MACHINE_RESET(digel804)

	/* video hardware */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* acia */
	MCFG_ACIA6551_ADD("acia")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
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
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, digel804_terminal_intf)

	/* acia */
	MCFG_ACIA6551_ADD("acia")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
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
COMP( 1985, digel804,   0,          0,      digel804,   digel804, digel804,      "Digelec, Inc",   "Digelec 804 EPROM Programmer", GAME_NOT_WORKING )
COMP( 1982, ep804,   digel804,          0,      ep804,   digel804, digel804,      "Wavetek/Digelec, Inc",   "EP804 EPROM Programmer", GAME_NOT_WORKING )
