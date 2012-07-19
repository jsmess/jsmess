/***************************************************************************

        DEC VK100 'GIGI'

        12/05/2009 Skeleton driver.
        28/07/2009 added Guru-readme(TM)

        Todo: attach hd46505sp CRTC
              hook up vram
              - vram space is now present but not hooked to anything
              emulate vector generator hardware
              - stubbing registers:
                done: X, Y, Err, Pattern, WOPS

 Tony DiCenzo, now the director of standards and architecture at Oracle, was on the team that developed the VK100
 see http://startup.nmnaturalhistory.org/visitorstories/view.php?ii=79

 The prototype name for the VK100 was 'SMAKY' (Smart Keyboard)

****************************************************************************/
/*
DEC VK100
DEC, 1982

This is a VK100 terminal, otherwise known as a DEC Gigi graphics terminal.
There's a technical manual dated 1982 here:
http://web.archive.org/web/20091015205827/http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf
Installation and owner's manual is at:
http://www.bitsavers.org/pdf/dec/terminal/gigi/EK-VK100-IN-002_GIGI_Terminal_Installation_and_Owners_Manual_Apr81.pdf

PCB Layout
----------

VK100 LOGICBOARD
    |-------|    |---------|  |---------|    |-| |-| |-|  |-|
|---|-20 mA-|----|---EIA---|--|HARD-COPY|----|B|-|G|-|R|--|-|----------|
|                                                         BW   DSW(8)  |
|                                                             POWER    |
|                 PR2                                                  |
|                           HD46505SP              4116 4116 4116 4116 |
|                                                                      |
|                                                  4116 4116 4116 4116 |
|      PR5        INTEL           ROM1                                 |
|         PR1 PR6 P8251A                           4116 4116 4116 4116 |
|                     45.6192MHz  ROM2  PR3                            |
|                                                  4116 4116 4116 4116 |
| 4116 4116 4116  INTEL           ROM3                                 |
|                 D8202A                                               |
| 4116 4116 4116       5.0688MHz  ROM4                       PR4       |
|                                                                      |
| 4116 4116       INTEL    SMC_5016T                            PIEZO  |
|                 D8085A                        IDC40   LM556   75452  |
|----------------------------------------------------------------------|
Notes:
      ROM1 - TP-01 (C) DEC 23-031E4-00 (M) SCM91276L 8114
      ROM2 - TP-01 (C) DEC 1980 23-017E4-00 MOSTEK MK36444N 8116
      ROM3 - TP-01 (C) MICROSOFT 1979 23-018E4-00 MOSTEK MK36445N 8113
      ROM4 - TP-01 (C) DEC 1980 23-190E2-00 P8316E AMD 35517 8117DPP

    LED meanings:
    The LEDS on the vk100 (there are 7) are set up above the keyboard as:
    Label: ON LINE   LOCAL     NO SCROLL BASIC     HARD-COPY L1        L2
    Bit:   !d5       d5        !d4       !d3       !d2       !d1       !d0 (of port 0x68)
according to manual from http://www.bitsavers.org/pdf/dec/terminal/gigi/EK-VK100-IN-002_GIGI_Terminal_Installation_and_Owners_Manual_Apr81.pdf
where X = on, 0 = off, ? = variable (- = off)
    - X 0 0 0 0 0 (0x1F) = Microprocessor error
    X - 0 X X X X (0x30) "

    - X 0 0 0 0 X (0x1E) = ROM error
    X - 0 0 ? ? ? (0x3x) "
1E 3F = rom error, rom 1 (0000-0fff)
1E 3E = rom error, rom 1 (1000-1fff)
1E 3D = rom error, rom 2 (2000-2fff)
1E 3C = rom error, rom 2 (3000-3fff)
1E 3B = rom error, rom 3 (4000-4fff)
1E 3A = rom error, rom 3 (5000-5fff)
1E 39 = rom error, rom 4 (6000-6fff)

    - X 0 0 0 X 0 (0x1D) = RAM error
    X - 0 ? ? ? ? (0x3x) "

    - X 0 0 0 X X (0x1C) = CRT Controller error
    X - 0 X X X X (0x30) "
This test writes 0xF to port 00 (crtc address reg) and writes a pattern to it
via port 01 (crtc data reg) then reads it back and checks to be sure the data
matches.

    - X 0 0 X 0 0 (0x1B) = CRT Controller time-out
    X - 0 X X X X (0x30) "
This test writes 00 to all the crtc registers and checks to be sure an rst7.5
(vblank) interrupt fires on the 8085 within a certain time period.

    - X 0 0 X 0 X (0x1A) = Vector time-out error
    X - 0 X X X X (0x30) "

*/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/beep.h"
#include "video/mc6845.h"
#include "machine/i8251.h"
#include "vk100.lh"

class vk100_state : public driver_device
{
public:
	vk100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen"),
		//m_crtc(*this, "hd46505sp"),
		m_speaker(*this, BEEPER_TAG)//,
		//m_uart(*this, "i8251")
		{ }
	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;
	//required_device<mc6845_device> m_crtc;
	required_device<device_t> m_speaker;
	//required_device<device_t> m_uart;

	UINT16 m_vgX;
	UINT16 m_vgY;
	UINT8 m_vgErr;
	UINT8 m_vgPat;
	UINT8 m_vgWOPS;
	
	DECLARE_WRITE8_MEMBER(vgLD_X);
	DECLARE_WRITE8_MEMBER(vgLD_Y);
	DECLARE_WRITE8_MEMBER(vgErr);
	
	DECLARE_WRITE8_MEMBER(vgPat);
	
	DECLARE_WRITE8_MEMBER(vgWOPS);
	
	DECLARE_WRITE8_MEMBER(vgEX_VEC);
	
	DECLARE_WRITE8_MEMBER(KBDW);
	DECLARE_READ8_MEMBER(vk100_keyboard_column_r);
};

/*
void drawVector(UINT16 px, UINT16 py, UINT16 x, UINT16 y, int pattern)
{
	UINT8 *gfx = machine.root_device().memregion("vram")->base();
}*/

/* ports 0x40 and 0x41: load low and high bytes of vector gen X register */
WRITE8_MEMBER(vk100_state::vgLD_X)
{
	m_vgX &= 0xFF << ((1-offset)*8);
	m_vgX |= ((UINT16)data) << (offset*8);
	logerror("VG: X Reg 0x%02X loaded with %04X, new X value is %04X\n", 0x40+offset, ((UINT16)data) << (offset*8), m_vgX);
}

/* ports 0x42 and 0x43: load low and high bytes of vector gen Y register */
WRITE8_MEMBER(vk100_state::vgLD_Y)
{
	m_vgY &= 0xFF << ((1-offset)*8);
	m_vgY |= ((UINT16)data) << (offset*8);
	logerror("VG: Y Reg 0x%02X loaded with %04X, new Y value is %04X\n", 0x42+offset, ((UINT16)data) << (offset*8), m_vgY);
}

/* port 0x44: "ERR" load bresenham line algorithm 'error' count */
WRITE8_MEMBER(vk100_state::vgErr)
{
	m_vgErr = data;
	logerror("VG: Err Reg 0x44 loaded with %02X\n", m_vgErr);
}

/* port 0x46: "Pat" load vg Pattern register */
WRITE8_MEMBER(vk100_state::vgPat)
{
	m_vgPat = data;
	logerror("VG: Pattern Reg 0x46 loaded with %02X\n", m_vgPat);
}

/* port 0x63: "WOPS" vector 'pixel' write options
 * --Attributes to change --   Enable --  Functions --
 * Blink  Green  Red    Blue   Attrib F1     F0     FN
 *                             Change
 * d7     d6     d5     d4     d3     d2     d1     d0
 */
WRITE8_MEMBER(vk100_state::vgWOPS)
{
	static const char *const functions[] = { "Overlay", "Replace", "Complement", "Erase" };
	m_vgWOPS = data;
	logerror("VG: WOPS loaded with %02X: KGRB %d%d%d%d, AttrChange %d, Function %s, Negate %d\n", data, (m_vgWOPS>>7)&1, (m_vgWOPS>>6)&1, (m_vgWOPS>>5)&1, (m_vgWOPS>>4)&1, (m_vgWOPS>>3)&1, functions[(m_vgWOPS>>1)&3], m_vgWOPS&1);
}

/* port 0x66: "EX VEC" execute a vector (start the state machine) */
WRITE8_MEMBER(vk100_state::vgEX_VEC)
{
	logerror("VG Execute Vector 0x66 written: stub!\n");
	// TODO: short term: do some calculations here and print the expected starting ram address etc
	// TODO: long term: fire a timer and actually draw the vector to ram with correct timing
}

/* port 0x68: "KBDW" d7 is beeper, d6 is keyclick, d5-d0 are keyboard LEDS */
WRITE8_MEMBER(vk100_state::KBDW)
{
	output_set_value("online_led",BIT(data, 5) ? 1 : 0);
	output_set_value("local_led", BIT(data, 5) ? 0 : 1);
	output_set_value("noscroll_led",BIT(data, 4) ? 1 : 0);
	output_set_value("basic_led", BIT(data, 3) ? 1 : 0);
	output_set_value("hardcopy_led", BIT(data, 2) ? 1 : 0);
	output_set_value("l1_led", BIT(data, 1) ? 1 : 0);
	output_set_value("l2_led", BIT(data, 0) ? 1 : 0);
	if (BIT(data, 6)) logerror("kb keyclick bit 6 set: not emulated yet (multivibrator)!\n");
	beep_set_state( m_speaker, BIT(data, 7));
	logerror("LED state: %02X: %s %s %s %s %s %s\n", data&0xFF, (data&0x20)?"------- LOCAL ":"ON LINE ----- ", (data&0x10)?"--------- ":"NO SCROLL ", (data&0x8)?"----- ":"BASIC ", (data&0x4)?"--------- ":"HARD-COPY ", (data&0x2)?"-- ":"L1 ", (data&0x1)?"-- ":"L2 ");
}

READ8_MEMBER(vk100_state::vk100_keyboard_column_r)
{
	UINT8 code;
	char kbdcol[8];
	sprintf(kbdcol,"COL%X", (offset&0xF));
	code = ioport(kbdcol)->read() | ioport("CAPSSHIFT")->read();
	logerror("Keyboard column %X read, returning %02X\n", offset&0xF, code);
	return code;
}

static ADDRESS_MAP_START(vk100_mem, AS_PROGRAM, 8, vk100_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x6fff ) AM_ROM
	AM_RANGE( 0x7000, 0x700f) AM_MIRROR(0x0ff0) AM_READ(vk100_keyboard_column_r)
	AM_RANGE( 0x8000, 0xbfff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(vk100_io, AS_IO, 8, vk100_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_RAM // hack to pass crtc test until crtc is attached
	//AM_RANGE(0x00, 0x00) AM_WRITE(crtc_addr_w)
	//AM_RANGE(0x01, 0x01) AM_READWRITE(crtc_data_r, crtc_data_w)
	// Comments are from page 118 (5-14) of http://web.archive.org/web/20091015205827/http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf
	AM_RANGE (0x40, 0x41) AM_WRITE(vgLD_X)  //LD X LO + HI 12 bits
	AM_RANGE (0x42, 0x43) AM_WRITE(vgLD_Y)  //LD Y LO + HI 12 bits
	AM_RANGE (0x44, 0x44) AM_WRITE(vgErr)    //LD ERR ('error' in bresenham algorithm)
	//AM_RANGE (0x45, 0x45) AM_WRITE(sops)   //LD SOPS (screen options)
	AM_RANGE (0x46, 0x46) AM_WRITE(vgPat)    //LD PAT (pattern register)
	//AM_RANGE (0x47, 0x47) AM_READWRITE(pmul)   //LD PMUL (pattern multiplier) (should this be write only?)
	//AM_RANGE (0x60, 0x60) AM_WRITE(du)     //LD DU
	//AM_RANGE (0x61, 0x61) AM_WRITE(dvm)    //LD DVM
	//AM_RANGE (0x62, 0x62) AM_WRITE(dir)    //LD DIR (direction)
	AM_RANGE (0x63, 0x63) AM_WRITE(vgWOPS)   //LD WOPS (write options)
	//AM_RANGE (0x64, 0x64) AM_WRITE(mov)    //EX MOV
	//AM_RANGE (0x65, 0x65) AM_WRITE(dot)    //EX DOT
	AM_RANGE (0x66, 0x66) AM_WRITE(vgEX_VEC)    //EX VEC
	//AM_RANGE (0x67, 0x67) AM_WRITE(er)     //EX ER
	AM_RANGE (0x68, 0x68) AM_WRITE(KBDW)   //KBDW
	//AM_RANGE (0x6C, 0x6C) AM_WRITE(baud)   //LD BAUD (baud rate clock divider setting for i8251 tx and rx clocks)
	//AM_RANGE (0x70, 0x70) AM_WRITE(comd)   //LD COMD (one of the i8251 regs)
	//AM_RANGE (0x71, 0x71) AM_WRITE(com)    //LD COM (the other i8251 reg)
	//AM_RANGE (0x74, 0x74) AM_WRITE(unknown_74)
	//AM_RANGE (0x78, 0x78) AM_WRITE(kbdw)   //KBDW ?(mirror?)
	//AM_RANGE (0x7C, 0x7C) AM_WRITE(unknown_7C)
	//AM_RANGE (0x40, 0x40) AM_READ(systat_a) // SYSTAT A (dipswitches?)
	//AM_RANGE (0x48, 0x48) AM_READ(systat_b) // SYSTAT B (dipswitches?)
	//AM_RANGE (0x50, 0x50) AM_READ(uart_0)   // UART O , low 2 bits control the i8251 dest: 00 = rs232/eia, 01 = 20ma, 02 = hardcopy, 03 = test/loopback
	//AM_RANGE (0x51, 0x51) AM_READ(uart_1)   // UAR
	//AM_RANGE (0x58, 0x58) AM_READ(unknown_58)
	//AM_RANGE (0x60, 0x60) AM_READ(unknown_60)
	//AM_RANGE (0x68, 0x68) AM_READ(unknown_68) // NOT USED
	//AM_RANGE (0x70, 0x70) AM_READ(unknown_70)
	//AM_RANGE (0x78, 0x7f) AM_READ(unknown_78)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vk100 )
	PORT_START("CAPSSHIFT") // CAPS LOCK and SHIFT appear as the high 2 bits on all rows
		PORT_BIT(0x3f, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps lock") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_START("COL0")
		PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED) // row 0 bit 6 is always low, checked by keyboard test
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED) // all rows have these bits left low to save a mask op later
	PORT_START("COL1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Set Up") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num Enter") PORT_CODE(KEYCODE_ENTER_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("No scroll") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 1") PORT_CODE(KEYCODE_1_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 2") PORT_CODE(KEYCODE_2_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 3") PORT_CODE(KEYCODE_3_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 4") PORT_CODE(KEYCODE_4_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 5") PORT_CODE(KEYCODE_5_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 6") PORT_CODE(KEYCODE_6_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL8")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 7") PORT_CODE(KEYCODE_7_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COL9")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 8") PORT_CODE(KEYCODE_8_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLA")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 9") PORT_CODE(KEYCODE_9_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLB")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 0") PORT_CODE(KEYCODE_0_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLC")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num -") PORT_CODE(KEYCODE_MINUS_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLD")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num ,") PORT_CODE(KEYCODE_PLUS_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLE")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line feed") PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num .") PORT_CODE(KEYCODE_DEL_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("COLF")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x18, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0xc0, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_RESET( vk100 )
{
	vk100_state *state = machine.driver_data<vk100_state>();
	beep_set_frequency( state->m_speaker, 116 ); //116 hz (page 172 of TM)
	output_set_value("online_led",1);
	output_set_value("local_led", 0);
	output_set_value("noscroll_led",1);
	output_set_value("basic_led", 1);
	output_set_value("hardcopy_led", 1);
	output_set_value("l1_led", 1);
	output_set_value("l2_led", 1);
	state->m_vgX = 0;
	state->m_vgY = 0;
	state->m_vgErr = 0;
	state->m_vgPat = 0;
	state->m_vgWOPS = 0;
}

static PALETTE_INIT( vk100 )
{
	int i;
	for (i = 0; i < 8; i++)
		palette_set_color_rgb( machine, i, (i&4)?0xFF:0x00, (i&2)?0xFF:0x00, (i&1)?0xFF:0x00);
}

static VIDEO_START( vk100 )
{
}

static SCREEN_UPDATE_IND16( vk100 )
{
	return 0;
}

static INTERRUPT_GEN( vk100_vertical_interrupt )
{
	device_set_input_line(device, I8085_RST75_LINE, ASSERT_LINE);
	device_set_input_line(device, I8085_RST75_LINE, CLEAR_LINE);
}

static MACHINE_CONFIG_START( vk100, vk100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8085A, XTAL_5_0688MHz)
	MCFG_CPU_PROGRAM_MAP(vk100_mem)
	MCFG_CPU_IO_MAP(vk100_io)
	MCFG_CPU_VBLANK_INT("screen", vk100_vertical_interrupt)

	MCFG_MACHINE_RESET(vk100)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	//MCFG_SCREEN_RAW_PARAMS(XTAL_45_6192Mhz/3, 882, 0, 720, 370, 0, 350 )
	//MCFG_SCREEN_UPDATE_DEVICE( MDA_MC6845_NAME, mc6845_device, screen_update )
	//MCFG_MC6845_ADD( "crtc", H46505, XTAL_45_6192Mhz/3/16, mc6845_mda_intf)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_VIDEO_START(vk100)
	MCFG_SCREEN_UPDATE_STATIC(vk100)
	MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT(vk100)


	MCFG_DEFAULT_LAYOUT( layout_vk100 )

	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( BEEPER_TAG, BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 0.25 )
MACHINE_CONFIG_END

/* ROM definition */
/* according to http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf page 5-10 (pdf pg 114),
The 4 firmware roms should go from 0x0000-0x1fff, 0x2000-0x3fff, 0x4000-0x5fff and 0x6000-0x63ff; The last rom is actually a little bit longer and goes to 6fff.
*/
ROM_START( vk100 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-031e4-00.rom1.ic51", 0x0000, 0x2000, CRC(c8596398) SHA1(a8dc833dcdfb7550c030ac3d4143e266b1eab03a))
	ROM_LOAD( "23-017e4-00.rom2.ic52", 0x2000, 0x2000, CRC(e857a01e) SHA1(914b2c51c43d0d181ffb74e3ea59d74e70ab0813))
	ROM_LOAD( "23-018e4-00.rom3.ic53", 0x4000, 0x2000, CRC(b3e7903b) SHA1(8ad6ed25cd9b04a9968aa09ab69ba526d35ca550))
	ROM_LOAD( "23-190e2-00.rom4.ic54", 0x6000, 0x1000, CRC(ad596fa5) SHA1(b30a24155640d32c1b47a3a16ea33cd8df2624f6))

	ROM_REGION( 0x8000, "vram", ROMREGION_ERASEFF ) // 32k of vram

	ROM_REGION( 0x400, "pattern", ROMREGION_ERASEFF )
    /* This is the "PATTERN ROM", (1k*4, 82s137)
     * it contains a table of 4 output bits based on input of: (from figure 5-18):
     * 4 input bits bank-selected from RAM (a0, a1, a2, a3)
     * 2 select bits to choose one of the 4 bits, sourced from the X reg LSBs (a4, a5)
     * 3 pattern function bits from WOPS (F0 "N" is a6, F1 and F2 are a7 and a8
     * and one bit from the pattern register shifter a9
i.e. addr bits 9876543210
               ||||||\\\\- input from ram A
               ||||\\----- bit select (from x reg lsb)
               |||\------- negate N
               |\\-------- function
               \---------- pattern bit P
          functions are:
          Overlay: M=A|(P^N)
          Replace: M=P^N
          Complement: M=A^(P^N)
          Erase: M=N
     */
	ROM_LOAD( "wb8201_656f1.m1-7643-5.pr4.ic17", 0x0000, 0x0400, CRC(e8ecf59f) SHA1(49e9d109dad3d203d45471a3f4ca4985d556161f)) // label verified from nigwil's board
	
	ROM_REGION( 0x10000, "proms", ROMREGION_ERASEFF )
	// this is either the "SYNC ROM" or the "VECTOR ROM" which handles timing related stuff. (256*4, 82s129)
	ROM_LOAD( "wb8151_573a2.6301.pr3.ic44", 0x0000, 0x0100, CRC(75885a9f) SHA1(c721dad6a69c291dd86dad102ed3a8ddd620ecc4)) // label verified from nigwil's and andy's board
	// this is probably the "DIRECTION ROM", but might not be. (256*8, 82s135)
	ROM_LOAD( "wb8146_058b1.6309.pr1.ic99", 0x0100, 0x0100, CRC(71b01864) SHA1(e552f5b0bc3f443299282b1da7e9dbfec60e12bf))  // label verified from nigwil's and andy's board
	// this is definitely the "TRANSLATOR ROM" described in figure 5-17 on page 5-27 (256*8, 82s135)
	ROM_LOAD( "wb---0_060b1.6309.pr2.ic77", 0x0200, 0x0100, CRC(198317fc) SHA1(00e97104952b3fbe03a4f18d800d608b837d10ae)) // label verified from nigwil's board
	// the following == mb6309 (256x8, 82s135)
	ROM_LOAD( "wb8141_059b1.tbp18s22n.pr5.ic108", 0x0300, 0x0100, NO_DUMP)  // label verified from andy's board
	// the following = mb6331 (32x8, 82s123)
	ROM_LOAD( "wb8214_297a1.74s288.pr6.ic89", 0x0400, 0x0100, NO_DUMP) // label verified from nigwil's and andy's board
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                       FULLNAME       FLAGS */
COMP( 1980, vk100,  0,      0,       vk100,     vk100,   0,  "Digital Equipment Corporation", "VK 100", GAME_NOT_WORKING | GAME_NO_SOUND)
