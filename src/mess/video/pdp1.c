/*
	vidhrdw/pdp1.c

	PDP1 video emulation.

	We emulate three display devices:
	* CRT screen
	* control panel
	* typewriter output

	For the actual emulation of these devices look at the machine/pdp1.c.  This
	file only includes the display routines.

	Raphael Nabet 2002-2004
	Based on earlier work by Chris Salomon
*/

#include <math.h>

#include "driver.h"

#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"
#include "video/crt.h"


static mame_bitmap *panel_bitmap;
static mame_bitmap *typewriter_bitmap;

static const rectangle typewriter_bitmap_bounds =
{
	0,	typewriter_window_width-1,	/* min_x, max_x */
	0,	typewriter_window_height-1,	/* min_y, max_y */
};

static const rectangle panel_bitmap_bounds =
{
	0,	panel_window_width-1,	/* min_x, max_x */
	0,	panel_window_height-1,	/* min_y, max_y */
};

static void pdp1_draw_panel_backdrop(mame_bitmap *bitmap);
static void pdp1_draw_panel(mame_bitmap *bitmap);

static lightpen_t lightpen_state, previous_lightpen_state;
static void pdp1_erase_lightpen(mame_bitmap *bitmap);
static void pdp1_draw_lightpen(mame_bitmap *bitmap);

/*
	video init
*/
VIDEO_START( pdp1 )
{
	/* alloc bitmaps for our private fun */
	panel_bitmap = auto_bitmap_alloc(panel_window_width, panel_window_height, BITMAP_FORMAT_INDEXED16);
	typewriter_bitmap = auto_bitmap_alloc(typewriter_window_width, typewriter_window_height, BITMAP_FORMAT_INDEXED16);

	/* set up out bitmaps */
	pdp1_draw_panel_backdrop(panel_bitmap);

	fillbitmap(typewriter_bitmap, Machine->pens[pen_typewriter_bg], &typewriter_bitmap_bounds);

	/* initialize CRT */
	if (video_start_crt(pen_crt_num_levels, crt_window_offset_x, crt_window_offset_y, crt_window_width, crt_window_height))
		return 1;

	return 0;
}


/*
	schedule a pixel to be plotted
*/
void pdp1_plot(int x, int y)
{
	/* compute pixel coordinates and plot */
	x = x*crt_window_width/01777;
	y = y*crt_window_height/01777;
	crt_plot(x, y);
}


/*
	video_update_pdp1: effectively redraw the screen
*/
VIDEO_UPDATE( pdp1 )
{
	pdp1_erase_lightpen(bitmap);
	video_update_crt(bitmap);
	pdp1_draw_lightpen(bitmap);

	pdp1_draw_panel(panel_bitmap);
	copybitmap(bitmap, panel_bitmap, 0, 0, panel_window_offset_x, panel_window_offset_y, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);

	copybitmap(bitmap, typewriter_bitmap, 0, 0, typewriter_window_offset_x, typewriter_window_offset_y, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	return 0;
}



/*
	Operator control panel code
*/

enum
{
	x_panel_col1_offset = 8,
	x_panel_col2_offset = x_panel_col1_offset+144+8,
	x_panel_col3_offset = x_panel_col2_offset+96+8
};

enum
{
	/* column 1: registers, test word, test address */
	y_panel_pc_offset = 0,
	y_panel_ma_offset = y_panel_pc_offset+2*8,
	y_panel_mb_offset = y_panel_ma_offset+2*8,
	y_panel_ac_offset = y_panel_mb_offset+2*8,
	y_panel_io_offset = y_panel_ac_offset+2*8,
	y_panel_ta_offset = y_panel_io_offset+2*8,	/* test address and extend switch */
	y_panel_tw_offset = y_panel_ta_offset+2*8,

	/* column 2: 1-bit indicators */
	y_panel_run_offset = 8,
	y_panel_cyc_offset = y_panel_run_offset+8,
	y_panel_defer_offset = y_panel_cyc_offset+8,
	y_panel_hs_cyc_offset = y_panel_defer_offset+8,
	y_panel_brk_ctr_1_offset = y_panel_hs_cyc_offset+8,
	y_panel_brk_ctr_2_offset = y_panel_brk_ctr_1_offset+8,
	y_panel_ov_offset = y_panel_brk_ctr_2_offset+8,
	y_panel_rim_offset = y_panel_ov_offset+8,
	y_panel_sbm_offset = y_panel_rim_offset+8,
	y_panel_exd_offset = y_panel_sbm_offset+8,
	y_panel_ioh_offset = y_panel_exd_offset+8,
	y_panel_ioc_offset = y_panel_ioh_offset+8,
	y_panel_ios_offset = y_panel_ioc_offset+8,

	/* column 3: power, single step, single inst, sense, flags, instr... */
	y_panel_power_offset = 8,
	y_panel_sngl_step_offset = y_panel_power_offset+8,
	y_panel_sngl_inst_offset = y_panel_sngl_step_offset+8,
	y_panel_sep1_offset = y_panel_sngl_inst_offset+8,
	y_panel_ss_offset = y_panel_sep1_offset+8,
	y_panel_sep2_offset = y_panel_ss_offset+3*8,
	y_panel_pf_offset = y_panel_sep2_offset+8,
	y_panel_ir_offset = y_panel_pf_offset+2*8
};

/* draw a small 8*8 LED (or is this a lamp? ) */
static void pdp1_draw_led(mame_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;

	for (yy=1; yy<7; yy++)
		for (xx=1; xx<7; xx++)
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[state ? pen_lit_lamp : pen_unlit_lamp]);
}

/* draw nb_bits leds which represent nb_bits bits in value */
static void pdp1_draw_multipleled(mame_bitmap *bitmap, int x, int y, int value, int nb_bits)
{
	while (nb_bits)
	{
		nb_bits--;

		pdp1_draw_led(bitmap, x, y, (value >> nb_bits) & 1);

		x += 8;
	}
}


/* draw a small 8*8 switch */
static void pdp1_draw_switch(mame_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;
	int i;

	/* erase area */
	for (yy=0; yy<8; yy++)
		for (xx=0; xx<8; xx++)
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[pen_panel_bg]);


	/* draw nut (-> circle) */
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+2+i, y+6, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+1, y+2+i, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+6, y+2+i, Machine->pens[pen_switch_nut]);
	}
	plot_pixel(bitmap, x+2, y+2, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+5, y+2, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+2, y+5, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+5, y+5, Machine->pens[pen_switch_nut]);

	/* draw button (->disc) */
	if (! state)
		y += 4;
	for (i=0; i<2;i++)
	{
		plot_pixel(bitmap, x+3+i, y, Machine->pens[pen_switch_button]);
		plot_pixel(bitmap, x+3+i, y+3, Machine->pens[pen_switch_button]);
	}
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[pen_switch_button]);
		plot_pixel(bitmap, x+2+i, y+2, Machine->pens[pen_switch_button]);
	}
}


/* draw nb_bits switches which represent nb_bits bits in value */
static void pdp1_draw_multipleswitch(mame_bitmap *bitmap, int x, int y, int value, int nb_bits)
{
	while (nb_bits)
	{
		nb_bits--;

		pdp1_draw_switch(bitmap, x, y, (value >> nb_bits) & 1);

		x += 8;
	}
}


/* write a single char on screen */
static void pdp1_draw_char(mame_bitmap *bitmap, char character, int x, int y, int color)
{
	drawgfx(bitmap, Machine->gfx[0], character-32, color, 0, 0,
				x+1, y, &Machine->screen[0].visarea, TRANSPARENCY_PEN, 0);
}

/* write a string on screen */
static void pdp1_draw_string(mame_bitmap *bitmap, const char *buf, int x, int y, int color)
{
	while (* buf)
	{
		pdp1_draw_char(bitmap, *buf, x, y, color);

		x += 8;
		buf++;
	}
}


/*
	draw the operator control panel (fixed backdrop)
*/
static void pdp1_draw_panel_backdrop(mame_bitmap *bitmap)
{
	/* fill with black */
	fillbitmap(panel_bitmap, Machine->pens[pen_panel_bg], &panel_bitmap_bounds);

	/* column 1: registers, test word, test address */
	pdp1_draw_string(bitmap, "program counter", x_panel_col1_offset, y_panel_pc_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "memory address", x_panel_col1_offset, y_panel_ma_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "memory buffer", x_panel_col1_offset, y_panel_mb_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "accumulator", x_panel_col1_offset, y_panel_ac_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "in-out", x_panel_col1_offset, y_panel_io_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "extend  address", x_panel_col1_offset-8, y_panel_ta_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "test word", x_panel_col1_offset, y_panel_tw_offset, color_panel_caption);

	/* column separator */
	plot_box(bitmap, x_panel_col2_offset-4, panel_window_offset_y+8, 1, 96, pen_panel_caption);

	/* column 2: 1-bit indicators */
	pdp1_draw_string(bitmap, "run", x_panel_col2_offset+8, y_panel_run_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "cycle", x_panel_col2_offset+8, y_panel_cyc_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "defer", x_panel_col2_offset+8, y_panel_defer_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "h. s. cycle", x_panel_col2_offset+8, y_panel_hs_cyc_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "brk. ctr. 1", x_panel_col2_offset+8, y_panel_brk_ctr_1_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "brk. ctr. 2", x_panel_col2_offset+8, y_panel_brk_ctr_2_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "overflow", x_panel_col2_offset+8, y_panel_ov_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "read in", x_panel_col2_offset+8, y_panel_rim_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "seq. break", x_panel_col2_offset+8, y_panel_sbm_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "extend", x_panel_col2_offset+8, y_panel_exd_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "i-o halt", x_panel_col2_offset+8, y_panel_ioh_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "i-o com'ds", x_panel_col2_offset+8, y_panel_ioc_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "i-o sync", x_panel_col2_offset+8, y_panel_ios_offset, color_panel_caption);

	/* column separator */
	plot_box(bitmap, x_panel_col3_offset-4, panel_window_offset_y+8, 1, 96, pen_panel_caption);

	/* column 3: power, single step, single inst, sense, flags, instr... */
	pdp1_draw_string(bitmap, "power", x_panel_col3_offset+16, y_panel_power_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "single step", x_panel_col3_offset+16, y_panel_sngl_step_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "single inst.", x_panel_col3_offset+16, y_panel_sngl_inst_offset, color_panel_caption);
	/* separator */
	plot_box(bitmap, x_panel_col3_offset+8, y_panel_sep1_offset+4, 96, 1, pen_panel_caption);
	pdp1_draw_string(bitmap, "sense switches", x_panel_col3_offset, y_panel_ss_offset, color_panel_caption);
	/* separator */
	plot_box(bitmap, x_panel_col3_offset+8, y_panel_sep2_offset+4, 96, 1, pen_panel_caption);
	pdp1_draw_string(bitmap, "program flags", x_panel_col3_offset, y_panel_pf_offset, color_panel_caption);
	pdp1_draw_string(bitmap, "instruction", x_panel_col3_offset, y_panel_ir_offset, color_panel_caption);
}

/*
	draw the operator control panel (dynamic elements)
*/
static void pdp1_draw_panel(mame_bitmap *bitmap)
{
	/* column 1: registers, test word, test address */
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y_panel_pc_offset+8, cpunum_get_reg(0, PDP1_PC), 16);
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y_panel_ma_offset+8, cpunum_get_reg(0, PDP1_MA), 16);
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y_panel_mb_offset+8, cpunum_get_reg(0, PDP1_MB), 18);
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y_panel_ac_offset+8, cpunum_get_reg(0, PDP1_AC), 18);
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y_panel_io_offset+8, cpunum_get_reg(0, PDP1_IO), 18);
	pdp1_draw_switch(bitmap, x_panel_col1_offset, y_panel_ta_offset+8, cpunum_get_reg(0, PDP1_EXTEND_SW));
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset+2*8, y_panel_ta_offset+8, cpunum_get_reg(0, PDP1_TA), 16);
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset, y_panel_tw_offset+8, cpunum_get_reg(0, PDP1_TW), 18);

	/* column 2: 1-bit indicators */
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_run_offset, cpunum_get_reg(0, PDP1_RUN));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_cyc_offset, cpunum_get_reg(0, PDP1_CYC));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_defer_offset, cpunum_get_reg(0, PDP1_DEFER));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_hs_cyc_offset, 0);	/* not emulated */
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_brk_ctr_1_offset, cpunum_get_reg(0, PDP1_BRK_CTR) & 1);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_brk_ctr_2_offset, cpunum_get_reg(0, PDP1_BRK_CTR) & 2);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_ov_offset, cpunum_get_reg(0, PDP1_OV));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_rim_offset, cpunum_get_reg(0, PDP1_RIM));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_sbm_offset, cpunum_get_reg(0, PDP1_SBM));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_exd_offset, cpunum_get_reg(0, PDP1_EXD));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_ioh_offset, cpunum_get_reg(0, PDP1_IOH));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_ioc_offset, cpunum_get_reg(0, PDP1_IOC));
	pdp1_draw_led(bitmap, x_panel_col2_offset, y_panel_ios_offset, cpunum_get_reg(0, PDP1_IOS));

	/* column 3: power, single step, single inst, sense, flags, instr... */
	pdp1_draw_led(bitmap, x_panel_col3_offset, y_panel_power_offset, 1);	/* always on */
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y_panel_power_offset, 1);	/* always on */
	pdp1_draw_led(bitmap, x_panel_col3_offset, y_panel_sngl_step_offset, cpunum_get_reg(0, PDP1_SNGL_STEP));
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y_panel_sngl_step_offset, cpunum_get_reg(0, PDP1_SNGL_STEP));
	pdp1_draw_led(bitmap, x_panel_col3_offset, y_panel_sngl_inst_offset, cpunum_get_reg(0, PDP1_SNGL_INST));
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y_panel_sngl_inst_offset, cpunum_get_reg(0, PDP1_SNGL_INST));
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y_panel_ss_offset+8, cpunum_get_reg(0, PDP1_SS), 6);
	pdp1_draw_multipleswitch(bitmap, x_panel_col3_offset, y_panel_ss_offset+2*8, cpunum_get_reg(0, PDP1_SS), 6);
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y_panel_pf_offset+8, cpunum_get_reg(0, PDP1_PF), 6);
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y_panel_ir_offset+8, cpunum_get_reg(0, PDP1_IR), 5);
}


/*
	Typewriter code
*/


static int pos;

static int case_shift;

static int color = color_typewriter_black;

enum
{
	typewriter_line_height = 8,
	typewriter_write_offset_y = typewriter_window_height-typewriter_line_height,
	typewriter_scroll_step = typewriter_line_height
};
static const rectangle typewriter_scroll_clear_window =
{
	0, typewriter_window_width-1,	/* min_x, max_x */
	typewriter_window_height-typewriter_scroll_step, typewriter_window_height-1,	/* min_y, max_y */
};

enum
{
	tab_step = 8
};


static void pdp1_typewriter_linefeed(void)
{
	UINT8 buf[typewriter_window_width];
	int y;

	for (y=0; y<typewriter_window_height-typewriter_scroll_step; y++)
	{
		extract_scanline8(typewriter_bitmap, 0, y+typewriter_scroll_step, typewriter_window_width, buf);
		draw_scanline8(typewriter_bitmap, 0, y, typewriter_window_width, buf, Machine->pens, -1);
	}

	fillbitmap(typewriter_bitmap, Machine->pens[pen_typewriter_bg], &typewriter_scroll_clear_window);
}

void pdp1_typewriter_drawchar(int character)
{
	static const char ascii_table[2][64] =
	{	/* n-s = non-spacing */
		{	/* lower case */
			' ',				'1',				'2',				'3',
			'4',				'5',				'6',				'7',
			'8',				'9',				'*',				'*',
			'*',				'*',				'*',				'*',
			'0',				'/',				's',				't',
			'u',				'v',				'w',				'x',
			'y',				'z',				'*',				',',
			'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
			'\200',/*n-s middle dot*/'j',			'k',				'l',
			'm',				'n',				'o',				'p',
			'q',				'r',				'*',				'*',
			'-',				')',				'\201',/*n-s overstrike*/'(',
			'*',				'a',				'b',				'c',
			'd',				'e',				'f',				'g',
			'h',				'i',				'*',/*Lower Case*/	'.',
			'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
		},
		{	/* upper case */
			' ',				'"',				'\'',				'~',
			'\202',/*implies*/	'\203',/*or*/		'\204',/*and*/		'<',
			'>',				'\205',/*up arrow*/	'*',				'*',
			'*',				'*',				'*',				'*',
			'\206',/*right arrow*/'?',				'S',				'T',
			'U',				'V',				'W',				'X',
			'Y',				'Z',				'*',				'=',
			'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
			'_',/*n-s???*/		'J',				'K',				'L',
			'M',				'N',				'O',				'P',
			'Q',				'R',				'*',				'*',
			'+',				']',				'|',/*n-s???*/		'[',
			'*',				'A',				'B',				'C',
			'D',				'E',				'F',				'G',
			'H',				'I',				'*',/*Lower Case*/	'\207',/*multiply*/
			'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
		}
	};



	character &= 0x3f;

	switch (character)
	{
	case 034:
		/* Black */
		color = color_typewriter_black;
		break;

	case 035:
		/* Red */
		color = color_typewriter_red;
		break;

	case 036:
		/* Tab */
		pos = pos + tab_step - (pos % tab_step);
		break;

	case 072:
		/* Lower case */
		case_shift = 0;
		break;

	case 074:
		/* Upper case */
		case_shift = 1;
		break;

	case 075:
		/* Backspace */
		if (pos)
			pos--;
		break;

	case 077:
		/* Carriage Return */
		pos = 0;
		pdp1_typewriter_linefeed();
		break;

	default:
		/* Any printable character... */

		if (pos >= 80)
		{	/* if past right border, wrap around. (Right???) */
			pdp1_typewriter_linefeed();	/* next line */
			pos = 0;					/* return to start of line */
		}

		/* print character (lookup ASCII equivalent in table) */
		pdp1_draw_char(typewriter_bitmap, ascii_table[case_shift][character],
						8*pos, typewriter_write_offset_y,
						color);	/* print char */

		if ((character!= 040) && (character!= 056))	/* 040 and 056 are non-spacing characters */
			pos++;		/* step carriage forward */

		break;
	}
}



/*
	lightpen code
*/

void pdp1_update_lightpen_state(const lightpen_t *new_state)
{
	lightpen_state = *new_state;
}

#if 1
static void pdp1_draw_circle(mame_bitmap *bitmap, int x, int y, int radius, int color_)
{
	int interval;
	int a;

	x = x*crt_window_width/01777;
	y = y*crt_window_width/01777;
	radius = radius*crt_window_width/01777;

	interval = ceil(radius/sqrt(2));

	for (a=0; a<=interval; a++)
	{
		int b = sqrt(radius*radius-a*a) + .5;

		if ((x-a >= 0) && (y-b >= 0))
			plot_pixel(bitmap, x-a, y-b, color_);
		if ((x-a >= 0) && (y+b <= crt_window_height-1))
			plot_pixel(bitmap, x-a, y+b, color_);
		if ((x+a <= crt_window_width-1) && (y-b >= 0))
			plot_pixel(bitmap, x+a, y-b, color_);
		if ((x+a <= crt_window_width-1) && (y+b <= crt_window_height-1))
			plot_pixel(bitmap, x+a, y+b, color_);

		if ((x-b >= 0) && (y-a >= 0))
			plot_pixel(bitmap, x-b, y-a, color_);
		if ((x-b >= 0) && (y+a <= crt_window_height-1))
			plot_pixel(bitmap, x-b, y+a, color_);
		if ((x+b <= crt_window_width-1) && (y-a >= 0))
			plot_pixel(bitmap, x+b, y-a, color_);
		if ((x+b <= crt_window_width-1) && (y+a <= crt_window_height-1))
			plot_pixel(bitmap, x+b, y+a, color_);
	}
}
#else
static void pdp1_draw_circle(mame_bitmap *bitmap, int x, int y, int radius, int color)
{
	float fx, fy;
	float interval;


	fx = (float)x*crt_window_width/01777;
	fy = (float)y*crt_window_height/01777;

	interval = radius/sqrt(2);

	for (x=/*ceil*/(fx-interval); x<=fx+interval; x++)
	{
		float dy = sqrt(radius*radius-(x-fx)*(x-fx));

		if ((x >= 0) && (x <= crt_window_width-1) && (fy-dy >= 0))
			plot_pixel(bitmap, x, fy-dy, color);
		if ((x >= 0) && (x <= crt_window_width-1) && (y+dy <= crt_window_height-1))
			plot_pixel(bitmap, x, fy+dy, color);
	}
	for (y=/*ceil*/(fy-interval); y<=fy+interval; y++)
	{
		float dx = sqrt(radius*radius-(y-fy)*(y-fy));

		if ((fx-dx >= 0) && (y >= 0) && (y <= crt_window_height-1))
			plot_pixel(bitmap, fx-dx, y, color);
		if ((fx+dx <= crt_window_width-1) && (y >= 0) && (y <= crt_window_height-1))
			plot_pixel(bitmap, fx+dx, y, color);
	}
}
#endif

static void pdp1_erase_lightpen(mame_bitmap *bitmap)
{
	if (previous_lightpen_state.active)
	{
		/*if (previous_lightpen_state.x>0)
			plot_pixel(bitmap, previous_lightpen_state.x/2-1, previous_lightpen_state.y/2, pen_black);
		if (previous_lightpen_state.x<1023)
			plot_pixel(bitmap, previous_lightpen_state.x/2+1, previous_lightpen_state.y/2, pen_black);
		if (previous_lightpen_state.y>0)
			plot_pixel(bitmap, previous_lightpen_state.x/2, previous_lightpen_state.y/2-1, pen_black);
		if (previous_lightpen_state.y<1023)
			plot_pixel(bitmap, previous_lightpen_state.x/2, previous_lightpen_state.y/2+1, pen_black);*/
		pdp1_draw_circle(bitmap, previous_lightpen_state.x, previous_lightpen_state.y, previous_lightpen_state.radius, pen_black);
	}
}

static void pdp1_draw_lightpen(mame_bitmap *bitmap)
{
	if (lightpen_state.active)
	{
		int color_ = lightpen_state.down ? pen_lightpen_pressed : pen_lightpen_nonpressed;
		/*if (lightpen_state.x>0)
			plot_pixel(bitmap, lightpen_state.x/2-1, lightpen_state.y/2, color);
		if (lightpen_state.x<1023)
			plot_pixel(bitmap, lightpen_state.x/2+1, lightpen_state.y/2, color);
		if (lightpen_state.y>0)
			plot_pixel(bitmap, lightpen_state.x/2, lightpen_state.y/2-1, color);
		if (lightpen_state.y<1023)
			plot_pixel(bitmap, lightpen_state.x/2, lightpen_state.y/2+1, color);*/
		pdp1_draw_circle(bitmap, lightpen_state.x, lightpen_state.y, lightpen_state.radius, color_);
	}
	previous_lightpen_state = lightpen_state;
}
