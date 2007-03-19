/*
	TI 911 VDT core.  To be operated with the TI 990 line of computers (can be connected to
	any model, as communication uses the CRU bus).

	Raphael Nabet 2002

TODO:
	* add more flexibility, so that we can create multiple-terminal configurations.
	* support test mode???
*/


#include "driver.h"
#include "911_vdt.h"
#include "911_chr.h"
#include "911_key.h"
#include "sound/beep.h"


#define MAX_VDT 1

static gfx_layout fontlayout_7bit =
{
	7, 10,			/* 7*10 characters */
	128,			/* 128 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 1, 2, 3, 4, 5, 6, 7 }, 			/* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8 			/* every char takes 10 consecutive bytes */
};

static gfx_layout fontlayout_8bit =
{
	7, 10,			/* 7*10 characters */
	128,			/* 128 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 1, 2, 3, 4, 5, 6, 7 }, 				/* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8 			/* every char takes 10 consecutive bytes */
};

gfx_decode vdt911_gfxdecodeinfo[] =
{	/* array must use same order as vdt911_model_t!!! */
	/* US */
	{ vdt911_chr_region, vdt911_US_chr_offset, &fontlayout_7bit, 0, 4 },
	/* UK */
	{ vdt911_chr_region, vdt911_UK_chr_offset, &fontlayout_7bit, 0, 4 },
	/* French */
	{ vdt911_chr_region, vdt911_US_chr_offset, &fontlayout_7bit, 0, 4 },
	/* German */
	{ vdt911_chr_region, vdt911_german_chr_offset, &fontlayout_7bit, 0, 4 },
	/* Swedish */
	{ vdt911_chr_region, vdt911_swedish_chr_offset, &fontlayout_7bit, 0, 4 },
	/* Norwegian */
	{ vdt911_chr_region, vdt911_norwegian_chr_offset, &fontlayout_7bit, 0, 4 },
	/* Japanese */
	{ vdt911_chr_region, vdt911_japanese_chr_offset, &fontlayout_8bit, 0, 4 },
	/* Arabic */
	/*{ vdt911_chr_region, vdt911_arabic_chr_offset, &fontlayout_8bit, 0, 4 },*/
	/* FrenchWP */
	{ vdt911_chr_region, vdt911_frenchWP_chr_offset, &fontlayout_7bit, 0, 4 },
	{ -1 }	/* end of array */
};

unsigned char vdt911_palette[vdt911_palette_size*3] =
{
	0x00,0x00,0x00,	/* black */
	0xC0,0xC0,0xC0,	/* low intensity */
	0xFF,0xFF,0xFF	/* high intensity */
};

unsigned short vdt911_colortable[vdt911_colortable_size] =
{
	0, 2,	/* high intensity */
	0, 1,	/* low intensity */
	2, 0,	/* high intensity, reverse */
	2, 1	/* low intensity, reverse */
};

typedef struct vdt_t
{
	vdt911_screen_size_t screen_size;	/* char_960 for 960-char, 12-line model; char_1920 for 1920-char, 24-line model */
	vdt911_model_t model;				/* country code */
	void (*int_callback)(int state);	/* interrupt callback, called when the state of irq changes */

	UINT8 data_reg;						/* vdt911 write buffer */
	UINT8 display_RAM[2048];			/* vdt911 char buffer (1kbyte for 960-char model, 2kbytes for 1920-char model) */

	unsigned int cursor_address;		/* current cursor address (controlled by the computer, affects both display and I/O protocol) */
	unsigned int cursor_address_mask;	/* 1023 for 960-char model, 2047 for 1920-char model */

	void *beep_timer;					/* beep clock (beeps ends when timer times out) */
	/*void *blink_clock;*/				/* cursor blink clock */

	UINT8 keyboard_data;				/* last code pressed on keyboard */
	unsigned int keyboard_data_ready : 1;		/* true if there is a new code in keyboard_data */
	unsigned int keyboard_interrupt_enable : 1;	/* true when keybord interrupts are enabled */

	unsigned int display_enable : 1;		/* screen is black when false */
	unsigned int dual_intensity_enable : 1;	/* if true, MSBit of ASCII codes controls character highlight */
	unsigned int display_cursor : 1;		/* if true, the current cursor location is displayed on screen */
	unsigned int blinking_cursor_enable : 1;/* if true, the cursor will blink when displayed */
	unsigned int blink_state : 1;			/* current cursor blink state */

	unsigned int word_select : 1;			/* CRU interface mode */
	unsigned int previous_word_select : 1;	/* value of word_select is saved here */
} vdt_t;

static vdt_t vdt[MAX_VDT];

/*
	Macros for model features
*/
/* TRUE for japanese and arabic terminals, which use 8-bit charcodes and keyboard shift modes */
#define USES_8BIT_CHARCODES(unit) ((vdt[unit].model == vdt911_model_Japanese) /*|| (vdt[unit].model == vdt911_model_Arabic)*/)
/* TRUE for keyboards which have this extra key (on the left of TAB/SKIP)
	(Most localized keyboards have it) */
#define HAS_EXTRA_KEY_67(unit) (! ((vdt[unit].model == vdt911_model_US) || (vdt[unit].model == vdt911_model_UK) || (vdt[unit].model == vdt911_model_French)))
/* TRUE for keyboards which have this extra key (on the right of space),
	AND do not use it as a modifier */
#define HAS_EXTRA_KEY_91(unit) ((vdt[unit].model == vdt911_model_German) || (vdt[unit].model == vdt911_model_Swedish) || (vdt[unit].model == vdt911_model_Norwegian))

static void blink_callback(int unit);
static void beep_callback(int unit);

/*
	Initialize vdt911 palette
*/
PALETTE_INIT( vdt911 )
{
	/*memcpy(palette, & vdt911_palette, sizeof(vdt911_palette));*/
	palette_set_colors(machine, 0, vdt911_palette, vdt911_palette_size);

	memcpy(colortable, & vdt911_colortable, sizeof(vdt911_colortable));
}

/*
	Copy a character bitmap array to another location in memory
*/
static void copy_character_matrix_array(const UINT8 char_array[128][10], UINT8 *dest)
{
	int i, j;

	for (i=0; i<128; i++)
		for (j=0; j<10; j++)
			*(dest++) = char_array[i][j];
}

/*
	Patch a character bitmap array according to an array of char_override_t
*/
static void apply_char_overrides(int nb_char_overrides, const char_override_t char_overrides[], UINT8 *dest)
{
	int i, j;

	for (i=0; i<nb_char_overrides; i++)
	{
		for (j=0; j<10; j++)
			dest[char_overrides[i].char_index*10+j] = char_defs[char_overrides[i].symbol_index][j];
	}
}

/*
	Initialize the 911 vdt core
*/
void vdt911_init(void)
{
	UINT8 *base;

	memset(vdt, 0, sizeof(vdt));

	/* set up US character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_US_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);

	/* set up UK character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_UK_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(UK_overrides)/sizeof(char_override_t), UK_overrides, base);

	/* French character set is identical to US character set */

	/* set up German character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_german_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(german_overrides)/sizeof(char_override_t), german_overrides, base);

	/* set up Swedish/Finnish character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_swedish_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(swedish_overrides)/sizeof(char_override_t), swedish_overrides, base);

	/* set up Norwegian/Danish character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_norwegian_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(norwegian_overrides)/sizeof(char_override_t), norwegian_overrides, base);

	/* set up Katakana Japanese character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_japanese_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(japanese_overrides)/sizeof(char_override_t), japanese_overrides, base);
	base = memory_region(vdt911_chr_region)+vdt911_japanese_chr_offset+128*vdt911_single_char_len;
	copy_character_matrix_array(char_defs+char_defs_katakana_base, base);

	/* set up Arabic character definitions */
	/*base = memory_region(vdt911_chr_region)+vdt911_arabic_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(arabic_overrides)/sizeof(char_override_t), arabic_overrides, base);
	base = memory_region(vdt911_chr_region)+vdt911_arabic_chr_offset+128*vdt911_single_char_len;
	copy_character_matrix_array(char_defs+char_defs_arabic_base, base);*/

	/* set up French word processing character definitions */
	base = memory_region(vdt911_chr_region)+vdt911_frenchWP_chr_offset;
	copy_character_matrix_array(char_defs+char_defs_US_base, base);
	apply_char_overrides(sizeof(frenchWP_overrides)/sizeof(char_override_t), frenchWP_overrides, base);
}

static void setup_beep(int unit)
{
	beep_set_frequency(unit, 2000);
}

/*
	Initialize one 911 vdt controller/terminal
*/
int vdt911_init_term(int unit, const vdt911_init_params_t *params)
{
	vdt[unit].screen_size = params->screen_size;
	vdt[unit].model = params->model;
	vdt[unit].int_callback = params->int_callback;

	if (vdt[unit].screen_size == char_960)
		vdt[unit].cursor_address_mask = 0x3ff;	/* 1kb of RAM */
	else
		vdt[unit].cursor_address_mask = 0x7ff;	/* 2 kb of RAM */

	timer_set(0.0, unit, setup_beep);

	/* set up cursor blink clock.  2Hz frequency -> .25s half-period. */
	/*vdt[unit].blink_clock =*/ timer_pulse(TIME_IN_SEC(.25), unit, blink_callback);

	/* alloc beep timer */
	vdt[unit].beep_timer = timer_alloc(beep_callback);

	return 0;
}

/*
	Reset the controller board
*/
void vdt911_reset(void)
{
}

/*
	timer callback to toggle blink state
*/
static void blink_callback(int unit)
{
	vdt[unit].blink_state = ! vdt[unit].blink_state;
}

/*
	timer callback to stop beep generator
*/
static void beep_callback(int unit)
{
	beep_set_state(unit, 0);
}

/*
	CRU interface read
*/
int vdt911_cru_r(int offset, int unit)
{
	int reply=0;

	offset &= 0x1;

	if (! vdt[unit].word_select)
	{	/* select word 0 */
		switch (offset)
		{
		case 0:
			reply = vdt[unit].display_RAM[vdt[unit].cursor_address];
			break;

		case 1:
			reply = vdt[unit].keyboard_data & 0x7f;
			if (vdt[unit].keyboard_data_ready)
				reply |= 0x80;
			break;
		}
	}
	else
	{	/* select word 1 */
		switch (offset)
		{
		case 0:
			reply = vdt[unit].cursor_address & 0xff;
			break;

		case 1:
			reply = (vdt[unit].cursor_address >> 8) & 0x07;
			if (vdt[unit].keyboard_data & 0x80)
				reply |= 0x08;
			/*if (! vdt[unit].terminal_ready)
				reply |= 0x10;*/
			if (vdt[unit].previous_word_select)
				reply |= 0x20;
			/*if (vdt[unit].keyboard_parity_error)
				reply |= 0x40;*/
			if (vdt[unit].keyboard_data_ready)
				reply |= 0x80;
			break;
		}
	}

	return reply;
}

/*
	CRU interface write
*/
void vdt911_cru_w(int offset, int data, int unit)
{
	offset &= 0xf;

	if (! vdt[unit].word_select)
	{	/* select word 0 */
		switch (offset)
		{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			/* display memory write data */
			if (data)
				vdt[unit].data_reg |= (1 << offset);
			else
				vdt[unit].data_reg &= ~ (1 << offset);
			break;

		case 0x8:
			/* write data strobe */
			 vdt[unit].display_RAM[vdt[unit].cursor_address] = vdt[unit].data_reg;
			break;

		case 0x9:
			/* test mode */
			/* ... */
			break;

		case 0xa:
			/* cursor move */
			if (data)
				vdt[unit].cursor_address--;
			else
				vdt[unit].cursor_address++;
			vdt[unit].cursor_address &= vdt[unit].cursor_address_mask;
			break;

		case 0xb:
			/* blinking cursor enable */
			vdt[unit].blinking_cursor_enable = data;
			break;

		case 0xc:
			/* keyboard interrupt enable */
			vdt[unit].keyboard_interrupt_enable = data;
			(*vdt[unit].int_callback)(vdt[unit].keyboard_interrupt_enable && vdt[unit].keyboard_data_ready);
			break;

		case 0xd:
			/* dual intensity enable */
			vdt[unit].dual_intensity_enable = data;
			break;

		case 0xe:
			/* display enable */
			vdt[unit].display_enable = data;
			break;

		case 0xf:
			/* select word */
			vdt[unit].previous_word_select = vdt[unit].word_select;
			vdt[unit].word_select = data;
			break;
		}
	}
	else
	{	/* select word 1 */
		switch (offset)
		{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
		case 0xa:
			/* cursor address */
			if (data)
				vdt[unit].cursor_address |= (1 << offset);
			else
				vdt[unit].cursor_address &= ~ (1 << offset);
			vdt[unit].cursor_address &= vdt[unit].cursor_address_mask;
			break;

		case 0xb:
			/* not used */
			break;

		case 0xc:
			/* display cursor */
			vdt[unit].display_cursor = data;
			break;

		case 0xd:
			/* keyboard acknowledge */
			if (vdt[unit].keyboard_data_ready)
			{
				vdt[unit].keyboard_data_ready = 0;
				if (vdt[unit].keyboard_interrupt_enable)
					(*vdt[unit].int_callback)(0);
			}
			/*vdt[unit].keyboard_parity_error = 0;*/
			break;

		case 0xe:
			/* beep enable strobe - not tested */
			beep_set_state(unit, 1);

			timer_adjust(vdt[unit].beep_timer, TIME_IN_SEC(.3), unit, 0.);
			break;

		case 0xf:
			/* select word */
			vdt[unit].previous_word_select = vdt[unit].word_select;
			vdt[unit].word_select = data;
			break;
		}
	}
}

 READ8_HANDLER(vdt911_0_cru_r)
{
	return vdt911_cru_r(offset, 0);
}

WRITE8_HANDLER(vdt911_0_cru_w)
{
	vdt911_cru_w(offset, data, 0);
}

/*
	Video refresh
*/
void vdt911_refresh(mame_bitmap *bitmap, int unit, int x, int y)
{
	const gfx_element *gfx = Machine->gfx[vdt[unit].model];
	int height = (vdt[unit].screen_size == char_960) ? 12 : /*25*/24;
	int use_8bit_charcodes = USES_8BIT_CHARCODES(unit);
	int address = 0;
	int i, j;
	int cur_char;
	int color;

	/*if (use_8bit_charcodes)
		color = vdt[unit].dual_intensity_enable ? 1 : 0;*/

	if (! vdt[unit].display_enable)
	{
		rectangle my_rect;

		my_rect.min_x = x;
		my_rect.max_x = x + 80*7 - 1;

		my_rect.min_y = y;
		my_rect.max_y = y + height*10 - 1;

		fillbitmap(bitmap, 0, &my_rect);
	}
	else
		for (i=0; i<height; i++)
		{
			for (j=0; j<80; j++)
			{
				cur_char = vdt[unit].display_RAM[address];
				/* does dual intensity work with 8-bit character set? */
				color = (vdt[unit].dual_intensity_enable && (cur_char & 0x80)) ? 1 : 0;
				if (! use_8bit_charcodes)
					cur_char &= 0x7f;

				/* display cursor in reverse video */
				if ((address == vdt[unit].cursor_address) && vdt[unit].display_cursor
						&& ((! vdt[unit].blinking_cursor_enable) || vdt[unit].blink_state))
					color += 2;

				address++;

				drawgfx(bitmap, gfx, cur_char, color, 0, 0,
						x+j*7, y+i*10, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
			}
		}
}

static unsigned char (*key_translate[])[91] =
{	/* array must use same order as vdt911_model_t!!! */
	/* US */
	US_key_translate,
	/* UK */
	US_key_translate,
	/* French */
	French_key_translate,
	/* German */
	German_key_translate,
	/* Swedish */
	Swedish_key_translate,
	/* Norwegian */
	Norwegian_key_translate,
	/* Japanese */
	Japanese_key_translate,
	/* Arabic */
	/*Arabic_key_translate,*/
	/* FrenchWP */
	FrenchWP_key_translate
};

/*
	keyboard handler: should be called regularly by machine code, for instance
	every Video Blank Interrupt.
*/
void vdt911_keyboard(int unit)
{
	typedef enum
	{
		/* states for western keyboards and katakana/arabic keyboards in romaji/latin mode */
		lower_case = 0, upper_case, shift, control,
		/* states for katakana/arabic keyboards in katakana/arabic mode */
		foreign, foreign_shift,
		/* special value to stop repeat if the modifier state changes */
		special_debounce = -1
	} modifier_state_t;

	static unsigned char repeat_timer;
	enum { repeat_delay = 5 /* approx. 1/10s */ };
	static char last_key_pressed = -1;
	static modifier_state_t last_modifier_state;
	static char foreign_mode;

	UINT16 key_buf[6];
	int i, j;
	modifier_state_t modifier_state;
	int repeat_mode;


	/* read current key state */
	for (i=0; i<6; i++)
		key_buf[i] = readinputport(i);


	/* parse modifier keys */
	if ((USES_8BIT_CHARCODES(unit))
		&& ((key_buf[5] & 0x0400) || ((! (key_buf[5] & 0x0100)) && foreign_mode)))
	{	/* we are in katakana/arabic mode */
		foreign_mode = TRUE;

		if ((key_buf[4] & 0x0400) || (key_buf[5] & 0x0020))
			modifier_state = foreign_shift;
		else
			modifier_state = foreign;
	}
	else
	{	/* we are using a western keyboard, or a katakana/arabic keyboard in
		romaji/latin mode */
		foreign_mode = FALSE;

		if (key_buf[3] & 0x0040)
			modifier_state = control;
		else if ((key_buf[4] & 0x0400) || (key_buf[5] & 0x0020))
			modifier_state = shift;
		else if ((key_buf[0] & 0x2000))
			modifier_state = upper_case;
		else
			modifier_state = lower_case;
	}


	/* test repeat key */
	repeat_mode = key_buf[2] & 0x0002;


	/* remove modifier keys */
	key_buf[0] &= ~0x2000;
	key_buf[2] &= ~0x0002;
	key_buf[3] &= ~0x0040;
	key_buf[4] &= ~0x0400;
	key_buf[5] &= ~0x0120;

	/* remove unused keys */
	if (! HAS_EXTRA_KEY_91(unit))
		key_buf[5] &= ~0x0400;

	if (! HAS_EXTRA_KEY_67(unit))
		key_buf[4] &= ~0x0004;


	if (! repeat_mode)
		/* reset REPEAT timer if the REPEAT key is not pressed */
		repeat_timer = 0;

	if ((last_key_pressed >= 0) && (key_buf[last_key_pressed >> 4] & (1 << (last_key_pressed & 0xf))))
	{
		/* last key has not been released */
		if (modifier_state == last_modifier_state)
		{
			/* handle REPEAT mode if applicable */
			if ((repeat_mode) && (++repeat_timer == repeat_delay))
			{
				if (vdt[unit].keyboard_data_ready)
				{	/* keyboard buffer full */
					repeat_timer--;
				}
				else
				{	/* repeat current key */
					vdt[unit].keyboard_data_ready = 1;
					repeat_timer = 0;
				}
			}
		}
		else
		{
			repeat_timer = 0;
			last_modifier_state = special_debounce;
		}
	}
	else
	{
		last_key_pressed = -1;

		if (vdt[unit].keyboard_data_ready)
		{	/* keyboard buffer full */
			/* do nothing */
		}
		else
		{
			for (i=0; i<6; i++)
			{
				for (j=0; j<16; j++)
				{
					if (key_buf[i] & (1 << j))
					{
						last_key_pressed = (i << 4) | j;
						last_modifier_state = modifier_state;

						vdt[unit].keyboard_data = (int)key_translate[vdt[unit].model][modifier_state][(int)last_key_pressed];
						vdt[unit].keyboard_data_ready = 1;
						if (vdt[unit].keyboard_interrupt_enable)
							(*vdt[unit].int_callback)(1);
						return;
					}
				}
			}
		}
	}
}
