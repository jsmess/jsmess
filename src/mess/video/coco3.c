/***************************************************************************

	CoCo 3 Video Hardware

	TODO:
		- Implement "burst phase invert" (it switches the artifact colors)
		- Figure out what Bit 3 of $FF98 is and implement it
		- Learn more about the "mystery" CoCo 3 video modes
		- Support the VSC register


	Mid frame raster effects (source John Kowalski)
		Here are the things that get changed mid-frame:
 			-palette registers ($ffb0-$ffbf)
 			-horizontal resolution (switches between 256 and 320 pixels, $ff99)
			-horizontal scroll position (bits 0-6 $ff9f)
			-horizontal virtual screen (bit 7 $ff9f)
			-pixel height (bits 0-2 $ff98)
			-border color ($ff9a)
		On the positive side, you don't have to worry about registers
		$ff9d/ff9e being changed mid-frame.  Even if they are changed
		mid-frame, they have no effect on the displayed image.  The video
		address only gets latched at the top of the frame.

***************************************************************************/

#include <assert.h>
#include <math.h>

#include "driver.h"
#include "machine/6821pia.h"
#include "machine/6883sam.h"
#include "video/m6847.h"
#include "video/generic.h"
#include "includes/coco.h"


#if defined(__GNUC__) && (__GNUC__ >= 3)
#define RESTRICT	__restrict__
#else /* !__GNUC__ */
#define RESTRICT
#endif /* __GNUC__ */

/*************************************
 *
 *	Global variables
 *
 *************************************/

typedef struct _coco3_scanline_record coco3_scanline_record;
struct _coco3_scanline_record
{
	UINT8 ff98;
	UINT8 ff99;
	UINT8 ff9a;
	UINT8 index;

	UINT8 palette[16];

	UINT8 data[160];
};

typedef struct _coco3_video coco3_video;
struct _coco3_video
{
	/* Info set up on initialization */
	UINT32 composite_palette[64];
	UINT32 rgb_palette[64];
	UINT8 fontdata[128][8];
	mame_timer *gime_fs_timer;

	/* CoCo 3 palette status */
	UINT8 palette_ram[16];
	UINT32 palette_colors[16];

	/* Incidentals */
	UINT32 legacy_video;
	UINT32 top_border_scanlines;
	UINT32 display_scanlines;
	UINT32 video_position;
	UINT8 line_in_row;
	UINT8 blink;
	UINT8 dirty[2];
	UINT8 video_type;

	/* video state; every scanline the video state for the scanline is copied
	 * here and only rendered in VIDEO_UPDATE */
	coco3_scanline_record scanlines[384];
};

static coco3_video *video;



/*************************************
 *
 *	Parameters
 *
 *************************************/

#define LOG_PALETTE	0
#define LOG_PREPARE 0



/*************************************
 *
 *	Video rendering
 *
 *************************************/

static void color_batch(UINT32 *results, const UINT8 *indexes, int count)
{
	UINT32 c;
	const UINT32 *current_palette;
	int is_black_white;
	int i;

	if (video->video_type)
	{
		/* RGB */
		current_palette = video->rgb_palette;
		is_black_white = FALSE;
	}
	else
	{
		/* Composite/TV */
		current_palette = video->composite_palette;
		is_black_white = (coco3_gimereg[8] & 0x10) ? TRUE : FALSE;
	}

	for (i = 0; i < count; i++)
	{
		c = current_palette[indexes[i] & 0x3F];
	
		if (is_black_white)
		{
			/* We are on a composite monitor/TV and the monochrome phase invert
			 * flag is on in the GIME.  This means we have to average out all
			 * colors
			 */
			UINT32 red   = ((c & 0xFF0000) >> 16);
			UINT32 green = ((c & 0x00FF00) >>  8);
			UINT32 blue  = ((c & 0x0000FF) >>  0);
			c = (red + green + blue) / 3;
		}
		
		results[i] = c;
	}
}



static UINT32 color(UINT8 index)
{
	UINT32 c;
	color_batch(&c, &index, 1);
	return c;
}



INLINE UINT8 get_char_info(const coco3_scanline_record *scanline_record,
	int i, int has_attrs, UINT32 *bg, UINT32 *fg)
{
	UINT8 byte, attr, char_data = 0x00;

	/* determine the basics */
	byte = scanline_record->data[i * (has_attrs ? 2 : 1)];
	if (scanline_record->index < 8)
		char_data = video->fontdata[(byte & 0x7F)][scanline_record->index];

	if (has_attrs)
	{
		/* has attributes */
		attr = scanline_record->data[i * (has_attrs ? 2 : 1) + 1];

		/* colors */
		*bg = video->palette_colors[((attr >> 0) & 0x07) + 0x00];
		*fg = video->palette_colors[((attr >> 3) & 0x07) + 0x08];

		/* underline attribute */
		if (attr & 0x40)
		{
			/* to quote SockMaster:
			*
			* The underline attribute will light up the bottom scan line of the character
			* if the lines are set to 8 or 9.  Not appear at all when less, or appear on
			* the 2nd to bottom scan line if set higher than 9.  Further exception being
			* the $x7 setting where the whole screen is filled with only one line of data
			* - but it's glitched - the line repeats over and over again every 16 scan
			* lines..  Nobody will use this mode, but that's what happens if you want to
			* make things really authentic :)
			*/
			switch(scanline_record->ff98 & 0x07)
			{
				case 0x03:	/* 8 scanlines per row */
					if (scanline_record->index == 7)
						char_data = 0xFF;
					break;

				case 0x04:	/* 9 scanlines per row */
				case 0x05:	/* 10 scanlines per row */
					if (scanline_record->index == 8)
						char_data = 0xFF;
					break;

				case 0x06:	/* 11 scanlines per row */
					if (scanline_record->index == 9)
						char_data = 0xFF;
					break;
			}
		}

		/* blink attribute */
		if ((attr & 0x80) && video->blink)
			char_data = 0x00;
	}
	else
	{
		/* no attributes */
		*bg = video->palette_colors[0];
		*fg = video->palette_colors[1];
	}
	return char_data;
}



static void wide_text_mode(UINT32 *line, const coco3_scanline_record *scanline_record,
	int char_count, int has_attrs)
{
	int i;
	UINT8 char_data;
	UINT32 bg, fg;

	for (i = 0; i < char_count; i++)
	{
		char_data = get_char_info(scanline_record, i, has_attrs, &bg, &fg);

		line[i * 16 +  0] = line[i * 16 +  1] = (char_data & 0x80) ? fg : bg;
		line[i * 16 +  2] = line[i * 16 +  3] = (char_data & 0x40) ? fg : bg;
		line[i * 16 +  4] = line[i * 16 +  5] = (char_data & 0x20) ? fg : bg;
		line[i * 16 +  6] = line[i * 16 +  7] = (char_data & 0x10) ? fg : bg;
		line[i * 16 +  8] = line[i * 16 +  9] = (char_data & 0x08) ? fg : bg;
		line[i * 16 + 10] = line[i * 16 + 11] = (char_data & 0x04) ? fg : bg;
		line[i * 16 + 12] = line[i * 16 + 13] = (char_data & 0x02) ? fg : bg;
		line[i * 16 + 14] = line[i * 16 + 15] = (char_data & 0x01) ? fg : bg;
	}
}



static void narrow_text_mode(UINT32 *line, const coco3_scanline_record *scanline_record,
	int char_count, int has_attrs)
{
	int i;
	UINT8 char_data;
	UINT32 bg, fg;

	for (i = 0; i < char_count; i++)
	{
		char_data = get_char_info(scanline_record, i, has_attrs, &bg, &fg);

		line[i * 8 + 0] = (char_data & 0x80) ? fg : bg;
		line[i * 8 + 1] = (char_data & 0x40) ? fg : bg;
		line[i * 8 + 2] = (char_data & 0x20) ? fg : bg;
		line[i * 8 + 3] = (char_data & 0x10) ? fg : bg;
		line[i * 8 + 4] = (char_data & 0x08) ? fg : bg;
		line[i * 8 + 5] = (char_data & 0x04) ? fg : bg;
		line[i * 8 + 6] = (char_data & 0x02) ? fg : bg;
		line[i * 8 + 7] = (char_data & 0x01) ? fg : bg;
	}
}



INLINE void graphics_mode(UINT32 *RESTRICT line, const coco3_scanline_record *scanline_record,
	int byte_count, int bpp, int pixel_width)
{
	int i, j, k;
	UINT8 byte;
	UINT32 c;
	UINT32 colors[16];

	color_batch(colors, scanline_record->palette, 1 << bpp);

	for (i = 0; i < byte_count; i++)
	{
		byte = scanline_record->data[i];
		for (j = 0; j < 8; j += bpp)
		{
			c = colors[(byte >> (8 - bpp - j)) & ((1 << bpp) - 1)];

			for (k = 0; k < pixel_width; k++)
				*(line++) = c;
		}
	}
}



static void graphics_none(UINT32 *line, int width)
{
	memset(line, 0, width * sizeof(*line));
}



static void coco3_render_scanline(mame_bitmap *bitmap, int scanline)
{
	const coco3_scanline_record *scanline_record;
	UINT32 *line;
	UINT32 border_color;
	int i;

	/* get the basics */
	line = BITMAP_ADDR32(bitmap, scanline, 0);
	scanline_record = &video->scanlines[scanline];
	border_color = color(scanline_record->ff9a);

	scanline -= video->top_border_scanlines;
	if ((scanline >= 0) && (scanline < video->display_scanlines))
	{
		/* display scanline */
		if (scanline_record->ff98 & 0x80)
		{
			/* graphics */
			switch(scanline_record->ff99 & 0x1F)
			{
				case 0x00:	graphics_mode(&line[64], scanline_record,  16, 1,  4);		break;
				case 0x01:	graphics_mode(&line[64], scanline_record,  16, 2,  8);		break;
				case 0x02:	graphics_mode(&line[64], scanline_record,  16, 4, 16);		break;
				case 0x03:	graphics_none(&line[64], 512);								break;
				case 0x04:	graphics_mode(&line[ 0], scanline_record,  20, 1,  4);		break;
				case 0x05:	graphics_mode(&line[ 0], scanline_record,  20, 2,  8);		break;
				case 0x06:	graphics_mode(&line[ 0], scanline_record,  20, 4, 16);		break;
				case 0x07:	graphics_none(&line[ 0], 640);								break;
				case 0x08:	graphics_mode(&line[64], scanline_record,  32, 1,  2);		break;
				case 0x09:	graphics_mode(&line[64], scanline_record,  32, 2,  4);		break;
				case 0x0A:	graphics_mode(&line[64], scanline_record,  32, 4,  8);		break;
				case 0x0B:	graphics_none(&line[64], 512);								break;
				case 0x0C:	graphics_mode(&line[ 0], scanline_record,  40, 1,  2);		break;
				case 0x0D:	graphics_mode(&line[ 0], scanline_record,  40, 2,  4);		break;
				case 0x0E:	graphics_mode(&line[ 0], scanline_record,  40, 4,  8);		break;
				case 0x0F:	graphics_none(&line[ 0], 640);								break;
				case 0x10:	graphics_mode(&line[64], scanline_record,  64, 1,  1);		break;
				case 0x11:	graphics_mode(&line[64], scanline_record,  64, 2,  2);		break;
				case 0x12:	graphics_mode(&line[64], scanline_record,  64, 4,  4);		break;
				case 0x13:	graphics_none(&line[64], 512);								break;
				case 0x14:	graphics_mode(&line[ 0], scanline_record,  80, 1,  1);		break;
				case 0x15:	graphics_mode(&line[ 0], scanline_record,  80, 2,  2);		break;
				case 0x16:	graphics_mode(&line[ 0], scanline_record,  80, 4,  4);		break;
				case 0x17:	graphics_none(&line[ 0], 640);								break;
				case 0x18:	graphics_none(&line[64], 512);								break;
				case 0x19:	graphics_mode(&line[64], scanline_record, 128, 2,  1);		break;
				case 0x1A:	graphics_mode(&line[64], scanline_record, 128, 4,  2);		break;
				case 0x1B:	graphics_none(&line[64], 512);								break;
				case 0x1C:	graphics_none(&line[ 0], 640);								break;
				case 0x1D:	graphics_mode(&line[ 0], scanline_record, 160, 2,  1);		break;
				case 0x1E:	graphics_mode(&line[ 0], scanline_record, 160, 4,  2);		break;
				case 0x1F:	graphics_none(&line[ 0], 640);								break;
			}
		}
		else
		{
			/* text */
			switch(scanline_record->ff99 & 0x15)
			{
				case 0x00:	wide_text_mode(&line[64], scanline_record, 32, FALSE);		break;
				case 0x01:	wide_text_mode(&line[64], scanline_record, 32, TRUE);		break;
				case 0x04:	wide_text_mode(&line[ 0], scanline_record, 40, FALSE);		break;
				case 0x05:	wide_text_mode(&line[ 0], scanline_record, 40, TRUE);		break;
				case 0x10:	narrow_text_mode(&line[64], scanline_record, 64, FALSE);	break;
				case 0x11:	narrow_text_mode(&line[64], scanline_record, 64, TRUE);		break;
				case 0x14:	narrow_text_mode(&line[ 0], scanline_record, 80, FALSE);	break;
				case 0x15:	narrow_text_mode(&line[ 0], scanline_record, 80, TRUE);		break;
			}
		}

		if (!(scanline_record->ff99 & 0x04))
		{
			/* side borders */
			for (i = 0; i < 64; i++)
				line[i] = border_color;
			for (i = 576; i < 640; i++)
				line[i] = border_color;
		}
	}
	else
	{
		/* border scanline */
		for (i = 0; i < 640; i++)
			line[i] = border_color;
	}
}



VIDEO_UPDATE( coco3 )
{
	int i, row;
	UINT32 *line;
	UINT32 rc = 0;

	/* choose video type */
	video->video_type = screen ? 1 : 0;

	/* set all of the palette colors */
	for (i = 0; i < 16; i++)
		video->palette_colors[i] = color(video->palette_ram[i]);

	if (video->legacy_video)
	{
		/* legacy CoCo 1/2 graphics */
		rc = video_update_m6847(machine, screen, bitmap, cliprect);

		if ((rc & UPDATE_HAS_NOT_CHANGED) == 0)
		{
			/* need to double up all pixels */
			for (row = cliprect->min_y; row <= cliprect->max_y; row++)
			{
				line = BITMAP_ADDR32(bitmap, row, 0);
				for (i = 319; i >= 0; i--)
					line[i * 2 + 0] = line[i * 2 + 1] = line[i];				
			}
		}
	}
	else
	{
		/* CoCo 3 graphics */
		if (video->dirty[screen])
		{
			for (row = cliprect->min_y; row <= cliprect->max_y; row++)
				coco3_render_scanline(bitmap, row);
			video->dirty[screen] = FALSE;
		}
		else
		{
			rc = UPDATE_HAS_NOT_CHANGED;
		}
	}
	return rc;
}



/*************************************
 *
 *	Miscellaneous
 *
 *************************************/

static void coco3_set_dirty(void)
{
	int i;
	for (i = 0; i < sizeof(video->dirty) / sizeof(video->dirty[0]); i++)
		video->dirty[i] = TRUE;
}



static int coco3_new_frame(void)
{
	int gime_field_sync = 0;
	
	/* changing from non-legacy to legacy video? */
	if (!video->legacy_video && (coco3_gimereg[0] & 0x80))
		coco3_set_dirty();

	video->legacy_video = (coco3_gimereg[0] & 0x80) ? TRUE : FALSE;
	video->line_in_row = (coco3_gimereg[12] & 0x0F);

	if (!video->legacy_video)
	{
		/* CoCo 3 video */
		switch(coco3_gimereg[9] & 0x60)
		{
			case 0x00:		/* 192 lines */
				video->top_border_scanlines = 26;
				video->display_scanlines = 192;
				gime_field_sync = video->top_border_scanlines + video->display_scanlines;
				break;

			case 0x20:		/* 200 lines */
				video->top_border_scanlines = 24;
				video->display_scanlines = 200;
				gime_field_sync = video->top_border_scanlines + video->display_scanlines - 1;
				break;

			case 0x40:		/* zero/infinite lines */
				video->top_border_scanlines = 0;
				video->display_scanlines = 0;
				gime_field_sync = video->top_border_scanlines + video->display_scanlines;
				break;

			case 0x60:		/* 225 lines */
				video->top_border_scanlines = 9;
				video->display_scanlines = 225;
				gime_field_sync = video->top_border_scanlines + video->display_scanlines;
				break;
		}
		video->video_position = coco3_get_video_base(0xFF, 0xFF);
	}
	else
	{
		/* legacy video; some of these are just filled in for fun */
		video->top_border_scanlines = 25;
		video->display_scanlines = 192;
		video->video_position = 0;
	}

	/* set up GIME field sync */
	mame_timer_adjust(video->gime_fs_timer,
		m6847_scanline_time(gime_field_sync),
		0,
		time_never);

	return video->legacy_video;
}



INLINE void memcpy_dirty(int *dirty, void *RESTRICT dest, const void *RESTRICT src, size_t len)
{
	if (!*dirty)
		*dirty = memcmp(dest, src, len) != 0;
	if (*dirty)
		memcpy(dest, src, len);
}



static void coco3_prepare_scanline(int scanline)
{
	static const UINT32 lines_per_row[] = { 1, 1, 2, 8, 9, 10, 11, ~0 };
	static const UINT32 gfx_bytes_per_row[] = { 16, 20, 32, 40, 64, 80, 128, 160 };
	UINT32 video_offset, video_data_size;
	UINT32 bytes_per_row, segment_length;
	coco3_scanline_record *scanline_record;
	const UINT8 *video_data;
	int dirty;

	if (LOG_PREPARE)
		logerror("coco3_prepare_scanline(): scanline=%d video_position=0x%06X\n", scanline, video->video_position);
	dirty = video->dirty ? TRUE : FALSE;

	/* copy the basics */
	scanline_record = &video->scanlines[scanline];
	memcpy_dirty(&dirty, &scanline_record->ff98, &coco3_gimereg[8], 1);
	memcpy_dirty(&dirty, &scanline_record->ff99, &coco3_gimereg[9], 1);
	memcpy_dirty(&dirty, &scanline_record->ff9a, &coco3_gimereg[10], 1);

	/* is this a display scanline? */
	scanline -= video->top_border_scanlines;
	if ((scanline >= 0) && (scanline <= video->display_scanlines))
	{
		/* yes, this is indeed a display scanline; copy the palette */
		memcpy_dirty(&dirty, scanline_record->palette, video->palette_ram, 16);

		/* get ready to copy the video memory; get position and offsets */
		video_data = &mess_ram[video->video_position % mess_ram_size];
		video_offset = (coco3_gimereg[15] & 0x7F) * 2;
		video_data_size = sizeof(scanline_record->data);

		/* FF9F offsets wrap around every 256 bytes, even if bit 7 is not set.
		 * Therefore, we must split things up into two separate segments */
		segment_length = MIN(video_data_size, 256 - video_offset);
		memcpy_dirty(&dirty, scanline_record->data, video_data + video_offset, segment_length);
		memcpy_dirty(&dirty, scanline_record->data + segment_length, video_data, video_data_size - segment_length);

		/* next line */
		memcpy_dirty(&dirty, &scanline_record->index, &video->line_in_row, sizeof(video->line_in_row));
		video->line_in_row++;

		/* do we have to advance to the next row? */
		if (video->line_in_row >= lines_per_row[coco3_gimereg[8] & 0x07])
		{
			/* on to the next row */
			video->line_in_row = 0;

			if (coco3_gimereg[15] & 0x80)
			{
				/* FF9F scrolling mode */
				bytes_per_row = 256;
			}
			else if (coco3_gimereg[8] & 0x80)
			{
				/* calc bytes per row for graphics */
				bytes_per_row = gfx_bytes_per_row[(coco3_gimereg[9] & 0x1C) >> 2];
			}
			else
			{
				/* calc bytes per row for text */
				bytes_per_row = 32;
				if (coco3_gimereg[9] & 0x04)
					bytes_per_row += 8;
				if (coco3_gimereg[9] & 0x10)
					bytes_per_row *= 2;
				if (coco3_gimereg[9] & 0x01)
					bytes_per_row *= 2;
			}

			video->video_position += bytes_per_row;
		}
	}
	else
	{
		if (scanline_record->index != 0)
		{
			scanline_record->index = 0;
			dirty = TRUE;
		}
	}
	if (dirty)
		coco3_set_dirty();
}



WRITE8_HANDLER(coco3_palette_w)
{
	data &= 0x3f;

	if (paletteram[offset] != data)
	{
		paletteram[offset] = data;
		if (LOG_PALETTE)
			logerror("CoCo3 Palette: %i <== $%02x\n", offset, data);
	}
}



UINT32 coco3_get_video_base(UINT8 ff9d_mask, UINT8 ff9e_mask)
{
	/* The purpose of the ff9d_mask and ff9e_mask is to mask out bits that are
	 * ignored in lo-res mode.  Specifically, $FF9D is masked with $E0, and
	 * $FF9E is masked with $3F
	 *
	 * John Kowalski confirms this behavior
	 */
	return	((offs_t) (coco3_gimereg[14] & ff9e_mask)	* 0x00008)
		|	((offs_t) (coco3_gimereg[13] & ff9d_mask)	* 0x00800)
		|	((offs_t) (coco3_gimereg[11] & 0x03)		* 0x80000);
}



static void gime_fs(int dummy)
{
	coco3_gime_field_sync_callback();
}



/*************************************
 *
 *	Initialization
 *
 *************************************/

static UINT32 get_composite_color(int color)
{
	/* CMP colors
	 *
	 * These colors are of the format IICCCC, where II is the intensity and
	 * CCCC is the base color.  There is some weirdness because intensity
	 * is often different for each base color.
	 *
	 * The code below is based on an algorithm specified in the following
	 * CoCo BASIC program was used to approximate composite colors.
	 * (Program by SockMaster):
	 * 
	 * 10 POKE65497,0:DIMR(63),G(63),B(63):WIDTH80:PALETTE0,0:PALETTE8,54:CLS1
	 * 20 SAT=92:CON=70:BRI=-50:L(0)=0:L(1)=47:L(2)=120:L(3)=255
	 * 30 W=.4195456981879*1.01:A=W*9.2:S=A+W*5:D=S+W*5:P=0:FORH=0TO3:P=P+1
	 * 40 BRI=BRI+CON:FORG=1TO15:R(P)=COS(A)*SAT+BRI
	 * 50 G(P)=(COS(S)*SAT)*1+BRI:B(P)=(COS(D)*SAT)*1+BRI:P=P+1
	 * 55 A=A+W:S=S+W:D=D+W:NEXT:R(P-16)=L(H):G(P-16)=L(H):B(P-16)=L(H)
	 * 60 NEXT:R(63)=R(48):G(63)=G(48):B(63)=B(48)
	 * 70 FORH=0TO63STEP1:R=INT(R(H)):G=INT(G(H)):B=INT(B(H)):IFR<0THENR=0
	 * 80 IFG<0THENG=0
	 * 90 IFB<0THENB=0
	 * 91 IFR>255THENR=255
	 * 92 IFG>255THENG=255
	 * 93 IFB>255THENB=255
	 * 100 PRINTRIGHT$(STR$(H),2);" $";:R=R+256:G=G+256:B=B+256
	 * 110 PRINTRIGHT$(HEX$(R),2);",$";RIGHT$(HEX$(G),2);",$";RIGHT$(HEX$(B),2)
	 * 115 IF(H AND15)=15 THENIFINKEY$=""THEN115ELSEPRINT
	 * 120 NEXT
	 *
	 *	At one point, we used a different SockMaster program, but the colors
	 *	produced were too dark for people's taste
	 *
	 *	10 POKE65497,0:DIMR(63),G(63),B(63):WIDTH80:PALETTE0,0:PALETTE8,54:CLS1
	 *	20 SAT=92:CON=53:BRI=-16:L(0)=0:L(1)=47:L(2)=120:L(3)=255
	 *	30 W=.4195456981879*1.01:A=W*9.2:S=A+W*5:D=S+W*5:P=0:FORH=0TO3:P=P+1
	 *	40 BRI=BRI+CON:FORG=1TO15:R(P)=COS(A)*SAT+BRI
	 *	50 G(P)=(COS(S)*SAT)*.50+BRI:B(P)=(COS(D)*SAT)*1.9+BRI:P=P+1
	 *	55 A=A+W:S=S+W:D=D+W:NEXT:R(P-16)=L(H):G(P-16)=L(H):B(P-16)=L(H)
	 *	60 NEXT:R(63)=R(48):G(63)=G(48):B(63)=B(48)
	 *	70 FORH=0TO63STEP1:R=INT(R(H)):G=INT(G(H)):B=INT(B(H)):IFR<0THENR=0
	 *	80 IFG<0THENG=0
	 *	90 IFB<0THENB=0
	 *	91 IFR>255THENR=255
	 *	92 IFG>255THENG=255
	 *	93 IFB>255THENB=255
	 *	100 PRINTRIGHT$(STR$(H),2);" $";:R=R+256:G=G+256:B=B+256
	 *	110 PRINTRIGHT$(HEX$(R),2);",$";RIGHT$(HEX$(G),2);",$";RIGHT$(HEX$(B),2)
	 *	115 IF(H AND15)=15 THENIFINKEY$=""THEN115ELSEPRINT
	 *	120 NEXT
	 */

	double saturation, brightness, contrast;
	int offset;
	double w;
	int r, g, b;

	switch(color) {
	case 0:
		r = g = b = 0;
		break;

	case 16:
		r = g = b = 47;
		break;

	case 32:
		r = g = b = 120;
		break;

	case 48:
	case 63:
		r = g = b = 255;
		break;

	default:
		w = .4195456981879*1.01;
		contrast = 70;
		saturation = 92;
		brightness = -50;
		brightness += ((color / 16) + 1) * contrast;
		offset = (color % 16) - 1 + (color / 16)*15;
		r = cos(w*(offset +  9.2)) * saturation + brightness;
		g = cos(w*(offset + 14.2)) * saturation + brightness;
		b = cos(w*(offset + 19.2)) * saturation + brightness;

		if (r < 0)
			r = 0;
		else if (r > 255)
			r = 255;

		if (g < 0)
			g = 0;
		else if (g > 255)
			g = 255;

		if (b < 0)
			b = 0;
		else if (b > 255)
			b = 255;
		break;
	}
	return (UINT32) ((r << 16) | (g << 8) | (b << 0));
}



static UINT32 get_rgb_color(int color)
{
	return	(((color >> 4) & 2) | ((color >> 2) & 1)) * 0x550000
		|	(((color >> 3) & 2) | ((color >> 1) & 1)) * 0x005500
		|	(((color >> 2) & 2) | ((color >> 0) & 1)) * 0x000055;
}



static void internal_video_start_coco3(m6847_type type)
{
	int i;
	m6847_config cfg;
	const UINT8 *rom;

	/* allocate video */
	video = auto_malloc(sizeof(*video));
	memset(video, 0, sizeof(*video));
	coco3_set_dirty();

	/* initialize palette */
	for (i = 0; i < 64; i++)
	{
		video->composite_palette[i] = get_composite_color(i);
		video->rgb_palette[i] = get_rgb_color(i);
	}

	/* inidentals */
	paletteram = video->palette_ram;

	/* font */
	rom = memory_region(REGION_CPU1);
	for (i = 0; i < 32; i++)
	{
		/* characters 0-31 are at $FA10 - $FB0F */
		memcpy(video->fontdata[i], &rom[0xFA10 - 0x8000 + (i * 8)], 8);	
	}
	for (i = 32; i < 128; i++)
	{
		/* characters 32-127 are at $F09D - $F39C */
		memcpy(video->fontdata[i], &rom[0xF09D - 0x8000 + ((i - 32) * 8)], 8);	
	}

	/* GIME field sync timer */
	video->gime_fs_timer = timer_alloc(gime_fs);

	/* initialize the CoCo video code */
	memset(&cfg, 0, sizeof(cfg));
	cfg.type = type;
	cfg.cpu0_timing_factor = 4;
	cfg.get_attributes = coco_get_attributes;
	cfg.get_video_ram = sam_m6847_get_video_ram;
	cfg.horizontal_sync_callback = coco3_horizontal_sync_callback;
	cfg.field_sync_callback = coco3_field_sync_callback;
	cfg.custom_palette = video->palette_colors;
	cfg.new_frame_callback = coco3_new_frame;
	cfg.custom_prepare_scanline = coco3_prepare_scanline;
	m6847_init(&cfg);

	/* save state stuff */
	state_save_register_global_array(video->palette_ram);
	state_save_register_global(video->legacy_video);
	state_save_register_global(video->top_border_scanlines);
	state_save_register_global(video->display_scanlines);
	state_save_register_func_postload(coco3_set_dirty);
}



VIDEO_START( coco3 )
{
	internal_video_start_coco3(M6847_VERSION_GIME_NTSC);
	return 0;
}

VIDEO_START( coco3p )
{
	internal_video_start_coco3(M6847_VERSION_GIME_PAL);
	return 0;
}



void coco3_vh_blink(void)
{
	video->blink = !video->blink;
	coco3_set_dirty();
}
