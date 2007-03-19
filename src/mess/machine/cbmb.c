/***************************************************************************
	commodore b series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/tpi6525.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/cbmieeeb.h"
#include "includes/vic6567.h"
#include "includes/crtc6845.h"
#include "mscommon.h"

#include "includes/cbmb.h"

/* keyboard lines */
static UINT8 cbmb_keyline[12] = { 0 };
static int cbmb_keyline_a, cbmb_keyline_b, cbmb_keyline_c;

static int cbm500=0;
UINT8 *cbmb_basic;
UINT8 *cbmb_kernal;
static UINT8 *cbmb_chargen;
UINT8 *cbmb_videoram;
UINT8 *cbmb_colorram;

/* tpi at 0xfde00
 in interrupt mode
 irq to m6509 irq
 pa7 ieee nrfd
 pa6 ieee ndac
 pa5 ieee eoi
 pa4 ieee dav
 pa3 ieee atn
 pa2 ieee ren
 pa1 ieee io chips te
 pa0 ieee io chip dc
 pb1 ieee seq
 pb0 ieee ifc
 cb ?
 ca chargen rom address line 12
 i4 acia irq
 i3 ?
 i2 cia6526 irq
 i1 self pb0
 i0 tod (50 or 60 hertz frequency)
 */
static int cbmb_tpi0_port_a_r(void)
{
	int data=0;
	if (cbm_ieee_nrfd_r()) data|=0x80;
	if (cbm_ieee_ndac_r()) data|=0x40;
	if (cbm_ieee_eoi_r()) data|=0x20;
	if (cbm_ieee_dav_r()) data|=0x10;
	if (cbm_ieee_atn_r()) data|=8;
/*	if (cbm_ieee_ren_r()) data|=4; */
	return data;
}

static void cbmb_tpi0_port_a_w(int data)
{
	cbm_ieee_nrfd_w(0,data&0x80);
	cbm_ieee_ndac_w(0,data&0x40);
	cbm_ieee_eoi_w(0,data&0x20);
	cbm_ieee_dav_w(0,data&0x10);
	cbm_ieee_atn_w(0,data&8);
/*	cbm_ieee_ren_w(0,data&4); */
	logerror("cbm ieee %d %d\n",data&2, data&1);
}

/* tpi at 0xfdf00
  cbm 500
   port c7 video address lines?
   port c6 ?
  cbm 600
   port c7 low
   port c6 ntsc 1, 0 pal
  cbm 700
   port c7 high
   port c6 ?
  port c5 .. c0 keyboard line select
  port a7..a0 b7..b0 keyboard input */
static void cbmb_keyboard_line_select_a(int line)
{
	cbmb_keyline_a=line;
}

static void cbmb_keyboard_line_select_b(int line)
{
	cbmb_keyline_b=line;
}

static void cbmb_keyboard_line_select_c(int line)
{
	cbmb_keyline_c=line;
}

static int cbmb_keyboard_line_a(void)
{
	int data=0;
	if (!(cbmb_keyline_c&1)) data|=cbmb_keyline[0];
	if (!(cbmb_keyline_c&2)) data|=cbmb_keyline[2];
	if (!(cbmb_keyline_c&4)) data|=cbmb_keyline[4];
	if (!(cbmb_keyline_c&8)) data|=cbmb_keyline[6];
	if (!(cbmb_keyline_c&0x10)) data|=cbmb_keyline[8];
	if (!(cbmb_keyline_c&0x20)) data|=cbmb_keyline[10];

	return data^0xff;
}

static int cbmb_keyboard_line_b(void)
{
	int data=0;
	if (!(cbmb_keyline_c&1)) data|=cbmb_keyline[1];
	if (!(cbmb_keyline_c&2)) data|=cbmb_keyline[3];
	if (!(cbmb_keyline_c&4)) data|=cbmb_keyline[5];
	if (!(cbmb_keyline_c&8)) data|=cbmb_keyline[7];
	if (!(cbmb_keyline_c&0x10)) data|=cbmb_keyline[9];
	if (!(cbmb_keyline_c&0x20)) data|=cbmb_keyline[11];

	return data^0xff;
}

static int cbmb_keyboard_line_c(void)
{
	int data=0;

	if ( (cbmb_keyline[0]&~cbmb_keyline_a)||
		 (cbmb_keyline[1]&~cbmb_keyline_b)) data|=1;
	if ( (cbmb_keyline[2]&~cbmb_keyline_a)||
		 (cbmb_keyline[3]&~cbmb_keyline_b)) data|=2;
	if ( (cbmb_keyline[4]&~cbmb_keyline_a)||
		 (cbmb_keyline[5]&~cbmb_keyline_b)) data|=4;
	if ( (cbmb_keyline[6]&~cbmb_keyline_a)||
		 (cbmb_keyline[7]&~cbmb_keyline_b)) data|=8;
	if ( (cbmb_keyline[8]&~cbmb_keyline_a)||
		 (cbmb_keyline[9]&~cbmb_keyline_b)) data|=0x10;
	if ( (cbmb_keyline[10]&~cbmb_keyline_a)||
		 (cbmb_keyline[11]&~cbmb_keyline_b)) data|=0x20;

	if (!cbm500) {
		if (!VIDEO_NTSC) data|=0x40;
		if (!MODELL_700) data|=0x80;
	}
	return data^0xff;
}

static void cbmb_irq (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6509", ("irq %s\n", level ? "start" : "end"));
		cpunum_set_input_line (0, M6502_IRQ_LINE, level);
		old_level = level;
	}
}

/*
  port a ieee in/out
  pa7 trigger gameport 2
  pa6 trigger gameport 1
  pa1 ?
  pa0 ?
  pb7 .. 4 gameport 2
  pb3 .. 0 gameport 1
 */
static UINT8 cbmb_cia_port_a_r(void)
{
	return cbm_ieee_data_r();
}

static void cbmb_cia_port_a_w(UINT8 data)
{
	cbm_ieee_data_w(0, data);
}

const cia6526_interface cbmb_cia =
{
	CIA6526,
	tpi6525_0_irq2_level,
	0.0, 60,

	{
		{ cbmb_cia_port_a_r, cbmb_cia_port_a_w },
		{ 0, 0 }
	}
};

WRITE8_HANDLER ( cbmb_colorram_w )
{
	cbmb_colorram[offset]=data|0xf0;
}

static int cbmb_dma_read(int offset)
{
	if (offset>=0x1000)
		return cbmb_videoram[offset&0x3ff];
	else
		return cbmb_chargen[offset&0xfff];
}

static int cbmb_dma_read_color(int offset)
{
	return cbmb_colorram[offset&0x3ff];
}

static void cbmb_change_font(int level)
{
	cbmb_vh_set_font(level);
}

static void cbmb_common_driver_init (void)
{
	cbmb_chargen=memory_region(REGION_CPU1)+0x100000;
	/*    memset(c64_memory, 0, 0xfd00); */

	cia_config(0, &cbmb_cia);

	tpi6525[0].a.read=cbmb_tpi0_port_a_r;
	tpi6525[0].a.output=cbmb_tpi0_port_a_w;
	tpi6525[0].ca.output=cbmb_change_font;
	tpi6525[0].interrupt.output=cbmb_irq;
	tpi6525[1].a.read=cbmb_keyboard_line_a;
	tpi6525[1].b.read=cbmb_keyboard_line_b;
	tpi6525[1].c.read=cbmb_keyboard_line_c;
	tpi6525[1].a.output=cbmb_keyboard_line_select_a;
	tpi6525[1].b.output=cbmb_keyboard_line_select_b;
	tpi6525[1].c.output=cbmb_keyboard_line_select_c;
	timer_pulse(0.01, 0, cbmb_frame_interrupt);

	cbm_ieee_open();
}

static struct crtc6845_config cbm600_crtc= { 1600000 /*?*/, cbmb_vh_cursor };

void cbm600_driver_init (void)
{
	cbmb_common_driver_init ();
	cbm600_vh_init();
	crtc6845_init(&cbm600_crtc);
}

void cbm600pal_driver_init (void)
{
	cbmb_common_driver_init ();
	cbm600_vh_init();
	crtc6845_init(&cbm600_crtc);
}

void cbm600hu_driver_init (void)
{
	cbmb_common_driver_init ();
	crtc6845_init(&cbm600_crtc);
}

static struct crtc6845_config cbm700_crtc= { 2000000 /*?*/, cbmb_vh_cursor };

void cbm700_driver_init (void)
{
	cbmb_common_driver_init ();
	cbm700_vh_init();
	crtc6845_init(&cbm700_crtc);
}

void cbm500_driver_init (void)
{
	cbmb_common_driver_init ();
	cbm500=1;
	vic6567_init (0, 0, cbmb_dma_read, cbmb_dma_read_color, NULL);
}

MACHINE_RESET( cbmb )
{
	sndti_reset(SOUND_SID6581, 0);
	cia_reset();
	tpi6525_0_reset();
	tpi6525_1_reset();

	cbm_drive_0_config (IEEE8ON ? IEEE : 0, 8);
	cbm_drive_1_config (IEEE9ON ? IEEE : 0, 9);
	cbmb_rom_load();
}

void cbmb_rom_load(void)
{
	int i;

	for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
			 &&(cbm_rom[i].size!=0); i++) {
		memcpy(cbmb_memory+cbm_rom[i].addr+0xf0000, cbm_rom[i].chip, cbm_rom[i].size);
	}
}

void cbmb_frame_interrupt (int param)
{
	static int level = 0;
	int value;

	tpi6525_0_irq0_level(level);
	level=!level;
	if (level) return ;

	value = 0;
	if (KEY_STOP)
		value |= 0x80;
	if (KEY_GRAPH)
		value |= 0x40;
	if (KEY_REVERSE)
		value |= 0x20;
	if (KEY_HOME)
		value |= 0x10;
	if (KEY_CURSOR_UP)
		value |= 8;
	if (KEY_CURSOR_DOWN)
		value |= 4;
	if (KEY_F10)
		value |= 2;
	if (KEY_F9)
		value |= 1;
	cbmb_keyline[0] = value;

	value = 0;
	if (KEY_F8)
		value |= 0x80;
	if (KEY_F7)
		value |= 0x40;
	if (KEY_F6)
		value |= 0x20;
	if (KEY_F5)
		value |= 0x10;
	if (KEY_F4)
		value |= 8;
	if (KEY_F3)
		value |= 4;
	if (KEY_F2)
		value |= 2;
	if (KEY_F1)
		value |= 1;
	cbmb_keyline[1] = value;

	value = 0;
	if (KEY_PAD_SLASH)
		value |= 0x80;
	if (KEY_PAD_ASTERIX)
		value |= 0x40;
	if (KEY_PAD_CE)
		value |= 0x20;
	if (KEY_PAD_HELP)
		value |= 0x10;
	if (KEY_CURSOR_LEFT)
		value |= 8;
	if (KEY_EQUALS)
		value |= 4;
	if (KEY_0)
		value |= 2;
	if (KEY_9)
		value |= 1;
	cbmb_keyline[2] = value;

	value = 0;
	if (KEY_8)
		value |= 0x80;
	if (KEY_7)
		value |= 0x40;
	if (KEY_5)
		value |= 0x20;
	if (KEY_4)
		value |= 0x10;
	if (KEY_3)
		value |= 8;
	if (KEY_2)
		value |= 4;
	if (KEY_1)
		value |= 2;
	if (KEY_ESC)
		value |= 1;
	cbmb_keyline[3] = value;

	value = 0;
	if (KEY_PAD_MINUS)
		value |= 0x80;
	if (KEY_PAD_9)
		value |= 0x40;
	if (KEY_PAD_8)
		value |= 0x20;
	if (KEY_PAD_7)
		value |= 0x10;
	if (KEY_CURSOR_RIGHT)
		value |= 8;
	if (KEY_ARROW_LEFT)
		value |= 4;
	if (KEY_MINUS)
		value |= 2;
	if (KEY_O)
		value |= 1;
	cbmb_keyline[4] = value;

	value = 0;
	if (KEY_I)
		value |= 0x80;
	if (KEY_U)
		value |= 0x40;
	if (KEY_6)
		value |= 0x20;
	if (KEY_R)
		value |= 0x10;
	if (KEY_E)
		value |= 8;
	if (KEY_W)
		value |= 4;
	if (KEY_Q)
		value |= 2;
	if (KEY_TAB)
		value |= 1;
	cbmb_keyline[5] = value;

	value = 0;
	if (KEY_PAD_PLUS)
		value |= 0x80;
	if (KEY_PAD_6)
		value |= 0x40;
	if (KEY_PAD_5)
		value |= 0x20;
	if (KEY_PAD_4)
		value |= 0x10;
	if (KEY_DEL)
		value |= 8;
	if (KEY_CLOSEBRACE)
		value |= 4;
	if (KEY_P)
		value |= 2;
	if (KEY_L)
		value |= 1;
	cbmb_keyline[6] = value;

	value = 0;
	if (KEY_K)
		value |= 0x80;
	if (KEY_J)
		value |= 0x40;
	if (KEY_Y)
		value |= 0x20;
	if (KEY_T)
		value |= 0x10;
	if (KEY_D)
		value |= 8;
	if (KEY_S)
		value |= 4;
	if (KEY_A)
		value |= 2;
	cbmb_keyline[7] = value;

	value = 0;
	if (KEY_PAD_ENTER)
		value |= 0x80;
	if (KEY_PAD_3)
		value |= 0x40;
	if (KEY_PAD_2)
		value |= 0x20;
	if (KEY_PAD_1)
		value |= 0x10;
	if (KEY_CBM)
		value |= 8;
	if (KEY_RETURN)
		value |= 4;
	if (KEY_OPENBRACE)
		value |= 2;
	if (KEY_SEMICOLON)
		value |= 1;
	cbmb_keyline[8] = value;

	value = 0;
	if (KEY_COMMA)
		value |= 0x80;
	if (KEY_M)
		value |= 0x40;
	if (KEY_H)
		value |= 0x20;
	if (KEY_G)
		value |= 0x10;
	if (KEY_F)
		value |= 8;
	if (KEY_X)
		value |= 4;
	if (KEY_Z)
		value |= 2;
	if (KEY_SHIFT)
		value |= 1;
	cbmb_keyline[9] = value;

	value = 0;
	if (KEY_PAD_00)
		value |= 0x40;
	if (KEY_PAD_POINT)
		value |= 0x20;
	if (KEY_PAD_0)
		value |= 0x10;
	if (KEY_PI)
		value |= 4;
	if (KEY_APOSTROPH)
		value |= 2;
	if (KEY_SLASH)
		value |= 1;
	cbmb_keyline[10] = value;

	value = 0;
	if (KEY_POINT)
		value |= 0x80;
	if (KEY_SPACE)
		value |= 0x40;
	if (KEY_N)
		value |= 0x20;
	if (KEY_B)
		value |= 0x10;
	if (KEY_V)
		value |= 8;
	if (KEY_C)
		value |= 4;
	if (KEY_CTRL)
		value |= 1;
	cbmb_keyline[11] = value;

#if 0
	value = 0xff;
	if (JOYSTICK1||JOYSTICK1_2BUTTON) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_RIGHT)
			value &= ~8;
		if (JOYSTICK_1_LEFT)
			value &= ~4;
		if (JOYSTICK_1_DOWN)
			value &= ~2;
		if (JOYSTICK_1_UP)
			value &= ~1;
	} else if (PADDLES12) {
		if (PADDLE2_BUTTON)
			value &= ~8;
		if (PADDLE1_BUTTON)
			value &= ~4;
	} else if (MOUSE1) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_BUTTON2)
			value &= ~1;
	}
	cbmb_keyline[8] = value;

	value2 = 0xff;
	if (JOYSTICK2||JOYSTICK2_2BUTTON) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_RIGHT)
			value2 &= ~8;
		if (JOYSTICK_2_LEFT)
			value2 &= ~4;
		if (JOYSTICK_2_DOWN)
			value2 &= ~2;
		if (JOYSTICK_2_UP)
			value2 &= ~1;
	} else if (PADDLES34) {
		if (PADDLE4_BUTTON)
			value2 &= ~8;
		if (PADDLE3_BUTTON)
			value2 &= ~4;
	} else if (MOUSE2) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_BUTTON2)
			value2 &= ~1;
	}
	cbmb_keyline[9] = value2;
#endif

	vic2_frame_interrupt ();

	set_led_status (1 /*KB_CAPSLOCK_FLAG */ , KEY_SHIFTLOCK ? 1 : 0);
#if 0
	set_led_status (0 /*KB_NUMLOCK_FLAG */ , JOYSTICK_SWAP ? 1 : 0);
#endif
}
