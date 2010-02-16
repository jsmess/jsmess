/***************************************************************************

    MOS video interface chip 6560
    PeT mess@utanet.at


    2010 FP: converted to a device and merged the sound code

    TODO:
      - plenty of cleanups!
      - investigate why some vic20 carts crash emulation

***************************************************************************/

#include "emu.h"
#include "streams.h"

#include "audio/vic6560.h"


typedef struct _vic656x_state  vic656x_state;
struct _vic656x_state
{
	vic656x_type  type;

	running_device *screen;

	UINT8 reg[16];

	bitmap_t *bitmap;

	int rasterline, lastline;
	double lightpenreadtime;

	int charheight, matrix8x16, inverted;
	int chars_x, chars_y;
	int xsize, ysize, xpos, ypos;
	int chargenaddr, videoaddr;

	/* values in videoformat */
	UINT16 backgroundcolor, framecolor, helpercolor;

	/* arrays for bit to color conversion without condition checking */
	UINT16 mono[2], monoinverted[2], multi[4], multiinverted[4];

	/* video chip settings */
	int total_xsize, total_ysize, total_lines, total_vretracerate;

	/* DMA */
	vic656x_dma_read          dma_read;
	vic656x_dma_read_color    dma_read_color;

	/* lightpen */
	vic656x_lightpen_button_callback lightpen_button_cb;
	vic656x_lightpen_x_callback lightpen_x_cb;
	vic656x_lightpen_y_callback lightpen_y_cb;

	/* paddles */
	vic656x_paddle_callback        paddle_cb[2];

	/* sound part */
	int tone1pos, tone2pos, tone3pos,
	tonesize, tone1samples, tone2samples, tone3samples,
	noisesize,		  /* number of samples */
	noisepos,         /* pos of tone */
	noisesamples;	  /* count of samples to give out per tone */

	sound_stream *channel;
	INT16 *tone;

	INT8 *noise;
};

/*****************************************************************************
    CONSTANTS
*****************************************************************************/

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(device->machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE vic656x_state *get_safe_token( running_device *device )
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == VIC656X);

	return (vic656x_state *)device->token;
}

INLINE const vic656x_interface *get_interface( running_device *device )
{
	assert(device != NULL);
	assert((device->type == VIC656X));
	return (const vic656x_interface *) device->baseconfig().static_config;
}

/*****************************************************************************
    IMPLEMENTATION
*****************************************************************************/

static void vic6560_soundport_w( running_device *device, int offset, int data );


/* 2008-05 FP: lightpen code needs to read input port from vc20.c */

#define LIGHTPEN_BUTTON		(vic6560->lightpen_button_cb(device->machine))
#define LIGHTPEN_X_VALUE	(vic6560->lightpen_x_cb(device->machine))
#define LIGHTPEN_Y_VALUE	(vic6560->lightpen_y_cb(device->machine))

/* lightpen delivers values from internal counters
 * they do not start with the visual area or frame area */
#define VIC6560_X_BEGIN 38
#define VIC6560_Y_BEGIN -6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6561_X_BEGIN 38
#define VIC6561_Y_BEGIN -6
#define VIC656X_X_BEGIN ((vic6560->type == VIC6561) ? VIC6561_X_BEGIN : VIC6560_X_BEGIN)
#define VIC656X_Y_BEGIN ((vic6560->type == VIC6561) ? VIC6561_Y_BEGIN : VIC6560_Y_BEGIN)

#define VIC656X_MAME_XPOS ((vic6560->type == VIC6561) ? VIC6561_MAME_XPOS : VIC6560_MAME_XPOS)
#define VIC656X_MAME_YPOS ((vic6560->type == VIC6561) ? VIC6561_MAME_YPOS : VIC6560_MAME_YPOS)

/* lightpen behaviour in pal or mono multicolor not tested */
#define VIC6560_X_VALUE ((LIGHTPEN_X_VALUE + VIC656X_X_BEGIN + VIC656X_MAME_XPOS)/2)
#define VIC6560_Y_VALUE ((LIGHTPEN_Y_VALUE + VIC656X_Y_BEGIN + VIC656X_MAME_YPOS)/2)

/* ntsc 1 - 8 */
/* pal 5 - 19 */
#define XPOS (((int)vic6560->reg[0] & 0x7f) * 4)
#define YPOS ((int)vic6560->reg[1] * 2)

/* ntsc values >= 31 behave like 31 */
/* pal value >= 32 behave like 32 */
#define CHARS_X ((int)vic6560->reg[2] & 0x7f)
#define CHARS_Y (((int)vic6560->reg[3] & 0x7e) >> 1)

#define MATRIX8X16 (vic6560->reg[3] & 0x01)	   /* else 8x8 */
#define CHARHEIGHT (MATRIX8X16 ? 16 : 8)
#define XSIZE (CHARS_X * 8)
#define YSIZE (CHARS_Y * CHARHEIGHT)

/* colorram and backgroundcolor are changed */
#define INVERTED (!(vic6560->reg[0x0f] & 8))

#define CHARGENADDR (((int)vic6560->reg[5] & 0x0f) << 10)
#define VIDEOADDR ((((int)vic6560->reg[5] & 0xf0) << (10 - 4)) | (((int)vic6560->reg[2] & 0x80) << (9-7)))
#define VIDEORAMSIZE (YSIZE * XSIZE)
#define CHARGENSIZE (256 * HEIGHTPIXEL)

#define HELPERCOLOR (vic6560->reg[0x0e] >> 4)
#define BACKGROUNDCOLOR (vic6560->reg[0x0f] >> 4)
#define FRAMECOLOR (vic6560->reg[0x0f] & 0x07)

static void vic6560_draw_character( running_device *device, int ybegin, int yend, int ch, int yoff, int xoff, UINT16 *color )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic6560->dma_read(device->machine, (vic6560->chargenaddr + ch * vic6560->charheight + y) & 0x3fff);
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 0) = color[code >> 7];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 1) = color[(code >> 6) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 2) = color[(code >> 5) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 3) = color[(code >> 4) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 4) = color[(code >> 3) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 5) = color[(code >> 2) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 6) = color[(code >> 1) & 1];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 7) = color[code & 1];
	}
}

static void vic6560_draw_character_multi( running_device *device, int ybegin, int yend, int ch, int yoff, int xoff, UINT16 *color )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic6560->dma_read(device->machine, (vic6560->chargenaddr + ch * vic6560->charheight + y) & 0x3fff);
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 0) =
			*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 1) = color[code >> 6];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 2) =
			*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 3) = color[(code >> 4) & 3];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 4) =
			*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 5) = color[(code >> 2) & 3];
		*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 6) =
			*BITMAP_ADDR16(vic6560->bitmap, y + yoff, xoff + 7) = color[code & 3];
	}
}

static void vic6560_drawlines( running_device *device, int first, int last )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int line, vline;
	int offs, yoff, xoff, ybegin, yend, i;
	int attr, ch;

	vic6560->lastline = last;
	if (first >= last)
		return;

	for (line = first; (line < vic6560->ypos) && (line < last); line++)
	{
		memset16(BITMAP_ADDR16(vic6560->bitmap, line, 0), vic6560->framecolor, vic6560->total_xsize);
	}

	for (vline = line - vic6560->ypos; (line < last) && (line < vic6560->ypos + vic6560->ysize);)
	{
		if (vic6560->matrix8x16)
		{
			offs = (vline >> 4) * vic6560->chars_x;
			yoff = (vline & ~0xf) + vic6560->ypos;
			ybegin = vline & 0xf;
			yend = (vline + 0xf < last - vic6560->ypos) ? 0xf : ((last - line) & 0xf) + ybegin;
		}
		else
		{
			offs = (vline >> 3) * vic6560->chars_x;
			yoff = (vline & ~7) + vic6560->ypos;
			ybegin = vline & 7;
			yend = (vline + 7 < last - vic6560->ypos) ? 7 : ((last - line) & 7) + ybegin;
		}

		if (vic6560->xpos > 0)
		{
			for (i = ybegin; i <= yend; i++)
				memset16(BITMAP_ADDR16(vic6560->bitmap, yoff + i, 0), vic6560->framecolor, vic6560->xpos);
		}
		for (xoff = vic6560->xpos; (xoff < vic6560->xpos + vic6560->xsize) && (xoff < vic6560->total_xsize); xoff += 8, offs++)
		{
			ch = vic6560->dma_read(device->machine, (vic6560->videoaddr + offs) & 0x3fff);
			attr = (vic6560->dma_read_color(device->machine, (vic6560->videoaddr + offs) & 0x3fff)) & 0xf;
			if (vic6560->inverted)
			{
				if (attr & 8)
				{
					vic6560->multiinverted[0] = attr & 7;
					vic6560_draw_character_multi(device, ybegin, yend, ch, yoff, xoff, vic6560->multiinverted);
				}
				else
				{
					vic6560->monoinverted[0] = attr;
					vic6560_draw_character(device, ybegin, yend, ch, yoff, xoff, vic6560->monoinverted);
				}
			}
			else
			{
				if (attr & 8)
				{
					vic6560->multi[2] = attr & 7;
					vic6560_draw_character_multi(device, ybegin, yend, ch, yoff, xoff, vic6560->multi);
				}
				else
				{
					vic6560->mono[1] = attr;
					vic6560_draw_character(device, ybegin, yend, ch, yoff, xoff, vic6560->mono);
				}
			}
		}
		if (xoff < vic6560->total_xsize)
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset16(BITMAP_ADDR16(vic6560->bitmap, yoff + i, xoff), vic6560->framecolor, vic6560->total_xsize - xoff);
			}
		}
		if (vic6560->matrix8x16)
		{
			vline = (vline + 16) & ~0xf;
			line = vline + vic6560->ypos;
		}
		else
		{
			vline = (vline + 8) & ~7;
			line = vline + vic6560->ypos;
		}
	}

	for (; line < last; line++)
	{
		memset16(BITMAP_ADDR16(vic6560->bitmap, line, 0), vic6560->framecolor, vic6560->total_xsize);
	}
}



WRITE8_DEVICE_HANDLER( vic6560_port_w )
{
	vic656x_state *vic6560 = get_safe_token(device);

	DBG_LOG(1, "vic6560_port_w", ("%.4x:%.2x\n", offset, data));

	switch (offset)
	{
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
		vic6560_soundport_w(device, offset, data);
		break;
	}

	if (vic6560->reg[offset] != data)
	{
		switch (offset)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
		case 0xe:
		case 0xf:
			vic6560_drawlines(device, vic6560->lastline, vic6560->rasterline);
			break;
		}
		vic6560->reg[offset] = data;

		switch (offset)
		{
		case 0:
			vic6560->xpos = XPOS;
			break;
		case 1:
			vic6560->ypos = YPOS;
			break;
		case 2:
			/* ntsc values >= 31 behave like 31 */
			/* pal value >= 32 behave like 32 */
			vic6560->chars_x = CHARS_X;
			vic6560->videoaddr = VIDEOADDR;
			vic6560->xsize = XSIZE;
			break;
		case 3:
			vic6560->matrix8x16 = MATRIX8X16;
			vic6560->charheight = CHARHEIGHT;
			vic6560->chars_y = CHARS_Y;
			vic6560->ysize = YSIZE;
			break;
		case 5:
			vic6560->chargenaddr = CHARGENADDR;
			vic6560->videoaddr = VIDEOADDR;
			break;
		case 0xe:
			vic6560->multi[3] = vic6560->multiinverted[3] = vic6560->helpercolor = HELPERCOLOR;
			break;
		case 0xf:
			vic6560->inverted = INVERTED;
			vic6560->multi[1] = vic6560->multiinverted[1] = vic6560->framecolor = FRAMECOLOR;
			vic6560->mono[0] = vic6560->monoinverted[1] = vic6560->multi[0] = vic6560->multiinverted[2] = vic6560->backgroundcolor = BACKGROUNDCOLOR;
			break;
		}
	}
}

READ8_DEVICE_HANDLER( vic6560_port_r )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int val;

	switch (offset)
	{
	case 3:
		val = ((vic6560->rasterline & 1) << 7) | (vic6560->reg[offset] & 0x7f);
		break;
	case 4:						   /*rasterline */
		vic6560_drawlines(device, vic6560->lastline, vic6560->rasterline);
		val = (vic6560->rasterline / 2) & 0xff;
		break;
	case 6:						   /*lightpen horizontal */
	case 7:						   /*lightpen vertical */
		if (LIGHTPEN_BUTTON && ((attotime_to_double(timer_get_time(device->machine)) - vic6560->lightpenreadtime) * VIC6560_VRETRACERATE >= 1))
		{
			/* only 1 update each frame */
			/* and diode must recognize light */
			if (1)
			{
				vic6560->reg[6] = VIC6560_X_VALUE;
				vic6560->reg[7] = VIC6560_Y_VALUE;
			}
			vic6560->lightpenreadtime = attotime_to_double(timer_get_time(device->machine));
		}
		val = vic6560->reg[offset];
		break;
	case 8:						   /* poti 1 */
	case 9:						   /* poti 2 */
		val = vic6560->paddle_cb[offset - 8](device->machine);
		break;
	default:
		val = vic6560->reg[offset];
		break;
	}
	DBG_LOG(3, "vic6560_port_r", ("%.4x:%.2x\n", offset, val));
	return val;
}


void vic656x_raster_interrupt_gen( running_device *device )
{
	vic656x_state *vic6560 = get_safe_token(device);

	vic6560->rasterline++;
	if (vic6560->rasterline >= vic6560->total_lines)
	{
		vic6560->rasterline = 0;
		vic6560_drawlines(device, vic6560->lastline, vic6560->total_lines);
		vic6560->lastline = 0;
	}
}

UINT32 vic656x_video_update( running_device *device, bitmap_t *bitmap, const rectangle *cliprect )
{
	vic656x_state *vic6560 = get_safe_token(device);

	copybitmap(bitmap, vic6560->bitmap, 0, 0, 0, 0, cliprect);
	return 0;
}

/* Sound emulation of the chip */

/*
 * assumed model:
 * each write to a ton/noise generated starts it new
 * each generator behaves like an timer
 * when it reaches 0, the next samplevalue is given out
 */

/*
 * noise channel
 * based on a document by diku0748@diku.dk (Asger Alstrup Nielsen)
 *
 * 23 bit shift register
 * initial value (0x7ffff8)
 * after shift bit 0 is set to bit 22 xor bit 17
 * dac sample bit22 bit20 bit16 bit13 bit11 bit7 bit4 bit2(lsb)
 *
 * emulation:
 * allocate buffer for 5 sec sampledata (fastest played frequency)
 * and fill this buffer in init with the required sample
 * fast turning off channel, immediate change of frequency
 */

#define NOISE_BUFFER_SIZE_SEC 5

#define TONE1_ON (vic6560->reg[0x0a] & 0x80)
#define TONE2_ON (vic6560->reg[0x0b] & 0x80)
#define TONE3_ON (vic6560->reg[0x0c] & 0x80)
#define NOISE_ON (vic6560->reg[0x0d] & 0x80)
#define VOLUME (vic6560->reg[0x0e] & 0x0f)

#define VIC656X_CLOCK ((vic6560->type == VIC6561) ? VIC6561_CLOCK : VIC6560_CLOCK)

#define TONE_FREQUENCY_MIN  (VIC656X_CLOCK/256/128)

#define TONE1_VALUE (8 * (128 - ((vic6560->reg[0x0a] + 1) & 0x7f)))
#define TONE1_FREQUENCY (VIC656X_CLOCK/32/TONE1_VALUE)

#define TONE2_VALUE (4 * (128 - ((vic6560->reg[0x0b] + 1) & 0x7f)))
#define TONE2_FREQUENCY (VIC656X_CLOCK/32/TONE2_VALUE)

#define TONE3_VALUE (2 * (128 - ((vic6560->reg[0x0c] + 1) & 0x7f)))
#define TONE3_FREQUENCY (VIC656X_CLOCK/32/TONE3_VALUE)

#define NOISE_VALUE (32 * (128 - ((vic6560->reg[0x0d] + 1) & 0x7f)))
#define NOISE_FREQUENCY (VIC656X_CLOCK/NOISE_VALUE)

#define NOISE_FREQUENCY_MAX (VIC656X_CLOCK/32/1)


static void vic6560_soundport_w( running_device *device, int offset, int data )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int old = vic6560->reg[offset];
	stream_update(vic6560->channel);

	switch (offset)
	{
	case 0x0a:
		vic6560->reg[offset] = data;
		if (!(old & 0x80) && TONE1_ON)
		{
			vic6560->tone1pos = 0;
			vic6560->tone1samples = device->machine->sample_rate / TONE1_FREQUENCY;
			if (!vic6560->tone1samples == 0)
				vic6560->tone1samples = 1;
		}
		DBG_LOG(1, "vic6560", ("tone1 %.2x %d\n", data, TONE1_FREQUENCY));
		break;
	case 0x0b:
		vic6560->reg[offset] = data;
		if (!(old & 0x80) && TONE2_ON)
		{
			vic6560->tone2pos = 0;
			vic6560->tone2samples = device->machine->sample_rate / TONE2_FREQUENCY;
			if (vic6560->tone2samples == 0)
				vic6560->tone2samples = 1;
		}
		DBG_LOG(1, "vic6560", ("tone2 %.2x %d\n", data, TONE2_FREQUENCY));
		break;
	case 0x0c:
		vic6560->reg[offset] = data;
		if (!(old & 0x80) && TONE3_ON)
		{
			vic6560->tone3pos = 0;
			vic6560->tone3samples = device->machine->sample_rate / TONE3_FREQUENCY;
			if (vic6560->tone3samples == 0)
				vic6560->tone3samples = 1;
		}
		DBG_LOG(1, "vic6560", ("tone3 %.2x %d\n", data, TONE3_FREQUENCY));
		break;
	case 0x0d:
		vic6560->reg[offset] = data;
		if (NOISE_ON)
		{
			vic6560->noisesamples = (int) ((double) NOISE_FREQUENCY_MAX * device->machine->sample_rate
								  * NOISE_BUFFER_SIZE_SEC / NOISE_FREQUENCY);
			DBG_LOG (1, "vic6560", ("noise %.2x %d sample:%d\n",
									data, NOISE_FREQUENCY, vic6560->noisesamples));
			if ((double) vic6560->noisepos / vic6560->noisesamples >= 1.0)
			{
				vic6560->noisepos = 0;
			}
		}
		else
		{
			vic6560->noisepos = 0;
		}
		break;
	case 0x0e:
		vic6560->reg[offset] = (old & ~0x0f) | (data & 0x0f);
		DBG_LOG (3, "vic6560", ("volume %d\n", data & 0x0f));
		break;
	}
}

/************************************/
/* Sound handler update             */
/************************************/

static STREAM_UPDATE( vic6560_update )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int i, v;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++)
	{
		v = 0;
		if (TONE1_ON /*||(vic6560->tone1pos != 0) */ )
		{
			v += vic6560->tone[vic6560->tone1pos * vic6560->tonesize / vic6560->tone1samples];
			vic6560->tone1pos++;
#if 0
			vic6560->tone1pos %= vic6560->tone1samples;
#else
			if (vic6560->tone1pos >= vic6560->tone1samples)
			{
				vic6560->tone1pos = 0;
				vic6560->tone1samples = device->machine->sample_rate / TONE1_FREQUENCY;
				if (vic6560->tone1samples == 0)
					vic6560->tone1samples = 1;
			}
#endif
		}
		if (TONE2_ON /*||(vic6560->tone2pos != 0) */ )
		{
			v += vic6560->tone[vic6560->tone2pos * vic6560->tonesize / vic6560->tone2samples];
			vic6560->tone2pos++;
#if 0
			vic6560->tone2pos %= vic6560->tone2samples;
#else
			if (vic6560->tone2pos >= vic6560->tone2samples)
			{
				vic6560->tone2pos = 0;
				vic6560->tone2samples = device->machine->sample_rate / TONE2_FREQUENCY;
				if (vic6560->tone2samples == 0)
					vic6560->tone2samples = 1;
			}
#endif
		}
		if (TONE3_ON /*||(vic6560->tone3pos != 0) */ )
		{
			v += vic6560->tone[vic6560->tone3pos * vic6560->tonesize / vic6560->tone3samples];
			vic6560->tone3pos++;
#if 0
			vic6560->tone3pos %= vic6560->tone3samples;
#else
			if (vic6560->tone3pos >= vic6560->tone3samples)
			{
				vic6560->tone3pos = 0;
				vic6560->tone3samples = device->machine->sample_rate / TONE3_FREQUENCY;
				if (vic6560->tone3samples == 0)
					vic6560->tone3samples = 1;
			}
#endif
		}
		if (NOISE_ON)
		{
			v += vic6560->noise[(int) ((double) vic6560->noisepos * vic6560->noisesize / vic6560->noisesamples)];
			vic6560->noisepos++;
			if ((double) vic6560->noisepos / vic6560->noisesamples >= 1.0)
			{
				vic6560->noisepos = 0;
			}
		}
		v = (v * VOLUME) << 2;
		if (v > 32767)
			buffer[i] = 32767;
		else if (v < -32767)
			buffer[i] = -32767;
		else
			buffer[i] = v;



	}
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

/************************************/
/* Sound handler start          */
/************************************/

static void vic656x_sound_start( running_device *device )
{
	vic656x_state *vic6560 = get_safe_token(device);
	int i;

	vic6560->channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, vic6560_update);

	/* buffer for fastest played sample for 5 second so we have enough data for min 5 second */
	vic6560->noisesize = NOISE_FREQUENCY_MAX * NOISE_BUFFER_SIZE_SEC;
	vic6560->noise = auto_alloc_array(device->machine, INT8, vic6560->noisesize);
	{
		int noiseshift = 0x7ffff8;
		char data;

		for (i = 0; i < vic6560->noisesize; i++)
		{
			data = 0;
			if (noiseshift & 0x400000)
				data |= 0x80;
			if (noiseshift & 0x100000)
				data |= 0x40;
			if (noiseshift & 0x010000)
				data |= 0x20;
			if (noiseshift & 0x002000)
				data |= 0x10;
			if (noiseshift & 0x000800)
				data |= 0x08;
			if (noiseshift & 0x000080)
				data |= 0x04;
			if (noiseshift & 0x000010)
				data |= 0x02;
			if (noiseshift & 0x000004)
				data |= 0x01;
			vic6560->noise[i] = data;
			if (((noiseshift & 0x400000) == 0) != ((noiseshift & 0x002000) == 0))
				noiseshift = (noiseshift << 1) | 1;
			else
				noiseshift <<= 1;
		}
	}
	vic6560->tonesize = device->machine->sample_rate / TONE_FREQUENCY_MIN;

	if (vic6560->tonesize > 0)
	{
		vic6560->tone = auto_alloc_array(device->machine, INT16, vic6560->tonesize);

		for (i = 0; i < vic6560->tonesize; i++)
		{
			vic6560->tone[i] = (INT16)(sin (2 * M_PI * i / vic6560->tonesize) * 127 + 0.5);
		}
	}
	else
	{
		vic6560->tone = NULL;
	}
}

static DEVICE_START( vic656x )
{
	vic656x_state *vic6560 = get_safe_token(device);
	const vic656x_interface *intf = (vic656x_interface *)device->baseconfig().static_config;
	int width, height;

	vic6560->screen = devtag_get_device(device->machine, intf->screen);
	width = video_screen_get_width(vic6560->screen);
	height = video_screen_get_height(vic6560->screen);

	vic6560->type = intf->type;

	vic6560->bitmap = auto_bitmap_alloc(device->machine, width, height, BITMAP_FORMAT_INDEXED16);

	vic6560->dma_read = intf->dma_read;
	vic6560->dma_read_color = intf->dma_read_color;

	vic6560->lightpen_button_cb = intf->button_cb;
	vic6560->lightpen_x_cb = intf->x_cb;
	vic6560->lightpen_y_cb = intf->y_cb;

	vic6560->paddle_cb[0] = intf->paddle0_cb;
	vic6560->paddle_cb[1] = intf->paddle1_cb;

	switch (vic6560->type)
	{
	case VIC6560:
		vic6560->total_xsize = VIC6560_XSIZE;
		vic6560->total_ysize = VIC6560_YSIZE;
		vic6560->total_lines = VIC6560_LINES;
		vic6560->total_vretracerate = VIC6560_VRETRACERATE;
		break;
	case VIC6561:
		vic6560->total_xsize = VIC6561_XSIZE;
		vic6560->total_ysize = VIC6561_YSIZE;
		vic6560->total_lines = VIC6561_LINES;
		vic6560->total_vretracerate = VIC6561_VRETRACERATE;
		break;
	}

	vic656x_sound_start(device);

	state_save_register_device_item(device, 0, vic6560->lightpenreadtime);
	state_save_register_device_item(device, 0, vic6560->rasterline);
	state_save_register_device_item(device, 0, vic6560->lastline);

	state_save_register_device_item(device, 0, vic6560->charheight);
	state_save_register_device_item(device, 0, vic6560->matrix8x16);
	state_save_register_device_item(device, 0, vic6560->inverted);
	state_save_register_device_item(device, 0, vic6560->chars_x);
	state_save_register_device_item(device, 0, vic6560->chars_y);
	state_save_register_device_item(device, 0, vic6560->xsize);
	state_save_register_device_item(device, 0, vic6560->ysize);
	state_save_register_device_item(device, 0, vic6560->xpos);
	state_save_register_device_item(device, 0, vic6560->ypos);
	state_save_register_device_item(device, 0, vic6560->chargenaddr);
	state_save_register_device_item(device, 0, vic6560->videoaddr);

	state_save_register_device_item(device, 0, vic6560->backgroundcolor);
	state_save_register_device_item(device, 0, vic6560->framecolor);
	state_save_register_device_item(device, 0, vic6560->helpercolor);

	state_save_register_device_item_array(device, 0, vic6560->reg);

	state_save_register_device_item_array(device, 0, vic6560->mono);
	state_save_register_device_item_array(device, 0, vic6560->monoinverted);
	state_save_register_device_item_array(device, 0, vic6560->multi);
	state_save_register_device_item_array(device, 0, vic6560->multiinverted);

	state_save_register_device_item_bitmap(device, 0, vic6560->bitmap);

	state_save_register_device_item(device, 0, vic6560->tone1pos);
	state_save_register_device_item(device, 0, vic6560->tone2pos);
	state_save_register_device_item(device, 0, vic6560->tone3pos);
	state_save_register_device_item(device, 0, vic6560->tone1samples);
	state_save_register_device_item(device, 0, vic6560->tone2samples);
	state_save_register_device_item(device, 0, vic6560->tone3samples);
	state_save_register_device_item(device, 0, vic6560->noisepos);
	state_save_register_device_item(device, 0, vic6560->noisesamples);
}

static DEVICE_RESET( vic656x )
{
	vic656x_state *vic6560 = get_safe_token(device);

	vic6560->lightpenreadtime = 0.0;
	vic6560->rasterline = 0;
	vic6560->lastline = 0;

	memset(vic6560->reg, 0, 16);

	vic6560->charheight = 0;
	vic6560->matrix8x16 = 0;
	vic6560->inverted = 0;
	vic6560->chars_x = 0;
	vic6560->chars_y = 0;
	vic6560->xsize = 0;
	vic6560->ysize = 0;
	vic6560->xpos = 0;
	vic6560->ypos = 0;
	vic6560->chargenaddr = 0;
	vic6560->videoaddr = 0;

	vic6560->backgroundcolor = 0;
	vic6560->framecolor = 0;
	vic6560->helpercolor = 0;

	vic6560->mono[0] = 0;
	vic6560->mono[1] = 0;
	vic6560->monoinverted[0] = 0;
	vic6560->monoinverted[1] = 0;
	vic6560->multi[0] = 0;
	vic6560->multi[1] = 0;
	vic6560->multi[2] = 0;
	vic6560->multi[3] = 0;
	vic6560->multiinverted[0] = 0;
	vic6560->multiinverted[1] = 0;
	vic6560->multiinverted[2] = 0;
	vic6560->multiinverted[3] = 0;

	vic6560->tone1pos = 0;
	vic6560->tone2pos = 0;
	vic6560->tone3pos = 0;
	vic6560->tone1samples = 1;
	vic6560->tone2samples = 1;
	vic6560->tone3samples = 1;
	vic6560->noisepos = 0;
	vic6560->noisesamples = 1;
}


/*-------------------------------------------------
    device definition
-------------------------------------------------*/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##vic656x##s
#define DEVTEMPLATE_FEATURES			DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME				"6560 / 6561 VIC"
#define DEVTEMPLATE_FAMILY				"6560 / 6561 VIC"
#include "devtempl.h"
