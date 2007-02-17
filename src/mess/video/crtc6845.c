/***************************************************************************

	Motorola 6845 CRT controller and emulation

	This code emulates the functionality of the 6845 chip, and also
	supports the functionality of chips related to the 6845

	Peter Trauner
	Nathan Woods

	Registers
		00 - Horiz. total characters
		01 - Horiz. displayed characters per line
		02 - Horiz. synch position
		03 - Horiz. synch width in characters
		04 - Vert. total lines
		05 - Vert. total adjust (scan lines)
		06 - Vert. displayed rows
		07 - Vert. synch position (character rows)
		08 - Interlace mode
		09 - Maximum scan line address
		0A - Cursor start (scan line)
		0B - Cursor end (scan line)
		0C - Start address (MSB)
		0D - Start address (LSB)
		0E - Cursor address (MSB) (read/write)
		0F - Cursor address (LSB) (read/write)
		10 - Light pen (MSB)   (read only)
		11 - Light pen (LSB)   (read only)

***************************************************************************/

#include <stdio.h>
#include "mame.h"
#include "video/generic.h"
#include "state.h"

#include "mscommon.h"
#include "includes/crtc6845.h"

/***************************************************************************

	Constants & macros

***************************************************************************/

#define VERBOSE 0

#if VERBOSE
#define DBG_LOG(N,M,A)      \
    if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(N,M,A)
#endif

/***************************************************************************

	Type definitions

***************************************************************************/

struct crtc6845
{
	struct crtc6845_config config;
	UINT8 reg[18];
	UINT8 idx;
	double cursor_time;
	int cursor_on;
};

struct reg_mask
{
	UINT8 store_mask;
	UINT8 read_mask;
};

struct crtc6845 *crtc6845;

/***************************************************************************

	Local variables

***************************************************************************/

/*-------------------------------------------------
	crtc6845_reg_mask - an array specifying how
	much of any given register "registers", per
	m6845 personality
-------------------------------------------------*/

static const struct reg_mask crtc6845_reg_mask[2][18] =
{
	{
		/* M6845_PERSONALITY_GENUINE */
		{ 0xff, 0x00 },	{ 0xff, 0x00 },	{ 0xff, 0x00 },	{ 0xff, 0x00 },
		{ 0x7f, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x7f, 0x00 },
		{ 0x3f, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x1f, 0x00 },
		{ 0x3f, 0x3f },	{ 0xff, 0xff },	{ 0x3f, 0x3f },	{ 0xff, 0xff },
		{ 0xff, 0x3f },	{ 0xff, 0xff }
	},
	{
		/* M6845_PERSONALITY_PC1512 */
		{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },
		{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },	{ 0x00, 0x00 },
		{ 0x00, 0x00 },	{ 0x1f, 0x00 },	{ 0x7f, 0x00 },	{ 0x1f, 0x00 },
		{ 0x3f, 0x3f },	{ 0xff, 0xff },	{ 0x3f, 0x3f },	{ 0xff, 0xff },
		{ 0xff, 0x3f },	{ 0xff, 0xff }
	}
};

/* The PC1512 has not got a full MC6845; the first 9 registers act as if they
 * had these hardwired values: */
static UINT8 pc1512_defaults[] =
{
	113, 80, 90, 10, 127, 6, 100, 112, 2 
};

/***************************************************************************

	Functions

***************************************************************************/

static void crtc6845_state_postload(void)
{
	if (dirtybuffer && videoram_size)
		memset(dirtybuffer, 1, videoram_size);
}

struct crtc6845 *crtc6845_init(const struct crtc6845_config *config)
{
	struct crtc6845 *crtc;
	int idx;

	crtc = auto_malloc(sizeof(struct crtc6845));
	memset(crtc, 0, sizeof(*crtc));
	crtc->cursor_time = timer_get_time();
	crtc->config = *config;
	crtc6845 = crtc;

	/* Hardwire the values which can't be changed in the PC1512 version */
	if (config->personality == M6845_PERSONALITY_PC1512)
	{
		for (idx = 0; idx < sizeof(pc1512_defaults); idx++)
		{
			crtc->reg[idx] = pc1512_defaults[idx];
		}
	}

	state_save_register_item_array("crtc6845", 0, crtc->reg);
	state_save_register_item("crtc6845", 0, crtc->idx);
	state_save_register_func_postload(crtc6845_state_postload);
	return crtc;
}

#define REG(x) (crtc->reg[x])

static int crtc6845_clocks_in_frame(struct crtc6845 *crtc)
{
	int clocks=CRTC6845_COLUMNS*CRTC6845_LINES;
	switch (CRTC6845_INTERLACE_MODE) {
	case CRTC6845_INTERLACE_SIGNAL: // interlace generation of video signals only
	case CRTC6845_INTERLACE: // interlace
		return clocks/2;
	default:
		return clocks;
	}
}

void crtc6845_set_clock(struct crtc6845 *crtc, int freq)
{
	assert(crtc);
	crtc->config.freq = freq;
}

void crtc6845_time(struct crtc6845 *crtc)
{
	double neu, ftime;
	struct crtc6845_cursor cursor;

	neu = timer_get_time();

	if (crtc6845_clocks_in_frame(crtc) == 0.0)
		return;

	ftime = crtc6845_clocks_in_frame(crtc) * 16.0 / crtc->config.freq;

	if (CRTC6845_CURSOR_MODE==CRTC6845_CURSOR_32FRAMES)
		ftime*=2;

	if (neu-crtc->cursor_time>ftime)
	{
		crtc->cursor_time += ftime;
		crtc6845_get_cursor(crtc, &cursor);

		if (crtc->config.cursor_changed)
			crtc->config.cursor_changed(&cursor);

		crtc->cursor_on ^= 1;
	}
}

int crtc6845_get_char_columns(struct crtc6845 *crtc) 
{ 
	return CRTC6845_CHAR_COLUMNS;
}

int crtc6845_get_char_height(struct crtc6845 *crtc) 
{
	return CRTC6845_CHAR_HEIGHT;
}

int crtc6845_get_char_lines(struct crtc6845 *crtc) 
{ 
	return CRTC6845_CHAR_LINES;
}

int crtc6845_get_start(struct crtc6845 *crtc) 
{
	return CRTC6845_VIDEO_START;
}


void crtc6845_set_char_columns(struct crtc6845 *crtc, UINT8 columns)
{ 
	crtc->reg[1] = columns;
}


void crtc6845_set_char_lines(struct crtc6845 *crtc, UINT8 lines)
{ 
	crtc->reg[6] = lines;
}


int crtc6845_get_personality(struct crtc6845 *crtc)
{
	return crtc->config.personality;
}

void crtc6845_get_cursor(struct crtc6845 *crtc, struct crtc6845_cursor *cursor)
{
	switch (CRTC6845_CURSOR_MODE) {
	case CRTC6845_CURSOR_OFF:
		cursor->on = 0;
		break;

	case CRTC6845_CURSOR_16FRAMES:
	case CRTC6845_CURSOR_32FRAMES:
		cursor->on = crtc->cursor_on;
		break;

	default:
		cursor->on = 1;
		break;
	}

	cursor->pos = CRTC6845_CURSOR_POS;
	cursor->top = CRTC6845_CURSOR_TOP;
	cursor->bottom = CRTC6845_CURSOR_BOTTOM;
}

UINT8 crtc6845_port_r(struct crtc6845 *crtc, int offset)
{
	UINT8 val = 0xff;
	int idx;

	if (offset & 1)
	{
		idx = crtc->idx & 0x1f;
		if (idx < (sizeof(crtc->reg) / sizeof(crtc->reg[0])))
			val = crtc->reg[idx] & crtc6845_reg_mask[crtc->config.personality][idx].read_mask;
	}
	else
	{
		val = crtc->idx;
	}
	return val;
}

int crtc6845_port_w(struct crtc6845 *crtc, int offset, UINT8 data)
{
	struct crtc6845_cursor cursor;
	int idx;
	UINT8 mask;

	if (offset & 1)
	{
		/* write to a 6845 register, if supported */
		idx = crtc->idx & 0x1f;
		if (idx < (sizeof(crtc->reg) / sizeof(crtc->reg[0])))
		{
			mask = crtc6845_reg_mask[crtc->config.personality][idx].store_mask;
			/* Don't zero out bits not covered by the mask. */
			crtc->reg[crtc->idx] &= ~mask;
			crtc->reg[crtc->idx] |= (data & mask);

			/* are there special consequences to writing to this register? */
			switch (idx) {
			case 0xa:
			case 0xb:
			case 0xe:
			case 0xf:
				crtc6845_get_cursor(crtc, &cursor);
				if (crtc->config.cursor_changed)
					crtc->config.cursor_changed(&cursor);
				break;
			}

			/* since the PC1512 does not support the number of lines register directly, 
			 * this value is keyed off of register 9
			 */
			if ((crtc->config.personality == M6845_PERSONALITY_PC1512) && (idx == 9))
			{
				UINT8 char_height;
				char_height = crtc6845_get_char_height(crtc);
				crtc6845_set_char_lines(crtc, 200 ); // / char_height);
			}
			return TRUE;
		}
	}
	else
	{
		/* change the idx register */
		crtc->idx = data;
	}
	return FALSE;
}

 READ8_HANDLER ( crtc6845_0_port_r )
{
	return crtc6845_port_r(crtc6845, offset);
}

WRITE8_HANDLER ( crtc6845_0_port_w )
{
	crtc6845_port_w(crtc6845, offset, data);
}

