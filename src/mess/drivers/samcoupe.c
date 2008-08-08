/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton


    Sam Coupe Memory Map - Based around the current spectrum.c (for obvious reasons!!)

    CPU:
        0000-7fff Banked rom/ram
        8000-ffff Banked rom/ram


Interrupts:

Changes:

 V0.2   - Added FDC support. - Based on 1771 document. Coupe had a 1772... (any difference?)
            floppy supports only read sector single mode at present will add write sector
            in next version.
          Fixed up palette - had red & green wrong way round.


 KT 26-Aug-2000 - Changed to use wd179x code. This is the same as the 1772.
                - Coupe supports the basic disk image format, but can be changed in
                  the future to support others

Note on the bioses:
 SAM Coupé ROM Images
 --------------------

 This archive contains many versions of the SAM Coupé 32K ROM image, released
 with kind permission from the ROM author, Dr Andy Wright.

 Thanks to Simon N Goodwin for supplying the files, which include two dumped
 from pre-production hardware (and which only work with the early hardware!).
	
 Beware - the early ROMs are very buggy, and tend to go mad once BASIC starts
 paging.  ROM 10 (version 1.0) requires the CALL after F9 or BOOT because the
 ROM loads the bootstrap but fails to execute it. The address depends on the
 RAM size:

 On a 256K SAM:

    CALL 229385

 Or on a 512K (or larger) machine:

    CALL 491529

 --
 Uploaded by Simon Owen <simon.owen@simcoupe.org>

***************************************************************************/

/* core includes */
#include "driver.h"
#include "includes/samcoupe.h"

/* components */
#include "machine/wd17xx.h"
#include "sound/saa1099.h"
#include "sound/speaker.h"
#include "machine/msm6242.h"

/* devices */
#include "devices/mflopimg.h"
#include "formats/coupedsk.h"


#define SAMCOUPE_XTAL_X1  XTAL_24MHz
#define SAMCOUPE_XTAL_X2  XTAL_4_433619MHz



/*************************************
 *
 *  I/O port handling
 *
 *************************************/

static READ8_HANDLER( samcoupe_disk_r )
{
	/* drive and side is encoded into bit 5 and 3 */
	wd17xx_set_drive((offset >> 4) & 1);
	wd17xx_set_side((offset >> 2) & 1);

	/* bit 1 and 2 select the controller register */
	switch (offset & 0x03)
	{
	case 0: return wd17xx_status_r(machine, 0);
	case 1: return wd17xx_track_r(machine, 0);
	case 2: return wd17xx_sector_r(machine, 0);
	case 3: return wd17xx_data_r(machine, 0);
	}

	return 0xff;
}


static WRITE8_HANDLER( samcoupe_disk_w )
{
	/* drive and side is encoded into bit 5 and 3 */
	wd17xx_set_drive((offset >> 4) & 1);
	wd17xx_set_side((offset >> 2) & 1);

	/* bit 1 and 2 select the controller register */
	switch (offset & 0x03)
	{
	case 0: wd17xx_command_w(machine, 0, data); break;
	case 1: wd17xx_track_w(machine, 0, data);   break;
	case 2: wd17xx_sector_w(machine, 0, data);  break;
	case 3: wd17xx_data_w(machine, 0, data);    break;
	}
}


static READ8_HANDLER( samcoupe_pen_r )
{
	UINT8 data;

	if (offset & 0x100)
	{
		/* return either the current line or 192 for the vblank area */
		int line = video_screen_get_vpos(machine->primary_screen); 
		data = video_screen_get_vblank(machine->primary_screen) ? 192 : line;
	}
	else
	{
		/* horizontal position is encoded into bits 3 to 8 */
		data = video_screen_get_hpos(machine->primary_screen) & 0xfc;
	}

	return data;
}


static WRITE8_HANDLER( samcoupe_clut_w )
{	
	samcoupe_regs.clut[(offset >> 8) & 0x0f] = data & 0x7f;
}


static READ8_HANDLER( samcoupe_status_r )
{
	UINT8 data = 0xe0;
	UINT8 row = ~(offset >> 8);

	if (row & 0x80) data &= input_port_read(machine, "keyboard_row_7f") & 0xe0;
	if (row & 0x40) data &= input_port_read(machine, "keyboard_row_bf") & 0xe0;
	if (row & 0x20) data &= input_port_read(machine, "keyboard_row_df") & 0xe0;
	if (row & 0x10) data &= input_port_read(machine, "keyboard_row_ef") & 0xe0;
	if (row & 0x08) data &= input_port_read(machine, "keyboard_row_f7") & 0xe0;
	if (row & 0x04) data &= input_port_read(machine, "keyboard_row_fb") & 0xe0;
	if (row & 0x02) data &= input_port_read(machine, "keyboard_row_fd") & 0xe0;
	if (row & 0x01) data &= input_port_read(machine, "keyboard_row_fe") & 0xe0;

	return data | samcoupe_regs.status;
}


static WRITE8_HANDLER( samcoupe_line_int_w )
{
	samcoupe_regs.line_int = data;
}


static READ8_HANDLER( samcoupe_lmpr_r )
{
	return samcoupe_regs.lmpr;
}


static WRITE8_HANDLER( samcoupe_lmpr_w )
{
	samcoupe_regs.lmpr = data;
	samcoupe_update_memory(machine);
}


static READ8_HANDLER( samcoupe_hmpr_r )
{
	return samcoupe_regs.hmpr;
}


static WRITE8_HANDLER( samcoupe_hmpr_w )
{
	samcoupe_regs.hmpr = data;
	samcoupe_update_memory(machine);
}


static READ8_HANDLER( samcoupe_vmpr_r )
{
	return samcoupe_regs.vmpr;
}


static WRITE8_HANDLER( samcoupe_vmpr_w )
{
	samcoupe_regs.vmpr = data;
	samcoupe_update_memory(machine);
}


static READ8_HANDLER( samcoupe_midi_r )
{
	logerror("Read from midi port\n");
	return 0xff;
}


static WRITE8_HANDLER( samcoupe_midi_w )
{
	logerror("Write to midi port: 0x%02x\n", data);
}


static READ8_HANDLER( samcoupe_keyboard_r )
{
	UINT8 data = 0xff;
	UINT8 row = ~(offset >> 8);

	if (row == 0)
	{
		data &= input_port_read(machine, "keyboard_row_ff") & 0x1f;
	}
	else
	{
		if (row & 0x80) data &= input_port_read(machine, "keyboard_row_7f") & 0x1f;
		if (row & 0x40) data &= input_port_read(machine, "keyboard_row_bf") & 0x1f;
		if (row & 0x20) data &= input_port_read(machine, "keyboard_row_df") & 0x1f;
		if (row & 0x10) data &= input_port_read(machine, "keyboard_row_ef") & 0x1f;
		if (row & 0x08) data &= input_port_read(machine, "keyboard_row_f7") & 0x1f;
		if (row & 0x04) data &= input_port_read(machine, "keyboard_row_fb") & 0x1f;
		if (row & 0x02) data &= input_port_read(machine, "keyboard_row_fd") & 0x1f;
		if (row & 0x01) data &= input_port_read(machine, "keyboard_row_fe") & 0x1f;
	}

	return data | 0xe0;
}


static WRITE8_HANDLER( samcoupe_border_w )
{
	samcoupe_regs.border = data;

	/* DAC output state */
	speaker_level_w(0,(data >> 4) & 0x01);
}


static READ8_HANDLER( samcoupe_attributes_r )
{
	if (video_screen_get_vblank(machine->primary_screen))
	{
		/* Border areas return 0xff */
		return 0xff;
	}
	else
	{
		/* TODO: This actually needs to return various attributes
		 * of the currently displayed screen data */
		return 0x00;
	}
}


static WRITE8_HANDLER( samcoupe_sound_w )
{
	if (offset & 0x100)
		saa1099_control_port_0_w(machine, 0, data);
	else
		saa1099_write_port_0_w(machine, 0, data);
}



/*************************************
 *
 *  Address maps
 *
 *************************************/

static ADDRESS_MAP_START( samcoupe_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK(2)
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(3)
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK(4)
ADDRESS_MAP_END


static ADDRESS_MAP_START( samcoupe_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x80, 0x81) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_WRITE(samcoupe_ext_mem_w)
	AM_RANGE(0xe0, 0xe7) AM_MIRROR(0xff10) AM_MASK(0xffff) AM_READWRITE(samcoupe_disk_r, samcoupe_disk_w)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_pen_r, samcoupe_clut_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_status_r, samcoupe_line_int_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_lmpr_r, samcoupe_lmpr_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_hmpr_r, samcoupe_hmpr_w)
	AM_RANGE(0xfc, 0xfc) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_vmpr_r, samcoupe_vmpr_w)
	AM_RANGE(0xfd, 0xfd) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_midi_r, samcoupe_midi_w)
	AM_RANGE(0xfe, 0xfe) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_keyboard_r, samcoupe_border_w)
	AM_RANGE(0xff, 0xff) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_attributes_r, samcoupe_sound_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Interrupts
 *
 *************************************/

static TIMER_CALLBACK( irq_off )
{
	/* clear interrupt */
	cpunum_set_input_line(machine, 0, 0, CLEAR_LINE);
	
	/* adjust STATUS register */
	samcoupe_regs.status |= param;
}


void samcoupe_irq(running_machine *machine, UINT8 src)
{
	/* set irq and a timer to set it off again */
	cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
	timer_set(ATTOTIME_IN_USEC(20), NULL, src, irq_off);
	
	/* adjust STATUS register */
	samcoupe_regs.status &= ~src;
}


static INTERRUPT_GEN( samcoupe_frame_interrupt )
{
	/* signal frame interrupt */
	samcoupe_irq(machine, 0x08);
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( samcoupe )
	PORT_START("keyboard_row_fe")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)     PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)     PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR('?')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)     PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)     PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("keyboard_row_fd")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)     PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)     PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)     PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)     PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)     PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("keyboard_row_fb")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)     PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)     PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)     PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)     PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('[')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)     PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(']')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(F9))

	PORT_START("keyboard_row_f7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)        PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)        PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)        PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)        PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)        PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)    PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)      PORT_CHAR('\t')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("keyboard_row_ef")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)         PORT_CHAR('0') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)         PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)         PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)         PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)         PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)     PORT_CHAR('-') PORT_CHAR('/')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)    PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DELETE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)

	PORT_START("keyboard_row_df")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)          PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)          PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)          PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('=') PORT_CHAR('_')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('"') PORT_CHAR('\xa9')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(F10))

	PORT_START("keyboard_row_bf")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)     PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR('\xa3')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)     PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)     PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR('^')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START("keyboard_row_7f")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)     PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)     PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)     PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV") PORT_CODE(KEYCODE_SLASH) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CHAR('\\')

	PORT_START("keyboard_row_ff")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)     PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)   PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)   PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("config")
	PORT_CONFNAME(0x01, 0x00, "Real Time Clock")
	PORT_CONFSETTING(   0x00, DEF_STR(None))
	PORT_CONFSETTING(   0x01, "SAMBUS")
INPUT_PORTS_END



/*************************************
 *
 *  Palette
 *
 *************************************/

/*
    Decode colours for the palette as follows:

    bit     7       6       5       4       3       2       1       0
         nothing   G+4     R+4     B+4    ALL+1    G+2     R+2     B+2

    These values scaled up to 0-255 range would give modifiers of:

      +4 = +(4*36), +2 = +(2*36), +1 = *(1*36)

    Not quite max of 255 but close enough for me!
 */
static PALETTE_INIT( samcoupe )
{
	int i;

	for (i = 0; i < 128; i++)
	{
		UINT8 r = 0, g = 0, b = 0;

		/* colors */
		if (i & 0x01) b += 2*36;
		if (i & 0x02) r += 2*36;
		if (i & 0x04) g += 2*36;
		if (i & 0x10) b += 4*36;
		if (i & 0x20) r += 4*36;
		if (i & 0x40) g += 4*36;

		/* intensity bit */
		if (i & 0x08)
		{
			r += 1*36;
			g += 1*36;
			b += 1*36;
		}

		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( samcoupe )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, SAMCOUPE_XTAL_X1/4) /* 6 Mhz */
	MDRV_CPU_PROGRAM_MAP(samcoupe_mem, 0)
	MDRV_CPU_IO_MAP(samcoupe_io, 0)
	MDRV_CPU_VBLANK_INT("main", samcoupe_frame_interrupt)

	MDRV_MACHINE_START(samcoupe)
	MDRV_MACHINE_RESET(samcoupe)

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_RAW_PARAMS(SAMCOUPE_XTAL_X1/2, 768, 0, 512, 312, 0, 192) /* border area? */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(samcoupe)

	MDRV_VIDEO_UPDATE(samcoupe)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_SCANLINE)

	/* devices */
	MDRV_DEVICE_ADD("sambus_clock", MSM6242)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099", SAA1099, SAMCOUPE_XTAL_X1/3) /* 8 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

/* 
	The bios is actually 32K. This is the combined version of the two old 16K MESS roms.
	It does match the 3.0 one the most, but the first half differs in one byte 
	and in the second half, the case of the "plc" in the company string differs.
*/
ROM_START( samcoupe )
	ROM_REGION( 0x8000, "main", 0 )
	ROM_SYSTEM_BIOS( 0,  "31",  "v3.1" )
	ROMX_LOAD( "rom31.z5",  0x0000, 0x8000, CRC(0b7e3585) SHA1(c86601633fb61a8c517f7657aad9af4e6870f2ee), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1,  "30",  "v3.0" )
	ROMX_LOAD( "rom30.z5",  0x0000, 0x8000, CRC(e535c25d) SHA1(d390f0be420dfb12b1e54a4f528b5055d7d97e2a), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2,  "25",  "v2.5" )
	ROMX_LOAD( "rom25.z5",  0x0000, 0x8000, CRC(ddadd358) SHA1(a25ed85a0f1134ac3a481a3225f24a8bd3a790cf), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3,  "24",  "v2.4" )
	ROMX_LOAD( "rom24.z5",  0x0000, 0x8000, CRC(bb23fee4) SHA1(10cd911ba237dd2cf0c2637be1ad6745b7cc89b9), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4,  "21",  "v2.1" )
	ROMX_LOAD( "rom21.z5",  0x0000, 0x8000, CRC(f6804b46) SHA1(11dcac5fdea782cdac03b4d0d7ac25d88547eefe), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS( 5,  "20",  "v2.0" )
	ROMX_LOAD( "rom20.z5",  0x0000, 0x8000, CRC(eaf32054) SHA1(41736323f0236649f2d5fe111f900def8db93a13), ROM_BIOS(6) )
	ROM_SYSTEM_BIOS( 6,  "181", "v1.81" )
	ROMX_LOAD( "rom181.z5", 0x0000, 0x8000, CRC(d25e1de1) SHA1(cb0fa79e4d5f7df0b57ede08ea7ecc9ae152f534), ROM_BIOS(7) )
	ROM_SYSTEM_BIOS( 7,  "18",  "v1.8" )
	ROMX_LOAD( "rom18.z5",  0x0000, 0x8000, CRC(f626063f) SHA1(485e7d9e9a4f8a70c0f93cd6e69ff12269438829), ROM_BIOS(8) )
	ROM_SYSTEM_BIOS( 8,  "14",  "v1.4" )
	ROMX_LOAD( "rom14.z5",  0x0000, 0x8000, CRC(08799596) SHA1(b4e596051f2748dee9481ea4af7d15ccddc1e1b5), ROM_BIOS(9) )
	ROM_SYSTEM_BIOS( 9,  "13",  "v1.3" )
	ROMX_LOAD( "rom13.z5",  0x0000, 0x8000, CRC(2093768c) SHA1(af8d348fd080b18a4cbe9ed69d254be7be330146), ROM_BIOS(10) )
	ROM_SYSTEM_BIOS( 10, "12",  "v1.2" )
	ROMX_LOAD( "rom12.z5",  0x0000, 0x8000, CRC(7fe37dd8) SHA1(9339a0c1f72e8512c6f32dec15ab7d6c3bb04151), ROM_BIOS(11) )
	ROM_SYSTEM_BIOS( 11, "10",  "v1.0" )
	ROMX_LOAD( "rom10.z5",  0x0000, 0x8000, CRC(3659d31f) SHA1(d3de7bb74e04d5b4dc7477f70de54d540b1bcc07), ROM_BIOS(12) )
	ROM_SYSTEM_BIOS( 12, "04",  "v0.4" )
	ROMX_LOAD( "rom04.z5",  0x0000, 0x8000, CRC(f439e84e) SHA1(8bc457a5c764b0bb0aa7008c57f28c30248fc6a4), ROM_BIOS(13) )
	ROM_SYSTEM_BIOS( 13, "01",  "v0.1" )
	ROMX_LOAD( "rom01.z5",  0x0000, 0x8000, CRC(c04acfdf) SHA1(8976ed005c14905eec1215f0a5c28aa686a7dda2), ROM_BIOS(14) )
ROM_END



/*************************************
 *
 *  Devices
 *
 *************************************/

static void samcoupe_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:	info->p = (void *) floppyoptions_coupe; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_NAME+0:			strcpy(info->s = device_temp_str(), "floppydisk0"); break;
		case MESS_DEVINFO_STR_NAME+1:			strcpy(info->s = device_temp_str(), "floppydisk1"); break;
		case MESS_DEVINFO_STR_SHORT_NAME+0:		strcpy(info->s = device_temp_str(), "flop0"); break;
		case MESS_DEVINFO_STR_SHORT_NAME+1:		strcpy(info->s = device_temp_str(), "flop1"); break;
		case MESS_DEVINFO_STR_DESCRIPTION+0:	strcpy(info->s = device_temp_str(), "Floppy #0"); break;
		case MESS_DEVINFO_STR_DESCRIPTION+1:	strcpy(info->s = device_temp_str(), "Floppy #1"); break;

		default:								floppy_device_getinfo(devclass, state, info); break;
	}
}


static SYSTEM_CONFIG_START(samcoupe)
	CONFIG_RAM(256 * 1024)
	CONFIG_RAM_DEFAULT(512 * 1024)
	CONFIG_RAM(256 * 1024 + 1 * 1024 * 1024)
	CONFIG_RAM(512 * 1024 + 1 * 1024 * 1024)
	CONFIG_RAM(256 * 1024 + 2 * 1024 * 1024)
	CONFIG_RAM(512 * 1024 + 2 * 1024 * 1024)
	CONFIG_RAM(256 * 1024 + 3 * 1024 * 1024)
	CONFIG_RAM(512 * 1024 + 3 * 1024 * 1024)
	CONFIG_RAM(256 * 1024 + 4 * 1024 * 1024)
	CONFIG_RAM(512 * 1024 + 4 * 1024 * 1024)
	CONFIG_DEVICE(samcoupe_floppy_getinfo)
SYSTEM_CONFIG_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/*    YEAR  NAME      PARENT  COMPAT  MACHINE  INPUT  INIT   CONFIG  COMPANY                        FULLNAME     FLAGS */
COMP( 1989, samcoupe, 0,      0,   samcoupe,  samcoupe, 0,   samcoupe,  "Miles Gordon Technology plc", "Sam Coupe", 0 )
