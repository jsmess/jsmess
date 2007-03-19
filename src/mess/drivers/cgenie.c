/***************************************************************************
HAD to change the PORT_ANALOG defs in this file...	please check ;-)

Colour Genie memory map

CPU #1:
0000-3fff ROM basic & bios		  R   D0-D7

4000-bfff RAM
c000-dfff ROM dos				  R   D0-D7
e000-efff ROM extra 			  R   D0-D7
f000-f3ff color ram 			  W/R D0-D3
f400-f7ff font ram				  W/R D0-D7
f800-f8ff keyboard matrix		  R   D0-D7
ffe0-ffe3 floppy motor			  W   D0-D2
		  floppy head select	  W   D3
ffec-ffef FDC WD179x			  R/W D0-D7
		  ffec command			  W
		  ffec status			  R
		  ffed track			  R/W
		  ffee sector			  R/W
		  ffef data 			  R/W

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/cgenie.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "sound/ay8910.h"

static ADDRESS_MAP_START (cgenie_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
//	AM_RANGE(0x4000, 0xbfff) AM_RAM	// set up in MACHINE_START
//	AM_RANGE(0xc000, 0xdfff) AM_ROM	// installed in cgenie_init_machine
//	AM_RANGE(0xe000, 0xefff) AM_ROM	// installed in cgenie_init_machine
	AM_RANGE(0xf000, 0xf3ff) AM_READWRITE( cgenie_colorram_r, cgenie_colorram_w ) AM_BASE( &colorram )
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE( cgenie_fontram_r, cgenie_fontram_w) AM_BASE( &cgenie_fontram )
	AM_RANGE(0xf800, 0xf8ff) AM_READ( cgenie_keyboard_r )
	AM_RANGE(0xf900, 0xffdf) AM_NOP
	AM_RANGE(0xffe0, 0xffe3) AM_READWRITE( cgenie_irq_status_r, cgenie_motor_w )
	AM_RANGE(0xffe4, 0xffeb) AM_NOP
	AM_RANGE(0xffec, 0xffec) AM_READWRITE( cgenie_status_r, cgenie_command_w )
	AM_RANGE(0xffe4, 0xffeb) AM_NOP
	AM_RANGE(0xffec, 0xffec) AM_WRITE( cgenie_command_w )
	AM_RANGE(0xffed, 0xffed) AM_READWRITE( cgenie_track_r, cgenie_track_w )
	AM_RANGE(0xffee, 0xffee) AM_READWRITE( cgenie_sector_r, cgenie_sector_w )
	AM_RANGE(0xffef, 0xffef) AM_READWRITE( cgenie_data_r, cgenie_data_w )
	AM_RANGE(0xfff0, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START (cgenie_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0xf8, 0xf8) AM_READWRITE( cgenie_sh_control_port_r, cgenie_sh_control_port_w )
	AM_RANGE(0xf9, 0xf9) AM_READWRITE( cgenie_sh_data_port_r, cgenie_sh_data_port_w )
	AM_RANGE(0xfa, 0xfa) AM_READWRITE( cgenie_index_r, cgenie_index_w )
	AM_RANGE(0xfb, 0xfb) AM_READWRITE( cgenie_register_r, cgenie_register_w )
	AM_RANGE(0xff, 0xff) AM_READWRITE( cgenie_port_ff_r, cgenie_port_ff_w )
ADDRESS_MAP_END

INPUT_PORTS_START( cgenie )
	PORT_START /* IN0 */
	PORT_DIPNAME(	  0x80, 0x80, "Floppy Disc Drives")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPNAME(	  0x40, 0x40, "CG-DOS ROM C000-DFFF")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME(	  0x20, 0x00, "Extension  E000-EFFF")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_BIT(	  0x10, 0x10, IPT_DIPSWITCH_NAME) PORT_NAME("Video Display accuracy") PORT_CODE(KEYCODE_F5) PORT_TOGGLE
	PORT_DIPSETTING(	0x10, "TV set" )
	PORT_DIPSETTING(	0x00, "RGB monitor" )
	PORT_BIT(	  0x08, 0x08, IPT_DIPSWITCH_NAME) PORT_NAME("Virtual tape support") PORT_CODE(KEYCODE_F6) PORT_TOGGLE
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_BIT(0x07, 0x07, IPT_UNUSED)

/**************************************************************************
   +-------------------------------+	 +-------------------------------+
   | 0	 1	 2	 3	 4	 5	 6	 7 |	 | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ |F-1|F-2|F-3|F-4|  |3 | x | y | z | { |F-5|F-6|F-7|F-8|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
***************************************************************************/

	PORT_START /* IN1 KEY ROW 0 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0.0: @")
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("0.1: A  a") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0.2: B  b") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("0.3: C  c") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("0.4: D  d") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("0.5: E  e") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0.6: F  f") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("0.7: G  g") PORT_CODE(KEYCODE_G)

	PORT_START /* IN2 KEY ROW 1 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("1.0: H  h") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1.1: I  i") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("1.2: J  j") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("1.3: K  k") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("1.4: L  l") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("1.5: M  m") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("1.6: N  n") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1.7: O  o") PORT_CODE(KEYCODE_O)

	PORT_START /* IN3 KEY ROW 2 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2.0: P  p") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("2.1: Q  q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2.2: R  r") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("2.3: S  s") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("2.4: T  t") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("2.5: U  u") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("2.6: V  v") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("2.7: W  w") PORT_CODE(KEYCODE_W)

	PORT_START /* IN4 KEY ROW 3 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("3.0: X  x") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3.1: Y  y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("3.2: Z  z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3.3: [  {") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("3.4: F1 F5") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("3.5: F2 F6") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("3.6: F3 F7") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("3.7: F4 F8") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("3.4: \\  |") PORT_CODE(KEYCODE_ASTERISK)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("3.5: ]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("3.6: ^  ~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("3.7: _") PORT_CODE(KEYCODE_EQUALS)

	PORT_START /* IN5 KEY ROW 4 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("4.0: 0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("4.1: 1  !") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4.2: 2  \"") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("4.3: 3  #") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4.4: 4  $") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("4.5: 5  %") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("4.6: 6  &") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("4.7: 7  '") PORT_CODE(KEYCODE_7)

		PORT_START /* IN6 KEY ROW 5 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("5.0: 8  (") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("5.1: 9  )") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("5.2: :  +") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5.3: ;  *") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("5.4: ,  <") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5.5: -  =") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("5.6: .  >") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("5.7: /  ?") PORT_CODE(KEYCODE_SLASH)

	PORT_START /* IN7 KEY ROW 6 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("6.0: ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("6.1: CLEAR") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("6.2: BREAK") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("6.3: UP") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6.4: DOWN") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: LEFT") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6.6: RIGHT") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("6.7: SPACE") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: (BSP)") PORT_CODE(KEYCODE_BACKSPACE)

	PORT_START /* IN8 KEY ROW 7 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("7.0: SHIFT") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("7.1: MODSEL") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("7.2: (PGUP)") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("7.3: (PGDN)") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("7.4: (INS)") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7.5: (DEL)") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("7.6: (CTRL)") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7.7: (ALTGR)") PORT_CODE(KEYCODE_RALT)

	PORT_START /* IN9 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_X) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(1)

	PORT_START /* IN10 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_Y) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(1) PORT_REVERSE

	PORT_START /* IN11 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_X) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(2)

	PORT_START /* IN12 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_Y) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(2) PORT_REVERSE

	/* Joystick Keypad */
	/* keypads were organized a 3 x 4 matrix and it looked	   */
	/* exactly like a our northern telephone numerical pads    */
	/* The addressing was done with a single clear bit 0..6    */
	/* on i/o port A,  while all other bits were set.		   */
	/* (e.g. 0xFE addresses keypad1 row 0, 0xEF keypad2 row 1) */

	/*		 bit  0   1   2   3   */
	/* FE/F7  0  [3] [6] [9] [#]  */
	/* FD/EF  1  [2] [5] [8] [0]  */
	/* FB/DF  2  [1] [4] [7] [*]  */

	PORT_START /* IN13 */
	PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [3]") PORT_CODE(KEYCODE_3_PAD) PORT_CODE(JOYCODE_1_BUTTON3)
	PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [6]") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_1_BUTTON6)
	PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [9]") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x08, 0x00, IPT_BUTTON2) PORT_NAME("Joy 1 [#]") PORT_CODE(KEYCODE_SLASH_PAD) PORT_CODE(JOYCODE_1_BUTTON1)
	PORT_BIT(0x10, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [2]") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_1_BUTTON2)
	PORT_BIT(0x20, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [5]") PORT_CODE(KEYCODE_5_PAD) PORT_CODE(JOYCODE_1_BUTTON5)
	PORT_BIT(0x40, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [8]") PORT_CODE(KEYCODE_8_PAD) 
	PORT_BIT(0x80, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [0]") PORT_CODE(KEYCODE_0_PAD) 
	PORT_START /* IN14 */
	PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [1]") PORT_CODE(KEYCODE_1_PAD) PORT_CODE(JOYCODE_1_BUTTON3)
	PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [4]") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_1_BUTTON6)
	PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [7]") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x08, 0x00, IPT_BUTTON1) PORT_NAME("Joy 1 [*]") PORT_CODE(KEYCODE_ASTERISK) PORT_CODE(JOYCODE_1_BUTTON1)
	PORT_BIT(0x10, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [3]") PORT_CODE(JOYCODE_2_BUTTON2)
	PORT_BIT(0x20, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [6]") PORT_CODE(JOYCODE_2_BUTTON5)
	PORT_BIT(0x40, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [9]") 
	PORT_BIT(0x80, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [#]") PORT_CODE(JOYCODE_2_BUTTON1)

	PORT_START /* IN15 */
	PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [2]") PORT_CODE(JOYCODE_2_BUTTON2)
	PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [5]") PORT_CODE(JOYCODE_2_BUTTON5)
	PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [8]")
	PORT_BIT(0x08, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [0]")
	PORT_BIT(0x10, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [1]") PORT_CODE(JOYCODE_2_BUTTON1)
	PORT_BIT(0x20, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [4]") PORT_CODE(JOYCODE_2_BUTTON4)
	PORT_BIT(0x40, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [7]")
	PORT_BIT(0x80, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [*]") PORT_CODE(JOYCODE_2_BUTTON1)
INPUT_PORTS_END

gfx_layout cgenie_charlayout =
{
	8,8,		   /* 8*8 characters */
	384,		   /* 256 fixed + 128 defineable characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },   /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 		   /* every char takes 8 bytes */
};

static gfx_layout cgenie_gfxlayout =
{
	8,8,			/* 4*8 characters */
	256,			/* 256 graphics patterns */
	2,				/* 2 bits per pixel */
	{ 0, 1 },		/* two bitplanes; 2 bit per pixel */
	{ 0, 0, 2, 2, 4, 4, 6, 6}, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 			/* every char takes 8 bytes */
};

static gfx_decode cgenie_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &cgenie_charlayout,	  0, 3*16},
	{ REGION_GFX2, 0, &cgenie_gfxlayout, 3*16*2, 3*4},
	{ -1 } /* end of array */
};

static unsigned char cgenie_palette[] = {
	 0*4,  0*4,  0*4,  /* background   */

/* this is the 'RGB monitor' version, strong and clean */
	15*4, 15*4, 15*4,  /* gray		   */
	 0*4, 48*4, 48*4,  /* cyan		   */
	60*4,  0*4,  0*4,  /* red		   */
	47*4, 47*4, 47*4,  /* white 	   */
	55*4, 55*4,  0*4,  /* yellow	   */
	 0*4, 56*4,  0*4,  /* green 	   */
	42*4, 32*4,  0*4,  /* orange	   */
	63*4, 63*4,  0*4,  /* light yellow */
	 0*4,  0*4, 48*4,  /* blue		   */
	 0*4, 24*4, 63*4,  /* light blue   */
	60*4,  0*4, 38*4,  /* pink		   */
	38*4,  0*4, 60*4,  /* purple	   */
	31*4, 31*4, 31*4,  /* light gray   */
	 0*4, 63*4, 63*4,  /* light cyan   */
	58*4,  0*4, 58*4,  /* magenta	   */
	63*4, 63*4, 63*4,  /* bright white */

/* this is the 'TV screen' version, weak and blurred by repeating pixels */
	15*2+80, 15*2+80, 15*2+80,	/* gray 		*/
	 0*2+80, 48*2+80, 48*2+80,	/* cyan 		*/
	60*2+80,  0*2+80,  0*2+80,	/* red			*/
	47*2+80, 47*2+80, 47*2+80,	/* white		*/
	55*2+80, 55*2+80,  0*2+80,	/* yellow		*/
	 0*2+80, 56*2+80,  0*2+80,	/* green		*/
	42*2+80, 32*2+80,  0*2+80,	/* orange		*/
	63*2+80, 63*2+80,  0*2+80,	/* light yellow */
	 0*2+80,  0*2+80, 48*2+80,	/* blue 		*/
	 0*2+80, 24*2+80, 63*2+80,	/* light blue	*/
	60*2+80,  0*2+80, 38*2+80,	/* pink 		*/
	38*2+80,  0*2+80, 60*2+80,	/* purple		*/
	31*2+80, 31*2+80, 31*2+80,	/* light gray	*/
	 0*2+80, 63*2+80, 63*2+80,	/* light cyan	*/
	58*2+80,  0*2+80, 58*2+80,	/* magenta		*/
	63*2+80, 63*2+80, 63*2+80,	/* bright white */

	15*2+96, 15*2+96, 15*2+96,	/* gray 		*/
	 0*2+96, 48*2+96, 48*2+96,	/* cyan 		*/
	60*2+96,  0*2+96,  0*2+96,	/* red			*/
	47*2+96, 47*2+96, 47*2+96,	/* white		*/
	55*2+96, 55*2+96,  0*2+96,	/* yellow		*/
	 0*2+96, 56*2+96,  0*2+96,	/* green		*/
	42*2+96, 32*2+96,  0*2+96,	/* orange		*/
	63*2+96, 63*2+96,  0*2+96,	/* light yellow */
	 0*2+96,  0*2+96, 48*2+96,	/* blue 		*/
	 0*2+96, 24*2+96, 63*2+96,	/* light blue	*/
	60*2+96,  0*2+96, 38*2+96,	/* pink 		*/
	38*2+96,  0*2+96, 60*2+96,	/* purple		*/
	31*2+96, 31*2+96, 31*2+96,	/* light gray	*/
	 0*2+96, 63*2+96, 63*2+96,	/* light cyan	*/
	58*2+96,  0*2+96, 58*2+96,	/* magenta		*/
	63*2+96, 63*2+96, 63*2+96,	/* bright white */


};

static unsigned short cgenie_colortable[] =
{
	0, 1, 0, 2, 0, 3, 0, 4, /* RGB monitor set of text colors */
	0, 5, 0, 6, 0, 7, 0, 8,
	0, 9, 0,10, 0,11, 0,12,
	0,13, 0,14, 0,15, 0,16,

	0,17, 0,18, 0,19, 0,20, /* TV set text colors: darker */
	0,21, 0,22, 0,23, 0,24,
	0,25, 0,26, 0,27, 0,28,
	0,29, 0,30, 0,31, 0,32,

	0,33, 0,34, 0,35, 0,36, /* TV set text colors: a bit brighter */
	0,37, 0,38, 0,39, 0,40,
	0,41, 0,42, 0,43, 0,44,
	0,45, 0,46, 0,47, 0,48,

	0,	  9,	7,	  6,	/* RGB monitor graphics colors */
	0,	  25,	23,   22,	/* TV set graphics colors: darker */
	0,	  41,	39,   38,	/* TV set graphics colors: a bit brighter */
};

/* Initialise the palette */
static PALETTE_INIT( cgenie )
{
	palette_set_colors(machine, 0, cgenie_palette, sizeof(cgenie_palette) / 3);
	memcpy(colortable, cgenie_colortable, sizeof(cgenie_colortable));
}


static struct AY8910interface ay8910_interface =
{
	cgenie_psg_port_a_r,
	cgenie_psg_port_b_r,
	cgenie_psg_port_a_w,
	cgenie_psg_port_b_w
};



static MACHINE_DRIVER_START( cgenie )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2216800)        /* 2,2168 Mhz */
	MDRV_CPU_PROGRAM_MAP(cgenie_mem, 0)
	MDRV_CPU_IO_MAP(cgenie_io, 0)
	MDRV_CPU_VBLANK_INT(cgenie_frame_interrupt,1)
	MDRV_CPU_PERIODIC_INT(cgenie_timer_interrupt, TIME_IN_HZ(40))
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_START( cgenie )
	
    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(48*8, (32)*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 48*8-1,0*8,32*8-1)
	MDRV_GFXDECODE( cgenie_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(cgenie_palette) / sizeof(cgenie_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(cgenie_colortable) / sizeof(cgenie_colortable[0]))
	MDRV_PALETTE_INIT( cgenie )

	MDRV_VIDEO_START( cgenie )
	MDRV_VIDEO_UPDATE( cgenie )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)	
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (cgenie)
	ROM_REGION(0x13000,REGION_CPU1,0)
	ROM_LOAD ("cgenie.rom",  0x00000, 0x4000, CRC(d359ead7) SHA1(d8c2fc389ad38c45fba0ed556a7d91abac5463f4))
	ROM_LOAD ("cgdos.rom",   0x10000, 0x2000, CRC(2a96cf74) SHA1(6dcac110f87897e1ee7521aefbb3d77a14815893))
	ROM_CART_LOAD(0, "rom\0", 0x12000, 0x1000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION(0x0c00,REGION_GFX1,0)
	ROM_LOAD ("cgenie1.fnt", 0x0000, 0x0800, CRC(4fed774a) SHA1(d53df8212b521892cc56be690db0bb474627d2ff))

	/* Empty memory region for the character generator */
	ROM_REGION(0x0800,REGION_GFX2,0)

ROM_END

static void cgenie_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_cgenie_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void cgenie_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_CREATABLE:						info->i = 0;	/* INVALID */ break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_cgenie_cassette; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cas"); break;
	}
}

SYSTEM_CONFIG_START(cgenie)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(cgenie_floppy_getinfo)
	CONFIG_DEVICE(cgenie_cassette_getinfo)
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT	  CONFIG     COMPANY	FULLNAME */
COMP( 1982, cgenie,   0,		0,		cgenie,   cgenie,	0,        cgenie,    "EACA Computers Ltd.",  "Colour Genie EG2000" , 0)
