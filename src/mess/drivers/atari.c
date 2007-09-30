/******************************************************************************
    Atari 400/800

    MESS Driver

	Juergen Buchmueller, June 1998
******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/atari.h"
#include "devices/cartslot.h"
#include "sound/pokey.h"
#include "machine/6821pia.h"
#include "video/gtia.h"
#include "inputx.h"
#include "mslegacy.h"

/******************************************************************************
    Atari 800 memory map (preliminary)

    ***************** read access *******************
    range     short   description
	0000-9FFF RAM	  main memory
	A000-BFFF RAM/ROM RAM or (banked) ROM cartridges
    C000-CFFF ROM     unused or monitor ROM

    ********* GTIA    ********************************
    
	D000      m0pf    missile 0 playfield collisions
    D001      m1pf    missile 1 playfield collisions
    D002      m2pf    missile 2 playfield collisions
    D003      m3pf    missile 3 playfield collisions
    D004      p0pf    player 0 playfield collisions
    D005      p1pf    player 1 playfield collisions
    D006      p2pf    player 2 playfield collisions
    D007      p3pf    player 3 playfield collisions
    D008      m0pl    missile 0 player collisions
    D009      m1pl    missile 1 player collisions
    D00A      m2pl    missile 2 player collisions
    D00B      m3pl    missile 3 player collisions
    D00C      p0pl    player 0 player collisions
    D00D      p1pl    player 1 player collisions
    D00E      p2pl    player 2 player collisions
    D00F      p3pl    player 3 player collisions
    D010      but0    button stick 0
    D011      but1    button stick 1
    D012      but2    button stick 2
    D013      but3    button stick 3
	D014	  xff	  unused
	D015	  xff	  unused
	D016	  xff	  unused
	D017	  xff	  unused
	D018	  xff	  unused
	D019	  xff	  unused
	D01A	  xff	  unused
	D01B	  xff	  unused
	D01C	  xff	  unused
	D01D	  xff	  unused
	D01E	  xff	  unused
    D01F      cons    console keys
    D020-D0FF repeated 7 times

    D100-D1FF xff

	********* POKEY   ********************************
    D200      pot0    paddle 0
    D201      pot1    paddle 1
    D202      pot2    paddle 2
    D203      pot3    paddle 3
    D204      pot4    paddle 4
    D205      pot5    paddle 5
    D206      pot6    paddle 6
    D207      pot7    paddle 7
    D208      potb    all paddles
    D209      kbcode  keyboard scan code
    D20A      random  random number generator
	D20B	  xff	  unused
	D20C	  xff	  unused
    D20D      serin   serial input
    D20E      irqst   IRQ status
    D20F      skstat  sk status
    D210-D2FF repeated 15 times

	********* PIO	  ********************************
    D300      porta   read pio port A
    D301      portb   read pio port B
    D302      pactl   read pio port A control
    D303      pbctl   read pio port B control
    D304-D3FF repeated 63 times

	********* ANTIC   ********************************
	D400	  xff	  unused
	D401	  xff	  unused
	D402	  xff	  unused
	D403	  xff	  unused
	D404	  xff	  unused
	D405	  xff	  unused
	D406	  xff	  unused
	D407	  xff	  unused
	D408	  xff	  unused
	D409	  xff	  unused
	D40A	  xff	  unused
    D40B      vcount  vertical (scanline) counter
    D40C      penh    light pen horizontal pos
    D40D      penv    light pen vertical pos
	D40E	  xff	  unused
    D40F      nmist   NMI status

    D500-D7FF xff     unused memory

    D800-DFFF ROM     floating point ROM
    E000-FFFF ROM     bios ROM

    ***************** write access *******************
    range     short   description
	0000-9FFF RAM	  main memory
	A000-BFFF RAM/ROM RAM or (banked) ROM
	C000-CFFF ROM	  unused or monitor ROM

    ********* GTIA    ********************************
    D000      hposp0  player 0 horz position
    D001      hposp1  player 1 horz position
    D002      hposp2  player 2 horz position
    D003      hposp3  player 3 horz position
    D004      hposm0  missile 0 horz position
    D005      hposm1  missile 0 horz position
    D006      hposm2  missile 0 horz position
    D007      hposm3  missile 0 horz position
    D008      sizep0  size player 0
    D009      sizep1  size player 0
    D00A      sizep2  size player 0
    D00B      sizep3  size player 0
    D00C      sizem   size missiles
    D00D      grafp0  graphics data for player 0
    D00E      grafp1  graphics data for player 1
    D00F      grafp2  graphics data for player 2
    D010      grafp3  graphics data for player 3
    D011      grafm   graphics data for missiles
    D012      colpm0  color for player/missile 0
    D013      colpm1  color for player/missile 1
    D014      colpm2  color for player/missile 2
    D015      colpm3  color for player/missile 3
    D016      colpf0  color 0 playfield
    D017      colpf1  color 1 playfield
    D018      colpf2  color 2 playfield
    D019      colpf3  color 3 playfield
    D01A      colbk   background playfield
    D01B      prior   priority select
    D01C      vdelay  delay until vertical retrace
    D01D      gractl  graphics control
    D01E      hitclr  clear collisions
    D01F      wcons   write console (speaker)
    D020-D0FF repeated 7 times

	D100-D1FF xff	  unused

	********* POKEY   ********************************
    D200      audf1   frequency audio chan #1
	D201	  audc1   control audio chan #1
	D202	  audf2   frequency audio chan #2
	D203	  audc2   control audio chan #2
	D204	  audf3   frequency audio chan #3
	D205	  audc3   control audio chan #3
	D206	  audf4   frequency audio chan #4
	D207	  audc4   control audio chan #4
    D208      audctl  audio control
    D209      stimer  start timer
    D20A      skres   sk reset
    D20B      potgo   start pot AD conversion
	D20C	  xff	  unused
    D20D      serout  serial output
    D20E      irqen   IRQ enable
    D20F      skctl   sk control
    D210-D2FF repeated 15 times

	********* PIO	  ********************************
    D300      porta   write pio port A (output or mask)
	D301	  portb   write pio port B (output or mask)
    D302      pactl   write pio port A control
    D303      pbctl   write pio port B control
	D304-D3FF		  repeated

	********* ANTIC   ********************************
    D400      dmactl  write DMA control
    D401      chactl  write character control
    D402      dlistl  write display list lo
    D403      dlisth  write display list hi
    D404      hscrol  write horz scroll
    D405      vscrol  write vert scroll
	D406	  xff	  unused
    D407      pmbash  player/missile base addr hi
	D408	  xff	  unused
    D409      chbash  character generator base addr hi
    D40A      wsync   wait for hsync
	D40B	  xff	  unused
	D40C	  xff	  unused
	D40D	  xff	  unused
	D40E	  nmien   NMI enable
    D40F      nmires  NMI reset

    D500-D7FF xff     unused memory

	D800-DFFF ROM	  floating point ROM
	E000-FFFF ROM	  BIOS ROM
******************************************************************************/

static ADDRESS_MAP_START(a400_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x9fff) AM_NOP	/* RAM installed at runtime */
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(pia_0_alt_r, pia_0_alt_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(a800_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x9fff) AM_NOP	/* RAM installed at runtime */
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(pia_0_alt_r, pia_0_alt_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(a800xl_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x4fff) AM_RAM
	AM_RANGE(0x5000, 0x57ff) AM_READWRITE(MRA8_BANK2, MWA8_BANK2)
	AM_RANGE(0x5800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK3)
	AM_RANGE(0xd000, 0xd0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd100, 0xd1ff) AM_NOP
	AM_RANGE(0xd200, 0xd2ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xd300, 0xd3ff) AM_READWRITE(pia_0_alt_r, pia_0_alt_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xd500, 0xd7ff) AM_NOP
	AM_RANGE(0xd800, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK4)
ADDRESS_MAP_END


static ADDRESS_MAP_START(a5200_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc0ff) AM_READWRITE(atari_gtia_r, atari_gtia_w)
	AM_RANGE(0xd400, 0xd5ff) AM_READWRITE(atari_antic_r, atari_antic_w)
	AM_RANGE(0xe800, 0xe8ff) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END



int atari_input_disabled(void)
{
	return 0;
}



#define JOYSTICK_DELTA			10
#define JOYSTICK_SENSITIVITY	200

static INPUT_PORTS_START( atari_artifacting )
    PORT_START_TAG("artifacts")
	PORT_CONFNAME(0x40, 0x00, "Television Artifacts" )
	PORT_CONFSETTING(0x00, DEF_STR( Off ))
    PORT_CONFSETTING(0x40, DEF_STR( On ))
INPUT_PORTS_END



static INPUT_PORTS_START( atari_console )
	PORT_START_TAG("console")
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("CONS.2: Option") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("CONS.1: Select") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("CONS.0: Start") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END



static INPUT_PORTS_START( atari_digital_joystick2 )
	PORT_START_TAG("djoy_0_1")	/* IN1 digital joystick #1 + #2 (PIA port A) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(1)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(1)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(1)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(2)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(2)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(2)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2)

	PORT_START_TAG("djoy_2_3")	/* IN2 digital joystick #3 + #4 (PIA port B) */
	PORT_BIT(0x01, 0x01, IPT_UNUSED)
	PORT_BIT(0x02, 0x02, IPT_UNUSED)
	PORT_BIT(0x04, 0x04, IPT_UNUSED)
	PORT_BIT(0x08, 0x08, IPT_UNUSED)
	PORT_BIT(0x10, 0x10, IPT_UNUSED)
	PORT_BIT(0x20, 0x20, IPT_UNUSED)
	PORT_BIT(0x40, 0x40, IPT_UNUSED)
	PORT_BIT(0x80, 0x80, IPT_UNUSED)

	PORT_START_TAG("djoy_b")	/* IN3 digital joystick buttons (GTIA button bits) */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x04, 0x04, IPT_UNUSED)
	PORT_BIT(0x08, 0x08, IPT_UNUSED)
	PORT_BIT(0x10, 0x10, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x40, 0x40, IPT_UNUSED)
	PORT_BIT(0x80, 0x80, IPT_UNUSED)
INPUT_PORTS_END



static INPUT_PORTS_START( atari_digital_joystick4 )
	PORT_START_TAG("djoy_0_1")	/* digital joystick #1 + #2 (PIA port A) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(1)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(1)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(1)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(1)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(2)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(2)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(2)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(2)

	PORT_START_TAG("djoy_2_3")	/* digital joystick #3 + #4 (PIA port B) */
	PORT_BIT(0x01, 0x01, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(3)
	PORT_BIT(0x02, 0x02, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(3)
	PORT_BIT(0x04, 0x04, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(3)
	PORT_BIT(0x08, 0x08, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(3)
	PORT_BIT(0x10, 0x10, IPT_JOYSTICK_UP)    PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_Y_UP_SWITCH)    PORT_PLAYER(4)
	PORT_BIT(0x20, 0x20, IPT_JOYSTICK_DOWN)  PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)  PORT_PLAYER(4)
	PORT_BIT(0x40, 0x40, IPT_JOYSTICK_LEFT)  PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_X_LEFT_SWITCH)  PORT_PLAYER(4)
	PORT_BIT(0x80, 0x80, IPT_JOYSTICK_RIGHT) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(4)

	PORT_START_TAG("djoy_b")	/* digital joystick buttons (GTIA button bits) */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x04, 0x04, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON3) PORT_PLAYER(3)
	PORT_BIT(0x08, 0x08, IPT_BUTTON1) PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_BUTTON4) PORT_PLAYER(4)
	PORT_BIT(0x10, 0x10, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x40, 0x40, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON3) PORT_PLAYER(3)
	PORT_BIT(0x80, 0x80, IPT_BUTTON2) PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_BUTTON4) PORT_PLAYER(4)
INPUT_PORTS_END



static INPUT_PORTS_START( atari_keyboard )
	PORT_START_TAG("keyboard_0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backsp") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)

	PORT_START_TAG("keyboard_1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)

	PORT_START_TAG("keyboard_2")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ \\") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("* ^") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)

	PORT_START_TAG("keyboard_3")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", [") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". ]") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH2)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Atari") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Insert") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN)
INPUT_PORTS_END



static INPUT_PORTS_START( atari_analog_paddles )
	PORT_START_TAG("analog_0") /* IN8 analog in #1 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(1) PORT_REVERSE

	PORT_START_TAG("analog_1") /* IN9 analog in #2 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(2) PORT_REVERSE

	PORT_START_TAG("analog_2") /* IN10 analog in #3 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(3) PORT_REVERSE

	PORT_START_TAG("analog_3") /* IN11 analog in #4 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(4) PORT_REVERSE

	PORT_START_TAG("analog_4") /* IN12 analog in #5 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_REVERSE /* PORT_PLAYER(5) */

	PORT_START_TAG("analog_5") /* IN13 analog in #6 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_REVERSE /* PORT_PLAYER(6) */

	PORT_START_TAG("analog_6") /* IN14 analog in #7 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_REVERSE /* PORT_PLAYER(7) */

	PORT_START_TAG("analog_7") /* IN15 analog in #8 */
	PORT_BIT(0xff, 0x74, IPT_PADDLE) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_REVERSE /* PORT_PLAYER(8) */
INPUT_PORTS_END



static INPUT_PORTS_START( a800 )
	PORT_INCLUDE( atari_artifacting )
	PORT_INCLUDE( atari_console )
	PORT_INCLUDE( atari_digital_joystick4 )
	PORT_INCLUDE( atari_keyboard )
	PORT_INCLUDE( atari_analog_paddles )
INPUT_PORTS_END



static INPUT_PORTS_START( a800xl )
	PORT_INCLUDE( atari_artifacting )
	PORT_INCLUDE( atari_console )
	PORT_INCLUDE( atari_digital_joystick2 )
	PORT_INCLUDE( atari_keyboard )
	PORT_INCLUDE( atari_analog_paddles )
INPUT_PORTS_END



static INPUT_PORTS_START( a5200 )
	PORT_INCLUDE( atari_artifacting )

	PORT_START_TAG("djoy_b")	/* lower/upper buttons */
	PORT_BIT(0x01, 0x01, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x02, 0x02, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT(0x04, 0x04, IPT_BUTTON1) PORT_PLAYER(3)
	PORT_BIT(0x08, 0x08, IPT_BUTTON1) PORT_PLAYER(4)
	PORT_BIT(0x10, 0x10, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT(0x20, 0x20, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x40, 0x40, IPT_BUTTON2) PORT_PLAYER(3)
    PORT_BIT(0x80, 0x80, IPT_BUTTON2) PORT_PLAYER(4)

	/* KBCODE						   */
	/* Key	   bits    Keypad code	   */
	/* -------------------			   */
	/* none    0000    $FF			   */
	/* #	   0001    $0B			   */
	/* 0	   0010    $00			   */
	/* *	   0011    $0A			   */
	/* Reset   0100    $0E			   */
	/* 9	   0101    $09			   */
	/* 8	   0110    $08			   */
	/* 7	   0111    $07			   */
	/* Pause   1000    $0D			   */
	/* 6	   1001    $06			   */
	/* 5	   1010    $05			   */
	/* 4	   1011    $04			   */
	/* Start   1100    $0C			   */
	/* 3	   1101    $03			   */
	/* 2	   1110    $02			   */
	/* 1	   1111    $01			   */

    PORT_START_TAG("keypad")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Break)") PORT_CODE(KEYCODE_PAUSE)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[#]") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[0]") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[*]") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Reset") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[9]") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[8]") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[7]") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[6]") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[5]") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[4]") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Start") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[3]") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[2]") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[1]") PORT_CODE(KEYCODE_1_PAD)

    PORT_START_TAG("analog_0")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(1)

    PORT_START_TAG("analog_1")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(1)

    PORT_START_TAG("analog_2")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(2)

    PORT_START_TAG("analog_3")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(2)

    PORT_START_TAG("analog_4")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(3)

    PORT_START_TAG("analog_5")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(3)

    PORT_START_TAG("analog_6")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(4)

    PORT_START_TAG("analog_7")
	PORT_BIT(0xff, 0x72, IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0x00,0xe4) PORT_PLAYER(4)

INPUT_PORTS_END


static const UINT8 atari_palette[256*3] =
{
	/* Grey */
    0x00,0x00,0x00, 0x25,0x25,0x25, 0x34,0x34,0x34, 0x4e,0x4e,0x4e,
	0x68,0x68,0x68, 0x75,0x75,0x75, 0x8e,0x8e,0x8e, 0xa4,0xa4,0xa4,
	0xb8,0xb8,0xb8, 0xc5,0xc5,0xc5, 0xd0,0xd0,0xd0, 0xd7,0xd7,0xd7,
	0xe1,0xe1,0xe1, 0xea,0xea,0xea, 0xf4,0xf4,0xf4, 0xff,0xff,0xff,
	/* Gold */
    0x41,0x20,0x00, 0x54,0x28,0x00, 0x76,0x37,0x00, 0x9a,0x50,0x00,
	0xc3,0x68,0x06, 0xe4,0x7b,0x07, 0xff,0x91,0x1a, 0xff,0xab,0x1d,
	0xff,0xc5,0x1f, 0xff,0xd0,0x3b, 0xff,0xd8,0x4c, 0xff,0xe6,0x51,
	0xff,0xf4,0x56, 0xff,0xf9,0x70, 0xff,0xff,0x90, 0xff,0xff,0xaa,
	/* Orange */
    0x45,0x19,0x04, 0x72,0x1e,0x11, 0x9f,0x24,0x1e, 0xb3,0x3a,0x20,
	0xc8,0x51,0x20, 0xe3,0x69,0x20, 0xfc,0x81,0x20, 0xfd,0x8c,0x25,
	0xfe,0x98,0x2c, 0xff,0xae,0x38, 0xff,0xb9,0x46, 0xff,0xbf,0x51,
	0xff,0xc6,0x6d, 0xff,0xd5,0x87, 0xff,0xe4,0x98, 0xff,0xe6,0xab,
	/* Red-Orange */
    0x5d,0x1f,0x0c, 0x7a,0x24,0x0d, 0x98,0x2c,0x0e, 0xb0,0x2f,0x0f,
	0xbf,0x36,0x24, 0xd3,0x4e,0x2a, 0xe7,0x62,0x3e, 0xf3,0x6e,0x4a,
	0xfd,0x78,0x54, 0xff,0x8a,0x6a, 0xff,0x98,0x7c, 0xff,0xa4,0x8b,
	0xff,0xb3,0x9e, 0xff,0xc2,0xb2, 0xff,0xd0,0xc3, 0xff,0xda,0xd0,
	/* Pink */
    0x4a,0x17,0x00, 0x72,0x1f,0x00, 0xa8,0x13,0x00, 0xc8,0x21,0x0a,
	0xdf,0x25,0x12, 0xec,0x3b,0x24, 0xfa,0x52,0x36, 0xfc,0x61,0x48,
	0xff,0x70,0x5f, 0xff,0x7e,0x7e, 0xff,0x8f,0x8f, 0xff,0x9d,0x9e,
	0xff,0xab,0xad, 0xff,0xb9,0xbd, 0xff,0xc7,0xce, 0xff,0xca,0xde,
	/* Purple */
    0x49,0x00,0x36, 0x66,0x00,0x4b, 0x80,0x03,0x5f, 0x95,0x0f,0x74,
	0xaa,0x22,0x88, 0xba,0x3d,0x99, 0xca,0x4d,0xa9, 0xd7,0x5a,0xb6,
	0xe4,0x67,0xc3, 0xef,0x72,0xce, 0xfb,0x7e,0xda, 0xff,0x8d,0xe1,
	0xff,0x9d,0xe5, 0xff,0xa5,0xe7, 0xff,0xaf,0xea, 0xff,0xb8,0xec,
	/* Purple-Blue */
    0x48,0x03,0x6c, 0x5c,0x04,0x88, 0x65,0x0d,0x90, 0x7b,0x23,0xa7,
	0x93,0x3b,0xbf, 0x9d,0x45,0xc9, 0xa7,0x4f,0xd3, 0xb2,0x5a,0xde,
	0xbd,0x65,0xe9, 0xc5,0x6d,0xf1, 0xce,0x76,0xfa, 0xd5,0x83,0xff,
	0xda,0x90,0xff, 0xde,0x9c,0xff, 0xe2,0xa9,0xff, 0xe6,0xb6,0xff,
	/* Blue 1 */
    0x05,0x1e,0x81, 0x06,0x26,0xa5, 0x08,0x2f,0xca, 0x26,0x3d,0xd4,
	0x44,0x4c,0xde, 0x4f,0x5a,0xec, 0x5a,0x68,0xff, 0x65,0x75,0xff,
	0x71,0x83,0xff, 0x80,0x91,0xff, 0x90,0xa0,0xff, 0x97,0xa9,0xff,
	0x9f,0xb2,0xff, 0xaf,0xbe,0xff, 0xc0,0xcb,0xff, 0xcd,0xd3,0xff,
	/* Blue 2 */
    0x0b,0x07,0x79, 0x20,0x1c,0x8e, 0x35,0x31,0xa3, 0x46,0x42,0xb4,
	0x57,0x53,0xc5, 0x61,0x5d,0xcf, 0x6d,0x69,0xdb, 0x7b,0x77,0xe9,
	0x89,0x85,0xf7, 0x91,0x8d,0xff, 0x9c,0x98,0xff, 0xa7,0xa4,0xff,
	0xb2,0xaf,0xff, 0xbb,0xb8,0xff, 0xc3,0xc1,0xff, 0xd3,0xd1,0xff,
	/* Light-Blue */
    0x1d,0x29,0x5a, 0x1d,0x38,0x76, 0x1d,0x48,0x92, 0x1d,0x5c,0xac,
	0x1d,0x71,0xc6, 0x32,0x86,0xcf, 0x48,0x9b,0xd9, 0x4e,0xa8,0xec,
	0x55,0xb6,0xff, 0x69,0xca,0xff, 0x74,0xcb,0xff, 0x82,0xd3,0xff,
	0x8d,0xda,0xff, 0x9f,0xd4,0xff, 0xb4,0xe2,0xff, 0xc0,0xeb,0xff,
	/* Turquoise */
    0x00,0x4b,0x59, 0x00,0x5d,0x6e, 0x00,0x6f,0x84, 0x00,0x84,0x9c,
	0x00,0x99,0xbf, 0x00,0xab,0xca, 0x00,0xbc,0xde, 0x00,0xd0,0xf5,
	0x10,0xdc,0xff, 0x3e,0xe1,0xff, 0x64,0xe7,0xff, 0x76,0xea,0xff,
	0x8b,0xed,0xff, 0x9a,0xef,0xff, 0xb1,0xf3,0xff, 0xc7,0xf6,0xff,
	/* Green-Blue */
    0x00,0x48,0x00, 0x00,0x54,0x00, 0x03,0x6b,0x03, 0x0e,0x76,0x0e,
	0x18,0x80,0x18, 0x27,0x92,0x27, 0x36,0xa4,0x36, 0x4e,0xb9,0x4e,
	0x51,0xcd,0x51, 0x72,0xda,0x72, 0x7c,0xe4,0x7c, 0x85,0xed,0x85,
	0x99,0xf2,0x99, 0xb3,0xf7,0xb3, 0xc3,0xf9,0xc3, 0xcd,0xfc,0xcd,
	/* Green */
    0x16,0x40,0x00, 0x1c,0x53,0x00, 0x23,0x66,0x00, 0x28,0x78,0x00,
	0x2e,0x8c,0x00, 0x3a,0x98,0x0c, 0x47,0xa5,0x19, 0x51,0xaf,0x23,
	0x5c,0xba,0x2e, 0x71,0xcf,0x43, 0x85,0xe3,0x57, 0x8d,0xeb,0x5f,
	0x97,0xf5,0x69, 0xa0,0xfe,0x72, 0xb1,0xff,0x8a, 0xbc,0xff,0x9a,
	/* Yellow-Green */
    0x2c,0x35,0x00, 0x38,0x44,0x00, 0x44,0x52,0x00, 0x49,0x56,0x00,
	0x60,0x71,0x00, 0x6c,0x7f,0x00, 0x79,0x8d,0x0a, 0x8b,0x9f,0x1c,
	0x9e,0xb2,0x2f, 0xab,0xbf,0x3c, 0xb8,0xcc,0x49, 0xc2,0xd6,0x53,
	0xcd,0xe1,0x53, 0xdb,0xef,0x6c, 0xe8,0xfc,0x79, 0xf2,0xff,0xab,
	/* Orange-Green */
    0x46,0x3a,0x09, 0x4d,0x3f,0x09, 0x54,0x45,0x09, 0x6c,0x58,0x09,
	0x90,0x76,0x09, 0xab,0x8b,0x0a, 0xc1,0xa1,0x20, 0xd0,0xb0,0x2f,
	0xde,0xbe,0x3d, 0xe6,0xc6,0x45, 0xed,0xcd,0x4c, 0xf5,0xd8,0x62,
	0xfb,0xe2,0x76, 0xfc,0xee,0x98, 0xfd,0xf3,0xa9, 0xfd,0xf3,0xbe,
	/* Light-Orange */
    0x40,0x1a,0x02, 0x58,0x1f,0x05, 0x70,0x24,0x08, 0x8d,0x3a,0x13,
	0xab,0x51,0x1f, 0xb5,0x64,0x27, 0xbf,0x77,0x30, 0xd0,0x85,0x3a,
	0xe1,0x93,0x44, 0xed,0xa0,0x4e, 0xf9,0xad,0x58, 0xfc,0xb7,0x5c,
	0xff,0xc1,0x60, 0xff,0xca,0x69, 0xff,0xcf,0x7e, 0xff,0xda,0x96
};

static unsigned short atari_colortable[] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

/* Initialise the palette */
static PALETTE_INIT( atari )
{
	palette_set_colors_rgb(machine, 0, atari_palette, sizeof(atari_palette) / 3);
	memcpy(colortable,atari_colortable,sizeof(atari_colortable));
}



static READ8_HANDLER( analog_0_r ) { return (UINT8) readinputportbytag("analog_0"); }
static READ8_HANDLER( analog_1_r ) { return (UINT8) readinputportbytag("analog_1"); }
static READ8_HANDLER( analog_2_r ) { return (UINT8) readinputportbytag("analog_2"); }
static READ8_HANDLER( analog_3_r ) { return (UINT8) readinputportbytag("analog_3"); }
static READ8_HANDLER( analog_4_r ) { return (UINT8) readinputportbytag("analog_4"); }
static READ8_HANDLER( analog_5_r ) { return (UINT8) readinputportbytag("analog_5"); }
static READ8_HANDLER( analog_6_r ) { return (UINT8) readinputportbytag("analog_6"); }
static READ8_HANDLER( analog_7_r ) { return (UINT8) readinputportbytag("analog_7"); }



static struct POKEYinterface pokey_interface =
{
	{
		analog_0_r,
		analog_1_r,
		analog_2_r,
		analog_3_r,
		analog_4_r,
		analog_5_r,
		analog_6_r,
		analog_7_r
	},
	0,
	atari_serin_r,
	atari_serout_w,
	atari_interrupt_cb,
};



static MACHINE_DRIVER_START( atari_common_nodac )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6510, FREQ_17_EXACT)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(1))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_VISIBLE_AREA(MIN_X, MAX_X, MIN_Y, MAX_Y)
	MDRV_PALETTE_LENGTH(sizeof(atari_palette) / sizeof(atari_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(atari_colortable) / sizeof(atari_colortable[0]))
	MDRV_PALETTE_INIT(atari)

	MDRV_VIDEO_START(atari)
	MDRV_VIDEO_UPDATE(atari)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(POKEY, FREQ_17_EXACT)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atari_common )
	MDRV_IMPORT_FROM( atari_common_nodac )
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a400 )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a400_mem, 0)
	MDRV_CPU_VBLANK_INT(a400_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_START( a400 )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a400pal )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a400_mem, 0)
	MDRV_CPU_VBLANK_INT(a400_interrupt, TOTAL_LINES_50HZ)

	MDRV_MACHINE_START( a400 )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_50HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_50HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800 )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800_mem, 0)
	MDRV_CPU_VBLANK_INT(a800_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_START( a800 )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800pal )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800_mem, 0)
	MDRV_CPU_VBLANK_INT(a800_interrupt, TOTAL_LINES_50HZ)

	MDRV_MACHINE_START( a800 )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_50HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_50HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a800xl )
	MDRV_IMPORT_FROM( atari_common )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a800xl_mem, 0)
	MDRV_CPU_VBLANK_INT(a800xl_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_START( a800xl )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a5200 )
	MDRV_IMPORT_FROM( atari_common_nodac )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(a5200_mem, 0)
	MDRV_CPU_VBLANK_INT(a5200_interrupt, TOTAL_LINES_60HZ)

	MDRV_MACHINE_START( a5200 )
	MDRV_SCREEN_REFRESH_RATE(FRAME_RATE_60HZ)
	MDRV_SCREEN_SIZE(HWIDTH*8, TOTAL_LINES_60HZ)
MACHINE_DRIVER_END


ROM_START(a400)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari400.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a400pal)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari400.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari800.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800pal)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 2 * 8K for cartridges */
	ROM_LOAD("floatpnt.rom", 0xd800, 0x0800, CRC(6a5d766e) SHA1(01a6044f7a81d409c938e7dfde0a1af5832229d2))
	ROM_LOAD("atari800.rom", 0xe000, 0x2000, CRC(cb4db9af) SHA1(4e784f4e2530110366f7e5d257d0f050de4201b2))
ROM_END

ROM_START(a800xl)
	ROM_REGION(0x18000,REGION_CPU1,0) /* 64K for the CPU + 16K + 2 * 8K for cartridges */
	ROM_LOAD("basic.rom",   0x10000, 0x2000, CRC(7d684184) SHA1(3693c9cb9bf3b41bae1150f7a8264992468fc8c0))
	ROM_LOAD("atarixl.rom", 0x14000, 0x4000, CRC(1f9cd270) SHA1(ae4f523ba08b6fd59f3cae515a2b2410bbd98f55))
ROM_END

ROM_START(a5200)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 16K for cartridges */
	ROM_LOAD("5200.rom", 0xf800, 0x0800, CRC(4248d3e3) SHA1(6ad7a1e8c9fad486fbec9498cb48bf5bc3adc530))
ROM_END

ROM_START(a5200a)
	ROM_REGION(0x14000,REGION_CPU1,0) /* 64K for the CPU + 16K for cartridges */
	ROM_LOAD("5200a.rom", 0xf800, 0x0800, CRC(c2ba2613) SHA1(1d2a3f00109d75d2d79fecb565775eb95b7d04d5))
ROM_END

static void atari_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_a800_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "atr,dsk,xfd"); break;
	}
}

SYSTEM_CONFIG_START(atari)
	CONFIG_DEVICE(atari_floppy_getinfo)
SYSTEM_CONFIG_END

static void a400_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_a800_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_a800_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom,bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(a400)
	CONFIG_IMPORT_FROM(atari)
	CONFIG_RAM_DEFAULT(40 * 1024)
	CONFIG_DEVICE(a400_cartslot_getinfo)
SYSTEM_CONFIG_END

static void a800_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_a800_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_a800_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom,bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(a800)
	CONFIG_IMPORT_FROM(atari)
	CONFIG_RAM_DEFAULT(40 * 1024)
	CONFIG_DEVICE(a800_cartslot_getinfo)
SYSTEM_CONFIG_END

static void a5200_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_a5200_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_a5200_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom,bin,a52"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(a5200)
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_DEVICE(a5200_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT    COMPAT	MACHINE		INPUT    INIT	CONFIG	COMPANY   FULLNAME */
COMP ( 1979, a400,	   0,		 0,		a400,		a800,	 0,		a400,	"Atari",  "Atari 400 (NTSC)" , 0)
COMP ( 1979, a400pal,  a400,	 0,		a400pal,	a800,	 0,		a400,	"Atari",  "Atari 400 (PAL)" , 0)
COMP ( 1979, a800,	   0,		 0,		a800,		a800,	 0,		a800,	"Atari",  "Atari 800 (NTSC)" , 0)
COMP ( 1979, a800pal,  a800,	 0,		a800pal,	a800,	 0,		a800,	"Atari",  "Atari 800 (PAL)" , 0)
COMP ( 1983, a800xl,   a800,	 0,		a800xl,		a800xl,	 0,		a800,	"Atari",  "Atari 800XL", GAME_NOT_WORKING )
CONS ( 1982, a5200,    0,		 0,		a5200,		a5200,	 0,		a5200,	"Atari",  "Atari 5200", 0)
CONS ( 1982, a5200a,   a5200,    0,		a5200,		a5200,	 0,		a5200,	"Atari",  "Atari 5200 (alt)", 0)
