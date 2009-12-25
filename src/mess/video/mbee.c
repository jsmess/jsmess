/***************************************************************************
    microbee.c

    video hardware
    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "includes/mbee.h"


typedef struct {		 // CRTC 6545
	UINT8 horizontal_total;
    UINT8 horizontal_displayed;
    UINT8 horizontal_sync_pos;
    UINT8 horizontal_length;
    UINT8 vertical_total;
    UINT8 vertical_adjust;
    UINT8 vertical_displayed;
    UINT8 vertical_sync_pos;
    UINT8 crt_mode;
    UINT8 scan_lines;
    UINT8 cursor_top;
    UINT8 cursor_bottom;
    UINT8 screen_address_hi;
    UINT8 screen_address_lo;
    UINT8 cursor_address_hi;
    UINT8 cursor_address_lo;
	UINT8 lpen_hi;
	UINT8 lpen_lo;
	UINT8 transp_hi;
	UINT8 transp_lo;
    UINT8 idx;
	UINT8 cursor_visible;
	UINT8 cursor_phase;
	UINT8 lpen_strobe;
	UINT8 update_strobe;
} CRTC6545;

static CRTC6545 crt;
static UINT8 framecnt = 0;
static UINT8 m6545_color_bank = 0;
static UINT8 m6545_video_bank = 0;
static UINT8 mbee_pcg_color_latch = 0;

UINT8 *mbee_pcgram;
static UINT8 *colorram;
static UINT8 mc6845_cursor[16];				// cursor shape
static void mc6845_cursor_configure(void);
static void mc6845_screen_configure(running_machine *machine);

WRITE8_HANDLER ( mbee_pcg_color_latch_w )
{
	mbee_pcg_color_latch = data;
	if (data & 0x40)
		memory_set_bank(space->machine, "bank3", 1);
	else
		memory_set_bank(space->machine, "bank3", 0);
}

READ8_HANDLER ( mbee_pcg_color_latch_r )
{
	return mbee_pcg_color_latch;
}

WRITE8_HANDLER ( mbee_videoram_w )
{
	space->machine->generic.videoram.u8[offset] = data;
}

WRITE8_HANDLER ( mbee_pcg_w )
{
	mbee_pcgram[0x0800 | offset] = data;
}

WRITE8_HANDLER ( mbee_pcg_color_w )
{
	if( (m6545_video_bank & 0x01) || (mbee_pcg_color_latch & 0x40) == 0 )
		mbee_pcgram[0x0800 | offset] = data;
	else
		colorram[offset] = data;
}

static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

/* The direction keys are used by the pc85 menu. Do not know what uses the "insert" key. */
static int keyboard_matrix_r(running_machine *machine, int offs)
{
	int port = (offs >> 7) & 7;
	int bit = (offs >> 4) & 7;
	int extra = input_port_read(machine, "EXTRA");
	int data = 0;

	data = (input_port_read(machine, keynames[port]) >> bit) & 1;

	if( extra & 0x01 )	/* extra: cursor up */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 5 ) data = 1;	/* E */
	}
	if( extra & 0x02 )	/* extra: cursor down */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 3 && bit == 0 ) data = 1;	/* X */
	}
	if( extra & 0x04 )	/* extra: cursor left */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 3 ) data = 1;	/* S */
	}
	if( extra & 0x08 )	/* extra: cursor right */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 4 ) data = 1;	/* D */
	}
	if( extra & 0x10 )	/* extra: insert */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 6 ) data = 1;	/* V */
	}

	if( data )
	{
		crt.lpen_lo = offs & 0xff;
		crt.lpen_hi = (offs >> 8) & 0x03;
		crt.lpen_strobe = 1;
//      logerror("mbee keyboard_matrix_r $%03X (port:%d bit:%d) = %d\n", offs, port, bit, data);
	}
	return data;
}

READ8_HANDLER ( mbee_color_bank_r )
{
/* Read of port 0A - set Telcom rom to first half */
	memory_set_bank(space->machine, "bank5", 0);
	return m6545_color_bank;
}

WRITE8_HANDLER ( mbee_color_bank_w )
{
	m6545_color_bank = data;
	memory_set_bank(space->machine, "bank4", data & 7);
}

WRITE8_HANDLER ( mbee_0a_w )
{
	m6545_color_bank = data;
	memory_set_bank(space->machine, "bank4", (data&15) >> 1);
}

READ8_HANDLER ( mbee_bank_netrom_r )
{
/* Read of port 10A - set Telcom rom to 2nd half */
	memory_set_bank(space->machine, "bank5", 1);
	return m6545_color_bank;
}

WRITE8_HANDLER( mbee_1c_w )
{
/*  d7 extended graphics - not emulated
    d5 bankswitch basic rom
    d4 select attribute ram - not emulated
    d3..d0 select videoram bank - not emulated */

	memory_set_bank(space->machine, "bank6", (data & 0x20) ? 1 : 0);
}

READ8_HANDLER ( mbee_video_bank_r )
{
	return m6545_video_bank;
}

WRITE8_HANDLER ( mbee_video_bank_w )
{
	m6545_video_bank = data;
	if (data & 1)
		memory_set_bank(space->machine, "bank2", 0);
	else
		memory_set_bank(space->machine, "bank2", 1);
}

static void m6545_update_strobe(running_machine *machine, int param)
{
	/*int data = */keyboard_matrix_r(machine, param);
	crt.update_strobe = 1;
//  if( data )
//      logerror("6545 update_strobe_cb $%04X = $%02X\n", param, data);
}

READ8_HANDLER ( m6545_status_r )
{
	const device_config *screen = video_screen_first(space->machine->config);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	int data = 0, y = video_screen_get_vpos(space->machine->primary_screen);

	if( y < visarea->min_y ||
		y > visarea->max_y )
		data |= 0x20;	/* vertical blanking */
	if( crt.lpen_strobe )
		data |= 0x40;	/* lpen register full */
	if( crt.update_strobe )
		data |= 0x80;	/* update strobe has occured */
//  logerror("6545 status_r $%02X\n", data);
	return data;
}

 READ8_HANDLER ( m6545_data_r )
{
	int addr, data = 0;

	switch( crt.idx )
	{
/* These are write only on a Rockwell 6545 */
#if 0
	case 0:
		return crt.horizontal_total;
	case 1:
		return crt.horizontal_displayed;
	case 2:
		return crt.horizontal_sync_pos;
	case 3:
		return crt.horizontal_length;
	case 4:
		return crt.vertical_total;
	case 5:
		return crt.vertical_adjust;
	case 6:
		return crt.vertical_displayed;
	case 7:
		return crt.vertical_sync_pos;
	case 8:
		return crt.crt_mode;
	case 9:
		return crt.scan_lines;
	case 10:
		return crt.cursor_top;
	case 11:
		return crt.cursor_bottom;
	case 12:
		return crt.screen_address_hi;
	case 13:
		return crt.screen_address_lo;
#endif
	case 14:
		data = crt.cursor_address_hi;
		break;
	case 15:
		data = crt.cursor_address_lo;
		break;
	case 16:
//      logerror("6545 lpen_hi_r $%02X (lpen:%d upd:%d)\n", crt.lpen_hi, crt.lpen_strobe, crt.update_strobe);
		crt.lpen_strobe = 0;
		crt.update_strobe = 0;
		data = crt.lpen_hi;
		break;
	case 17:
//      logerror("6545 lpen_lo_r $%02X (lpen:%d upd:%d)\n", crt.lpen_lo, crt.lpen_strobe, crt.update_strobe);
		crt.lpen_strobe = 0;
		crt.update_strobe = 0;
		data = crt.lpen_lo;
		break;
	case 18:
//      logerror("6545 transp_hi_r $%02X\n", crt.transp_hi);
		data = crt.transp_hi;
		break;
	case 19:
//      logerror("6545 transp_lo_r $%02X\n", crt.transp_lo);
		data = crt.transp_lo;
		break;
	case 31:
		/* shared memory latch */
		addr = (crt.transp_hi << 8) | crt.transp_lo;
//      logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(space->machine, addr);
		break;
	default:
		logerror("6545 read unmapped port $%X\n", crt.idx);
	}
	return data;
}

WRITE8_HANDLER ( m6545_index_w )
{
	crt.idx = data & 0x1f;
}

WRITE8_HANDLER ( m6545_data_w )
{
	int addr;

	switch( crt.idx )
	{
	case 0:
		if( crt.horizontal_total == data )
			break;
		crt.horizontal_total = data;
		break;
	case 1:
		crt.horizontal_displayed = data;
		mc6845_screen_configure(space->machine);
		break;
	case 2:
		if( crt.horizontal_sync_pos == data )
			break;
		crt.horizontal_sync_pos = data;
		break;
	case 3:
		crt.horizontal_length = data;
		break;
	case 4:
		if( crt.vertical_total == data )
			break;
		crt.vertical_total = data;
		break;
	case 5:
		if( crt.vertical_adjust == data )
			break;
		crt.vertical_adjust = data;
		break;
	case 6:
		crt.vertical_displayed = data;
		mc6845_screen_configure(space->machine);
		break;
	case 7:
		if( crt.vertical_sync_pos == data )
			break;
		crt.vertical_sync_pos = data;
		break;
	case 8:
		crt.crt_mode = data;

		{
			logerror("6545 mode_w $%02X\n", data);
			logerror("     interlace          %d\n", data & 3);
			logerror("     addr mode          %d\n", (data >> 2) & 1);
			logerror("     refresh RAM        %s\n", ((data >> 3) & 1) ? "transparent" : "shared");
			logerror("     disp enb, skew     %d\n", (data >> 4) & 3);
			logerror("     pin 34             %s\n", ((data >> 6) & 1) ? "update strobe" : "RA4");
			logerror("     update read mode   %s\n", ((data >> 7) & 1) ? "interleaved" : "during h/v-blank");
		}
		break;
	case 9:
		data &= 15;
		if( crt.scan_lines == data )
			break;
		crt.scan_lines = data;
		mc6845_screen_configure(space->machine);
		mc6845_cursor_configure();
		break;
	case 10:
		crt.cursor_top = data;
		mc6845_cursor_configure();
		break;
	case 11:
		crt.cursor_bottom = data;
		mc6845_cursor_configure();
		break;
	case 12:
		data &= 0x3f;
		if( crt.screen_address_hi == data )
			break;
		crt.screen_address_hi = data;
		addr = 0x17000+((data & 32) << 6);
		memcpy(mbee_pcgram, memory_region(space->machine, "maincpu")+addr, 0x800);
		break;
	case 13:
		crt.screen_address_lo = data;
		break;
	case 14:
		crt.cursor_address_hi = data & 0x3f;
		break;
	case 15:
		crt.cursor_address_lo = data;
		break;
	case 16:
		/* lpen hi is read only */
		break;
	case 17:
		/* lpen lo is read only */
		break;
	case 18:
		data &= 63;
		crt.transp_hi = data;
//      logerror("6545 transp_hi_w $%02X\n", data);
		break;
	case 19:
		crt.transp_lo = data;
//      logerror("6545 transp_lo_w $%02X\n", data);
		break;
	case 31:
		/* shared memory latch */
		addr = (crt.transp_hi << 8) | crt.transp_lo;
//      logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(space->machine, addr);
		break;
	default:
		logerror("6545 write unmapped port $%X <- $%02X\n", crt.idx, data);
	}
}

/* The 6845 can produce a variety of cursor shapes - all are emulated here
    Need to find out if the 6545 works the same way */
static void mc6845_cursor_configure(void)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
        0 = no cursor
        1 = partial cursor (only shows on a block of scan lines)
        2 = full cursor
        3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(mc6845_cursor); i++) mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = crt.scan_lines;					// number of scan lines - 1
	r10 = crt.cursor_top & 0x1f;				// cursor start line = last 5 bits
	r11 = crt.cursor_bottom+1;				// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(mc6845_cursor);i++) mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) mc6845_cursor[i]=0; // now take a bite out of the middle
}

/* Resize the screen within the limits of the hardware. Expand the image to fill the screen area */
static void mc6845_screen_configure(running_machine *machine)
{
	rectangle visarea;

	UINT16 width = crt.horizontal_displayed*8-1;							// width in pixels
	UINT16 height = crt.vertical_displayed*(crt.scan_lines+1)-1;					// height in pixels
	UINT16 bytes = crt.horizontal_displayed*crt.vertical_displayed-1;				// video ram needed

	if (width > 0x27f) width=0x27f;
	if (height > 0x10f) height=0x10f;

	/* Resize the screen */
	visarea.min_x = 0;
	visarea.max_x = width-1;
	visarea.min_y = 0;
	visarea.max_y = height-1;
	if (bytes < 0x800) video_screen_set_visarea(machine->primary_screen, 0, width, 0, height);
}

VIDEO_START( mbee )
{
	UINT8 *ram = memory_region(machine, "maincpu");
	machine->generic.videoram.u8 = ram+0x15000;
	mbee_pcgram = ram+0x11000;
}

VIDEO_START( mbeeic )
{
	UINT8 *ram = memory_region(machine, "maincpu");
	machine->generic.videoram.u8 = ram+0x15000;
	colorram = ram+0x15800;
	mbee_pcgram = ram+0x11000;
}

VIDEO_UPDATE( mbee )
{
	UINT8 y,ra,chr,gfx;
	UINT16 mem,sy=0,ma=0,x;
	UINT8 speed = crt.cursor_top&0x20, flash = crt.cursor_top&0x40;				// cursor modes
	UINT16 cursor = (crt.cursor_address_hi<<8) | crt.cursor_address_lo;			// get cursor position
	UINT16 screen_home = (crt.screen_address_hi<<8) | crt.screen_address_lo;		// screen home offset (usually zero)

	framecnt++;

	for (y = 0; y < crt.vertical_displayed; y++)						// for each row of chars
	{
		for (ra = 0; ra < (crt.scan_lines&15)+1; ra++)					// for each scanline - RA4 is used as an update strobe for the keyboard
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + crt.horizontal_displayed; x++)			// for each character
			{
				UINT8 inv=0;
				mem = (x + screen_home) & 0x7ff;
				chr = screen->machine->generic.videoram.u8[mem];
				if ((x & 15) == 0) keyboard_matrix_r(screen->machine, x);	// actually happens for every scanline of every character, but imposes too much unnecessary overhead */
				/* process cursor */
				if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
					((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
					((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
					(mem == cursor))					// displaying at cursor position?
						inv ^= mc6845_cursor[ra];			// cursor scan row

				/* get pattern of pixels for that character scanline */
				gfx = mbee_pcgram[(chr<<4) | ra] ^ inv;

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=crt.horizontal_displayed;
	}
	return 0;
}

VIDEO_UPDATE( mbeeic )
{
	UINT8 y,ra,chr,gfx,fg,bg;
	UINT16 mem,sy=0,ma=0,x,col;
	UINT8 speed = crt.cursor_top&0x20, flash = crt.cursor_top&0x40;				// cursor modes
	UINT16 cursor = (crt.cursor_address_hi<<8) | crt.cursor_address_lo;			// get cursor position
	UINT16 screen_home = (crt.screen_address_hi<<8) | crt.screen_address_lo;		// screen home offset (usually zero)
	UINT16 colourm = (mbee_pcg_color_latch & 0x0e) << 7;

	framecnt++;

	for (y = 0; y < crt.vertical_displayed; y++)						// for each row of chars
	{
		for (ra = 0; ra < (crt.scan_lines&15)+1; ra++)					// for each scanline - RA4 is used as an update strobe for the keyboard
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + crt.horizontal_displayed; x++)			// for each character
			{
				UINT8 inv=0;
				mem = (x + screen_home) & 0x7ff;
				chr = screen->machine->generic.videoram.u8[mem];
				col = colorram[mem] | colourm;					// read a byte of colour
				if ((x & 15) == 0) keyboard_matrix_r(screen->machine, x);	// actually happens for every scanline of every character, but imposes too much unnecessary overhead */

				/* process cursor */
				if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
					((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
					((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
					(mem == cursor))					// displaying at cursor position?
						inv ^= mc6845_cursor[ra];			// cursor scan row

				/* get pattern of pixels for that character scanline */
				gfx = mbee_pcgram[(chr<<4) | ra] ^ inv;
				fg = (col & 0x001f) | 64;					// map to foreground palette
				bg = (col & 0x07e0) >> 5;					// and background palette

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? fg : bg; p++;
				*p = ( gfx & 0x40 ) ? fg : bg; p++;
				*p = ( gfx & 0x20 ) ? fg : bg; p++;
				*p = ( gfx & 0x10 ) ? fg : bg; p++;
				*p = ( gfx & 0x08 ) ? fg : bg; p++;
				*p = ( gfx & 0x04 ) ? fg : bg; p++;
				*p = ( gfx & 0x02 ) ? fg : bg; p++;
				*p = ( gfx & 0x01 ) ? fg : bg; p++;
			}
		}
		ma+=crt.horizontal_displayed;
	}
	return 0;
}

PALETTE_INIT( mbeeic )
{
	UINT16 i;
	UINT8 r, b, g, k;
	UINT8 level[] = { 0, 0x80, 0xff, 0xff };	/* off, half, full intensity */

	/* set up background palette (00-63) */
	for (i = 0; i < 64; i++)
	{
		r = level[((i>>0)&1)|((i>>2)&2)];
		g = level[((i>>1)&1)|((i>>3)&2)];
		b = level[((i>>2)&1)|((i>>4)&2)];
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	/* set up foreground palette (64-95) by reading the prom */
	for (i = 0; i < 32; i++)
	{
		k = color_prom[i];
		r = level[((k>>2)&1)|((k>>4)&2)];
		g = level[((k>>1)&1)|((k>>3)&2)];
		b = level[((k>>0)&1)|((k>>2)&2)];
		palette_set_color(machine, i|64, MAKE_RGB(r, g, b));
	}
}
