/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "emu.h"
#include "includes/lynx.h"
#include "cpu/m6502/m6502.h"
#include "hashfile.h"
#include "imagedev/cartslot.h"
#include "hash.h"




#define MATH_A		0x55
#define MATH_B		0x54
#define MATH_C		0x53
#define MATH_D		0x52
#define MATH_E		0x63
#define MATH_F		0x62
#define MATH_G		0x61
#define MATH_H		0x60
#define MATH_J		0x6f
#define MATH_K		0x6e
#define MATH_L		0x6d
#define MATH_M		0x6c
#define MATH_N		0x57
#define MATH_P		0x56
#define SPRG0		0x91
#define SPRSYS		0x92


#define PAD_UP		0x80
#define PAD_DOWN	0x40
#define PAD_LEFT	0x20
#define PAD_RIGHT	0x10


/****************************************

    Graphics Drawing

****************************************/

/*
2008-10 FP:
Current implementation: lynx_blitter reads what will be drawn and sets which line_functions to use.
It then calls lynx_blit_lines which sets the various flip bits (horizontal and vertical) and calls
the chosen line_function. These functions (available in various versions, depending on how many
color bits are to be used) finally call lynx_plot_pixel which draws the sprite.

Notice however that, based on the problems in Electrocop, Jimmy Connors Tennis and Switchblade II
(among the others), it clearly seems that something is being lost in some of the passages. From
my partial understanding while comparing the code with the manual, I would suspect the loops in
the line_functions to be not completely correct.

This whole part will be eventually moved to video/ once I'm satisfied (or I give up).
*/

#define GET_WORD(mem, index) ((mem)[(index)]|((mem)[(index)+1]<<8))

/* modes from blitter command */
enum {
	BACKGROUND = 0,
	BACKGROUND_NO_COLL,
	BOUNDARY_SHADOW,
	BOUNDARY,
	NORMAL_SPRITE,
	NO_COLL,
	XOR_SPRITE,
	SHADOW
};


/* The pen numbers range from '0' to 'F. Pen numbers '1' thru 'D' are always collidable and opaque. The other
ones have different behavior depending on the sprite type: there are 8 types of sprites, each has different
characteristics relating to some or all of their pen numbers.

* Shadow Error: The hardware is missing an inverter in the 'shadow' generator. This causes sprite types that
did not invoke shadow to now invoke it and vice versa. The only actual functionality loss is that 'exclusive or'
sprites and 'background' sprites will have shadow enabled.

The sprite types relate to specific hardware functions according to the following table:


   -------------------------- SHADOW
  |   ----------------------- BOUNDARY_SHADOW
  |  |   -------------------- NORMAL_SPRITE
  |  |  |   ----------------- BOUNDARY
  |  |  |  |   -------------- BACKGROUND (+ shadow, due to bug in 'E' pen)
  |  |  |  |  |   ----------- BACKGROUND_NO_COLL
  |  |  |  |  |  |   -------- NO_COLL
  |  |  |  |  |  |  |   ----- XOR_SPRITE (+ shadow, due to bug in 'E' pen)
  |  |  |  |  |  |  |  |
  1  0  1  0  1  1  1  1      F is opaque
  0  0  1  1  0  0  0  0      E is collideable
  0  0  1  1  0  0  0  0      0 is opaque and collideable
  1  1  1  1  0  0  0  1      allow collision detect
  1  1  1  1  1  0  0  1      allow coll. buffer access
  0  0  0  0  0  0  0  1      exclusive-or the data
*/

INLINE void lynx_plot_pixel(lynx_state *state, const int mode, const int x, const int y, const int color)
{
	int back;
	UINT8 *screen;
	UINT8 *colbuf;

	state->m_blitter.everon = TRUE;
	screen = state->m_blitter.mem + state->m_blitter.screen + y * 80 + x / 2;
	colbuf = state->m_blitter.mem + state->m_blitter.colbuf + y * 80 + x / 2;

	switch (mode)
	{
		case NORMAL_SPRITE:
		/* A sprite may be set to 'normal'. This means that pen number '0' will be transparent and
        non-collideable. All other pens will be opaque and collideable */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
			{
				*screen = (*screen & 0x0f) | (color << 4);

				if(state->m_sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (state->m_blitter.spritenr << 4);
					if ((back >> 4) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back >> 4;
					state->m_blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if(state->m_sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (state->m_blitter.spritenr);
					if ((back & 0x0f) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back & 0x0f;
					state->m_blitter.memory_accesses += 2;
				}
			}
			state->m_blitter.memory_accesses++;
			break;

		case BOUNDARY:
		/* A sprite may be set to 'boundary'. This is a 'normal' sprite with the exception that pen
        number 'F' is transparent (and still collideable). */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0x0f) | (color << 4);
					state->m_blitter.memory_accesses++;
				}
				if(state->m_sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (state->m_blitter.spritenr << 4);
					if ((back >> 4) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back >> 4;
					state->m_blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0xf0) | color;
					state->m_blitter.memory_accesses++;
				}
				if(state->m_sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (state->m_blitter.spritenr);
					if ((back & 0x0f) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back & 0x0f;
					state->m_blitter.memory_accesses += 2;
				}
			}
			break;

		case SHADOW:
		/* A sprite may be set to 'shadow'. This is a 'normal' sprite with the exception that pen
        number 'E' is non-collideable (but still opaque) */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
			{
				*screen = (*screen & 0x0f) | (color << 4);

				if (state->m_sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (state->m_blitter.spritenr << 4);
					if ((back >> 4) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back >> 4;
					state->m_blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if (state->m_sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (state->m_blitter.spritenr);
					if ((back & 0x0f) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back & 0x0f;
					state->m_blitter.memory_accesses += 2;
				}
			}
			state->m_blitter.memory_accesses++;
			break;

		case BOUNDARY_SHADOW:
		/* This sprite is a 'normal' sprite with the characteristics of both 'boundary'
        and 'shadow'. That is, pen number 'F' is transparent (and still collideable) and
        pen number 'E' is non-collideable (but still opaque). */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0x0f) | (color << 4);
					state->m_blitter.memory_accesses++;
				}
				if (state->m_sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (state->m_blitter.spritenr << 4);
					if ((back >> 4) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back >> 4;
					state->m_blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0xf0) | color;
					state->m_blitter.memory_accesses++;
				}
				if (state->m_sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (state->m_blitter.spritenr);
					if ((back & 0x0f) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back & 0x0f;
					state->m_blitter.memory_accesses += 2;
				}
			}
			break;

		case BACKGROUND:
		/* A sprite may be set to 'background'. This sprite will overwrite the contents of the video and
        collision buffers. Pens '0' and 'F' are no longer transparent. This sprite is used to initialize
        the buffers at the start of a 'painting'. Additionally, no collision detection is done, and no write
        to the collision depository occurs. The 'E' error will cause the pen number 'E' to be non-collideable
        and therefore not clear the collision buffer */
			if (!(x & 0x01))		/* Upper nibble */
			{
				*screen = (*screen & 0x0f) | (color << 4);

				if (state->m_sprite_collide && (color != 0x0e))
				{
					*colbuf = (*colbuf & ~0xf0) | (state->m_blitter.spritenr << 4);
					state->m_blitter.memory_accesses++;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if (state->m_sprite_collide && (color != 0x0e))
				{
					*colbuf = (*colbuf & ~0x0f) | (state->m_blitter.spritenr);
					state->m_blitter.memory_accesses++;
				}
			}
			state->m_blitter.memory_accesses++;
			break;

		case BACKGROUND_NO_COLL:
		/* This is a 'background' sprite with the exception that no activity occurs in the collision buffer */
			if (!(x & 0x01))		/* Upper nibble */
				*screen = (*screen & 0x0f) | (color << 4);
			else					/* Lower nibble */
				*screen = (*screen & 0xf0) | color;
			state->m_blitter.memory_accesses++;
			break;

		case NO_COLL:
		/* A sprite may be set to 'non-collideable'. This means that it will have no affect on the contents of
        the collision buffer and all other collision activities are overridden (pen 'F' is not collideable). */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
				*screen = (*screen & 0x0f) | (color << 4);
			else					/* Lower nibble */
				*screen = (*screen & 0xf0) | color;
			state->m_blitter.memory_accesses++;
			break;

		case XOR_SPRITE:
		/* This is a 'normal' sprite with the exception that the data from the video buffer is exclusive-ored
        with the sprite data and written back out to the video buffer. Collision activity is 'normal'. The 'E'
        error will cause the pen number 'E' to be non-collideable and therefore not react with the collision
        buffer */
			if (color == 0)
				break;
			if (!(x & 0x01))		/* Upper nibble */
			{
				*screen = (*screen & 0x0f)^(color<<4);

				if (color != 0x0e)
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (state->m_blitter.spritenr << 4);
					if ((back >> 4) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back >> 4;
					state->m_blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0)^color;

				if (color != 0x0e)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (state->m_blitter.spritenr);
					if ((back & 0x0f) > state->m_blitter.mem[state->m_blitter.colpos])
						state->m_blitter.mem[state->m_blitter.colpos] = back & 0x0f;
					state->m_blitter.memory_accesses += 2;
				}
			}
			state->m_blitter.memory_accesses++;
			break;
	}
}

static void lynx_blit_do_work( lynx_state *state, const int y, const int xdir, const int bits, const int mask )
{
	int j, xi, wi, i;
	int b, p, color;

	i = state->m_blitter.mem[state->m_blitter.bitmap];
	state->m_blitter.memory_accesses++;		// ?

	for (xi = state->m_blitter.x, p = 0, b = 0, j = 1, wi = 0; j < i;)
	{
		if (p < bits)
		{
			b = (b << 8) | state->m_blitter.mem[state->m_blitter.bitmap + j];
			j++;
			p += 8;
			state->m_blitter.memory_accesses++;
		}
		for ( ; (p >= bits); )
		{
			color = state->m_blitter.color[(b >> (p - bits)) & mask];
			p -= bits;
			for ( ; wi < state->m_blitter.width; wi += 0x100, xi += xdir)
			{
				if ((xi >= 0) && (xi < 160))
					lynx_plot_pixel(state, state->m_blitter.mode, xi, y, color);
			}
			wi -= state->m_blitter.width;
		}
	}
}


static void lynx_blit_2color_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 1;
	const int mask = 0x01;

	lynx_blit_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_4color_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 2;
	const int mask = 0x03;

	lynx_blit_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_8color_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 3;
	const int mask = 0x07;

	lynx_blit_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_16color_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 4;
	const int mask = 0x0f;

	lynx_blit_do_work(state, y, xdir, bits, mask);
}

/*
2 color rle: ??
 0, 4 bit repeat count-1, 1 bit color
 1, 4 bit count of values-1, 1 bit color, ....
*/

/*
4 color rle:
 0, 4 bit repeat count-1, 2 bit color
 1, 4 bit count of values-1, 2 bit color, ....
*/

/*
8 color rle:
 0, 4 bit repeat count-1, 3 bit color
 1, 4 bit count of values-1, 3 bit color, ....
*/

/*
16 color rle:
 0, 4 bit repeat count-1, 4 bit color
 1, 4 bit count of values-1, 4 bit color, ....
*/

static void lynx_blit_rle_do_work( lynx_state *state, const int y, const int xdir, const int bits, const int mask )
{
	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p = 0, j = 0, b = 0, xi = state->m_blitter.x, wi = 0; ; )		/* through the rle entries */
	{
		if (p < 5 + bits) /* under 7 bits no complete entry */
		{
			j++;
			if (j >= state->m_blitter.mem[state->m_blitter.bitmap])
				return;

			p += 8;
			b = (b << 8) | state->m_blitter.mem[state->m_blitter.bitmap + j];
			state->m_blitter.memory_accesses++;
		}

		t = (b >> (p - 1)) & 0x01;
		p--;
		count = ((b >> (p - 4)) & 0x0f) + 1;
		p -= 4;

		if (t)		/* count of different pixels */
		{
			for ( ; count; count--)
			{
				if (p < bits)
				{
					j++;
					if (j >= state->m_blitter.mem[state->m_blitter.bitmap])
						return;
					p += 8;
					b = (b << 8) | state->m_blitter.mem[state->m_blitter.bitmap + j];
					state->m_blitter.memory_accesses++;
				}

				color = state->m_blitter.color[(b >> (p - bits)) & mask];
				p -= bits;
				for ( ; wi < state->m_blitter.width; wi += 0x100, xi += xdir)
				{
					if ((xi >= 0) && (xi < 160))
						lynx_plot_pixel(state, state->m_blitter.mode, xi, y, color);
				}
				wi -= state->m_blitter.width;
			}
		}
		else		/* count of same pixels */
		{
			if (count == 0)
				return;

			if (p < bits)
			{
				j++;
				if (j >= state->m_blitter.mem[state->m_blitter.bitmap])
					return;
				p += 8;
				b = (b << 8) | state->m_blitter.mem[state->m_blitter.bitmap + j];
				state->m_blitter.memory_accesses++;
			}

			color = state->m_blitter.color[(b >> (p - bits)) & mask];
			p -= bits;

			for ( ; count; count--)
			{
				for ( ; wi < state->m_blitter.width; wi += 0x100, xi += xdir)
				{
					if ((xi >= 0) && (xi < 160))
						lynx_plot_pixel(state, state->m_blitter.mode, xi, y, color);
				}
				wi -= state->m_blitter.width;
			}
		}
	}
}

static void lynx_blit_2color_rle_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 1;
	const int mask = 0x01;

	lynx_blit_rle_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_4color_rle_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 2;
	const int mask = 0x03;

	lynx_blit_rle_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_8color_rle_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 3;
	const int mask = 0x07;

	lynx_blit_rle_do_work(state, y, xdir, bits, mask);
}
static void lynx_blit_16color_rle_line(lynx_state *state, const int y, const int xdir)
{
	const int bits = 4;
	const int mask = 0x0f;

	lynx_blit_rle_do_work(state, y, xdir, bits, mask);
}


static void lynx_blit_lines(lynx_state *state)
{
	int i, hi, y;
	int ydir = 0, xdir = 0;
	int flip = 0;

	state->m_blitter.everon = FALSE;

	// flipping sprdemo3
	// fat bobby 0x10
	// mirror the sprite in gameplay?
	xdir = 1;

	if (state->m_blitter.mem[state->m_blitter.cmd] & 0x20)	/* Horizontal Flip */
	{
		xdir = -1;
		state->m_blitter.x--;	/*?*/
	}

	ydir = 1;

	if (state->m_blitter.mem[state->m_blitter.cmd] & 0x10)	/* Vertical Flip */
	{
		ydir = -1;
		state->m_blitter.y--;	/*?*/
	}

	switch (state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x03)	/* Start Left & Start Up */
	{
		case 0:
			flip =0;
			break;

		case 1: // blockout
			xdir *= -1;
			flip = 1;
			break;

		case 2: // fat bobby horicontal
			ydir *= -1;
			flip = 1;
			break;

		case 3:
			xdir *= -1;
			ydir *= -1;
			flip = 3;
			break;
	}

	for (y = state->m_blitter.y, hi = 0; state->m_blitter.memory_accesses++, i = state->m_blitter.mem[state->m_blitter.bitmap]; state->m_blitter.bitmap += i )
	{
		if (i == 1)
		{
			// centered sprites sprdemo3, fat bobby, blockout
			hi = 0;
			switch (flip & 0x03)
			{
				case 0:
				case 2:
					ydir *= -1;
					state->m_blitter.y += ydir;
					break;

				case 1:
				case 3:
					xdir *= -1;
					state->m_blitter.x += xdir;
					break;
			}

			y = state->m_blitter.y;
			flip++;
			continue;
		}

		for ( ; (hi < state->m_blitter.height); hi += 0x100, y += ydir)
		{
			if (y >= 0 && y < 102)
				state->m_blitter.line_function(state, y, xdir);
			state->m_blitter.width += state->m_blitter.stretch;
			state->m_blitter.x += state->m_blitter.tilt;
		}

		hi -= state->m_blitter.height;
	}

	if (state->m_suzy.data[SPRG0] & 0x04)
	{
		if (state->m_sprite_collide & !state->m_blitter.everon)
			state->m_blitter.mem[state->m_blitter.colpos] |= 0x80;
		else
			state->m_blitter.mem[state->m_blitter.colpos] &= 0x7f;
	}
}

static TIMER_CALLBACK(lynx_blitter_timer)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	state->m_suzy.data[SPRSYS] &= ~0x01; //state->m_blitter finished
}

/*
  control 0
   bit 7,6: 00 2 color
            01 4 color
            11 8 colors?
            11 16 color
   bit 5,4: 00 right down
            01 right up
            10 left down
            11 left up

#define SHADOW         (0x07)
#define XORSHADOW      (0x06)
#define NONCOLLIDABLE  (0x05)
#define NORMAL         (0x04)
#define BOUNDARY       (0x03)
#define BOUNDARYSHADOW (0x02)
#define BKGRNDNOCOL    (0x01)
#define BKGRND         (0x00)

  control 1
   bit 7: 0 bitmap rle encoded
          1 not encoded
   bit 3: 0 color info with command
          1 no color info with command

#define RELHVST        (0x30)
#define RELHVS         (0x20)
#define RELHV          (0x10)

#define SKIPSPRITE     (0x04)

#define DUP            (0x02)
#define DDOWN          (0x00)
#define DLEFT          (0x01)
#define DRIGHT         (0x00)


  coll
#define DONTCOLLIDE    (0x20)

  word next
  word data
  word x
  word y
  word width
  word height

  pixel c0 90 20 0000 datapointer x y 0100 0100 color (8 colorbytes)
  4 bit direct?
  datapointer 2 10 0
  98 (0 colorbytes)

  box c0 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  c1 98 00 4 bit direct without color bytes (raycast)

  40 10 20 4 bit rle (sprdemo2)

  line c1 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color (8 color bytes)
  or
  line c0 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color
  datapointer 2 11 0

  text ?(04) 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  stretch: hsize adder
  tilt: hpos adder

*/

static void lynx_blitter(running_machine &machine)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	static const int lynx_colors[4]={2,4,8,16};

	static void (* const blit_line[4])(lynx_state *state, const int y, const int xdir)= {
	lynx_blit_2color_line,
	lynx_blit_4color_line,
	lynx_blit_8color_line,
	lynx_blit_16color_line
	};

	static void (* const blit_rle_line[4])(lynx_state *state, const int y, const int xdir)= {
	lynx_blit_2color_rle_line,
	lynx_blit_4color_rle_line,
	lynx_blit_8color_rle_line,
	lynx_blit_16color_rle_line
	};
	int i; int o;int colors;

	state->m_blitter.memory_accesses = 0;
	state->m_blitter.mem = (UINT8*)machine.device("maincpu")->memory().space(AS_PROGRAM)->get_read_ptr(0x0000);

	state->m_blitter.xoff   = GET_WORD(state->m_suzy.data, 0x04);
	state->m_blitter.yoff   = GET_WORD(state->m_suzy.data, 0x06);
	state->m_blitter.screen = GET_WORD(state->m_suzy.data, 0x08);
	state->m_blitter.colbuf = GET_WORD(state->m_suzy.data, 0x0a);
	// hsizeoff GET_WORD(state->m_suzy.data, 0x28)
	// vsizeoff GET_WORD(state->m_suzy.data, 0x2a)

	// these might be never set by the state->m_blitter hardware
	state->m_blitter.width   = 0x100;
	state->m_blitter.height  = 0x100;
	state->m_blitter.stretch = 0;
	state->m_blitter.tilt    = 0;
	state->m_sprite_collide  = 0;

	state->m_blitter.memory_accesses += 2;

	for (state->m_blitter.cmd = GET_WORD(state->m_suzy.data, 0x10); state->m_blitter.cmd; )
	{
		state->m_blitter.memory_accesses += 1;

		if (!(state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x04))		// if 0, we skip this sprite
		{
			state->m_blitter.colpos = GET_WORD(state->m_suzy.data, 0x24) + state->m_blitter.cmd;

			state->m_blitter.bitmap = GET_WORD(state->m_blitter.mem,state->m_blitter.cmd + 5);
			state->m_blitter.x = (INT16)GET_WORD(state->m_blitter.mem, state->m_blitter.cmd + 7) - state->m_blitter.xoff;
			state->m_blitter.y = (INT16)GET_WORD(state->m_blitter.mem, state->m_blitter.cmd + 9) - state->m_blitter.yoff;
			state->m_blitter.memory_accesses += 6;

			state->m_blitter.mode = state->m_blitter.mem[state->m_blitter.cmd] & 0x07;

			if (state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x80)
				state->m_blitter.line_function = blit_line[state->m_blitter.mem[state->m_blitter.cmd] >> 6];
			else
				state->m_blitter.line_function = blit_rle_line[state->m_blitter.mem[state->m_blitter.cmd] >> 6];

			if (!(state->m_blitter.mem[state->m_blitter.cmd + 2] & 0x20) && !(state->m_suzy.data[SPRSYS] & 0x20))
			{
				switch (state->m_blitter.mode)
				{
					case BACKGROUND:
					case BOUNDARY_SHADOW:
					case BOUNDARY:
					case NORMAL_SPRITE:
					case XOR_SPRITE:
					case SHADOW:
						state->m_sprite_collide = 1;
						state->m_blitter.mem[state->m_blitter.colpos] = 0;
						state->m_blitter.spritenr = state->m_blitter.mem[state->m_blitter.cmd + 2] & 0x0f;
				}
			}

			/* Sprite Reload Bits */
			o = 0x0b;
			if (state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x30)
			{
				state->m_blitter.width  = GET_WORD(state->m_blitter.mem, state->m_blitter.cmd + 11);
				state->m_blitter.height = GET_WORD(state->m_blitter.mem, state->m_blitter.cmd + 13);
				state->m_blitter.memory_accesses += 4;
				o += 4;
			}
			if (state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x20)
			{
				state->m_blitter.stretch = GET_WORD(state->m_blitter.mem, state->m_blitter.cmd + o);
				state->m_blitter.memory_accesses += 2;
				o += 2;
				if (state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x10)
				{
					state->m_blitter.tilt = GET_WORD(state->m_blitter.mem, state->m_blitter.cmd+o);
					state->m_blitter.memory_accesses += 2;
					o += 2;
				}
			}

			/* Reload Palette Bit */
			colors = lynx_colors[state->m_blitter.mem[state->m_blitter.cmd] >> 6];

			if (!(state->m_blitter.mem[state->m_blitter.cmd + 1] & 0x08))
			{
				for (i = 0; i < colors / 2; i++)
				{
					state->m_blitter.color[i * 2]      = state->m_blitter.mem[state->m_blitter.cmd + o + i] >> 4;
					state->m_blitter.color[i * 2 + 1 ] = state->m_blitter.mem[state->m_blitter.cmd + o + i] & 0x0f;
					state->m_blitter.memory_accesses++;
				}
			}

			/* Draw Sprites */
			lynx_blit_lines(state);
		}

	state->m_blitter.cmd = GET_WORD(state->m_blitter.mem,  state->m_blitter.cmd + 3);
	state->m_blitter.memory_accesses += 2;

	if (!(state->m_blitter.cmd & 0xff00))
		break;
	}

	if (0)
		machine.scheduler().timer_set(machine.device<cpu_device>("maincpu")->cycles_to_attotime(state->m_blitter.memory_accesses*20), FUNC(lynx_blitter_timer));
}


/****************************************

    Suzy Emulation

****************************************/


/* Math bugs of the original hardware:

- in signed multiply, the hardware thinks that 8000 is a positive number
- in signed multiply, the hardware thinks that 0 is a negative number. This is not an immediate
problem for a multiply by zero, since the answer will be re-negated to the correct polarity of
zero. However, since it will set the sign flag, you can not depend on the sign flag to be correct
if you just load the lower byte after a multiply by zero.
- in divide, the remainder will have 2 possible errors, depending on its actual value (no further
notes on these errors available) */

static void lynx_divide( lynx_state *state )
{
	UINT32 left;
	UINT16 right;
	UINT32 res, mod;
	/*
    Hardware divide:
                EFGH
    *             NP
    ----------------
                ABCD
    Remainder (JK)LM
    */

	left = state->m_suzy.data[MATH_H] | (state->m_suzy.data[MATH_G] << 8) | (state->m_suzy.data[MATH_F] << 16) | (state->m_suzy.data[MATH_E] << 24);
	right = state->m_suzy.data[MATH_P] | (state->m_suzy.data[MATH_N] << 8);

	state->m_suzy.accumulate_overflow = FALSE;
	if (right == 0)
	{
		state->m_suzy.accumulate_overflow = TRUE;	/* during divisions, this bit is used to detect denominator = 0 */
		res = 0xffffffff;
		mod = 0; //?
	}
	else
	{
		res = left / right;
		mod = left % right;
	}
//  logerror("coprocessor %8x / %8x = %4x\n", left, right, res);
	state->m_suzy.data[MATH_D] = res & 0xff;
	state->m_suzy.data[MATH_C] = res >> 8;
	state->m_suzy.data[MATH_B] = res >> 16;
	state->m_suzy.data[MATH_A] = res >> 24;

	state->m_suzy.data[MATH_M] = mod & 0xff;
	state->m_suzy.data[MATH_L] = mod >> 8;
	state->m_suzy.data[MATH_K] = mod >> 16;
	state->m_suzy.data[MATH_J] = mod >> 24;
}

static void lynx_multiply( lynx_state *state )
{
	UINT16 left, right;
	UINT32 res, accu;
	/*
    Hardware multiply:
                  AB
    *             CD
    ----------------
                EFGH
    Accumulate  JKLM
    */
	state->m_suzy.accumulate_overflow = FALSE;

	left = state->m_suzy.data[MATH_B] | (state->m_suzy.data[MATH_A] << 8);
	right = state->m_suzy.data[MATH_D] | (state->m_suzy.data[MATH_C] << 8);

	res = left * right;

	if (state->m_suzy.data[SPRSYS] & 0x80)		/* signed math */
	{
		if (!(state->m_sign_AB + state->m_sign_CD))	/* different signs */
			res = (res ^ 0xffffffff) + 1;
	}

	state->m_suzy.data[MATH_H] = res & 0xff;
	state->m_suzy.data[MATH_G] = res >> 8;
	state->m_suzy.data[MATH_F] = res >> 16;
	state->m_suzy.data[MATH_E] = res >> 24;

	if (state->m_suzy.data[SPRSYS] & 0x40)		/* is accumulation allowed? */
	{
		accu = state->m_suzy.data[MATH_M] | state->m_suzy.data[MATH_L] << 8 | state->m_suzy.data[MATH_K] << 16 | state->m_suzy.data[MATH_J] << 24;
		accu += res;

		if (accu < res)
			state->m_suzy.accumulate_overflow = TRUE;

		state->m_suzy.data[MATH_M] = accu;
		state->m_suzy.data[MATH_L] = accu >> 8;
		state->m_suzy.data[MATH_K] = accu >> 16;
		state->m_suzy.data[MATH_J] = accu >> 24;
	}
}

static READ8_HANDLER( suzy_read )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	UINT8 value = 0, input;

	switch (offset)
	{
		case 0x88:
			value = 0x01; // must not be 0 for correct power up
			break;
		case 0x92:	/* Better check this with docs! */
			if (state->m_blitter.time==attotime::zero)
			{
				if (space->machine().device<cpu_device>("maincpu")->attotime_to_cycles(space->machine().time() - state->m_blitter.time) > state->m_blitter.memory_accesses * 20)
				{
					state->m_suzy.data[offset] &= ~0x01; //state->m_blitter finished
					state->m_blitter.time = attotime::zero;
				}
			}
			value = state->m_suzy.data[offset];
			value &= ~0x80; // math finished
			value &= ~0x40;
			if (state->m_suzy.accumulate_overflow)
				value |= 0x40;
			break;
		case 0xb0:
			input = input_port_read(space->machine(), "JOY");
			switch (state->m_rotate)
			{
				case 1:
					value = input;
					input &= 0x0f;
					if (value & PAD_UP) input |= PAD_LEFT;
					if (value & PAD_LEFT) input |= PAD_DOWN;
					if (value & PAD_DOWN) input |= PAD_RIGHT;
					if (value & PAD_RIGHT) input |= PAD_UP;
					break;
				case 2:
					value = input;
					input &= 0x0f;
					if (value & PAD_UP) input |= PAD_RIGHT;
					if (value & PAD_RIGHT) input |= PAD_DOWN;
					if (value & PAD_DOWN) input |= PAD_LEFT;
					if (value & PAD_LEFT) input |= PAD_UP;
					break;
			}
			if (state->m_suzy.data[SPRSYS] & 0x08) /* Left handed controls */
			{
				value = input & 0x0f;
				if (input & PAD_UP) value |= PAD_DOWN;
				if (input & PAD_DOWN) value |= PAD_UP;
				if (input & PAD_LEFT) value |= PAD_RIGHT;
				if (input & PAD_RIGHT) value |= PAD_LEFT;
			}
			else
				value = input;
			break;
		case 0xb1:
			value = input_port_read(space->machine(), "PAUSE");
			break;
		case 0xb2:
			value = *(space->machine().region("user1")->base() + (state->m_suzy.high * state->m_granularity) + state->m_suzy.low);
			state->m_suzy.low = (state->m_suzy.low + 1) & (state->m_granularity - 1);
			break;
		case 0xb3: /* we need bank 1 emulation!!! */
		default:
			value = state->m_suzy.data[offset];
	}
//  logerror("suzy read %.2x %.2x\n",offset,data);
	return value;
}

static WRITE8_HANDLER( suzy_write )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	state->m_suzy.data[offset] = data;

	/* Additional effects of a write */
	/* Even addresses are the LSB. Any CPU write to an LSB in 0x00-0x7f will set the MSB to 0. */
	/* This in particular holds for math quantities:  Writing to B (0x54), D (0x52),
    F (0x62), H (0x60), K (0x6e) or M (0x6c) will force a '0' to be written to A (0x55),
    C (0x53), E (0x63), G (0x61), J (0x6f) or L (0x6d) respectively */
	switch(offset)
	{
	case 0x00: case 0x02: case 0x04: case 0x06: case 0x08: case 0x0a: case 0x0c: case 0x0e:
	case 0x10: case 0x12: case 0x14: case 0x16: case 0x18: case 0x1a: case 0x1c: case 0x1e:
	case 0x20: case 0x22: case 0x24: case 0x26: case 0x28: case 0x2a: case 0x2c: case 0x2e:
	case 0x30: case 0x32: case 0x34: case 0x36: case 0x38: case 0x3a: case 0x3c: case 0x3e:
	case 0x40: case 0x42: case 0x44: case 0x46: case 0x48: case 0x4a: case 0x4c: case 0x4e:
	case 0x50: case 0x56: case 0x58: case 0x5a: case 0x5c: case 0x5e:
	case 0x64: case 0x66: case 0x68: case 0x6a:
	case 0x70: case 0x72: case 0x74: case 0x76: case 0x78: case 0x7a: case 0x7c: case 0x7e:
	/* B, D, F, H , K */
	case 0x52: case 0x54: case 0x60: case 0x62: case 0x6e:
		state->m_suzy.data[offset + 1] = 0;
		break;
	/* Writing to M (0x6c) will also clear the accumulator overflow bit */
	case 0x6c:
		state->m_suzy.data[offset + 1] = 0;
		state->m_suzy.accumulate_overflow = FALSE;
		break;
	case 0x53:
	/* If we are going to perform a signed multiplication, we store the sign and convert the number
    to an unsigned one */
		if (state->m_suzy.data[SPRSYS] & 0x80)		/* signed math */
		{
			UINT16 factor, temp;
			factor = state->m_suzy.data[MATH_D] | (state->m_suzy.data[MATH_C] << 8);
			if ((factor - 1) & 0x8000)		/* here we use -1 to cover the math bugs on the sign of 0 and 0x8000 */
			{
				temp = (factor ^ 0xffff) + 1;
				state->m_sign_CD = - 1;
				state->m_suzy.data[MATH_D] = temp & 0xff;
				state->m_suzy.data[MATH_C] = temp >> 8;
			}
			else
				state->m_sign_CD = 1;
		}
		break;
	/* Writing to A will start a 16 bit multiply */
	/* If we are going to perform a signed multiplication, we also store the sign and convert the
    number to an unsigned one */
	case 0x55:
		if (state->m_suzy.data[SPRSYS] & 0x80)		/* signed math */
		{
			UINT16 factor, temp;
			factor = state->m_suzy.data[MATH_B] | (state->m_suzy.data[MATH_A] << 8);
			if ((factor - 1) & 0x8000)		/* here we use -1 to cover the math bugs on the sign of 0 and 0x8000 */
			{
				temp = (factor ^ 0xffff) + 1;
				state->m_sign_AB = - 1;
				state->m_suzy.data[MATH_B] = temp & 0xff;
				state->m_suzy.data[MATH_A] = temp >> 8;
			}
			else
				state->m_sign_AB = 1;
		}
		lynx_multiply(state);
		break;
	/* Writing to E will start a 16 bit divide */
	case 0x63:
		lynx_divide(state);
		break;
	case 0x91:
		if (data & 0x01)
		{
			state->m_blitter.time = space->machine().time();
			lynx_blitter(space->machine());
		}
		break;
//  case 0xb2: case 0xb3: /* Cart Bank 0 & 1 */
	}
}


/****************************************

    Mikey emulation

****************************************/

/*
 0xfd0a r sync signal?
 0xfd81 r interrupt source bit 2 vertical refresh
 0xfd80 w interrupt quit
 0xfd87 w bit 1 !clr bit 0 blocknumber clk
 0xfd8b w bit 1 blocknumber hi B
 0xfd94 w 0
 0xfd95 w 4
 0xfda0-f rw farben 0..15
 0xfdb0-f rw bit0..3 farben 0..15
*/



static UINT8 lynx_read_vram(lynx_state *state, UINT16 address)
{
	UINT8 result = 0x00;
	if (address <= 0xfbff)
		result = state->m_mem_0000[address - 0x0000];
	else if (address <= 0xfcff)
		result = state->m_mem_fc00[address - 0xfc00];
	else if (address <= 0xfdff)
		result = state->m_mem_fd00[address - 0xfd00];
	else if (address <= 0xfff7)
		result = state->m_mem_fe00[address - 0xfe00];
	else if (address >= 0xfffa)
		result = state->m_mem_fffa[address - 0xfffa];
	return result;
}


/*
DISPCTL EQU $FD92       ; set to $D by INITMIKEY

; B7..B4        0
; B3    1 EQU color
; B2    1 EQU 4 bit mode
; B1    1 EQU flip screen
; B0    1 EQU video DMA enabled
*/
static void lynx_draw_lines(running_machine &machine, int newline)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	int h,w;
	int x, yend;
	UINT16 j; // clipping needed!
	UINT8 byte;
	UINT16 *line;

	if (newline == -1)
		yend = 102;
	else
		yend = newline;

	if (yend > 102)
		yend = 102;

	if (yend == state->m_line_y)
	{
		if (newline == -1)
			state->m_line_y = 0;
		return;
	}

	j = (state->m_mikey.data[0x94] | (state->m_mikey.data[0x95]<<8)) + state->m_line_y * 160 / 2;
	if (state->m_mikey.data[0x92] & 0x02)
		j -= 160 * 102 / 2 - 1;

	/* rotation */
	if (state->m_rotate & 0x03)
	{
		h = 160; w = 102;
		if (((state->m_rotate == 1) && (state->m_mikey.data[0x92] & 0x02)) || ((state->m_rotate == 2) && !(state->m_mikey.data[0x92] & 0x02)))
		{
			for ( ; state->m_line_y < yend; state->m_line_y++)
			{
				line = BITMAP_ADDR16(machine.generic.tmpbitmap, state->m_line_y, 0);
				for (x = 160 - 2; x >= 0; j++, x -= 2)
				{
					byte = lynx_read_vram(state, j);
					line[x + 1] = state->m_palette[(byte >> 4) & 0x0f];
					line[x + 0] = state->m_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for ( ; state->m_line_y < yend; state->m_line_y++)
			{
				line = BITMAP_ADDR16(machine.generic.tmpbitmap, 102 - 1 - state->m_line_y, 0);
				for (x = 0; x < 160; j++, x += 2)
				{
					byte = lynx_read_vram(state, j);
					line[x + 0] = state->m_palette[(byte >> 4) & 0x0f];
					line[x + 1] = state->m_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	else
	{
		w = 160; h = 102;
		if (state->m_mikey.data[0x92] & 0x02)
		{
			for ( ; state->m_line_y < yend; state->m_line_y++)
			{
				line = BITMAP_ADDR16(machine.generic.tmpbitmap, 102 - 1 - state->m_line_y, 0);
				for (x = 160 - 2; x >= 0; j++, x -= 2)
				{
					byte = lynx_read_vram(state, j);
					line[x + 1] = state->m_palette[(byte >> 4) & 0x0f];
					line[x + 0] = state->m_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for ( ; state->m_line_y < yend; state->m_line_y++)
			{
				line = BITMAP_ADDR16(machine.generic.tmpbitmap, state->m_line_y, 0);
				for (x = 0; x < 160; j++, x += 2)
				{
					byte = lynx_read_vram(state, j);
					line[x + 0] = state->m_palette[(byte >> 4) & 0x0f];
					line[x + 1] = state->m_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	if (newline == -1)
	{
		state->m_line_y = 0;
		if ((w != state->m_width) || (h != state->m_height))
		{
			state->m_width = w;
			state->m_height = h;
			machine.primary_screen->set_visible_area(0, w - 1, 0, h - 1);
		}
	}
}


/****************************************

    Timers

****************************************/

/*
HCOUNTER        EQU TIMER0
VCOUNTER        EQU TIMER2
SERIALRATE      EQU TIMER4

TIM_BAKUP       EQU 0   ; backup-value (count+1)
TIM_CNTRL1      EQU 1   ; timer-control register
TIM_CNT EQU 2   ; current counter
TIM_CNTRL2      EQU 3   ; dynamic control

; TIM_CNTRL1
TIM_IRQ EQU %10000000   ; enable interrupt (not TIMER4 !)
TIM_RESETDONE   EQU %01000000   ; reset timer done
TIM_MAGMODE     EQU %00100000   ; nonsense in Lynx !!
TIM_RELOAD      EQU %00010000   ; enable reload
TIM_COUNT       EQU %00001000   ; enable counter
TIM_LINK        EQU %00000111
; link timers (0->2->4 / 1->3->5->7->Aud0->Aud1->Aud2->Aud3->1
TIM_64us        EQU %00000110
TIM_32us        EQU %00000101
TIM_16us        EQU %00000100
TIM_8us EQU %00000011
TIM_4us EQU %00000010
TIM_2us EQU %00000001
TIM_1us EQU %00000000

;TIM_CNTRL2 (read-only)
; B7..B4 unused
TIM_DONE        EQU %00001000   ; set if timer's done; reset with TIM_RESETDONE
TIM_LAST        EQU %00000100   ; last clock (??)
TIM_BORROWIN    EQU %00000010
TIM_BORROWOUT   EQU %00000001
*/

#define NR_LYNX_TIMERS	8


static TIMER_CALLBACK(lynx_timer_shot);

static void lynx_timer_init(running_machine &machine, int which)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	memset( &state->m_timer[which], 0, sizeof(LYNX_TIMER) );
	state->m_timer[which].timer = machine.scheduler().timer_alloc(FUNC(lynx_timer_shot));
}

static void lynx_timer_signal_irq(running_machine &machine, int which)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	if ( ( state->m_timer[which].cntrl1 & 0x80 ) && ( which != 4 ) )
	{ // irq flag handling later
		state->m_mikey.data[0x81] |= ( 1 << which );
		cputag_set_input_line(machine, "maincpu", M65SC02_IRQ_LINE, ASSERT_LINE);
	}
	switch ( which )
	{
	case 0:
		lynx_timer_count_down( machine, 2 );
		state->m_line++;
		break;
	case 2:
		lynx_timer_count_down( machine, 4 );
		lynx_draw_lines( machine, -1 );
		state->m_line=0;
		break;
	case 1:
		lynx_timer_count_down( machine, 3 );
		break;
	case 3:
		lynx_timer_count_down( machine, 5 );
		break;
	case 5:
		lynx_timer_count_down( machine, 7 );
		break;
	case 7:
		lynx_audio_count_down( state->m_audio, 0 );
		break;
	}
}

void lynx_timer_count_down(running_machine &machine, int which)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	if ( ( state->m_timer[which].cntrl1 & 0x0f ) == 0x0f )
	{
		if ( state->m_timer[which].counter > 0 )
		{
			state->m_timer[which].counter--;
			return;
		}
		if ( state->m_timer[which].counter == 0 )
		{
			state->m_timer[which].cntrl2 |= 8;
			lynx_timer_signal_irq(machine, which);
			if ( state->m_timer[which].cntrl1 & 0x10 )
			{
				state->m_timer[which].counter = state->m_timer[which].bakup;
			}
			else
			{
				state->m_timer[which].counter--;
			}
			return;
		}
	}
}

static TIMER_CALLBACK(lynx_timer_shot)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	state->m_timer[param].cntrl2 |= 8;
	lynx_timer_signal_irq( machine, param );
	if ( ! ( state->m_timer[param].cntrl1 & 0x10 ) )
		state->m_timer[param].timer_active = 0;
}

static UINT32 lynx_time_factor(int val)
{
	switch(val)
	{
		case 0:	return 1000000;
		case 1: return 500000;
		case 2: return 250000;
		case 3: return 125000;
		case 4: return 62500;
		case 5: return 31250;
		case 6: return 15625;
		default: fatalerror("invalid value");
	}
}

static UINT8 lynx_timer_read(lynx_state *state, int which, int offset)
{
	UINT8 value = 0;

	switch (offset)
	{
		case 0:
			value = state->m_timer[which].bakup;
			break;
		case 1:
			value = state->m_timer[which].cntrl1;
			break;
		case 2:
			if ((state->m_timer[which].cntrl1 & 0x07) == 0x07)
			{
				value = state->m_timer[which].counter;
			}
			else
			{
				if ( state->m_timer[which].timer_active )
					value = (UINT8) ( state->m_timer[which].bakup - (state->m_timer[which].timer->remaining() * lynx_time_factor( state->m_timer[which].cntrl1 & 0x07 )).seconds);
			}
			break;

		case 3:
			value = state->m_timer[which].cntrl2;
			break;
	}
	logerror("timer %d read %x %.2x\n", which, offset, value);
	return value;
}

static void lynx_timer_write(lynx_state *state, int which, int offset, UINT8 data)
{
	logerror("timer %d write %x %.2x\n", which, offset, data);

	switch (offset)
	{
		case 0:
			state->m_timer[which].bakup = data;
			break;
		case 1:
			state->m_timer[which].cntrl1 = data;
			if (data & 0x40)
				state->m_timer[which].cntrl2 &= ~0x08;
			break;
		case 2:
//          state->m_timer[which].counter = data;   // why commented out?
			break;
		case 3:
			state->m_timer[which].cntrl2 = (state->m_timer[which].cntrl2 & 0x08) | (data & ~0x08);
			break;
	}

	/* Update timers */
	if ( offset < 3 )
	{
		state->m_timer[which].timer->reset();
		state->m_timer[which].timer_active = 0;
		if ((state->m_timer[which].cntrl1 & 0x08))		// 0x48?
		{
			if ((state->m_timer[which].cntrl1 & 0x07) != 0x07)
			{
				attotime t = (attotime::from_hz(lynx_time_factor(state->m_timer[which].cntrl1 & 0x07)) * (state->m_timer[which].bakup + 1));
				if (state->m_timer[which].cntrl1 & 0x10)
					state->m_timer[which].timer->adjust(attotime::zero, which, t);
				else
					state->m_timer[which].timer->adjust(t, which);
				state->m_timer[which].timer_active = 1;
			}
		}
	}
}


/****************************************

    UART Emulation

****************************************/


static void lynx_uart_reset(lynx_state *state)
{
	memset(&state->m_uart, 0, sizeof(state->m_uart));
}

static TIMER_CALLBACK(lynx_uart_timer)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	if (state->m_uart.buffer_loaded)
	{
		state->m_uart.data_to_send = state->m_uart.buffer;
		state->m_uart.buffer_loaded = FALSE;
		machine.scheduler().timer_set(attotime::from_usec(11), FUNC(lynx_uart_timer));
	}
	else
		state->m_uart.sending = FALSE;

//    state->m_mikey.data[0x80]|=0x10;
	if (state->m_uart.serctl & 0x80)
	{
		state->m_mikey.data[0x81] |= 0x10;
		cputag_set_input_line(machine, "maincpu", M65SC02_IRQ_LINE, ASSERT_LINE);
	}
}

static  READ8_HANDLER(lynx_uart_r)
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	UINT8 value = 0x00;
	switch (offset)
	{
		case 0x8c:
			if (!state->m_uart.buffer_loaded)
				value |= 0x80;
			if (state->m_uart.received)
				value |= 0x40;
			if (!state->m_uart.sending)
				value |= 0x20;
			break;

		case 0x8d:
			value = state->m_uart.data_received;
			break;
	}
	logerror("uart read %.2x %.2x\n", offset, value);
	return value;
}

static WRITE8_HANDLER(lynx_uart_w)
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	logerror("uart write %.2x %.2x\n", offset, data);
	switch (offset)
	{
		case 0x8c:
			state->m_uart.serctl = data;
			break;

		case 0x8d:
			if (state->m_uart.sending)
			{
				state->m_uart.buffer = data;
				state->m_uart.buffer_loaded = TRUE;
			}
			else
			{
				state->m_uart.sending = TRUE;
				state->m_uart.data_to_send = data;
				space->machine().scheduler().timer_set(attotime::from_usec(11), FUNC(lynx_uart_timer));
			}
			break;
	}
}


/****************************************

    Mikey memory handlers

****************************************/


static READ8_HANDLER( mikey_read )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	UINT8 direction, value = 0x00;

	switch (offset)
	{
	case 0x00: case 0x01: case 0x02: case 0x03:
	case 0x04: case 0x05: case 0x06: case 0x07:
	case 0x08: case 0x09: case 0x0a: case 0x0b:
	case 0x0c: case 0x0d: case 0x0e: case 0x0f:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		value = lynx_timer_read(state, offset >> 2, offset & 0x03);
		break;

	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		value = lynx_audio_read(state->m_audio, offset);
		break;

	case 0x80:
	case 0x81:
		value = state->m_mikey.data[offset];
		logerror( "mikey read %.2x %.2x\n", offset, value );
		break;

	case 0x84:
	case 0x85:
		value = 0x00;
		break;

	case 0x86:
		value = 0x80;
		break;

	case 0x88:
		value = 0x01;
		break;

	case 0x8b:
		direction = state->m_mikey.data[0x8a];
		value |= (direction & 0x01) ? (state->m_mikey.data[offset] & 0x01) : 0x01;	// External Power input
		value |= (direction & 0x02) ? (state->m_mikey.data[offset] & 0x02) : 0x00;	// Cart Address Data output (0 turns cart power on)
		value |= (direction & 0x04) ? (state->m_mikey.data[offset] & 0x04) : 0x04;	// noexp input
		// REST still to implement
		value |= (direction & 0x08) ? (state->m_mikey.data[offset] & 0x08) : 0x00;	// rest output
		value |= (direction & 0x10) ? (state->m_mikey.data[offset] & 0x10) : 0x10;	// audin input
		/* Hack: we disable COMLynx  */
		value |= 0x04;
		/* B5, B6 & B7 are not used */
		break;

	case 0x8c:
	case 0x8d:
		value = lynx_uart_r(space, offset);
		break;

	default:
		value = state->m_mikey.data[offset];
		logerror( "mikey read %.2x %.2x\n", offset, value );
	}

	return value;
}

static WRITE8_HANDLER( mikey_write )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	switch (offset)
	{
	case 0x00: case 0x01: case 0x02: case 0x03:
	case 0x04: case 0x05: case 0x06: case 0x07:
	case 0x08: case 0x09: case 0x0a: case 0x0b:
	case 0x0c: case 0x0d: case 0x0e: case 0x0f:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		lynx_timer_write(state, offset >> 2, offset & 3, data);
		return;

	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		lynx_audio_write(state->m_audio, offset, data);
		return;

	case 0x80:
		state->m_mikey.data[0x81] &= ~data; // clear interrupt source
		logerror("mikey write %.2x %.2x\n", offset, data);
		if (!state->m_mikey.data[0x81])
			cputag_set_input_line(space->machine(), "maincpu", M65SC02_IRQ_LINE, CLEAR_LINE);
		break;

	/* Is this correct? */
	case 0x81:
		state->m_mikey.data[offset] |= data;
		break;

	case 0x87:
		state->m_mikey.data[offset] = data;
		if (data & 0x02)		// Power (1 = on)
		{
			if (data & 0x01)	// Cart Address Strobe
			{
				state->m_suzy.high <<= 1;
				if (state->m_mikey.data[0x8b] & 0x02)
					state->m_suzy.high |= 1;
				state->m_suzy.high &= 0xff;
				state->m_suzy.low = 0;
			}
		}
		else
		{
			state->m_suzy.high = 0;
			state->m_suzy.low = 0;
		}
		break;

	case 0x8c: case 0x8d:
		lynx_uart_w(space, offset, data);
		break;

	case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
	case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		state->m_mikey.data[offset] = data;
		lynx_draw_lines(space->machine(), state->m_line);

		/* RED = 0xb- & 0x0f, GREEN = 0xa- & 0x0f, BLUE = (0xb- & 0xf0) >> 4 */
		state->m_palette[offset & 0x0f] = space->machine().pens[
			((state->m_mikey.data[0xb0 + (offset & 0x0f)] & 0x0f)) |
			((state->m_mikey.data[0xa0 + (offset & 0x0f)] & 0x0f) << 4) |
			((state->m_mikey.data[0xb0 + (offset & 0x0f)] & 0xf0) << 4)];
		break;

	/* TODO: properly implement these writes */
	case 0x8b:
		state->m_mikey.data[offset] = data;
		if (state->m_mikey.data[0x8a] & 0x10)
			logerror("Trying to enable bank 1 write. %d\n", state->m_mikey.data[offset] & 0x10);
		break;

//  case 0x90: // SDONEACK - Suzy Done Acknowledge
	case 0x91: // CPUSLEEP - CPU Bus Request Disable
		state->m_mikey.data[offset] = data;
		if (!data)
		{
			/* A write of '0' to this address will reset the CPU bus request flip flop */
			logerror("CPUSLEEP request \n");
		}
		break;

	default:
		state->m_mikey.data[offset]=data;
		logerror("mikey write %.2x %.2x\n",offset,data);
		break;
	}
}

/****************************************

    Init / Config

****************************************/

READ8_HANDLER( lynx_memory_config_r )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	return state->m_memory_config;
}

WRITE8_HANDLER( lynx_memory_config_w )
{
	lynx_state *state = space->machine().driver_data<lynx_state>();
	/* bit 7: hispeed, uses page mode accesses (4 instead of 5 cycles )
     * when these are safe in the cpu */
	state->m_memory_config = data;

	if (data & 1) {
		space->install_readwrite_bank(0xfc00, 0xfcff, "bank1");
	} else {
		space->install_legacy_readwrite_handler(0xfc00, 0xfcff, FUNC(suzy_read), FUNC(suzy_write));
	}
	if (data & 2) {
		space->install_readwrite_bank(0xfd00, 0xfdff, "bank2");
	} else {
		space->install_legacy_readwrite_handler(0xfd00, 0xfdff, FUNC(mikey_read), FUNC(mikey_write));
	}

	if (data & 1)
		memory_set_bankptr(space->machine(), "bank1", state->m_mem_fc00);
	if (data & 2)
		memory_set_bankptr(space->machine(), "bank2", state->m_mem_fd00);
	memory_set_bank(space->machine(), "bank3", (data & 4) ? 1 : 0);
	memory_set_bank(space->machine(), "bank4", (data & 8) ? 1 : 0);
}

static void lynx_reset(running_machine &machine)
{
	lynx_state *state = machine.driver_data<lynx_state>();
	lynx_memory_config_w(machine.device("maincpu")->memory().space(AS_PROGRAM), 0, 0);

	cputag_set_input_line(machine, "maincpu", M65SC02_IRQ_LINE, CLEAR_LINE);

	memset(&state->m_suzy, 0, sizeof(state->m_suzy));
	memset(&state->m_mikey, 0, sizeof(state->m_mikey));

	state->m_suzy.data[0x88]  = 0x01;
	state->m_suzy.data[0x90]  = 0x00;
	state->m_suzy.data[0x91]  = 0x00;
	state->m_mikey.data[0x80] = 0x00;
	state->m_mikey.data[0x81] = 0x00;
	state->m_mikey.data[0x88] = 0x01;
	state->m_mikey.data[0x8a] = 0x00;
	state->m_mikey.data[0x8c] = 0x00;
	state->m_mikey.data[0x90] = 0x00;
	state->m_mikey.data[0x92] = 0x00;

	lynx_uart_reset(state);

	// hack to allow current object loading to work
#if 1
	lynx_timer_write( state, 0, 0, 160 );
	lynx_timer_write( state, 0, 1, 0x10 | 0x8 | 0 );
	lynx_timer_write( state, 2, 0, 102 );
	lynx_timer_write( state, 2, 1, 0x10 | 0x8 | 7 );
#endif
}

static STATE_POSTLOAD( lynx_postload )
{
	lynx_state *state = machine.driver_data<lynx_state>();
	lynx_memory_config_w( machine.device("maincpu")->memory().space(AS_PROGRAM), 0, state->m_memory_config);
}

MACHINE_START( lynx )
{
	lynx_state *state = machine.driver_data<lynx_state>();
	int i;
	state->save_item(NAME(state->m_memory_config));
	state->save_pointer(NAME(state->m_mem_fe00), state->m_mem_fe00_size);
	machine.state().register_postload(lynx_postload, NULL);

	memory_configure_bank(machine, "bank3", 0, 1, machine.region("maincpu")->base() + 0x0000, 0);
	memory_configure_bank(machine, "bank3", 1, 1, state->m_mem_fe00, 0);
	memory_configure_bank(machine, "bank4", 0, 1, machine.region("maincpu")->base() + 0x01fa, 0);
	memory_configure_bank(machine, "bank4", 1, 1, state->m_mem_fffa, 0);

	state->m_audio = machine.device("custom");
	state->m_height = -1;
	state->m_width = -1;

	memset(&state->m_suzy, 0, sizeof(state->m_suzy));

	machine.add_notifier(MACHINE_NOTIFY_RESET, lynx_reset);

	for (i = 0; i < NR_LYNX_TIMERS; i++)
		lynx_timer_init(machine, i);

}


/****************************************

    Image handling

****************************************/


static void lynx_partialhash(hash_collection &dest, const unsigned char *data,
	unsigned long length, const char *functions)
{
	if (length <= 64)
		return;
	dest.compute(&data[64], length - 64, functions);
}

int lynx_verify_cart (char *header, int kind)
{
	if (kind)
	{
		if (strncmp("BS93", &header[6], 4))
		{
			logerror("This is not a valid Lynx image\n");
			return IMAGE_VERIFY_FAIL;
		}
	}
	else
	{
		if (strncmp("LYNX",&header[0],4))
		{
			if (!strncmp("BS93", &header[6], 4))
			{
				logerror("This image is probably a Quickload image with .lnx extension\n");
				logerror("Try to load it with -quickload\n");
			}
			else
				logerror("This is not a valid Lynx image\n");
			return IMAGE_VERIFY_FAIL;
		}
	}

	return IMAGE_VERIFY_PASS;
}

INTERRUPT_GEN( lynx_frame_int )
{
	lynx_state *state = device->machine().driver_data<lynx_state>();
	state->m_rotate = state->m_rotate0;
	if ((input_port_read(device->machine(), "ROTATION") & 0x03) != 0x03)
		state->m_rotate=input_port_read(device->machine(), "ROTATION") & 0x03;
}

void lynx_crc_keyword(device_image_interface &image)
{
	lynx_state *state = image.device().machine().driver_data<lynx_state>();
	const char *info = NULL;

	info = hashfile_extrainfo(image);

	state->m_rotate0 = 0;	
	if (info)
	{
		if(strcmp(info, "ROTATE90DEGREE") == 0)
			state->m_rotate0 = 1;
		else if (strcmp(info, "ROTATE270DEGREE") == 0)
			state->m_rotate0 = 2;
	}
}

static DEVICE_IMAGE_LOAD( lynx_cart )
{
	lynx_state *state = image.device().machine().driver_data<lynx_state>();
	UINT8 *rom = image.device().machine().region("user1")->base();
	UINT32 size;
	UINT8 header[0x40];

	if (image.software_entry() == NULL)
	{
		const char *filetype;
		size = image.length();
/* 64 byte header
   LYNX
   intelword lower counter size
   0 0 1 0
   32 chars name
   22 chars manufacturer
*/

		filetype = image.filetype();

		if (!mame_stricmp (filetype, "lnx"))
		{
			if (image.fread( header, 0x40) != 0x40)
				return IMAGE_INIT_FAIL;

			/* Check the image */
			if (lynx_verify_cart((char*)header, LYNX_CART) == IMAGE_VERIFY_FAIL)
				return IMAGE_INIT_FAIL;

			/* 2008-10 FP: According to Handy source these should be page_size_bank0. Are we using
            it correctly in MESS? Moreover, the next two values should be page_size_bank1. We should
            implement this as well */
			state->m_granularity = header[4] | (header[5] << 8);

			logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
					header + 10, size / 1024, state->m_granularity, header + 42);

			size -= 0x40;
		}
		else if (!mame_stricmp (filetype, "lyx"))
		{
			/* 2008-10 FP: FIXME: .lyx file don't have an header, hence they miss "lynx_granularity"
            (see above). What if bank 0 has to be loaded elsewhere? And what about bank 1?
            These should work with most .lyx files, but we need additional info on raw cart images */
			if (size == 0x20000)
				state->m_granularity = 0x0200;
			else if (size == 0x80000)
				state->m_granularity = 0x0800;
			else
				state->m_granularity = 0x0400;
		}

		if (image.fread( rom, size) != size)
			return IMAGE_INIT_FAIL;

		lynx_crc_keyword(image);
	}
	else
	{
		size = image.get_software_region_length("rom");

		/* here we assume images to be in .lnx format and to have an header.
         we should eventually remove them, though! */
		memcpy(header, image.get_software_region("rom"), 0x40);

		/* Check the image */
		if (lynx_verify_cart((char*)header, LYNX_CART) == IMAGE_VERIFY_FAIL)
			return IMAGE_INIT_FAIL;

		/* 2008-10 FP: According to Handy source these should be page_size_bank0. Are we using
         it correctly in MESS? Moreover, the next two values should be page_size_bank1. We should
         implement this as well */
		state->m_granularity = header[4] | (header[5] << 8);

		logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
				  header + 10, size / 1024, state->m_granularity, header + 42);

		size -= 0x40;
		memcpy(rom, image.get_software_region("rom") + 0x40, size);
	}

	return IMAGE_INIT_PASS;
}

MACHINE_CONFIG_FRAGMENT(lynx_cartslot)
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("lnx,lyx")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_INTERFACE("lynx_cart")
	MCFG_CARTSLOT_LOAD(lynx_cart)
	MCFG_CARTSLOT_PARTIALHASH(lynx_partialhash)

	/* Software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","lynx")
MACHINE_CONFIG_END
