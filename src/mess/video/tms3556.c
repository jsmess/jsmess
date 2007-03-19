/*
	tms3556 emulation

	TODO:
	* implement remaining flags in control registers
	* test the whole thing
	* find the bloody tms3556 manual.  I mean the register and VRAM interfaces
	  are mostly guesswork full of hacks, and I'd like to compare it with
	  documentation.

	Raphael Nabet, 2004
*/

#include "driver.h"
#include "tms3556.h"

static struct
{
	/* registers */
	UINT8 controlRegs[8];
	UINT16 addressRegs[8];
	UINT16 writePtr;
	/* register interface */
	int reg_ptr;
	int reg_access_phase;
	int magical_mystery_flag;
	/* memory */
	UINT8 *vram;
	int vram_size;
    /* scanline counter */
	int scanline;
    /* blinking */
    int blink, blink_count;
    /* background color for current line */
    int bg_color;
    /* current offset in name table */
	int name_offset;
	/* c/g flag (mixed mode only) */
	int cg_flag;
	/* character line counter (decrements from 10, 0 when we have reached last
	line of character row) */
	int char_line_counter;
	/* double height phase flags (one per horizontal character position) */
	int dbl_h_phase[40];
} vdp;

static mame_bitmap *tmpbitmap;

#define TOP_BORDER 1
#define BOTTOM_BORDER 1
#define LEFT_BORDER 8
#define RIGHT_BORDER 8
#define TOTAL_WIDTH (320 + LEFT_BORDER + RIGHT_BORDER)
#define TOTAL_HEIGHT (250 + TOP_BORDER + BOTTOM_BORDER)

/* if DOUBLE_WIDTH set, the horizontal resolution is doubled */
#define DOUBLE_WIDTH 1

#define TMS3556_MODE_OFF	0
#define TMS3556_MODE_TEXT	1
#define TMS3556_MODE_BITMAP	2
#define TMS3556_MODE_MIXED	3

/*static const char *tms3556_mode_names[] = { "DISPLAY OFF", "TEXT", "GRAPHIC", "MIXED" };*/

static PALETTE_INIT(tms3556);
static VIDEO_UPDATE(tms3556);


#if 0
#pragma mark BUS INTERFACE
#endif
/*
	bus interface
*/

/*
	tms3556_vram_r

	read a byte from tms3556 VRAM port
*/
READ8_HANDLER(tms3556_vram_r)
{
	int reply;

	if (vdp.magical_mystery_flag)
	{
		vdp.writePtr = ((vdp.controlRegs[2] << 8) | vdp.controlRegs[1]) + 1;
		vdp.magical_mystery_flag = 0;
	}

	reply = vdp.vram[vdp.addressRegs[1]];
	vdp.addressRegs[1] ++;

	return reply;
}

/*
	tms3556_vram_w

	write a byte to tms3556 VRAM port
*/
WRITE8_HANDLER(tms3556_vram_w)
{
	if (vdp.magical_mystery_flag)
	{
		vdp.writePtr = (vdp.controlRegs[2] << 8) | vdp.controlRegs[1];
		vdp.magical_mystery_flag = 0;
	}

	vdp.vram[vdp.writePtr] = data;
	vdp.writePtr++;
}

/*
	tms3556_reg_r

	read a byte from tms3556 register port
*/
READ8_HANDLER(tms3556_reg_r)
{
	int reply = 0;

	if (vdp.reg_ptr < 8)
	{
		reply = vdp.controlRegs[vdp.reg_ptr];
		vdp.reg_access_phase = 0;
	}
	/*else
	{	// ???
	}*/

	return reply;
}

/*
	tms3556_reg_w

	write a byte to tms3556 register port
*/
WRITE8_HANDLER(tms3556_reg_w)
{
	if ((vdp.reg_access_phase == 3) && (data))
		vdp.reg_access_phase = 0;	/* ???????????? */
	switch (vdp.reg_access_phase)
	{
	case 0:
		vdp.reg_ptr = data & 0x0f;
		vdp.reg_access_phase = 1;
		break;
	case 1:
		if (vdp.reg_ptr < 8)
		{
			vdp.controlRegs[vdp.reg_ptr] = data;
			vdp.reg_access_phase = 0;
			if (vdp.reg_ptr == 2)
				vdp.magical_mystery_flag = 1;
		}
		else if (vdp.reg_ptr == 9)
		{	/* I don't understand what is going on, but it is the only way to
			get this to work */
			vdp.addressRegs[vdp.reg_ptr-8] = ((vdp.controlRegs[2] << 8) | vdp.controlRegs[1]) + 1;
			vdp.reg_access_phase = 0;
			vdp.magical_mystery_flag = 0;
		}
		else
		{
			vdp.addressRegs[vdp.reg_ptr-8] = (vdp.addressRegs[vdp.reg_ptr-8] & 0xff00) | vdp.controlRegs[1];
			vdp.reg_access_phase = 2;
			vdp.magical_mystery_flag = 0;
		}
		break;
	case 2:
		vdp.addressRegs[vdp.reg_ptr-8] = (vdp.addressRegs[vdp.reg_ptr-8] & 0x00ff) | (vdp.controlRegs[2] << 8);
		if ((vdp.reg_ptr <= 10) || (vdp.reg_ptr == 15))
			vdp.addressRegs[vdp.reg_ptr-8] ++;
		else
			vdp.addressRegs[vdp.reg_ptr-8] += 2;
		vdp.reg_access_phase = 3;
		break;
	case 3:
		vdp.reg_access_phase = 0;
		break;
	}
}


#if 0
#pragma mark -
#pragma mark INITIALIZATION CODE
#endif
/*
	initialization code
*/

/*
	mdrv_tms3556

	machine struct initialization helper
*/
void mdrv_tms3556(machine_config *machine)
{
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
#if DOUBLE_WIDTH
	MDRV_SCREEN_SIZE(TOTAL_WIDTH*2, TOTAL_HEIGHT*2)
	MDRV_SCREEN_VISIBLE_AREA(0, TOTAL_WIDTH*2-1, 0, TOTAL_HEIGHT*2-1)
#else
	MDRV_SCREEN_SIZE(TOTAL_WIDTH, TOTAL_HEIGHT*2)
	MDRV_SCREEN_VISIBLE_AREA(0, TOTAL_WIDTH-1, 0, TOTAL_HEIGHT*2-1)
#endif
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(tms3556)

	/*MDRV_VIDEO_START(tms3556)*/
	MDRV_VIDEO_UPDATE(tms3556)
}

/*
	palette_init_tms3556

	Create our 8-color RGB palette
*/
static PALETTE_INIT(tms3556)
{
	int	i, red, green, blue;

	/* create the 8 color palette */
	for (i=0;i<8;i++)
	{
		red = (i & 1) ? 255 : 0;	/* red */
		green = (i & 2) ? 255 : 0;	/* green */
		blue = (i & 4) ? 255 : 0;	/* blue */
		palette_set_color(machine, i, red, green, blue);
	}
}

/*
	tms3556_init

	tms3556 core init (called at video init time)
*/
int tms3556_init(int vram_size)
{
	memset(&vdp, 0, sizeof (vdp));

	vdp.vram_size = vram_size;

	tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	if (!tmpbitmap)
		return 1;

	/* allocate VRAM */
	vdp.vram = auto_malloc(0x10000);
	memset (vdp.vram, 0, 0x10000);
	if (vdp.vram_size < 0x10000)
	{
		/* set unavailable RAM to 0xff */
		memset(vdp.vram + vdp.vram_size, 0xff, (0x10000 - vdp.vram_size) );
	}

	return 0;
}

/*
	tms3556_reset

	tms3556 reset (should be called at machine init time)
*/
void tms3556_reset(void)
{
	int i;

	/* register reset */
	for (i=0;i<8;i++)
	{
		vdp.controlRegs[i] = 0;
		vdp.addressRegs[i] = 0;
	}
	vdp.writePtr = 0;
	vdp.reg_ptr = 0;
	vdp.reg_access_phase = 0;
	vdp.magical_mystery_flag = 0;
	vdp.scanline = 0;
}


#if 0
#pragma mark -
#pragma mark REDRAW CODE
#endif
/*
	redraw code
*/

/*
	tms3556_draw_line_empty

	draw an empty line (used for top and bottom borders, and screen off mode)
*/
static void tms3556_draw_line_empty(UINT16 *ln)
{
	int	i;

	for (i=0; i<TOTAL_WIDTH; i++)
#if DOUBLE_WIDTH
		*ln++ =
#endif
			*ln++ = vdp.bg_color;
}

/*
	tms3556_draw_line_text_common

	draw a line of text (called by tms3556_draw_line_text and
	tms3556_draw_line_mixed)
*/
static void tms3556_draw_line_text_common(UINT16 *ln)
{
	int pattern, x, xx, i, name_offset;
	UINT16 fg, bg;
	UINT8 *nametbl, *patterntbl[4];
	int name_hi, name_lo;
	int pattern_ix;
	int alphanumeric_mode, dbl_w, dbl_h, dbl_w_phase = 0;


	nametbl = vdp.vram + vdp.addressRegs[2];
	for (i=0; i<4; i++)
		patterntbl[i] = vdp.vram + vdp.addressRegs[i+3];

	for (xx=0; xx<LEFT_BORDER; xx++)
#if DOUBLE_WIDTH
		*ln++ =
#endif
			*ln++ = vdp.bg_color;

	name_offset = vdp.name_offset;

	for (x=0;x<40;x++)
	{
		name_hi = nametbl[name_offset];
		name_lo = nametbl[name_offset+1];
		pattern_ix = ((name_hi >> 2) & 2) | ((name_hi >> 4) & 1);
		alphanumeric_mode = (pattern_ix < 2) || ((pattern_ix == 3) && !(vdp.controlRegs[7] & 0x08));
		fg = Machine->pens[(name_hi >> 5) & 0x7];
		if (alphanumeric_mode)
		{
			if (name_hi & 4)
			{	/* inverted color */
				bg = fg;
				fg = vdp.bg_color;
			}
			else
				bg = vdp.bg_color;
			dbl_w = name_hi & 0x2;
			dbl_h = name_hi & 0x1;
		}
		else
		{
			bg = name_hi & 0x7;
			dbl_w = 0;
			dbl_h = 0;
		}
		if ((name_lo & 0x80) && vdp.blink)
			fg = bg;	/* blink off time */
		if (! dbl_h)
		{	/* single height */
			pattern = patterntbl[pattern_ix][(name_lo & 0x7f) + 128*vdp.char_line_counter];
			if (vdp.char_line_counter == 0)
				vdp.dbl_h_phase[x] = 0;
		}
		else
		{	/* double height */
			if (! vdp.dbl_h_phase[x])
				/* first phase: pattern from upper half */
				pattern = patterntbl[pattern_ix][(name_lo & 0x7f) + 128*(5+(vdp.char_line_counter>>1))];
			else
				/* second phase: pattern from lower half */
				pattern = patterntbl[pattern_ix][(name_lo & 0x7f) + 128*(vdp.char_line_counter>>1)];
			if (vdp.char_line_counter == 0)
				vdp.dbl_h_phase[x] = ! vdp.dbl_h_phase[x];
		}
		if (! dbl_w)
		{	/* single width */
			for (xx=0;xx<8;xx++)
			{
#if DOUBLE_WIDTH
				*ln++ =
#endif
					*ln++ = (pattern & 0x80) ? fg : bg;
				pattern <<= 1;
			}
			dbl_w_phase = 0;
		}
		else
		{	/* double width */
			if (dbl_w_phase)
				/* second phase: display right half */
				pattern <<= 4;
			for (xx=0;xx<4;xx++)
			{
#if DOUBLE_WIDTH
				*ln++ = *ln++ =
#endif
					*ln++ = *ln++ = (pattern & 0x80) ? fg : bg;
				pattern <<= 1;
			}
			dbl_w_phase = !dbl_w_phase;
		}
		name_offset += 2;
	}

	for (xx=0; xx<RIGHT_BORDER; xx++)
#if DOUBLE_WIDTH
		*ln++ =
#endif
			*ln++ = vdp.bg_color;

	if (vdp.char_line_counter == 0)
		vdp.name_offset = name_offset;
}

/*
	tms3556_draw_line_bitmap_common

	draw a line of bitmap (called by tms3556_draw_line_bitmap and
	tms3556_draw_line_mixed)
*/
static void tms3556_draw_line_bitmap_common(UINT16 *ln)
{
	int x, xx;
	UINT8 *nametbl;
	int name_b, name_g, name_r;


	nametbl = vdp.vram + vdp.addressRegs[2];

	for (xx=0; xx<LEFT_BORDER; xx++)
#if DOUBLE_WIDTH
		*ln++ =
#endif
			*ln++ = vdp.bg_color;

	for (x=0;x<40;x++)
	{
		name_b = nametbl[vdp.name_offset];
		name_g = nametbl[vdp.name_offset+1];
		name_r = nametbl[vdp.name_offset+2];
		for (xx=0;xx<8;xx++)
		{
#if DOUBLE_WIDTH
			*ln++ =
#endif
				*ln++ = Machine->pens[((name_b >> 5) & 0x4) | ((name_g >> 6) & 0x2) | ((name_r >> 7) & 0x1)];
			name_b <<= 1;
			name_g <<= 1;
			name_r <<= 1;
		}
		vdp.name_offset += 3;
	}

	for (xx=0; xx<RIGHT_BORDER; xx++)
#if DOUBLE_WIDTH
		*ln++ =
#endif
			*ln++ = vdp.bg_color;
}

/*
	tms3556_draw_line_text

	draw a line in text mode
*/
static void tms3556_draw_line_text(UINT16 *ln)
{
	if (vdp.char_line_counter == 0)
		vdp.char_line_counter = 10;
	vdp.char_line_counter--;
	tms3556_draw_line_text_common(ln);
}

/*
	tms3556_draw_line_bitmap

	draw a line in bitmap mode
*/
static void tms3556_draw_line_bitmap(UINT16 *ln)
{
	UINT8 *nametbl;


	tms3556_draw_line_bitmap_common(ln);
	nametbl = vdp.vram + vdp.addressRegs[2];
	vdp.bg_color = Machine->pens[(nametbl[vdp.name_offset] >> 5) & 0x7];
	vdp.name_offset += 2;
}

/*
	tms3556_draw_line_mixed

	draw a line in mixed mode
*/
static void tms3556_draw_line_mixed(UINT16 *ln)
{
	UINT8 *nametbl;


	if (vdp.cg_flag)
	{	/* bitmap line */
		tms3556_draw_line_bitmap_common(ln);
		nametbl = vdp.vram + vdp.addressRegs[2];
		vdp.bg_color = Machine->pens[(nametbl[vdp.name_offset] >> 5) & 0x7];
		vdp.cg_flag = (nametbl[vdp.name_offset] >> 4) & 0x1;
		vdp.name_offset += 2;
	}
	else
	{	/* text line */
		if (vdp.char_line_counter == 0)
			vdp.char_line_counter = 10;
		vdp.char_line_counter--;
		tms3556_draw_line_text_common(ln);
		if (vdp.char_line_counter == 0)
		{
			nametbl = vdp.vram + vdp.addressRegs[2];
			vdp.bg_color = Machine->pens[(nametbl[vdp.name_offset] >> 5) & 0x7];
			vdp.cg_flag = (nametbl[vdp.name_offset] >> 4) & 0x1;
			vdp.name_offset += 2;
		}
	}
}

/*
	tms3556_draw_line

	draw a line.  If non-interlaced mode, duplicate the line.
*/
static void tms3556_draw_line(mame_bitmap *bmp, int line)
{
	int double_lines;
	UINT16 *ln, *ln2 = NULL;

	double_lines = 0;

	/*if (vdp.controlRegs[4] & 0x??)
	{	// interlaced mode
		ln = (UINT16*)bmp->line[line*2+vdp.field];
	}
	else*/
	{	/* non-interlaced mode */
		ln = (UINT16*)bmp->line[line*2];
		ln2 = (UINT16*)bmp->line[line*2+1];
		double_lines = 1;
	}

	if ((line < TOP_BORDER) || (line >= (TOP_BORDER + 250)))
	{	/* draw top and bottom borders */
		tms3556_draw_line_empty(ln);
	}
	else
	{	/* draw useful area */
		switch (vdp.controlRegs[6] >> 6)
		{
		case TMS3556_MODE_OFF:
			tms3556_draw_line_empty(ln);
			break;
		case TMS3556_MODE_TEXT:
			tms3556_draw_line_text(ln);
			break;
		case TMS3556_MODE_BITMAP:
			tms3556_draw_line_bitmap(ln);
			break;
		case TMS3556_MODE_MIXED:
			tms3556_draw_line_mixed(ln);
			break;
		}
	}

	if (double_lines)
		memcpy (ln2, ln, TOTAL_WIDTH*(DOUBLE_WIDTH ? 2 : 1) * 2);
}

/*
	video_update_tms3556

	video update function
*/
static VIDEO_UPDATE(tms3556)
{
	/* already been rendered, since we're using scanline stuff */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	return 0;
}

/*
	tms3556_interrupt_start_vblank

	Do vblank-time tasks
*/
static void tms3556_interrupt_start_vblank(void)
{
	int i;


	/* at every frame, vdp switches fields */
	//vdp.field = !vdp.field;

	/* color blinking */
	if (vdp.blink_count)
		vdp.blink_count--;
	if (!vdp.blink_count)
	{
		vdp.blink = !vdp.blink;
		vdp.blink_count = 60;	/*no idea what the real value is*/
	}
	/* reset background color */
	vdp.bg_color = Machine->pens[(vdp.controlRegs[7] >> 5) & 0x7];
	/* reset name offset */
	vdp.name_offset = 0;
	/* reset character line counter */
	vdp.char_line_counter = 0;
	/* reset c/g flag */
	vdp.cg_flag = 0;
	/* reset double height phase flags */
	for (i=0; i<40; i++)
		vdp.dbl_h_phase[i] = 0;
}

/*
	tms3556_interrupt

	scanline handler
*/
void tms3556_interrupt(void)
{
	/* check for start of vblank */
	if (vdp.scanline == 310)	/*no idea what the real value is*/
		tms3556_interrupt_start_vblank();

	/* render the current line */
	if ((vdp.scanline >= 0) && (vdp.scanline < TOTAL_HEIGHT))
	{
		//if (!video_skip_this_frame())
			tms3556_draw_line(tmpbitmap, vdp.scanline);
	}

	if (++vdp.scanline == 313)
		vdp.scanline = 0;
}

