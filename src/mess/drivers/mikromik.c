#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "cpu/i8085/i8085.h"
#include "machine/8237dma.h"
#include "machine/i8212.h"
#include "machine/upd765.h"
#include "machine/pit8253.h"
#include "machine/upd7201.h"
#include "video/i8275.h"
#include "video/upd7220.h"
#include "sound/speaker.h"
#include "machine/ram.h"
#include "includes/mikromik.h"

/*

    TODO:

    - add HRTC/VRTC output to i8275
    - NEC uPD7220 GDC
    - accurate video timing
    - floppy DRQ during RECALL = 0
    - PCB layout
    - NEC uPD7201 MPSC

*/

/*

    Nokia Elektroniikka pj

    Controller ILC 9534
    FDC-Interface ILC 9530

    Parts:

    6,144 MHz xtal (CPU clock)
    18,720 MHz xtal (pixel clock)
    16 MHz xtal (FDC clock)
    Intel I8085AP (CPU)
    Intel 8253-5P (PIT)
    Intel 8275P (CRTC)
    Intel 8212P (I/OP)
    Intel 8237A-5P (DMAC)
    NEC uPD7220C (GDC)
    NEC uPD7201P (MPSC=uart)
    NEC uPD765 (FDC)
    TMS4116-15 (16Kx4 DRAM)*4 = 32KB Video RAM for 7220
    2164-6P (64Kx1 DRAM)*8 = 64KB Work RAM

    DMA channels:

    0   CRT
    1   MPSC transmit
    2   MPSC receive
    3   FDC

    Interrupts:

    INTR    MPSC INT
    RST5.5  FDC IRQ
    RST6.5  8212 INT
    RST7.5  DMA EOP

*/

/* Read/Write Handlers */

WRITE8_MEMBER( mm1_state::ls259_w )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	int d = BIT(data, 0);

	switch (offset)
	{
	case 0: /* IC24 A8 */
		//logerror("IC24 A8 %u\n", d);
		memory_set_bank(m_machine, "bank1", d);

		if (d)
		{
			program->install_readwrite_bank(0x0000, 0x0fff, "bank1");
		}
		else
		{
			program->install_read_bank(0x0000, 0x0fff, "bank1");
			program->unmap_write(0x0000, 0x0fff);
		}
		break;

	case 1: /* RECALL */
		//logerror("RECALL %u\n", d);
		m_recall = d;
		upd765_reset_w(m_fdc, d);
		break;

	case 2: /* _RV28/RX21 */
		m_rx21 = d;
		break;

	case 3: /* _TX21 */
		m_tx21 = d;
		break;

	case 4: /* _RCL */
		m_rcl = d;
		break;

	case 5: /* _INTC */
		m_intc = d;
		break;

	case 6: /* LLEN */
		//logerror("LLEN %u\n", d);
		m_llen = d;
		break;

	case 7: /* MOTOR ON */
		//logerror("MOTOR %u\n", d);
		floppy_mon_w(m_floppy0, !d);
		floppy_mon_w(m_floppy1, !d);
		floppy_drive_set_ready_state(m_floppy0, d, 1);
		floppy_drive_set_ready_state(m_floppy1, d, 1);

		if (input_port_read(m_machine, "T5")) upd765_ready_w(m_fdc, d);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, AS_PROGRAM, 8, mm1_state )
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0xfeff) AM_RAM
	AM_RANGE(0xff00, 0xff0f) AM_MIRROR(0x80) AM_DEVREADWRITE_LEGACY(I8237_TAG, i8237_r, i8237_w)
	AM_RANGE(0xff10, 0xff13) AM_MIRROR(0x8c) AM_DEVREADWRITE_LEGACY(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)
    AM_RANGE(0xff20, 0xff21) AM_MIRROR(0x8e) AM_DEVREADWRITE_LEGACY(I8275_TAG, i8275_r, i8275_w)
	AM_RANGE(0xff30, 0xff33) AM_MIRROR(0x8c) AM_DEVREADWRITE_LEGACY(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xff40, 0xff40) AM_MIRROR(0x8f) AM_DEVREADWRITE(I8212_TAG, i8212_device, data_r, data_w)
	AM_RANGE(0xff50, 0xff50) AM_MIRROR(0x8e) AM_DEVREAD_LEGACY(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xff51, 0xff51) AM_MIRROR(0x8e) AM_DEVREADWRITE_LEGACY(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xff60, 0xff67) AM_MIRROR(0x88) AM_WRITE(ls259_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mm1m6_map, AS_PROGRAM, 8, mm1_state )
	AM_IMPORT_FROM(mm1_map)
	AM_RANGE(0xff70, 0xff71) AM_MIRROR(0x8e) AM_DEVREADWRITE_LEGACY(UPD7220_TAG, upd7220_r, upd7220_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mm1_upd7220_map, AS_0, 8, mm1_state )
	AM_RANGE(0x00000, 0x3ffff) AM_DEVREADWRITE_LEGACY(UPD7220_TAG,upd7220_vram_r,upd7220_vram_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC2\xB4 '") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00B4) PORT_CHAR('`')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x96")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(a_RING " " A_RING) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('~') PORT_CHAR('^')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LF")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('@') PORT_CHAR('*')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)

	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(a_UMLAUT " " A_UMLAUT) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(o_UMLAUT " " O_UMLAUT) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right SHIFT") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("T5")
	PORT_CONFNAME( 0x01, 0x00, "Floppy Drive Type")
	PORT_CONFSETTING( 0x00, "640 KB" )
	PORT_CONFSETTING( 0x01, "160/320 KB" )
INPUT_PORTS_END

/* Video */

static PALETTE_INIT( mm1 )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color_rgb(machine, 1, 0x00, 0xc0, 0x00); /* green */
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00); /* bright green */
}

static I8275_DISPLAY_PIXELS( crtc_display_pixels )
{
	mm1_state *state = device->machine().driver_data<mm1_state>();

	UINT8 romdata = state->m_char_rom[(charcode << 4) | linecount];

	int d0 = BIT(romdata, 0);
	int d7 = BIT(romdata, 7);
	int gpa0 = BIT(gpa, 0);
	int llen = state->m_llen;
	int i;

	UINT8 data = (romdata << 1) | (d7 & d0);

	for (i = 0; i < 8; i++)
	{
		int qh = BIT(data, i);
		int video_in = ((((d7 & llen) | !vsp) & !gpa0) & qh) | lten;
		int compl_in = rvv;
		int hlt_in = hlgt;

		int color = hlt_in ? 2 : (video_in ^ compl_in);

		*BITMAP_ADDR16(device->machine().generic.tmpbitmap, y, x + i) = color;
	}
}

static const i8275_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	0,
	DEVCB_DEVICE_LINE(I8237_TAG, i8237_dreq0_w),
	DEVCB_NULL,
	crtc_display_pixels
};

/* TODO */
static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (BIT(data, i)) *BITMAP_ADDR16(bitmap, y, x + i) = 1;
	}
}

static UPD7220_INTERFACE( hgdc_intf )
{
	SCREEN_TAG,
	hgdc_display_pixels,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

void mm1_state::video_start()
{
	// find memory regions
	m_char_rom = m_machine.region("chargen")->base();

	VIDEO_START_NAME(generic_bitmapped)(m_machine);
}

bool mm1_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	/* text */
	i8275_update(m_crtc, &bitmap, &cliprect);
	copybitmap(&bitmap, screen.machine().generic.tmpbitmap, 0, 0, 0, 0, &cliprect);

	/* graphics */
	upd7220_update(m_hgdc, &bitmap, &cliprect);

	return 0;
}

static const gfx_layout tiles8x16_layout =
{
	8, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static GFXDECODE_START( mm1 )
	GFXDECODE_ENTRY( "chargen", 0, tiles8x16_layout, 0, 0x100 )
GFXDECODE_END

/* 8212 Interface */

READ8_MEMBER( mm1_state::kb_r )
{
	return m_keydata;
}

static I8212_INTERFACE( mm1_i8212_intf )
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST65_LINE),
	DEVCB_DRIVER_MEMBER(mm1_state, kb_r),
	DEVCB_NULL
};

/* 8237 Interface */

WRITE_LINE_MEMBER( mm1_state::dma_hrq_changed )
{
	device_set_input_line(m_maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w(m_dmac, state);
}

READ8_MEMBER( mm1_state::mpsc_dack_r )
{
	/* clear data request */
	i8237_dreq2_w(m_dmac, CLEAR_LINE);

	return upd7201_dtra_r(m_mpsc);
}

WRITE8_MEMBER( mm1_state::mpsc_dack_w )
{
	upd7201_hai_w(m_mpsc, data);

	/* clear data request */
	i8237_dreq1_w(m_dmac, CLEAR_LINE);
}

WRITE_LINE_MEMBER( mm1_state::tc_w )
{
	if (!m_dack3)
	{
		/* floppy terminal count */
		upd765_tc_w(m_fdc, !state);
	}

	m_tc = !state;

	device_set_input_line(m_maincpu, I8085_RST75_LINE, state);
}

WRITE_LINE_MEMBER( mm1_state::dack3_w )
{
	m_dack3 = state;

	if (!m_dack3)
	{
		/* floppy terminal count */
		upd765_tc_w(m_fdc, m_tc);
	}
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static I8237_INTERFACE( mm1_dma8237_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(mm1_state, dma_hrq_changed),
	DEVCB_DRIVER_LINE_MEMBER(mm1_state, tc_w),
	DEVCB_MEMORY_HANDLER(I8085A_TAG, PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER(I8085A_TAG, PROGRAM, memory_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DRIVER_MEMBER(mm1_state, mpsc_dack_r), DEVCB_DEVICE_HANDLER(UPD765_TAG, upd765_dack_r) },
	{ DEVCB_DEVICE_HANDLER(I8275_TAG, i8275_dack_w), DEVCB_DRIVER_MEMBER(mm1_state, mpsc_dack_w), DEVCB_NULL, DEVCB_DEVICE_HANDLER(UPD765_TAG, upd765_dack_w) },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DRIVER_LINE_MEMBER(mm1_state, dack3_w) }
};

/* uPD765 Interface */

static const upd765_interface fdc_intf =
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST55_LINE),
	DEVCB_DEVICE_LINE(I8237_TAG, i8237_dreq3_w),
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* 8253 Interface */

WRITE_LINE_MEMBER( mm1_state::itxc_w )
{
	if (!m_intc)
	{
		upd7201_txca_w(m_mpsc, state);
	}
}

WRITE_LINE_MEMBER( mm1_state::irxc_w )
{
	if (!m_intc)
	{
		upd7201_rxca_w(m_mpsc, state);
	}
}

WRITE_LINE_MEMBER( mm1_state::auxc_w )
{
	upd7201_txcb_w(m_mpsc, state);
	upd7201_rxcb_w(m_mpsc, state);
}

static const struct pit8253_config mm1_pit8253_intf =
{
	{
		{
			XTAL_6_144MHz/2/2,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(mm1_state, itxc_w)
		}, {
			XTAL_6_144MHz/2/2,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(mm1_state, irxc_w)
		}, {
			XTAL_6_144MHz/2/2,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(mm1_state, auxc_w)
		}
	}
};

/* uPD7201 Interface */

WRITE_LINE_MEMBER( mm1_state::drq2_w )
{
	if (state)
	{
		i8237_dreq2_w(m_dmac, ASSERT_LINE);
	}
}

WRITE_LINE_MEMBER( mm1_state::drq1_w )
{
	if (state)
	{
		i8237_dreq1_w(m_dmac, ASSERT_LINE);
	}
}

static UPD7201_INTERFACE( mpsc_intf )
{
	DEVCB_NULL,					/* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_DRIVER_LINE_MEMBER(mm1_state, drq2_w),	/* receive DRQ */
			DEVCB_DRIVER_LINE_MEMBER(mm1_state, drq1_w),	/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}, {
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/* I8085A Interface */

READ_LINE_MEMBER( mm1_state::dsra_r )
{
	return 1;
}

static I8085_CONFIG( i8085_intf )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_DRIVER_LINE_MEMBER(mm1_state, dsra_r),	/* SID changed callback (I8085A only) */
	DEVCB_DEVICE_LINE(SPEAKER_TAG, speaker_level_w)	/* SOD changed callback (I8085A only) */
};

/* Keyboard */

void mm1_state::scan_keyboard()
{
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };

	UINT8 data = input_port_read(m_machine, keynames[m_drive]);
	UINT8 special = input_port_read(m_machine, "SPECIAL");
	int ctrl = BIT(special, 0);
	int shift = BIT(special, 2) & BIT(special, 1);
	UINT8 keydata = 0xff;

	if (!BIT(data, m_sense))
	{
		/* get key data from PROM */
		keydata = m_key_rom[(ctrl << 8) | (shift << 7) | (m_drive << 3) | (m_sense)];
	}

	if (m_keydata != keydata)
	{
		/* latch key data */
		m_keydata = keydata;

		if (keydata != 0xff)
		{
			/* strobe in key data */
			m_iop->stb_w(1);
			m_iop->stb_w(0);
		}
	}

	if (keydata == 0xff)
	{
		/* increase scan counters */
		m_sense++;

		if (m_sense == 8)
		{
			m_sense = 0;
			m_drive++;

			if (m_drive == 10)
			{
				m_drive = 0;
			}
		}
	}

}

static TIMER_DEVICE_CALLBACK( kbclk_tick )
{
	mm1_state *state = timer.machine().driver_data<mm1_state>();

	state->scan_keyboard();
}

/* Floppy Configuration */

static FLOPPY_OPTIONS_START( mm1 )
	FLOPPY_OPTION( mm1_640kb, "dsk", "Nokia MikroMikko 1 640KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([8]) /* 3:1 sector skew (1,4,7,2,5,8,3,6) */
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static FLOPPY_OPTIONS_START( mm2 )
	FLOPPY_OPTION( mm2_360kb, "dsk", "Nokia MikroMikko 2 360KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))

	FLOPPY_OPTION( mm2_720kb, "dsk", "Nokia MikroMikko 2 720KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config mm1_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(mm1),
	NULL
};

/* Machine Initialization */

void mm1_state::machine_start()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	/* find memory regions */
	m_key_rom = m_machine.region("keyboard")->base();

	/* setup memory banking */
	program->install_read_bank(0x0000, 0x0fff, "bank1");
	program->unmap_write(0x0000, 0x0fff);
	memory_configure_bank(m_machine, "bank1", 0, 1, m_machine.region("bios")->base(), 0);
	memory_configure_bank(m_machine, "bank1", 1, 1, ram_get_ptr(m_machine.device(RAM_TAG)), 0);
	memory_set_bank(m_machine, "bank1", 0);

	/* register for state saving */
	state_save_register_global(m_machine, m_sense);
	state_save_register_global(m_machine, m_drive);
	state_save_register_global(m_machine, m_llen);
	state_save_register_global(m_machine, m_intc);
	state_save_register_global(m_machine, m_rx21);
	state_save_register_global(m_machine, m_tx21);
	state_save_register_global(m_machine, m_rcl);
	state_save_register_global(m_machine, m_recall);
	state_save_register_global(m_machine, m_dack3);
}

void mm1_state::machine_reset()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	int i;

	/* reset LS259 */
	for (i = 0; i < 8; i++) ls259_w(*program, i, 0);

	/* set FDC ready */
	if (!input_port_read(m_machine, "T5")) upd765_ready_w(m_fdc, 1);

	/* reset FDC */
	upd765_reset_w(m_fdc, 1);
	upd765_reset_w(m_fdc, 0);
}

/* Machine Drivers */

static MACHINE_CONFIG_START( mm1, mm1_state )
	/* basic system hardware */
	MCFG_CPU_ADD(I8085A_TAG, I8085A, XTAL_6_144MHz)
	MCFG_CPU_PROGRAM_MAP(mm1_map)
	MCFG_CPU_CONFIG(i8085_intf)

	MCFG_TIMER_ADD_PERIODIC("kbclk", kbclk_tick, attotime::from_hz(2500)) //attotime::from_hz(XTAL_6_144MHz/2/8))

	/* video hardware */
	MCFG_SCREEN_ADD( SCREEN_TAG, RASTER )
	MCFG_SCREEN_REFRESH_RATE( 50 )
	MCFG_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MCFG_SCREEN_SIZE( 800, 400 )
	MCFG_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 400-1 )
	//MCFG_SCREEN_RAW_PARAMS(XTAL_18_720MHz, ...)

	MCFG_GFXDECODE(mm1)
	MCFG_PALETTE_LENGTH(3)
	MCFG_PALETTE_INIT(mm1)

	MCFG_I8275_ADD(I8275_TAG, crtc_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* peripheral hardware */
	MCFG_I8212_ADD(I8212_TAG, mm1_i8212_intf)
	MCFG_I8237_ADD(I8237_TAG, XTAL_6_144MHz/2, mm1_dma8237_intf)
	MCFG_PIT8253_ADD(I8253_TAG, mm1_pit8253_intf)
	MCFG_UPD765A_ADD(UPD765_TAG, /* XTAL_16MHz/2/2, */ fdc_intf)
	MCFG_UPD7201_ADD(UPD7201_TAG, XTAL_6_144MHz/2, mpsc_intf)

	MCFG_FLOPPY_2_DRIVES_ADD(mm1_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mm1m6, mm1 )
	/* basic system hardware */
	MCFG_CPU_MODIFY(I8085A_TAG)
	MCFG_CPU_PROGRAM_MAP(mm1m6_map)

	/* video hardware */
	MCFG_UPD7220_ADD(UPD7220_TAG, XTAL_18_720MHz/8, hgdc_intf, mm1_upd7220_map)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x4000, "bios", 0 ) /* BIOS */
	ROM_LOAD( "9081b.ic2", 0x0000, 0x2000, CRC(2955feb3) SHA1(946a6b0b8fb898be3f480c04da33d7aaa781152b) )

	/*

        Address Decoder PROM outputs

        D0      IOEN/_MEMEN
        D1      RAMEN
        D2      not connected
        D3      _CE4
        D4      _CE0
        D5      _CE1
        D6      _CE2
        D7      _CE3

    */
	ROM_REGION( 0x200, "address", 0 ) /* address decoder */
	ROM_LOAD( "720793a.ic24", 0x0000, 0x0200, CRC(deea87a6) SHA1(8f19e43252c9a0b1befd02fc9d34fe1437477f3a) )

	ROM_REGION( 0x200, "keyboard", 0 ) /* keyboard encoder */
	ROM_LOAD( "mmi6349-1j.bin", 0x0000, 0x0200, CRC(4ab3bf03) SHA1(925c9ee22db13566416cdbc505c03d4116ff8d5f) )

	ROM_REGION( 0x1000, "chargen", 0 ) /* character generator */
	ROM_LOAD( "6807b.ic61", 0x0000, 0x1000, CRC(32b36220) SHA1(8fe7a181badea3f7e656dfaea21ee9e4c9baf0f1) )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY           FULLNAME                FLAGS
COMP( 1981, mm1m6,		0,		0,		mm1m6,		mm1,		0,		"Nokia Data",		"MikroMikko 1 M6",		GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
COMP( 1981, mm1m7,		mm1m6,	0,		mm1m6,		mm1,		0,		"Nokia Data",		"MikroMikko 1 M7",		GAME_NOT_WORKING)
