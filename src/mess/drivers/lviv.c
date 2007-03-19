/*******************************************************************************

PK-01 Lviv driver by Krzysztof Strzecha

Big thanks go to:
Anton V. Ignatichev for informations about Lviv hardware.
Dr. Volodimir Mosorov for two Lviv machines.

What's new:
-----------
28.02.2003      Snapshot veryfing function added.
07.01.2003	Support for .SAV snapshots. Joystick support (there are strange
		problems with "Doroga (1991)(-)(Ru).lvt".
21.12.2002	Cassette support rewritten, WAVs saving and loading are working now.
08.12.2002	Comments on emulation status updated. Changed 'lvive' to 'lvivp'.
		ADC r instruction in 8080 core fixed (Arkanoid works now).
		Orginal keyboard layout added.
20.07.2002	"Reset" key fixed. 8080 core fixed (all BASIC commands works).
		now). Unsupported .lvt files versions aren't now loaded.
xx.07.2002	Improved port and memory mapping (Raphael Nabet).
		Hardware description updated (Raphael Nabet).
27.03.2002	CPU clock changed to 2.5MHz.
		New Lviv driver added for different ROM revision.
24.03.2002	Palette emulation.
		Bit 7 of port 0xc1 emulated - speaker enabled/disabled.
		Some notes about hardware added.
		"Reset" key added.
23.03.2002	Hardware description and notes on emulation status added.
		Few changes in keyboard mapping.

Notes on emulation status and to do list:
-----------------------------------------
1. LIMITATION: Printer is not emulated. 
2. LIMITATION: Timings are not implemented, due to it emulated machine runs
   twice fast as orginal.
3. LIMITATION: .RSS files are not supported.
4. LIMITATION: Some usage notes and trivia are needed in sysinfo.dat.

Lviv technical information
==========================

CPU:
----
	8080 2.5MHz (2MHz in first machines)

Memory map:
-----------
	start-up map (cleared by the first I/O write operation done by the CPU):
	0000-3fff ROM mirror #1
	4000-7fff ROM mirror #2
	8000-bfff ROM mirror #3
	c000-ffff ROM

	normal map with video RAM off:
	0000-3fff RAM
	4000-7fff RAM
	8000-bfff RAM
	c000-ffff ROM

	normal map with video RAM on:
	0000-3fff mirrors 8000-bfff
	4000-7fff video RAM
	8000-bfff RAM
	c000-ffff ROM

Interrupts:
-----------
	No interrupts in Lviv.

Ports:
------
	Only A4-A5 are decoded.  A2-A3 is ignored in the console, but could be used by extension
	devices.

	C0-C3	8255 PPI
		Port A: extension slot output, printer data
			bits 0-4 joystick scanner output
		Port B: palette control, extension slot input or output
			sound on/off
			bit 7 sound on/off
			bits 0-6 palette select
		Port C: memory page changing, tape input and output,
			printer control, sound
			bits 0-3 extension slot input
			bits 4-7 extension slot output
			bit 7: joystick scanner input
			bit 6: printer control AC/busy
			bit 5: not used
			bit 4: tape in
			bit 3: not used
			bit 2: printer control SC/strobe
			bit 1: memory paging, 0 - video ram, 1 - ram
			bit 0: tape out, sound

	D0-D3	8255 PPI
		Port A:
			keyboard scaning
		Port B:
			keyboard reading
		Port C:
			keyboard scaning/reading

Keyboard:
---------
	Reset - connected to CPU reset line

				     Port D0 				
	--------T-------T-------T-------T-------T-------T-------T-------ª        
	|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
	+-------+-------+-------+-------+-------+-------+-------+-------+---ª
	| Shift |   ;   |       |  CLS  | Space |   R   |   G   |   6   | 0 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   Q   |Russian|       |  (G)  |   B   |   O   |   [   |   7   | 1 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   ^   |  Key  |   J   |  (B)  |   @   |   L   |   ]   |   8   | 2 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   X   |   P   |   N   |   5   |  Alt  |  Del  | Enter | Ready | 3 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+ Port D1
	|   T   |   A   |   E   |   4   |   _   |   .   |  Run  |  Tab  | 4 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   I   |   W   |   K   |   3   | Latin |   \   |   :   |   -   | 5 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   M   |   Y   |   U   |   2   |   /   |   V   |   H   |   0   | 6 |
	+-------+-------+-------+-------+-------+-------+-------+-------+---+
	|   S   |   F   |   C   |   1   |   ,   |   D   |   Z   |   9   | 7 |
	L-------+-------+-------+-------+-------+-------+-------+-------+----

		     Port D2
	--------T-------T-------T-------ª
	|   3   |   2   |   1   |   0   |
	+-------+-------+-------+-------+-----ª
	| Right | Home  |ScrPrn |PrnLock|  4  |
	+-------+-------+-------+-------+-----+
	|  Up   |  F5   |  F0   |ScrLock|  5  |
	+-------+-------+-------+-------+-----+ Port D2
	| Left  |  F4   |  F1   | Sound |  6  |
	+-------+-------+-------+-------+-----+
	| Down  |  F3   |  F2   |  (R)  |  7  |
	L-------+-------+-------+-------+------

	Notes:
		CLS	- clear screen
		(G)	- clear screen with border and set COLOR 0,0,0
		(B)	- clear screen with border and set COLOR 1,0,6
		(R)	- clear screen with border and set COLOR 0,7,3
		Sound	- sound on/off
		ScrLock	- screen lock
		PrnLock	- printer on/off
		ScrPrn	- screen and printer output mode
		Russian	- russian keyboard mode
		Latin	- latin keyboard mode
		Right	- cursor key
		Up	- cursor key
		Left	- cursor key
		Down	- cursor key
		Keyword	- BASIC keyword


Video:
-----
	Screen resolution is 256x256 pixels. 4 colors at once are possible,
	but there is a posiibility of palette change. Bits 0..6 of port 0xc1
	are used for palette setting.

	One byte of video-RAM sets 4 pixels. Colors of pixels are corrected
	by current palette. Each bits combination (2 bits sets one pixel on
	the display), corrected with palette register, sets REAL pixel color.

	PBx - bit of port 0xC1 numbered x
	R,G,B - output color components
	== - "is equal"
	! - inversion

	00   R = PB3 == PB4; G = PB5; B = PB2 == PB6;
	01   R = PB4; G = !PB5; B = PB6;
	10   R = PB0 == PB4; G = PB5; B = !PB6;
	11   R = !PB4; G = PB1 == PB5; B = PB6;

	Bit combinations are result of concatenation of approprate bits of
	high and low byte halfs.

	Example:
	~~~~~~~~

	Some byte of video RAM:  1101 0001
	Value of port 0xC1:      x000 1110

	1101
	0001
	----
	10 10 00 11

	1st pixel (10): R = 1; G = 0; B = 1;
	2nd pixel (10): R = 1; G = 0; B = 1;
	3rd pixel (00): R = 0; G = 0; B = 0;
	4th pixel (11): R = 1; G = 0; B = 0;


Sound:
------
	Buzzer connected to port 0xc2 (bit 0).
	Bit 7 of port 0xc1 - enable/disable speaker.


Timings:
--------

	The CPU timing is controlled by a KR580GF24 (Sovietic copy of i8224) connected to a 18MHz(?)
	oscillator. CPU frequency must be 18MHz/9 = 2MHz.

	Memory timing uses a 8-phase clock, derived from a 20MHz(?) video clock (called VCLK0 here:
	in the schematics, it comes from pin 6 of V8, and it is labelled "0'" in the video clock bus).
	This clock is divided by G7, G6 and D5 to generate the signals we call VCLK1-VCLK11.  The memory
	clock phases Phi0-Phi7 are generated in D7, whereas PHI'14 and PHI'15 are generated in D8.

	When the CPU accesses RAM, wait states are inserted until the RAM transfer is complete.

	CPU clock: 18MHz/9 = 2MHz
	memory cycle time: 20MHz/8 = 2.5MHz
	CPU memory access time: (min) approx. 9/20MHz = 450ns
	                        (max) approx. 25/20MHz = 1250ns
	pixel clock: 20MHz/4 = 5MHz
	screen size: 256*256
	HBL: 64 pixel clock cycles
	VBL: 64 lines
	horizontal frequency: 5MHZ/(256+64) = 15.625kHz
	vertical frequency: 15.625kHz/(256+64) = 48.83Hz

			 |<--------VIDEO WINDOW--------->|<----------CPU WINDOW--------->|<--
			_   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
	VCLK0	 |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |
			_     ___     ___     ___     ___     ___     ___     ___     ___     ___
	VCLK1	 |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |
			_         _______         _______         _______         _______
	VCLK2	 |_______|       |_______|       |_______|       |_______|       |______|
			_                 _______________                 _______________
	VCLK3	 |_______________|               |_______________|               |_______
			_                                 _______________________________
	VCLK4	 |_______________________________|                               |_______

			  _                               _                               _
	PHI0	_| |_____________________________| |_____________________________| |_____
			      _                               _                               _
	PHI1	_____| |_____________________________| |_____________________________| |_
			          _                               _
	PHI2	_________| |_____________________________| |_____________________________
			              _                               _
	PHI3	_____________| |_____________________________| |_________________________
			                  _                               _
	PHI4	_________________| |_____________________________| |_____________________
			                      _                               _
	PHI5	_____________________| |_____________________________| |_________________
			                          _                               _
	PHI6	_________________________| |_____________________________| |_____________
			                              _                               _
	PHI7	_____________________________| |_____________________________| |_________
			                                                          _
	PHI'14	_________________________________________________________| |_____________
			                                                              _
	PHI'15	_____________________________________________________________| |_________
			__________             __________________________________________________
	RAS*	          \___________/                   \_a_________/
			______________                 __________________________________________
	CAS*	              \_______________/               \_a_____________/
			_________________________________________________________________________
	WR*		                                                  \_b_________////////
			_________________________________________________________________________
	WRM*	\\\\\\\\\\\\\\\\\\\\\\\\\\_b__________________________________///////////
                        _________________________________________________________________________
        RDM*    \\\\\\\\\\\\\\\\\\\\\\\\\\_c __________________________________///////////
                        _________________________________________________________________________
	RA		\\\\\\\\\\\\\\\\\\\\\\\\\\_a__________________________________/

	DRAM
	ADDRESS	video row /\ video column /XXX\CPU row (a)/\  CPU column (a)  /\ video row

	a: only if the CPU is requesting a RAM read/write
	b: only if the CPU is requesting a RAM write
	c: only if the CPU is requesting a RAM read

*******************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "video/generic.h"
#include "includes/lviv.h"
#include "devices/snapquik.h"
#include "devices/cassette.h"
#include "formats/lviv_lvt.h"

/* I/O ports */

ADDRESS_MAP_START( lviv_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_READ( lviv_io_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( lviv_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0xff) AM_WRITE( lviv_io_w )
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( lviv_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
ADDRESS_MAP_END


/* keyboard input */
INPUT_PORTS_START (lviv)
	PORT_START /* 2nd PPI port A bit 0 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ready") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_START /* 2nd PPI port A bit 1 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_U) PORT_CHAR('g')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_I) PORT_CHAR('[')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_O) PORT_CHAR(']')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Run") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("*  :") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('*') PORT_CHAR(':')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('h')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_P) PORT_CHAR('z')
	PORT_START /* 2nd PPI port A bit 2 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_H) PORT_CHAR('r')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_J) PORT_CHAR('o')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_K) PORT_CHAR('l')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >") PORT_CODE(KEYCODE_END) PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_COLON) PORT_CHAR('v')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_L) PORT_CHAR('d')
	PORT_START /* 2nd PPI port A bit 3 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_COMMA) PORT_CHAR('b')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_STOP) PORT_CHAR('@')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Alt") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_MAMEKEY(RSHIFT))
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('_')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Latin") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/  ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <") PORT_CODE(KEYCODE_HOME) PORT_CHAR(',') PORT_CHAR('<')
	PORT_START /* 2nd PPI port A bit 4 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cls") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(G)") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(B)") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_START /* 2nd PPI port A bit 5 low */
		PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_Q) PORT_CHAR('j')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_Y) PORT_CHAR('n')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_T) PORT_CHAR('e')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_R) PORT_CHAR('k')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_E) PORT_CHAR('u')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_W) PORT_CHAR('c')
	PORT_START /* 2nd PPI port A bit 6 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(';') PORT_CHAR('+')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Russian") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Key") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_G) PORT_CHAR('p')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_F) PORT_CHAR('a')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_D) PORT_CHAR('w')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_S) PORT_CHAR('y')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_A) PORT_CHAR('f')
	PORT_START /* 2nd PPI port A bit 7 low */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Z) PORT_CHAR('q')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_X) PORT_CHAR('^')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_M) PORT_CHAR('x')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_N) PORT_CHAR('t')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_B) PORT_CHAR('i')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_V) PORT_CHAR('m')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_C) PORT_CHAR('s')
	PORT_START /* 2nd PPI port C bit 0 low */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PrnLck") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ScrLck") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Sound") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(R)") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_START /* 2nd PPI port C bit 1 low */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ScrPrn") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_START /* 2nd PPI port C bit 2 low */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(PGUP))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_SCRLOCK) PORT_CHAR(UCHAR_MAMEKEY(SCRLOCK))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_MAMEKEY(F12))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_START /* 2nd PPI port C bit 3 low */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_START /* CPU */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Reset") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_START /* Joystick */
		PORT_BIT(0x01,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_UP)
		PORT_BIT(0x02,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_DOWN)
		PORT_BIT(0x04,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_RIGHT)
		PORT_BIT(0x08,	IP_ACTIVE_HIGH,	IPT_JOYSTICK_LEFT)
		PORT_BIT(0x10,	IP_ACTIVE_HIGH,	IPT_BUTTON1)
		PORT_BIT(0x20,	IP_ACTIVE_HIGH,	IPT_UNUSED)
		PORT_BIT(0x40,	IP_ACTIVE_HIGH,	IPT_UNUSED)
		PORT_BIT(0x80,	IP_ACTIVE_HIGH,	IPT_UNUSED)
INPUT_PORTS_END



/* machine definition */
static MACHINE_DRIVER_START( lviv )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2500000)
	MDRV_CPU_PROGRAM_MAP(lviv_mem, 0)
	MDRV_CPU_IO_MAP(lviv_readport, lviv_writeport)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( lviv )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(sizeof (lviv_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (lviv_colortable))
	MDRV_PALETTE_INIT( lviv )

	MDRV_VIDEO_START( lviv )
	MDRV_VIDEO_UPDATE( lviv )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static void lviv_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) lviv_lvt_format; break;

		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_CASSETTE_DEFAULT_STATE:		info->i = CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void lviv_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sav"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_lviv; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_BIOS_START( lviv )
SYSTEM_BIOS_ADD( 0, "lviv", "Lviv/L'vov" )
SYSTEM_BIOS_ADD( 1, "lviva", "Lviv/L'vov (alternate)" )
SYSTEM_BIOS_ADD( 2, "lvivp", "Lviv/L'vov (prototype)" )
SYSTEM_BIOS_END

ROM_START(lviv)
ROM_REGION(0x14000,REGION_CPU1,0)
ROMX_LOAD("lviv.bin", 0x10000, 0x4000, CRC(44a347d9) SHA1(74e067493b2b7d9ab17333202009a1a4f5e460fd), ROM_BIOS(1))
ROMX_LOAD("lviva.bin", 0x10000, 0x4000, CRC(551622f5) SHA1(b225f3542b029d767b7db9dce562e8a3f77f92a2), ROM_BIOS(2))
ROMX_LOAD("lvivp.bin", 0x10000, 0x4000, CRC(f171c282) SHA1(c7dc2bdb02400e6b5cdcc50040eb06f506a7ed84), ROM_BIOS(3))
ROM_END

SYSTEM_CONFIG_START(lviv)
	CONFIG_RAM_DEFAULT(64 * 1024)
	/* 9-Oct-2003 - Changed to lvt because lv? is an invalid file extension */
	CONFIG_DEVICE(lviv_cassette_getinfo)
	CONFIG_DEVICE(lviv_snapshot_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT    BIOS    COMPAT  MACHINE   INPUT     INIT                CONFIG          COMPANY          FULLNAME */
COMPB( 1989,	lviv,	0,      lviv,      0,		lviv,      lviv,     0,       lviv,    "V. I. Lenin",	"Lviv" , 0)
