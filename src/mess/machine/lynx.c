/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "emu.h"
#include "includes/lynx.h"
#include "cpu/m6502/m6502.h"

#include "devices/cartslot.h"
#include "hash.h"

static UINT16 lynx_granularity;
static int lynx_line;
static int lynx_line_y;
static int sign_AB = 0, sign_CD = 0;

static UINT32 lynx_palette[0x10];

static int rotate0 = 0;
static int lynx_rotate;
static running_device *lynx_audio;

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

typedef struct
{
	UINT8 data[0x100];
	int accumulate_overflow;
	UINT8 high;
	int low;
} SUZY;

static SUZY suzy;

static struct
{
	UINT8 *mem;
	// global
	UINT16 screen;
	UINT16 colbuf;
	UINT16 colpos; // byte where value of collision is written
	UINT16 xoff, yoff;
	// in command
	int mode;
	UINT16 cmd;
	UINT8 spritenr;
	int x,y;
	UINT16 width, height; // uint16 important for blue lightning
	UINT16 stretch, tilt; // uint16 important
	UINT8 color[16]; // or stored
	void (*line_function)(const int y, const int xdir);
	UINT16 bitmap;

	int everon;
	int memory_accesses;
	attotime time;
} blitter;

UINT8 *lynx_mem_0000;
UINT8 *lynx_mem_fc00;
UINT8 *lynx_mem_fd00;
UINT8 *lynx_mem_fe00;
UINT8 *lynx_mem_fffa;
size_t lynx_mem_fe00_size;

static UINT8 lynx_memory_config;

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

static UINT8 sprite_collide;

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

INLINE void lynx_plot_pixel(const int mode, const int x, const int y, const int color)
{
	int back;
	UINT8 *screen;
	UINT8 *colbuf;

	blitter.everon = TRUE;
	screen = blitter.mem + blitter.screen + y * 80 + x / 2;
	colbuf = blitter.mem + blitter.colbuf + y * 80 + x / 2;

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

				if(sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (blitter.spritenr << 4);
					if ((back >> 4) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back >> 4;
					blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if(sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (blitter.spritenr);
					if ((back & 0x0f) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back & 0x0f;
					blitter.memory_accesses += 2;
				}
			}
			blitter.memory_accesses++;
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
					blitter.memory_accesses++;
				}
				if(sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (blitter.spritenr << 4);
					if ((back >> 4) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back >> 4;
					blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0xf0) | color;
					blitter.memory_accesses++;
				}
				if(sprite_collide)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (blitter.spritenr);
					if ((back & 0x0f) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back & 0x0f;
					blitter.memory_accesses += 2;
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

				if (sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (blitter.spritenr << 4);
					if ((back >> 4) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back >> 4;
					blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if (sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (blitter.spritenr);
					if ((back & 0x0f) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back & 0x0f;
					blitter.memory_accesses += 2;
				}
			}
			blitter.memory_accesses++;
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
					blitter.memory_accesses++;
				}
				if (sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0xf0) | (blitter.spritenr << 4);
					if ((back >> 4) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back >> 4;
					blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				if (color != 0x0f)
				{
					*screen = (*screen & 0xf0) | color;
					blitter.memory_accesses++;
				}
				if (sprite_collide && (color != 0x0e))
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (blitter.spritenr);
					if ((back & 0x0f) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back & 0x0f;
					blitter.memory_accesses += 2;
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

				if (sprite_collide && (color != 0x0e))
				{
					*colbuf = (*colbuf & ~0xf0) | (blitter.spritenr << 4);
					blitter.memory_accesses++;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0) | color;

				if (sprite_collide && (color != 0x0e))
				{
					*colbuf = (*colbuf & ~0x0f) | (blitter.spritenr);
					blitter.memory_accesses++;
				}
			}
			blitter.memory_accesses++;
			break;

		case BACKGROUND_NO_COLL:
		/* This is a 'background' sprite with the exception that no activity occurs in the collision buffer */
			if (!(x & 0x01))		/* Upper nibble */
				*screen = (*screen & 0x0f) | (color << 4);
			else					/* Lower nibble */
				*screen = (*screen & 0xf0) | color;
			blitter.memory_accesses++;
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
			blitter.memory_accesses++;
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
					*colbuf = (back & ~0xf0) | (blitter.spritenr << 4);
					if ((back >> 4) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back >> 4;
					blitter.memory_accesses += 2;
				}
			}
			else					/* Lower nibble */
			{
				*screen = (*screen & 0xf0)^color;

				if (color != 0x0e)
				{
					back = *colbuf;
					*colbuf = (back & ~0x0f) | (blitter.spritenr);
					if ((back & 0x0f) > blitter.mem[blitter.colpos])
						blitter.mem[blitter.colpos] = back & 0x0f;
					blitter.memory_accesses += 2;
				}
			}
			blitter.memory_accesses++;
			break;
	}
}

static void lynx_blit_do_work( const int y, const int xdir, const int bits, const int mask )
{
	int j, xi, wi, i;
	int b, p, color;

	i = blitter.mem[blitter.bitmap];
	blitter.memory_accesses++;		// ?

	for (xi = blitter.x, p = 0, b = 0, j = 1, wi = 0; j < i;)
	{
		if (p < bits)
		{
			b = (b << 8) | blitter.mem[blitter.bitmap + j];
			j++;
			p += 8;
			blitter.memory_accesses++;
		}
		for ( ; (p >= bits); )
		{
			color = blitter.color[(b >> (p - bits)) & mask];
			p -= bits;
			for ( ; wi < blitter.width; wi += 0x100, xi += xdir)
			{
				if ((xi >= 0) && (xi < 160))
					lynx_plot_pixel(blitter.mode, xi, y, color);
			}
			wi -= blitter.width;
		}
	}
}


static void lynx_blit_2color_line(const int y, const int xdir)
{
	const int bits = 1;
	const int mask = 0x01;

	lynx_blit_do_work(y, xdir, bits, mask);
}
static void lynx_blit_4color_line(const int y, const int xdir)
{
	const int bits = 2;
	const int mask = 0x03;

	lynx_blit_do_work(y, xdir, bits, mask);
}
static void lynx_blit_8color_line(const int y, const int xdir)
{
	const int bits = 3;
	const int mask = 0x07;

	lynx_blit_do_work(y, xdir, bits, mask);
}
static void lynx_blit_16color_line(const int y, const int xdir)
{
	const int bits = 4;
	const int mask = 0x0f;

	lynx_blit_do_work(y, xdir, bits, mask);
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

static void lynx_blit_rle_do_work( const int y, const int xdir, const int bits, const int mask )
{
	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p = 0, j = 0, b = 0, xi = blitter.x, wi = 0; ; )		/* through the rle entries */
	{
		if (p < 5 + bits) /* under 7 bits no complete entry */
		{
			j++;
			if (j >= blitter.mem[blitter.bitmap])
				return;

			p += 8;
			b = (b << 8) | blitter.mem[blitter.bitmap + j];
			blitter.memory_accesses++;
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
					if (j >= blitter.mem[blitter.bitmap])
						return;
					p += 8;
					b = (b << 8) | blitter.mem[blitter.bitmap + j];
					blitter.memory_accesses++;
				}

				color = blitter.color[(b >> (p - bits)) & mask];
				p -= bits;
				for ( ; wi < blitter.width; wi += 0x100, xi += xdir)
				{
					if ((xi >= 0) && (xi < 160))
						lynx_plot_pixel(blitter.mode, xi, y, color);
				}
				wi -= blitter.width;
			}
		}
		else		/* count of same pixels */
		{
			if (count == 0)
				return;

			if (p < bits)
			{
				j++;
				if (j >= blitter.mem[blitter.bitmap])
					return;
				p += 8;
				b = (b << 8) | blitter.mem[blitter.bitmap + j];
				blitter.memory_accesses++;
			}

			color = blitter.color[(b >> (p - bits)) & mask];
			p -= bits;

			for ( ; count; count--)
			{
				for ( ; wi < blitter.width; wi += 0x100, xi += xdir)
				{
					if ((xi >= 0) && (xi < 160))
						lynx_plot_pixel(blitter.mode, xi, y, color);
				}
				wi -= blitter.width;
			}
		}
	}
}

static void lynx_blit_2color_rle_line(const int y, const int xdir)
{
	const int bits = 1;
	const int mask = 0x01;

	lynx_blit_rle_do_work(y, xdir, bits, mask);
}
static void lynx_blit_4color_rle_line(const int y, const int xdir)
{
	const int bits = 2;
	const int mask = 0x03;

	lynx_blit_rle_do_work(y, xdir, bits, mask);
}
static void lynx_blit_8color_rle_line(const int y, const int xdir)
{
	const int bits = 3;
	const int mask = 0x07;

	lynx_blit_rle_do_work(y, xdir, bits, mask);
}
static void lynx_blit_16color_rle_line(const int y, const int xdir)
{
	const int bits = 4;
	const int mask = 0x0f;

	lynx_blit_rle_do_work(y, xdir, bits, mask);
}


static void lynx_blit_lines(void)
{
	int i, hi, y;
	int ydir = 0, xdir = 0;
	int flip = 0;

	blitter.everon = FALSE;

	// flipping sprdemo3
	// fat bobby 0x10
	// mirror the sprite in gameplay?
	xdir = 1;

	if (blitter.mem[blitter.cmd] & 0x20)	/* Horizontal Flip */
	{
		xdir = -1;
		blitter.x--;	/*?*/
	}

	ydir = 1;

	if (blitter.mem[blitter.cmd] & 0x10)	/* Vertical Flip */
	{
		ydir = -1;
		blitter.y--;	/*?*/
	}

	switch (blitter.mem[blitter.cmd + 1] & 0x03)	/* Start Left & Start Up */
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

	for (y = blitter.y, hi = 0; blitter.memory_accesses++, i = blitter.mem[blitter.bitmap]; blitter.bitmap += i )
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
					blitter.y += ydir;
					break;

				case 1:
				case 3:
					xdir *= -1;
					blitter.x += xdir;
					break;
			}

			y = blitter.y;
			flip++;
			continue;
		}

		for ( ; (hi < blitter.height); hi += 0x100, y += ydir)
		{
			if (y >= 0 && y < 102)
				blitter.line_function(y, xdir);
			blitter.width += blitter.stretch;
			blitter.x += blitter.tilt;
		}

		hi -= blitter.height;
	}

	if (suzy.data[SPRG0] & 0x04)
	{
		if (sprite_collide & !blitter.everon)
			blitter.mem[blitter.colpos] |= 0x80;
		else
			blitter.mem[blitter.colpos] &= 0x7f;
	}
}

static TIMER_CALLBACK(lynx_blitter_timer)
{
	suzy.data[SPRSYS] &= ~0x01; //blitter finished
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

static void lynx_blitter(running_machine *machine)
{
	static const int lynx_colors[4]={2,4,8,16};

	static void (* const blit_line[4])(const int y, const int xdir)= {
	lynx_blit_2color_line,
	lynx_blit_4color_line,
	lynx_blit_8color_line,
	lynx_blit_16color_line
	};

	static void (* const blit_rle_line[4])(const int y, const int xdir)= {
	lynx_blit_2color_rle_line,
	lynx_blit_4color_rle_line,
	lynx_blit_8color_rle_line,
	lynx_blit_16color_rle_line
	};
	int i; int o;int colors;

	blitter.memory_accesses = 0;
	blitter.mem = (UINT8*)cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM)->get_read_ptr(0x0000);

	blitter.xoff   = GET_WORD(suzy.data, 0x04);
	blitter.yoff   = GET_WORD(suzy.data, 0x06);
	blitter.screen = GET_WORD(suzy.data, 0x08);
	blitter.colbuf = GET_WORD(suzy.data, 0x0a);
	// hsizeoff GET_WORD(suzy.data, 0x28)
	// vsizeoff GET_WORD(suzy.data, 0x2a)

	// these might be never set by the blitter hardware
	blitter.width   = 0x100;
	blitter.height  = 0x100;
	blitter.stretch = 0;
	blitter.tilt    = 0;
	sprite_collide  = 0;

	blitter.memory_accesses += 2;

	for (blitter.cmd = GET_WORD(suzy.data, 0x10); blitter.cmd; )
	{
		blitter.memory_accesses += 1;

		if (!(blitter.mem[blitter.cmd + 1] & 0x04))		// if 0, we skip this sprite
		{
			blitter.colpos = GET_WORD(suzy.data, 0x24) + blitter.cmd;

			blitter.bitmap = GET_WORD(blitter.mem,blitter.cmd + 5);
			blitter.x = (INT16)GET_WORD(blitter.mem, blitter.cmd + 7) - blitter.xoff;
			blitter.y = (INT16)GET_WORD(blitter.mem, blitter.cmd + 9) - blitter.yoff;
			blitter.memory_accesses += 6;

			blitter.mode = blitter.mem[blitter.cmd] & 0x07;

			if (blitter.mem[blitter.cmd + 1] & 0x80)
				blitter.line_function = blit_line[blitter.mem[blitter.cmd] >> 6];
			else
				blitter.line_function = blit_rle_line[blitter.mem[blitter.cmd] >> 6];

			if (!(blitter.mem[blitter.cmd + 2] & 0x20) && !(suzy.data[SPRSYS] & 0x20))
			{
				switch (blitter.mode)
				{
					case BACKGROUND:
					case BOUNDARY_SHADOW:
					case BOUNDARY:
					case NORMAL_SPRITE:
					case XOR_SPRITE:
					case SHADOW:
						sprite_collide = 1;
						blitter.mem[blitter.colpos] = 0;
						blitter.spritenr = blitter.mem[blitter.cmd + 2] & 0x0f;
				}
			}

			/* Sprite Reload Bits */
			o = 0x0b;
			if (blitter.mem[blitter.cmd + 1] & 0x30)
			{
				blitter.width  = GET_WORD(blitter.mem, blitter.cmd + 11);
				blitter.height = GET_WORD(blitter.mem, blitter.cmd + 13);
				blitter.memory_accesses += 4;
				o += 4;
			}
			if (blitter.mem[blitter.cmd + 1] & 0x20)
			{
				blitter.stretch = GET_WORD(blitter.mem, blitter.cmd + o);
				blitter.memory_accesses += 2;
				o += 2;
				if (blitter.mem[blitter.cmd + 1] & 0x10)
				{
					blitter.tilt = GET_WORD(blitter.mem, blitter.cmd+o);
					blitter.memory_accesses += 2;
					o += 2;
				}
			}

			/* Reload Palette Bit */
			colors = lynx_colors[blitter.mem[blitter.cmd] >> 6];

			if (!(blitter.mem[blitter.cmd + 1] & 0x08))
			{
				for (i = 0; i < colors / 2; i++)
				{
					blitter.color[i * 2]      = blitter.mem[blitter.cmd + o + i] >> 4;
					blitter.color[i * 2 + 1 ] = blitter.mem[blitter.cmd + o + i] & 0x0f;
					blitter.memory_accesses++;
				}
			}

			/* Draw Sprites */
			lynx_blit_lines();
		}

	blitter.cmd = GET_WORD(blitter.mem,  blitter.cmd + 3);
	blitter.memory_accesses += 2;

	if (!(blitter.cmd & 0xff00))
		break;
	}

	if (0)
		timer_set(machine, machine->device<cpu_device>("maincpu")->cycles_to_attotime(blitter.memory_accesses*20), NULL, 0, lynx_blitter_timer);
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

static void lynx_divide( void )
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

	left = suzy.data[MATH_H] | (suzy.data[MATH_G] << 8) | (suzy.data[MATH_F] << 16) | (suzy.data[MATH_E] << 24);
	right = suzy.data[MATH_P] | (suzy.data[MATH_N] << 8);

	suzy.accumulate_overflow = FALSE;
	if (right == 0)
	{
		suzy.accumulate_overflow = TRUE;	/* during divisions, this bit is used to detect denominator = 0 */
		res = 0xffffffff;
		mod = 0; //?
	}
	else
	{
		res = left / right;
		mod = left % right;
	}
//  logerror("coprocessor %8x / %8x = %4x\n", left, right, res);
	suzy.data[MATH_D] = res & 0xff;
	suzy.data[MATH_C] = res >> 8;
	suzy.data[MATH_B] = res >> 16;
	suzy.data[MATH_A] = res >> 24;

	suzy.data[MATH_M] = mod & 0xff;
	suzy.data[MATH_L] = mod >> 8;
	suzy.data[MATH_K] = mod >> 16;
	suzy.data[MATH_J] = mod >> 24;
}

static void lynx_multiply( void )
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
	suzy.accumulate_overflow = FALSE;

	left = suzy.data[MATH_B] | (suzy.data[MATH_A] << 8);
	right = suzy.data[MATH_D] | (suzy.data[MATH_C] << 8);

	res = left * right;

	if (suzy.data[SPRSYS] & 0x80)		/* signed math */
	{
		if (!(sign_AB + sign_CD))	/* different signs */
			res = (res ^ 0xffffffff) + 1;
	}

	suzy.data[MATH_H] = res & 0xff;
	suzy.data[MATH_G] = res >> 8;
	suzy.data[MATH_F] = res >> 16;
	suzy.data[MATH_E] = res >> 24;

	if (suzy.data[SPRSYS] & 0x40)		/* is accumulation allowed? */
	{
		accu = suzy.data[MATH_M] | suzy.data[MATH_L] << 8 | suzy.data[MATH_K] << 16 | suzy.data[MATH_J] << 24;
		accu += res;

		if (accu < res)
			suzy.accumulate_overflow = TRUE;

		suzy.data[MATH_M] = accu;
		suzy.data[MATH_L] = accu >> 8;
		suzy.data[MATH_K] = accu >> 16;
		suzy.data[MATH_J] = accu >> 24;
	}
}

static READ8_HANDLER( suzy_read )
{
	UINT8 value = 0, input;

	switch (offset)
	{
		case 0x88:
			value = 0x01; // must not be 0 for correct power up
			break;
		case 0x92:	/* Better check this with docs! */
			if (!attotime_compare(blitter.time, attotime_zero))
			{
				if (space->machine->device<cpu_device>("maincpu")->attotime_to_cycles(attotime_sub(timer_get_time(space->machine), blitter.time)) > blitter.memory_accesses * 20)
				{
					suzy.data[offset] &= ~0x01; //blitter finished
					blitter.time = attotime_zero;
				}
			}
			value = suzy.data[offset];
			value &= ~0x80; // math finished
			value &= ~0x40;
			if (suzy.accumulate_overflow)
				value |= 0x40;
			break;
		case 0xb0:
			input = input_port_read(space->machine, "JOY");
			switch (lynx_rotate)
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
			if (suzy.data[SPRSYS] & 0x08) /* Left handed controls */
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
			value = input_port_read(space->machine, "PAUSE");
			break;
		case 0xb2:
			value = *(memory_region(space->machine, "user1") + (suzy.high * lynx_granularity) + suzy.low);
			suzy.low = (suzy.low + 1) & (lynx_granularity - 1);
			break;
		case 0xb3: /* we need bank 1 emulation!!! */
		default:
			value = suzy.data[offset];
	}
//  logerror("suzy read %.2x %.2x\n",offset,data);
	return value;
}

static WRITE8_HANDLER( suzy_write )
{
	suzy.data[offset] = data;

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
		suzy.data[offset + 1] = 0;
		break;
	/* Writing to M (0x6c) will also clear the accumulator overflow bit */
	case 0x6c:
		suzy.data[offset + 1] = 0;
		suzy.accumulate_overflow = FALSE;
		break;
	case 0x53:
	/* If we are going to perform a signed multiplication, we store the sign and convert the number
    to an unsigned one */
		if (suzy.data[SPRSYS] & 0x80)		/* signed math */
		{
			UINT16 factor, temp;
			factor = suzy.data[MATH_D] | (suzy.data[MATH_C] << 8);
			if ((factor - 1) & 0x8000)		/* here we use -1 to cover the math bugs on the sign of 0 and 0x8000 */
			{
				temp = (factor ^ 0xffff) + 1;
				sign_CD = - 1;
				suzy.data[MATH_D] = temp & 0xff;
				suzy.data[MATH_C] = temp >> 8;
			}
			else
				sign_CD = 1;
		}
		break;
	/* Writing to A will start a 16 bit multiply */
	/* If we are going to perform a signed multiplication, we also store the sign and convert the
    number to an unsigned one */
	case 0x55:
		if (suzy.data[SPRSYS] & 0x80)		/* signed math */
		{
			UINT16 factor, temp;
			factor = suzy.data[MATH_B] | (suzy.data[MATH_A] << 8);
			if ((factor - 1) & 0x8000)		/* here we use -1 to cover the math bugs on the sign of 0 and 0x8000 */
			{
				temp = (factor ^ 0xffff) + 1;
				sign_AB = - 1;
				suzy.data[MATH_B] = temp & 0xff;
				suzy.data[MATH_A] = temp >> 8;
			}
			else
				sign_AB = 1;
		}
		lynx_multiply();
		break;
	/* Writing to E will start a 16 bit divide */
	case 0x63:
		lynx_divide();
		break;
	case 0x91:
		if (data & 0x01)
		{
			blitter.time = timer_get_time(space->machine);
			lynx_blitter(space->machine);
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

typedef struct {
	UINT8 data[0x100];
} MIKEY;

static MIKEY mikey;


static UINT8 lynx_read_vram(UINT16 address)
{
	UINT8 result = 0x00;
	if (address <= 0xfbff)
		result = lynx_mem_0000[address - 0x0000];
	else if (address <= 0xfcff)
		result = lynx_mem_fc00[address - 0xfc00];
	else if (address <= 0xfdff)
		result = lynx_mem_fd00[address - 0xfd00];
	else if (address <= 0xfff7)
		result = lynx_mem_fe00[address - 0xfe00];
	else if (address >= 0xfffa)
		result = lynx_mem_fffa[address - 0xfffa];
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
static int lynx_height, lynx_width;
static void lynx_draw_lines(running_machine *machine, int newline)
{
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

	if (yend == lynx_line_y)
	{
		if (newline == -1)
			lynx_line_y = 0;
		return;
	}

	j = (mikey.data[0x94] | (mikey.data[0x95]<<8)) + lynx_line_y * 160 / 2;
	if (mikey.data[0x92] & 0x02)
		j -= 160 * 102 / 2 - 1;

	/* rotation */
	if (lynx_rotate & 0x03)
	{
		h = 160; w = 102;
		if (((lynx_rotate == 1) && (mikey.data[0x92] & 0x02)) || ((lynx_rotate == 2) && !(mikey.data[0x92] & 0x02)))
		{
			for ( ; lynx_line_y < yend; lynx_line_y++)
			{
				line = BITMAP_ADDR16(machine->generic.tmpbitmap, lynx_line_y, 0);
				for (x = 160 - 2; x >= 0; j++, x -= 2)
				{
					byte = lynx_read_vram(j);
					line[x + 1] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 0] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for ( ; lynx_line_y < yend; lynx_line_y++)
			{
				line = BITMAP_ADDR16(machine->generic.tmpbitmap, 102 - 1 - lynx_line_y, 0);
				for (x = 0; x < 160; j++, x += 2)
				{
					byte = lynx_read_vram(j);
					line[x + 0] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 1] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	else
	{
		w = 160; h = 102;
		if (mikey.data[0x92] & 0x02)
		{
			for ( ; lynx_line_y < yend; lynx_line_y++)
			{
				line = BITMAP_ADDR16(machine->generic.tmpbitmap, 102 - 1 - lynx_line_y, 0);
				for (x = 160 - 2; x >= 0; j++, x -= 2)
				{
					byte = lynx_read_vram(j);
					line[x + 1] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 0] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
		else
		{
			for ( ; lynx_line_y < yend; lynx_line_y++)
			{
				line = BITMAP_ADDR16(machine->generic.tmpbitmap, lynx_line_y, 0);
				for (x = 0; x < 160; j++, x += 2)
				{
					byte = lynx_read_vram(j);
					line[x + 0] = lynx_palette[(byte >> 4) & 0x0f];
					line[x + 1] = lynx_palette[(byte >> 0) & 0x0f];
				}
			}
		}
	}
	if (newline == -1)
	{
		lynx_line_y = 0;
		if ((w != lynx_width) || (h != lynx_height))
		{
			lynx_width = w;
			lynx_height = h;
			machine->primary_screen->set_visible_area(0, w - 1, 0, h - 1);
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
typedef struct {
	UINT8	bakup;
	UINT8	cntrl1;
	UINT8	cntrl2;
	int		counter;
	emu_timer	*timer;
	int		timer_active;
} LYNX_TIMER;

#define NR_LYNX_TIMERS	8

static LYNX_TIMER lynx_timer[NR_LYNX_TIMERS];

static TIMER_CALLBACK(lynx_timer_shot);

static void lynx_timer_init(running_machine *machine, int which)
{
	memset( &lynx_timer[which], 0, sizeof(LYNX_TIMER) );
	lynx_timer[which].timer = timer_alloc(machine,  lynx_timer_shot , NULL);
}

static void lynx_timer_signal_irq(running_machine *machine, int which)
{
	if ( ( lynx_timer[which].cntrl1 & 0x80 ) && ( which != 4 ) )
	{ // irq flag handling later
		mikey.data[0x81] |= ( 1 << which );
		cputag_set_input_line(machine, "maincpu", M65SC02_IRQ_LINE, ASSERT_LINE);
	}
	switch ( which )
	{
	case 0:
		lynx_timer_count_down( machine, 2 );
		lynx_line++;
		break;
	case 2:
		lynx_timer_count_down( machine, 4 );
		lynx_draw_lines( machine, -1 );
		lynx_line=0;
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
		lynx_audio_count_down( lynx_audio, 0 );
		break;
	}
}

void lynx_timer_count_down(running_machine *machine, int which)
{
	if ( ( lynx_timer[which].cntrl1 & 0x0f ) == 0x0f )
	{
		if ( lynx_timer[which].counter > 0 )
		{
			lynx_timer[which].counter--;
			return;
		}
		if ( lynx_timer[which].counter == 0 )
		{
			lynx_timer[which].cntrl2 |= 8;
			lynx_timer_signal_irq(machine, which);
			if ( lynx_timer[which].cntrl1 & 0x10 )
			{
				lynx_timer[which].counter = lynx_timer[which].bakup;
			}
			else
			{
				lynx_timer[which].counter--;
			}
			return;
		}
	}
}

static TIMER_CALLBACK(lynx_timer_shot)
{
	lynx_timer[param].cntrl2 |= 8;
	lynx_timer_signal_irq( machine, param );
	if ( ! ( lynx_timer[param].cntrl1 & 0x10 ) )
		lynx_timer[param].timer_active = 0;
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

static UINT8 lynx_timer_read(int which, int offset)
{
	UINT8 value = 0;

	switch (offset)
	{
		case 0:
			value = lynx_timer[which].bakup;
			break;
		case 1:
			value = lynx_timer[which].cntrl1;
			break;
		case 2:
			if ((lynx_timer[which].cntrl1 & 0x07) == 0x07)
			{
				value = lynx_timer[which].counter;
			}
			else
			{
				if ( lynx_timer[which].timer_active )
					value = (UINT8) ( lynx_timer[which].bakup - attotime_mul(timer_timeleft(lynx_timer[which].timer), lynx_time_factor( lynx_timer[which].cntrl1 & 0x07 )).seconds);
			}
			break;

		case 3:
			value = lynx_timer[which].cntrl2;
			break;
	}
	logerror("timer %d read %x %.2x\n", which, offset, value);
	return value;
}

static void lynx_timer_write(int which, int offset, UINT8 data)
{
	logerror("timer %d write %x %.2x\n", which, offset, data);

	switch (offset)
	{
		case 0:
			lynx_timer[which].bakup = data;
			break;
		case 1:
			lynx_timer[which].cntrl1 = data;
			if (data & 0x40)
				lynx_timer[which].cntrl2 &= ~0x08;
			break;
		case 2:
//          lynx_timer[which].counter = data;   // why commented out?
			break;
		case 3:
			lynx_timer[which].cntrl2 = (lynx_timer[which].cntrl2 & 0x08) | (data & ~0x08);
			break;
	}

	/* Update timers */
	if ( offset < 3 )
	{
		timer_reset(lynx_timer[which].timer, attotime_never);
		lynx_timer[which].timer_active = 0;
		if ((lynx_timer[which].cntrl1 & 0x08))		// 0x48?
		{
			if ((lynx_timer[which].cntrl1 & 0x07) != 0x07)
			{
				attotime t = attotime_mul(ATTOTIME_IN_HZ(lynx_time_factor(lynx_timer[which].cntrl1 & 0x07)), lynx_timer[which].bakup + 1);
				if (lynx_timer[which].cntrl1 & 0x10)
					timer_adjust_periodic(lynx_timer[which].timer, attotime_zero, which, t);
				else
					timer_adjust_oneshot(lynx_timer[which].timer, t, which);
				lynx_timer[which].timer_active = 1;
			}
		}
	}
}


/****************************************

    UART Emulation

****************************************/


static struct {
	UINT8 serctl;
	UINT8 data_received, data_to_send, buffer;

	int received;
	int sending;
	int buffer_loaded;
} uart;

static void lynx_uart_reset(void)
{
	memset(&uart, 0, sizeof(uart));
}

static TIMER_CALLBACK(lynx_uart_timer)
{
	if (uart.buffer_loaded)
	{
		uart.data_to_send = uart.buffer;
		uart.buffer_loaded = FALSE;
		timer_set(machine, ATTOTIME_IN_USEC(11), NULL, 0, lynx_uart_timer);
	}
	else
		uart.sending = FALSE;

//    mikey.data[0x80]|=0x10;
	if (uart.serctl & 0x80)
	{
		mikey.data[0x81] |= 0x10;
		cputag_set_input_line(machine, "maincpu", M65SC02_IRQ_LINE, ASSERT_LINE);
	}
}

static  READ8_HANDLER(lynx_uart_r)
{
	UINT8 value = 0x00;
	switch (offset)
	{
		case 0x8c:
			if (!uart.buffer_loaded)
				value |= 0x80;
			if (uart.received)
				value |= 0x40;
			if (!uart.sending)
				value |= 0x20;
			break;

		case 0x8d:
			value = uart.data_received;
			break;
	}
	logerror("uart read %.2x %.2x\n", offset, value);
	return value;
}

static WRITE8_HANDLER(lynx_uart_w)
{
	logerror("uart write %.2x %.2x\n", offset, data);
	switch (offset)
	{
		case 0x8c:
			uart.serctl = data;
			break;

		case 0x8d:
			if (uart.sending)
			{
				uart.buffer = data;
				uart.buffer_loaded = TRUE;
			}
			else
			{
				uart.sending = TRUE;
				uart.data_to_send = data;
				timer_set(space->machine, ATTOTIME_IN_USEC(11), NULL, 0, lynx_uart_timer);
			}
			break;
	}
}


/****************************************

    Mikey memory handlers

****************************************/


static READ8_HANDLER( mikey_read )
{
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
		value = lynx_timer_read(offset >> 2, offset & 0x03);
		break;

	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		value = lynx_audio_read(lynx_audio, offset);
		break;

	case 0x80:
	case 0x81:
		value = mikey.data[offset];
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
		direction = mikey.data[0x8a];
		value |= (direction & 0x01) ? (mikey.data[offset] & 0x01) : 0x01;	// External Power input
		value |= (direction & 0x02) ? (mikey.data[offset] & 0x02) : 0x00;	// Cart Address Data output (0 turns cart power on)
		value |= (direction & 0x04) ? (mikey.data[offset] & 0x04) : 0x04;	// noexp input
		// REST still to implement
		value |= (direction & 0x08) ? (mikey.data[offset] & 0x08) : 0x00;	// rest output
		value |= (direction & 0x10) ? (mikey.data[offset] & 0x10) : 0x10;	// audin input
		/* Hack: we disable COMLynx  */
		value |= 0x04;
		/* B5, B6 & B7 are not used */
		break;

	case 0x8c:
	case 0x8d:
		value = lynx_uart_r(space, offset);
		break;

	default:
		value = mikey.data[offset];
		logerror( "mikey read %.2x %.2x\n", offset, value );
	}

	return value;
}

static WRITE8_HANDLER( mikey_write )
{
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
		lynx_timer_write(offset >> 2, offset & 3, data);
		return;

	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		lynx_audio_write(lynx_audio, offset, data);
		return;

	case 0x80:
		mikey.data[0x81] &= ~data; // clear interrupt source
		logerror("mikey write %.2x %.2x\n", offset, data);
		if (!mikey.data[0x81])
			cputag_set_input_line(space->machine, "maincpu", M65SC02_IRQ_LINE, CLEAR_LINE);
		break;

	/* Is this correct? */
	case 0x81:
		mikey.data[offset] |= data;
		break;

	case 0x87:
		mikey.data[offset] = data;
		if (data & 0x02)		// Power (1 = on)
		{
			if (data & 0x01)	// Cart Address Strobe
			{
				suzy.high <<= 1;
				if (mikey.data[0x8b] & 0x02)
					suzy.high |= 1;
				suzy.high &= 0xff;
				suzy.low = 0;
			}
		}
		else
		{
			suzy.high = 0;
			suzy.low = 0;
		}
		break;

	case 0x8c: case 0x8d:
		lynx_uart_w(space, offset, data);
		break;

	case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
	case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		mikey.data[offset] = data;
		lynx_draw_lines(space->machine, lynx_line);

		/* RED = 0xb- & 0x0f, GREEN = 0xa- & 0x0f, BLUE = (0xb- & 0xf0) >> 4 */
		lynx_palette[offset & 0x0f] = space->machine->pens[
			((mikey.data[0xb0 + (offset & 0x0f)] & 0x0f)) |
			((mikey.data[0xa0 + (offset & 0x0f)] & 0x0f) << 4) |
			((mikey.data[0xb0 + (offset & 0x0f)] & 0xf0) << 4)];
		break;

	/* TODO: properly implement these writes */
	case 0x8b:
		mikey.data[offset] = data;
		if (mikey.data[0x8a] & 0x10)
			logerror("Trying to enable bank 1 write. %d\n", mikey.data[offset] & 0x10);
		break;

//  case 0x90: // SDONEACK - Suzy Done Acknowledge
	case 0x91: // CPUSLEEP - CPU Bus Request Disable
		mikey.data[offset] = data;
		if (!data)
		{
			/* A write of '0' to this address will reset the CPU bus request flip flop */
			logerror("CPUSLEEP request \n");
		}
		break;

	default:
		mikey.data[offset]=data;
		logerror("mikey write %.2x %.2x\n",offset,data);
		break;
	}
}

/****************************************

    Init / Config

****************************************/

READ8_HANDLER( lynx_memory_config_r )
{
	return lynx_memory_config;
}

WRITE8_HANDLER( lynx_memory_config_w )
{
	/* bit 7: hispeed, uses page mode accesses (4 instead of 5 cycles )
     * when these are safe in the cpu */
	lynx_memory_config = data;

	if (data & 1) {
		memory_install_readwrite_bank(space, 0xfc00, 0xfcff, 0, 0, "bank1");
	} else {
		memory_install_readwrite8_handler(space, 0xfc00, 0xfcff, 0, 0, suzy_read, suzy_write);
	}
	if (data & 2) {
		memory_install_readwrite_bank(space, 0xfd00, 0xfdff, 0, 0, "bank2");
	} else {
		memory_install_readwrite8_handler(space, 0xfd00, 0xfdff, 0, 0, mikey_read, mikey_write);
	}

	if (data & 1)
		memory_set_bankptr(space->machine, "bank1", lynx_mem_fc00);
	if (data & 2)
		memory_set_bankptr(space->machine, "bank2", lynx_mem_fd00);
	memory_set_bank(space->machine, "bank3", (data & 4) ? 1 : 0);
	memory_set_bank(space->machine, "bank4", (data & 8) ? 1 : 0);
}

static void lynx_reset(running_machine &machine)
{
	lynx_memory_config_w(cputag_get_address_space(&machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, 0);

	cputag_set_input_line(&machine, "maincpu", M65SC02_IRQ_LINE, CLEAR_LINE);

	memset(&suzy, 0, sizeof(suzy));
	memset(&mikey, 0, sizeof(mikey));

	suzy.data[0x88]  = 0x01;
	suzy.data[0x90]  = 0x00;
	suzy.data[0x91]  = 0x00;
	mikey.data[0x80] = 0x00;
	mikey.data[0x81] = 0x00;
	mikey.data[0x88] = 0x01;
	mikey.data[0x8a] = 0x00;
	mikey.data[0x8c] = 0x00;
	mikey.data[0x90] = 0x00;
	mikey.data[0x92] = 0x00;

	lynx_uart_reset();

	// hack to allow current object loading to work
#if 1
	lynx_timer_write( 0, 0, 160 );
	lynx_timer_write( 0, 1, 0x10 | 0x8 | 0 );
	lynx_timer_write( 2, 0, 102 );
	lynx_timer_write( 2, 1, 0x10 | 0x8 | 7 );
#endif
}

static STATE_POSTLOAD( lynx_postload )
{
	lynx_memory_config_w( cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, lynx_memory_config);
}

MACHINE_START( lynx )
{
	int i;
	state_save_register_global(machine, lynx_memory_config);
	state_save_register_global_pointer(machine, lynx_mem_fe00, lynx_mem_fe00_size);
	state_save_register_postload(machine, lynx_postload, NULL);

	memory_configure_bank(machine, "bank3", 0, 1, memory_region(machine, "maincpu") + 0x0000, 0);
	memory_configure_bank(machine, "bank3", 1, 1, lynx_mem_fe00, 0);
	memory_configure_bank(machine, "bank4", 0, 1, memory_region(machine, "maincpu") + 0x01fa, 0);
	memory_configure_bank(machine, "bank4", 1, 1, lynx_mem_fffa, 0);

	lynx_audio = machine->device("custom");
	lynx_height = -1;
	lynx_width = -1;

	memset(&suzy, 0, sizeof(suzy));

	machine->add_notifier(MACHINE_NOTIFY_RESET, lynx_reset);

	for (i = 0; i < NR_LYNX_TIMERS; i++)
		lynx_timer_init(machine, i);

}


/****************************************

    Image handling

****************************************/


static void lynx_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 64)
		return;
	hash_compute(dest, &data[64], length - 64, functions);
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
	lynx_rotate = rotate0;
	if ((input_port_read(device->machine, "ROTATION") & 0x03) != 0x03)
		lynx_rotate=input_port_read(device->machine, "ROTATION") & 0x03;
}

void lynx_crc_keyword(device_image_interface &image)
{
	const char *info = NULL;

	if (strcmp(image.extrainfo(), ""))
		info = image.extrainfo();

	rotate0 = 0;

	if (info)
	{
		if(strcmp(info, "ROTATE90DEGREE") == 0)
			rotate0 = 1;
		else if (strcmp(info, "ROTATE270DEGREE") == 0)
			rotate0 = 2;
	}
}

static DEVICE_IMAGE_LOAD( lynx_cart )
{
	UINT8 *rom = memory_region(image.device().machine, "user1");
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
			lynx_granularity = header[4] | (header[5] << 8);

			logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
					header + 10, size / 1024, lynx_granularity, header + 42);

			size -= 0x40;
		}
		else if (!mame_stricmp (filetype, "lyx"))
		{
			/* 2008-10 FP: FIXME: .lyx file don't have an header, hence they miss "lynx_granularity"
            (see above). What if bank 0 has to be loaded elsewhere? And what about bank 1?
            These should work with most .lyx files, but we need additional info on raw cart images */
			if (size == 0x20000)
				lynx_granularity = 0x0200;
			else if (size == 0x80000)
				lynx_granularity = 0x0800;
			else
				lynx_granularity = 0x0400;
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
		lynx_granularity = header[4] | (header[5] << 8);

		logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
				  header + 10, size / 1024, lynx_granularity, header + 42);

		size -= 0x40;
		memcpy(rom, image.get_software_region("rom") + 0x40, size);
	}

	return IMAGE_INIT_PASS;
}

MACHINE_CONFIG_FRAGMENT(lynx_cartslot)
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("lnx,lyx")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("lynx_cart")
	MDRV_CARTSLOT_LOAD(lynx_cart)
	MDRV_CARTSLOT_PARTIALHASH(lynx_partialhash)

	/* Software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","lynx")
MACHINE_CONFIG_END
