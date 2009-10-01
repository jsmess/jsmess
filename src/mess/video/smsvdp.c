/*
    For more information, please see:
    - http://cgfm2.emuviews.com/txt/msvdp.txt
    - http://www.smspower.org/forums/viewtopic.php?p=44198

A scanline contains the following sections:
  - horizontal sync     1  ED      => HSYNC high/increment line counter/generate interrupts/etc
  - left blanking       2  ED-EE
  - color burst        14  EE-EF
  - left blanking       8  F5-F9
  - left border        13  F9-FF
  - active display    256  00-7F
  - right border       15  80-87
  - right blanking      8  87-8B
  - horizontal sync    25  8B-97   => HSYNC low


NTSC frame timing
                       256x192         256x224        256x240 (doesn't work on real hardware)
  - vertical blanking   3  D5-D7        3  E5-E7       3  EE-F0
  - top blanking       13  D8-E4       13  E8-F4      13  F1-FD
  - top border         27  E5-FF       11  F5-FF       3  FD-FF
  - active display    192  00-BF      224  00-DF     240  00-EF
  - bottom border      24  C0-D7        8  E0-E7       0  F0-F0
  - bottom blanking     3  D8-DA        3  E8-EA       3  F0-F2


PAL frame timing
                       256x192         256x224        256x240
  - vertical blanking   3  BA-BC        3  CA-CC       3  D2-D4
  - top blanking       13  BD-C9       13  CD-D9      13  D5-E1
  - top border         54  CA-FF       38  DA-FF      30  E2-FF
  - active display    192  00-BF      224  00-DF     240  00-EF
  - bottom border      48  C0-EF       32  E0-FF      24  F0-07
  - bottom blanking     3  F0-F2        3  00-02       3  08-0A

*/

#include "driver.h"
#include "video/smsvdp.h"
#include "includes/sms.h"


#define IS_SMS1_VDP			( smsvdp.features & MODEL_315_5124 )
#define IS_SMS2_VDP			( smsvdp.features & MODEL_315_5246 )
#define IS_GAMEGEAR_VDP		( smsvdp.features & MODEL_315_5378 )

#define STATUS_VINT			(0x80)	/* Pending vertical interrupt flag */
#define STATUS_SPROVR		(0x40)	/* Sprite overflow flag */
#define STATUS_SPRCOL		(0x20)	/* Object collision flag */
#define STATUS_HINT			(0x02)	/* Pending horizontal interrupt flag */

#define GG_CRAM_SIZE		(0x40)	/* 32 colors x 2 bytes per color = 64 bytes */
#define SMS_CRAM_SIZE		(0x20)	/* 32 colors x 1 bytes per color = 32 bytes */
#define MAX_CRAM_SIZE		0x40

#define VRAM_SIZE			(0x4000)

#define PRIORITY_BIT		0x1000
#define BACKDROP_COLOR		( ( smsvdp.vdp_mode == 4 ? 0x10 : 0x00 ) + (smsvdp.reg[0x07] & 0x0F))

#define NUM_OF_REGISTER		(0x10)  /* 16 registers */

#define INIT_VCOUNT			0
#define VERTICAL_BLANKING	1
#define TOP_BLANKING		2
#define TOP_BORDER			3
#define ACTIVE_DISPLAY_V	4
#define BOTTOM_BORDER		5
#define BOTTOM_BLANKING		6

static const UINT8 sms_ntsc_192[7] = { 0xD5, 3, 13, 27, 192, 24, 3 };
static const UINT8 sms_ntsc_224[7] = { 0xE5, 3, 13, 11, 224,  8, 3 };
static const UINT8 sms_ntsc_240[7] = { 0xEE, 3, 13,  3, 240,  0, 3 };
static const UINT8 sms_pal_192[7]  = { 0xBA, 3, 13, 54, 192, 48, 3 };
static const UINT8 sms_pal_224[7]  = { 0xCA, 3, 13, 38, 224, 32, 3 };
static const UINT8 sms_pal_240[7]  = { 0xD2, 3, 13, 30, 240, 24, 3 };

static struct _smsvdp
{
	UINT32		features;
	UINT8		reg[NUM_OF_REGISTER];		/* All the registers */
	UINT8		status;						/* Status register */
	UINT8		reg9copy;					/* Internal copy of register 9 */
	UINT8		addrmode;					/* Type of VDP action */
	UINT16		addr;						/* Contents of internal VDP address register */
	UINT8		cram_mask;					/* Mask to switch between SMS and GG CRAM sizes */
	int			cram_dirty;					/* Have there been any changes to the CRAM area */
	int			pending;
	UINT8		buffer;
	int			gg_sms_mode;				/* Shrunk SMS screen on GG lcd mode flag */
	int			irq_state;					/* The status of the IRQ line of the VDP */
	int			vdp_mode;					/* Current mode of the VDP: 0,1,2,3,4 */
	int			y_pixels;					/* 192, 224, 240 */
	int			line_counter;
	UINT8		*VRAM;						/* Pointer to VRAM */
	UINT8		*CRAM;						/* Pointer to CRAM */
	const UINT8	*sms_frame_timing;
	bitmap_t	*prev_bitmap;
	int			prev_bitmap_saved;
	UINT8		*collision_buffer;

	/* lineBuffer will be used to hold 5 lines of line data. Line #0 is the regular blitting area.
       Lines #1-#4 will be used as a kind of cache to be used for vertical scaling in the gamegear
       sms compatibility mode.
     */
	int			*line_buffer;
	int			current_palette[32];
	void (*int_callback)(running_machine*,int);
	emu_timer	*smsvdp_display_timer;
} smsvdp;

static TIMER_CALLBACK(smsvdp_display_callback);
static void sms_refresh_line(running_machine *machine, bitmap_t *bitmap, int offsetx, int offsety, int line);
static void sms_update_palette(void);

static void set_display_settings( running_machine *machine )
{
	const device_config *screen = video_screen_first(machine->config);
	int height = video_screen_get_height(screen);
	int M1, M2, M3, M4;
	M1 = smsvdp.reg[0x01] & 0x10;
	M2 = smsvdp.reg[0x00] & 0x02;
	M3 = smsvdp.reg[0x01] & 0x08;
	M4 = smsvdp.reg[0x00] & 0x04;
	smsvdp.y_pixels = 192;
	if ( M4 )
	{
		/* mode 4 */
		smsvdp.vdp_mode = 4;
		if ( M2 && ( IS_SMS2_VDP || IS_GAMEGEAR_VDP ) )
		{
			/* Is it 224-line display */
			if ( M1 && ! M3 )
			{
				smsvdp.y_pixels = 224;
			}
			else if ( ! M1 && M3 )
			{
				/* 240-line display */
				smsvdp.y_pixels = 240;
			}
		}
	}
	else
	{
		/* original TMS9918 mode */
		if ( ! M1 && ! M2 && ! M3 )
		{
			smsvdp.vdp_mode = 0;
		}
		else
//      if ( M1 && ! M2 && ! M3 )
//      {
//          smsvdp.vdp_mode = 1;
//      }
//      else
		if ( ! M1 && M2 && ! M3 )
		{
			smsvdp.vdp_mode = 2;
//      }
//      else
//      if ( ! M1 && ! M2 && M3 )
//      {
//          smsvdp.vdp_mode = 3;
		}
		else
		{
			logerror( "Unknown video mode detected (M1=%c, M2=%c, M3=%c, M4=%c)\n", M1 ? '1' : '0', M2 ? '1' : '0', M3 ? '1' : '0', M4 ? '1' : '0');
		}
	}

	switch( smsvdp.y_pixels )
	{
	case 192:
		smsvdp.sms_frame_timing = ( height == PAL_Y_PIXELS ) ? sms_pal_192 : sms_ntsc_192;
		break;

	case 224:
		smsvdp.sms_frame_timing = ( height == PAL_Y_PIXELS ) ? sms_pal_224 : sms_ntsc_224;
		break;

	case 240:
		smsvdp.sms_frame_timing = ( height == PAL_Y_PIXELS ) ? sms_pal_240 : sms_ntsc_240;
		break;
	}
	smsvdp.cram_dirty = 1;
}


 READ8_HANDLER(sms_vdp_vcount_r)
{
	return ( smsvdp.sms_frame_timing[ INIT_VCOUNT ] + video_screen_get_vpos(space->machine->primary_screen) ) & 0xFF;
}


READ8_HANDLER(sms_vdp_hcount_r)
{
	return video_screen_get_hpos(space->machine->primary_screen) >> 1;
}


void sms_set_ggsmsmode( int mode )
{
	smsvdp.gg_sms_mode = mode;
}


int smsvdp_video_init( running_machine *machine, const smsvdp_configuration *config )
{
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);

	memset( &smsvdp, 0, sizeof smsvdp );

	smsvdp.features = config->model;
	smsvdp.int_callback = config->int_callback;

	/* Allocate video RAM
       In theory the driver could have a "gfx1" and/or "gfx2" memory region
       of it's own. So this code could potentially cause a clash.
    */
	smsvdp.VRAM = memory_region_alloc( machine, "gfx1", VRAM_SIZE, ROM_REQUIRED );
	smsvdp.CRAM = memory_region_alloc( machine, "gfx2", MAX_CRAM_SIZE, ROM_REQUIRED );
	smsvdp.line_buffer = auto_alloc_array(machine, int, 256 * 5);
	memset( smsvdp.line_buffer, 0, 256 * 5 * sizeof(int) );

	/* Clear RAM */
	smsvdp.cram_dirty = 1;
	memset(smsvdp.VRAM, 0, VRAM_SIZE);
	memset(smsvdp.CRAM, 0, MAX_CRAM_SIZE);
	smsvdp.reg[0x02] = 0x0E;			/* power up default */
	smsvdp.reg[0x0a] = 0xff;

	smsvdp.cram_mask = ( IS_GAMEGEAR_VDP && ! smsvdp.gg_sms_mode ) ? ( GG_CRAM_SIZE - 1 ) : ( SMS_CRAM_SIZE - 1 );

	smsvdp.collision_buffer = auto_alloc_array(machine, UINT8, SMS_X_PIXELS);

	/* Make temp bitmap for rendering */
	tmpbitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED32);

	smsvdp.prev_bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED32);

	set_display_settings( machine );

	smsvdp.smsvdp_display_timer = timer_alloc(machine,  smsvdp_display_callback , NULL);
	timer_adjust_periodic(smsvdp.smsvdp_display_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0 ), 0, video_screen_get_scan_period( machine->primary_screen ));
	return 0;
}


static TIMER_CALLBACK(smsvdp_set_irq)
{
	smsvdp.irq_state = 1;
	if ( smsvdp.int_callback )
	{
		smsvdp.int_callback( machine, ASSERT_LINE );
	}
}


static TIMER_CALLBACK(smsvdp_display_callback)
{
	rectangle rec;
	int vpos = video_screen_get_vpos(machine->primary_screen);
	int vpos_limit = smsvdp.sms_frame_timing[VERTICAL_BLANKING] + smsvdp.sms_frame_timing[TOP_BLANKING]
	               + smsvdp.sms_frame_timing[TOP_BORDER] + smsvdp.sms_frame_timing[ACTIVE_DISPLAY_V]
	               + smsvdp.sms_frame_timing[BOTTOM_BORDER] + smsvdp.sms_frame_timing[BOTTOM_BLANKING];

	rec.min_y = rec.max_y = vpos;

	/* Check if we're on the last line of a frame */
	if ( vpos == vpos_limit - 1 )
	{
		sms_check_pause_button( cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM) );
		return;
	}

	vpos_limit -= smsvdp.sms_frame_timing[BOTTOM_BLANKING];

	/* Check if we're below the bottom border */
	if ( vpos >= vpos_limit )
	{
		return;
	}

	vpos_limit -= smsvdp.sms_frame_timing[BOTTOM_BORDER];

	/* Check if we're in the bottom border area */
	if ( vpos >= vpos_limit )
	{
		if ( vpos == vpos_limit )
		{
			if ( smsvdp.line_counter == 0x00 )
			{
				smsvdp.line_counter = smsvdp.reg[0x0A];
				smsvdp.status |= STATUS_HINT;

				if ( smsvdp.reg[0x00] & 0x10 )
				{
					/* Delay triggering of interrupt to allow software to read the status bit before the irq */
					timer_set(machine,  video_screen_get_time_until_pos(machine->primary_screen, video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen) + 1 ), NULL, 0, smsvdp_set_irq );
				}
			}

		}

		if ( vpos == vpos_limit + 1 )
		{
			smsvdp.status |= STATUS_VINT;

			if ( smsvdp.reg[0x01] & 0x20 )
			{
				/* Delay triggering of interrupt to allow software to read the status bit before the irq */
				timer_set(machine,  video_screen_get_time_until_pos(machine->primary_screen, video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen) + 1 ), NULL, 0, smsvdp_set_irq );
			}
		}

		if ( video_skip_this_frame() )
		{
			return;
		}
		sms_update_palette();

		/* Draw left border */
		rec.min_x = LBORDER_START;
		rec.max_x = LBORDER_START + LBORDER_X_PIXELS - 1;
		bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);

		/* Draw right border */
		rec.min_x = LBORDER_START + LBORDER_X_PIXELS + 256;
		rec.max_x = rec.min_x + RBORDER_X_PIXELS - 1;
		bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);

		/* Draw middle of the border */
		/* We need to do this through the regular drawing function so it will */
		/* be included in the gamegear scaling functions */
		sms_refresh_line( machine, tmpbitmap, LBORDER_START + LBORDER_X_PIXELS, vpos_limit - smsvdp.sms_frame_timing[ACTIVE_DISPLAY_V], vpos - ( vpos_limit - smsvdp.sms_frame_timing[ACTIVE_DISPLAY_V] ) );
		return;
	}

	vpos_limit -= smsvdp.sms_frame_timing[ACTIVE_DISPLAY_V];

	/* Check if we're in the active display area */
	if ( vpos >= vpos_limit )
	{
		if ( vpos == vpos_limit )
		{
			smsvdp.line_counter = smsvdp.reg[0x0A];
			smsvdp.reg9copy = smsvdp.reg[0x09];
		}

		if ( smsvdp.line_counter == 0x00 )
		{
			smsvdp.line_counter = smsvdp.reg[0x0A];
			smsvdp.status |= STATUS_HINT;

			if ( smsvdp.reg[0x00] & 0x10 )
			{
				/* Delay triggering of interrupt to allow software to read the status bit before the irq */
				timer_set(machine,  video_screen_get_time_until_pos(machine->primary_screen, video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen) + 1 ), NULL, 0, smsvdp_set_irq );
			}
		}
		else
		{
			smsvdp.line_counter -= 1;
		}

		if ( video_skip_this_frame() )
		{
			return;
		}

		sms_update_palette();

		/* Check if display is disabled */
		if ( ! ( smsvdp.reg[0x01] & 0x40 ) )
		{
			/* set whole line to backdrop color */
			rec.min_x = LBORDER_START;
			rec.max_x = LBORDER_START + LBORDER_X_PIXELS + 255 + RBORDER_X_PIXELS;
			bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);
		}
		else
		{
			/* Draw left border */
			rec.min_x = LBORDER_START;
			rec.max_x = LBORDER_START + LBORDER_X_PIXELS - 1;
			bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);

			/* Draw right border */
			rec.min_x = LBORDER_START + LBORDER_X_PIXELS + 256;
			rec.max_x = rec.min_x + RBORDER_X_PIXELS - 1;
			bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);
			sms_refresh_line( machine, tmpbitmap, LBORDER_START + LBORDER_X_PIXELS, vpos_limit, vpos - vpos_limit );
		}
		return;
	}

	vpos_limit -= smsvdp.sms_frame_timing[TOP_BORDER];

	/* Check if we're in the top border area */
	if ( vpos >= vpos_limit )
	{
		if ( video_skip_this_frame() )
		{
			return;
		}

		sms_update_palette();

		/* Draw left border */
		rec.min_x = LBORDER_START;
		rec.max_x = LBORDER_START + LBORDER_X_PIXELS - 1;
		bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);

		/* Draw right border */
		rec.min_x = LBORDER_START + LBORDER_X_PIXELS + 256;
		rec.max_x = rec.min_x + RBORDER_X_PIXELS - 1;
		bitmap_fill( tmpbitmap, &rec , machine->pens[smsvdp.current_palette[BACKDROP_COLOR]]);

		/* Draw middle of the border */
		/* We need to do this through the regular drawing function so it will */
		/* be included in the gamegear scaling functions */
		sms_refresh_line( machine, tmpbitmap, LBORDER_START + LBORDER_X_PIXELS, vpos_limit + smsvdp.sms_frame_timing[TOP_BORDER], vpos - ( vpos_limit + smsvdp.sms_frame_timing[TOP_BORDER] ) );
		return;
	}
}


 READ8_HANDLER(sms_vdp_data_r)
{
	UINT8 temp;

	/* SMS 2 & GG behaviour. Seems like the latched data is passed straight through */
	/* to the address register when in the middle of doing a command.               */
	/* Cosmic Spacehead needs this, among others                                    */
	/* Clear pending write flag */
	smsvdp.pending = 0;

	/* Return read buffer contents */
	temp = smsvdp.buffer;

	/* Load read buffer */
	smsvdp.buffer = smsvdp.VRAM[(smsvdp.addr & 0x3FFF)];

	/* Bump internal address register */
	smsvdp.addr += 1;
	return (temp);
}


 READ8_HANDLER(sms_vdp_ctrl_r)
{
	UINT8 temp = smsvdp.status;

	/* Clear pending write flag */
	smsvdp.pending = 0;

	smsvdp.status &= ~( STATUS_VINT | STATUS_SPROVR | STATUS_SPRCOL | STATUS_HINT );

	if (smsvdp.irq_state == 1)
	{
		smsvdp.irq_state = 0;

		if ( smsvdp.int_callback )
		{
			smsvdp.int_callback( space->machine, CLEAR_LINE );
		}
	}

	return (temp);
}


WRITE8_HANDLER(sms_vdp_data_w)
{
	/* SMS 2 & GG behaviour. Seems like the latched data is passed straight through */
	/* to the address register when in the middle of doing a command.               */
	/* Cosmic Spacehead needs this, among others                                    */
	/* Clear pending write flag */
	smsvdp.pending = 0;

	switch(smsvdp.addrmode)
	{
		case 0x00:
		case 0x01:
		case 0x02:
			{
				int address = (smsvdp.addr & 0x3FFF);

				smsvdp.VRAM[address] = data;
			}
			break;

		case 0x03:
			{
				int address = smsvdp.addr & smsvdp.cram_mask;

				if (data != smsvdp.CRAM[address])
				{
					smsvdp.CRAM[address] = data;
					smsvdp.cram_dirty = 1;
				}
			}
			break;
	}

	smsvdp.buffer = data;
	smsvdp.addr += 1;
}


WRITE8_HANDLER(sms_vdp_ctrl_w)
{
	int regNum;

	if (smsvdp.pending == 0)
	{
		smsvdp.addr = ( smsvdp.addr & 0xff00 ) | data;
		smsvdp.pending = 1;
	}
	else
	{
		/* Clear pending write flag */
		smsvdp.pending = 0;

		smsvdp.addrmode = (data >> 6) & 0x03;
		smsvdp.addr = ( data << 8 ) | ( smsvdp.addr & 0xff );
		switch( smsvdp.addrmode )
		{
		case 0:		/* VRAM reading mode */
			smsvdp.buffer = smsvdp.VRAM[ smsvdp.addr & 0x3FFF ];
			smsvdp.addr += 1;
			break;

		case 1:		/* VRAM writing mode */
			break;

		case 2:		/* VDP register write */
			regNum = data & 0x0F;
			smsvdp.reg[regNum] = smsvdp.addr & 0xff;
			if (regNum == 0 && smsvdp.addr & 0x02)
			{
				logerror("overscan enabled.\n");
			}
			if ( regNum == 0 || regNum == 1 )
			{
				set_display_settings( space->machine );
			}
			if ( ( regNum == 1 ) && ( smsvdp.reg[0x01] & 0x20 ) && ( smsvdp.status & STATUS_VINT ) )
			{
				smsvdp.irq_state = 1;
				smsvdp.int_callback( space->machine, ASSERT_LINE );
			}
			smsvdp.addrmode = 0;
			break;

		case 3:		/* CRAM writing mode */
			break;
		}
	}
}


static void sms_refresh_line_mode4(int *lineBuffer, int line)
{
	int tileColumn;
	int xScroll, yScroll, xScrollStartColumn;
	int spriteIndex;
	int pixelX, pixelPlotX, prioritySelected[256];
	int spriteX, spriteY, spriteLine, spriteTileSelected, spriteHeight, spriteZoom;
	int spriteBuffer[8], spriteBufferCount, spriteBufferIndex;
	int bitPlane0, bitPlane1, bitPlane2, bitPlane3;
	int scroll_mod;
	UINT16 nameTableAddress;
	UINT8 *nameTable;
	UINT8 *spriteTable = smsvdp.VRAM + ( (smsvdp.reg[0x05] << 7) & 0x3F00 );

	if ( smsvdp.y_pixels != 192 )
	{
		nameTableAddress = ((smsvdp.reg[0x02] & 0x0C) << 10) | 0x0700;
		scroll_mod = 256;
	}
	else
	{
		nameTableAddress = (smsvdp.reg[0x02] << 10) & 0x3800;
		scroll_mod = 224;
	}

	if ( IS_SMS1_VDP )
	{
		nameTableAddress = nameTableAddress & (((smsvdp.reg[0x02] & 0x01) << 10) | 0x3BFF);
	}

	/* if top 2 rows of screen not affected by horizontal scrolling, then xScroll = 0 */
	/* else xScroll = reg[0x08]                                                       */
	xScroll = (((smsvdp.reg[0x00] & 0x40) && (line < 16)) ? 0 : 0x0100 - smsvdp.reg[0x08]);

	xScrollStartColumn = (xScroll >> 3);			 /* x starting column tile */

	/* Draw background layer */
	for (tileColumn = 0; tileColumn < 33; tileColumn++)
	{
		UINT16 tileData;
		int tileSelected, palletteSelected, horizSelected, vertSelected, prioritySelect;
		int tileLine;

		/* Rightmost 8 columns for SMS (or 2 columns for GG) not affected by */
		/* vertical scrolling when bit 7 of reg[0x00] is set */
		yScroll = ((smsvdp.reg[0x00] & 0x80) && (tileColumn > 23)) ? 0 : smsvdp.reg9copy;

		nameTable = smsvdp.VRAM + nameTableAddress + ((((line + yScroll) % scroll_mod) >> 3) << 6);

		tileLine = ( ( tileColumn + xScrollStartColumn ) & 0x1F ) * 2;
		tileData = nameTable[ tileLine ] | ( nameTable[ tileLine + 1 ] << 8 );

		tileSelected = (tileData & 0x01FF);
		prioritySelect = tileData & PRIORITY_BIT;
		palletteSelected = (tileData >> 11) & 0x01;
		vertSelected = (tileData >> 10) & 0x01;
		horizSelected = (tileData >> 9) & 0x01;

		tileLine = line - ((0x07 - (yScroll & 0x07)) + 1);
		if (vertSelected)
		{
			tileLine = 0x07 - tileLine;
		}
		bitPlane0 = smsvdp.VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x00];
		bitPlane1 = smsvdp.VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x01];
		bitPlane2 = smsvdp.VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x02];
		bitPlane3 = smsvdp.VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x03];

		for (pixelX = 0; pixelX < 8 ; pixelX++)
		{
			UINT8 penBit0, penBit1, penBit2, penBit3;
			UINT8 penSelected;

			penBit0 = (bitPlane0 >> (7 - pixelX)) & 0x01;
			penBit1 = (bitPlane1 >> (7 - pixelX)) & 0x01;
			penBit2 = (bitPlane2 >> (7 - pixelX)) & 0x01;
			penBit3 = (bitPlane3 >> (7 - pixelX)) & 0x01;

			penSelected = (penBit3 << 3 | penBit2 << 2 | penBit1 << 1 | penBit0);
			if (palletteSelected)
			{
				penSelected |= 0x10;
			}

			if (!horizSelected)
			{
				pixelPlotX = pixelX;
			}
			else
			{
				pixelPlotX = 7 - pixelX;
			}
			pixelPlotX = (0 - (xScroll & 0x07) + (tileColumn << 3) + pixelPlotX);
			if (pixelPlotX >= 0 && pixelPlotX < 256)
			{
//              logerror("%x %x\n", pixelPlotX + pixelOffsetX, pixelPlotY);
				lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
				prioritySelected[pixelPlotX] = prioritySelect | ( penSelected & 0x0F );
			}
		}
	}

	/* Draw sprite layer */
	spriteHeight = (smsvdp.reg[0x01] & 0x02 ? 16 : 8);
	spriteZoom = 1;
	if (smsvdp.reg[0x01] & 0x01)
	{
		/* sprite doubling */
		spriteZoom = 2;
	}
	spriteBufferCount = 0;
	for (spriteIndex = 0; (spriteIndex < 64) && (spriteTable[spriteIndex] != 0xD0 || smsvdp.y_pixels != 192) && (spriteBufferCount < 9); spriteIndex++)
	{
		spriteY = spriteTable[spriteIndex] + 1; /* sprite y position starts at line 1 */
		if (spriteY > 240)
		{
			spriteY -= 256; /* wrap from top if y position is > 240 */
		}
		if ((line >= spriteY) && (line < (spriteY + spriteHeight * spriteZoom)))
		{
			if (spriteBufferCount < 8)
			{
				spriteBuffer[spriteBufferCount] = spriteIndex;
			}
			else
			{
				/* Too many sprites per line */
				smsvdp.status |= STATUS_SPROVR;
			}
			spriteBufferCount++;
		}
	}
	if ( spriteBufferCount > 8 )
	{
		spriteBufferCount = 8;
	}
	memset(smsvdp.collision_buffer, 0, SMS_X_PIXELS);
	spriteBufferCount--;
	for (spriteBufferIndex = spriteBufferCount; spriteBufferIndex >= 0; spriteBufferIndex--)
	{
		spriteIndex = spriteBuffer[spriteBufferIndex];
		spriteY = spriteTable[spriteIndex] + 1; /* sprite y position starts at line 1 */
		if (spriteY > 240)
		{
			spriteY -= 256; /* wrap from top if y position is > 240 */
		}

		spriteX = spriteTable[0x80 + (spriteIndex << 1)];
		if (smsvdp.reg[0x00] & 0x08)
		{
			spriteX -= 0x08;	 /* sprite shift */
		}
		spriteTileSelected = spriteTable[0x81 + (spriteIndex << 1)];
		if (smsvdp.reg[0x06] & 0x04)
		{
			spriteTileSelected += 256; /* pattern table select */
		}
		if (smsvdp.reg[0x01] & 0x02)
		{
			spriteTileSelected &= 0x01FE; /* force even index */
		}
		spriteLine = ( line - spriteY ) / spriteZoom;

		if (spriteLine > 0x07)
		{
			spriteTileSelected += 1;
		}

		bitPlane0 = smsvdp.VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x00];
		bitPlane1 = smsvdp.VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x01];
		bitPlane2 = smsvdp.VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x02];
		bitPlane3 = smsvdp.VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x03];

		for (pixelX = 0; pixelX < 8 ; pixelX++)
		{
			UINT8 penBit0, penBit1, penBit2, penBit3;
			int penSelected;

			penBit0 = (bitPlane0 >> (7 - pixelX)) & 0x01;
			penBit1 = (bitPlane1 >> (7 - pixelX)) & 0x01;
			penBit2 = (bitPlane2 >> (7 - pixelX)) & 0x01;
			penBit3 = (bitPlane3 >> (7 - pixelX)) & 0x01;

			penSelected = (penBit3 << 3 | penBit2 << 2 | penBit1 << 1 | penBit0) | 0x10;

			if ( penSelected == 0x10 )		/* Transparent palette so skip draw */
			{
				continue;
			}

			if (smsvdp.reg[0x01] & 0x01)
			{
				/* sprite doubling is enabled */
				pixelPlotX = spriteX + (pixelX << 1);

				/* check to prevent going outside of active display area */
				if ( pixelPlotX < 0 || pixelPlotX > 255 )
				{
					continue;
				}

				if ( ! ( prioritySelected[pixelPlotX] & PRIORITY_BIT ) )
				{
					lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
					lineBuffer[pixelPlotX+1] = smsvdp.current_palette[penSelected];
				}
				else
				{
					if ( prioritySelected[pixelPlotX] == PRIORITY_BIT )
					{
						lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
					}
					if ( prioritySelected[pixelPlotX + 1] == PRIORITY_BIT )
					{
						lineBuffer[pixelPlotX+1] = smsvdp.current_palette[penSelected];
					}
				}
				if (smsvdp.collision_buffer[pixelPlotX] != 1)
				{
					smsvdp.collision_buffer[pixelPlotX] = 1;
				}
				else
				{
					/* sprite collision occurred */
					smsvdp.status |= STATUS_SPRCOL;
				}
				if (smsvdp.collision_buffer[pixelPlotX + 1] != 1)
				{
					smsvdp.collision_buffer[pixelPlotX + 1] = 1;
				}
				else
				{
					/* sprite collision occurred */
					smsvdp.status |= STATUS_SPRCOL;
				}
			}
			else
			{
				pixelPlotX = spriteX + pixelX;

				/* check to prevent going outside of active display area */
				if ( pixelPlotX < 0 || pixelPlotX > 255 )
				{
					continue;
				}

				if ( ! ( prioritySelected[pixelPlotX] & PRIORITY_BIT ) )
				{
					lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
				}
				else
				{
					if ( prioritySelected[pixelPlotX] == PRIORITY_BIT )
					{
						lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
					}
				}
				if (smsvdp.collision_buffer[pixelPlotX] != 1)
				{
					smsvdp.collision_buffer[pixelPlotX] = 1;
				}
				else
				{
					/* sprite collision occurred */
					smsvdp.status |= STATUS_SPRCOL;
				}
			}
		}
	}

	/* Fill column 0 with overscan color from reg[0x07] */
	if (smsvdp.reg[0x00] & 0x20)
	{
		lineBuffer[0] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[1] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[2] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[3] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[4] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[5] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[6] = smsvdp.current_palette[BACKDROP_COLOR];
		lineBuffer[7] = smsvdp.current_palette[BACKDROP_COLOR];
	}
}


static void sms_refresh_tms9918_sprites(int *lineBuffer, int line)
{
	int pixelPlotX;
	int spriteHeight, spriteBufferCount, spriteIndex, spriteBuffer[4], spriteBufferIndex;
	UINT8 *spriteTable, *spritePatternTable;

	/* Draw sprite layer */
	spriteTable = smsvdp.VRAM + ( ( smsvdp.reg[0x05] & 0x7F ) << 7 );
	spritePatternTable = smsvdp.VRAM + ( ( smsvdp.reg[0x06] & 0x07 ) << 11 );
	spriteHeight = 8;

	if ( smsvdp.reg[0x01] & 0x02 )                         /* Check if SI is set */
		spriteHeight = spriteHeight * 2;
	if ( smsvdp.reg[0x01] & 0x01 )                         /* Check if MAG is set */
		spriteHeight = spriteHeight * 2;

	spriteBufferCount = 0;
	for ( spriteIndex = 0; (spriteIndex < 32*4 ) && ( spriteTable[spriteIndex] != 0xD0 ) && ( spriteBufferCount < 5); spriteIndex+= 4 )
	{
		int spriteY = spriteTable[spriteIndex] + 1;

		if ( spriteY > 240 )
		{
			spriteY -= 256;
		}

		if ( ( line >= spriteY ) && ( line < ( spriteY + spriteHeight ) ) )
		{
			if ( spriteBufferCount < 5 )
			{
				spriteBuffer[spriteBufferCount] = spriteIndex;
			}
			else
			{
				/* Too many sprites per line */
				smsvdp.status |= STATUS_SPROVR;
			}
			spriteBufferCount++;
		}
	}

	if ( spriteBufferCount > 4 )
	{
		spriteBufferCount = 4;
	}

	memset( smsvdp.collision_buffer, 0, SMS_X_PIXELS );
	spriteBufferCount--;

	for ( spriteBufferIndex = spriteBufferCount; spriteBufferIndex >= 0; spriteBufferIndex-- )
	{
		int penSelected;
		int spriteLine, pixelX, spriteX, spriteTileSelected;
		int spriteY;
		UINT8 pattern;

		spriteIndex = spriteBuffer[ spriteBufferIndex ];
		spriteY = spriteTable[ spriteIndex ] + 1;

		if ( spriteY > 240 )
		{
			spriteY -= 256;
		}

		spriteX = spriteTable[ spriteIndex + 1 ];
		penSelected = spriteTable[ spriteIndex + 3 ] & 0x0F;

		if ( IS_GAMEGEAR_VDP )
		{
			penSelected |= 0x10;
		}

		if ( spriteTable[ spriteIndex + 3 ] & 0x80 )
		{
			spriteX -= 32;
		}

		spriteTileSelected = spriteTable[ spriteIndex + 2 ];
		spriteLine = line - spriteY;

		if ( smsvdp.reg[0x01] & 0x01 )
		{
			spriteLine >>= 1;
		}

		if ( smsvdp.reg[0x01] & 0x02 )
		{
			spriteTileSelected &= 0xFC;

			if ( spriteLine > 0x07 )
			{
				spriteTileSelected += 1;
				spriteLine -= 8;
			}
		}

		pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];

		for ( pixelX = 0; pixelX < 8; pixelX++ )
		{
			if ( smsvdp.reg[0x01] & 0x01 )
			{
				pixelPlotX = spriteX + pixelX * 2;
				if ( pixelPlotX < 0 || pixelPlotX > 255 )
				{
					continue;
				}

				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) )
				{
					lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];

					if ( smsvdp.collision_buffer[pixelPlotX] != 1 )
					{
						smsvdp.collision_buffer[pixelPlotX] = 1;
					}
					else
					{
						smsvdp.status |= STATUS_SPRCOL;
					}

					lineBuffer[pixelPlotX+1] = smsvdp.current_palette[penSelected];

					if ( smsvdp.collision_buffer[pixelPlotX + 1] != 1 )
					{
						smsvdp.collision_buffer[pixelPlotX + 1] = 1;
					}
					else
					{
						smsvdp.status |= STATUS_SPRCOL;
					}
				}
			}
			else
			{
				pixelPlotX = spriteX + pixelX;

				if ( pixelPlotX < 0 || pixelPlotX > 255 )
				{
					continue;
				}

				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) )
				{
					lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];

					if ( smsvdp.collision_buffer[pixelPlotX] != 1 )
					{
						smsvdp.collision_buffer[pixelPlotX] = 1;
					}
					else
					{
						smsvdp.status |= STATUS_SPRCOL;
					}
				}
			}
		}

		if ( smsvdp.reg[0x01] & 0x02 )
		{
			spriteTileSelected += 2;
			pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];
			spriteX += ( smsvdp.reg[0x01] & 0x01 ? 16 : 8 );

			for ( pixelX = 0; pixelX < 8; pixelX++ )
			{
				if ( smsvdp.reg[0x01] & 0x01 )
				{
					pixelPlotX = spriteX + pixelX * 2;

					if ( pixelPlotX < 0 || pixelPlotX > 255 )
					{
						continue;
					}

					if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) )
					{
						lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];

						if ( smsvdp.collision_buffer[pixelPlotX] != 1 )
						{
							smsvdp.collision_buffer[pixelPlotX] = 1;
						}
						else
						{
							smsvdp.status |= STATUS_SPRCOL;
						}

						lineBuffer[pixelPlotX+1] = smsvdp.current_palette[penSelected];

						if ( smsvdp.collision_buffer[pixelPlotX + 1] != 1 )
						{
							smsvdp.collision_buffer[pixelPlotX + 1] = 1;
						}
						else
						{
							smsvdp.status |= STATUS_SPRCOL;
						}
					}
				}
				else
				{
					pixelPlotX = spriteX + pixelX;

					if ( pixelPlotX < 0 || pixelPlotX > 255 )
					{
						continue;
					}

					if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) )
					{
						lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];

						if ( smsvdp.collision_buffer[pixelPlotX] != 1 )
						{
							smsvdp.collision_buffer[pixelPlotX] = 1;
						}
						else
						{
							smsvdp.status |= STATUS_SPRCOL;
						}
					}
				}
			}
		}
	}
}


static void sms_refresh_line_mode2(int *lineBuffer, int line)
{
	int tileColumn;
	int pixelX, pixelPlotX;
	UINT8 *nameTable, *colorTable, *patternTable;
	int patternMask, colorMask, patternOffset;

	/* Draw background layer */
	nameTable = smsvdp.VRAM + ( ( smsvdp.reg[0x02] & 0x0F ) << 10 ) + ( ( line >> 3 ) * 32 );
	colorTable = smsvdp.VRAM + ( ( smsvdp.reg[0x03] & 0x80 ) << 6 );
	colorMask = ( ( smsvdp.reg[0x03] & 0x7F ) << 3 ) | 0x07;
	patternTable = smsvdp.VRAM + ( ( smsvdp.reg[0x04] & 0x04 ) << 11 );
	patternMask = ( ( smsvdp.reg[0x04] & 0x03 ) << 8 ) | 0xFF;
	patternOffset = ( line & 0xC0 ) << 2;

	for ( tileColumn = 0; tileColumn < 32; tileColumn++ )
	{
		UINT8 pattern;
		UINT8 colors;

		pattern = patternTable[ ( ( ( patternOffset + nameTable[tileColumn] ) & patternMask ) * 8 ) + ( line & 0x07 ) ];
		colors = colorTable[ ( ( ( patternOffset + nameTable[tileColumn] ) & colorMask ) * 8 ) + ( line & 0x07 ) ];

		for ( pixelX = 0; pixelX < 8; pixelX++ )
		{
			UINT8 penSelected;

			if ( pattern & ( 1 << ( 7 - pixelX ) ) )
			{
				penSelected = colors >> 4;
			}
			else
			{
				penSelected = colors & 0x0F;
			}

			if ( ! penSelected )
			{
				penSelected = BACKDROP_COLOR;
			}

			pixelPlotX = ( tileColumn << 3 ) + pixelX;

			if ( IS_GAMEGEAR_VDP )
			{
				penSelected |= 0x10;
			}

			lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
		}
	}

	/* Draw sprite layer */
	sms_refresh_tms9918_sprites( lineBuffer, line );
}


static void sms_refresh_line_mode0(int *lineBuffer, int line)
{
	int tileColumn;
	int pixelX, pixelPlotX;
	UINT8 *nameTable, *colorTable, *patternTable;

	/* Draw background layer */
	nameTable = smsvdp.VRAM + ( ( smsvdp.reg[0x02] & 0x0F ) << 10 ) + ( ( line >> 3 ) * 32 );
	colorTable = smsvdp.VRAM + ( ( smsvdp.reg[0x03] << 6 ) & ( VRAM_SIZE - 1 ) );
	patternTable = smsvdp.VRAM + ( ( smsvdp.reg[0x04] << 11 ) & ( VRAM_SIZE - 1 ) );

	for ( tileColumn = 0; tileColumn < 32; tileColumn++ )
	{
		UINT8 pattern;
		UINT8 colors;

		pattern = patternTable[ ( nameTable[tileColumn] * 8 ) + ( line & 0x07 ) ];
		colors = colorTable[ nameTable[tileColumn] >> 3 ];

		for ( pixelX = 0; pixelX < 8; pixelX++ )
		{
			int penSelected;

			if ( pattern & ( 1 << ( 7 - pixelX ) ) )
			{
				penSelected = colors >> 4;
			}
			else
			{
				penSelected = colors & 0x0F;
			}

			if ( IS_GAMEGEAR_VDP )
			{
				penSelected |= 0x10;
			}

			pixelPlotX = ( tileColumn << 3 ) + pixelX;
			lineBuffer[pixelPlotX] = smsvdp.current_palette[penSelected];
		}
	}

	/* Draw sprite layer */
	sms_refresh_tms9918_sprites( lineBuffer, line );
}


static void sms_refresh_line( running_machine *machine, bitmap_t *bitmap, int pixelOffsetX, int pixelPlotY, int line )
{
	int x;
	int *blitLineBuffer = smsvdp.line_buffer;

	if ( line >= 0 && line < smsvdp.sms_frame_timing[ACTIVE_DISPLAY_V] )
	{
		switch( smsvdp.vdp_mode )
		{
		case 0:
			sms_refresh_line_mode0( blitLineBuffer, line );
			break;

		case 2:
			sms_refresh_line_mode2( blitLineBuffer, line );
			break;

		case 4:
		default:
			sms_refresh_line_mode4( blitLineBuffer, line );
			break;
		}
	}
	else
	{
		for ( x = 0; x < 256; x++ )
		{
			blitLineBuffer[x] = smsvdp.current_palette[BACKDROP_COLOR];
		}
	}

	if ( IS_GAMEGEAR_VDP && smsvdp.gg_sms_mode )
	{
		int *combineLineBuffer = smsvdp.line_buffer + ( ( line & 0x03 ) + 1 ) * 256;
		int plotX = 48;

		/* Do horizontal scaling */
		for( x = 8; x < 248; )
		{
			int	combined;

			/* Take red and green from first pixel, and blue from second pixel */
			combined = ( blitLineBuffer[x] & 0x00FF ) | ( blitLineBuffer[x+1] & 0x0F00 );
			combineLineBuffer[plotX] = combined;

			/* Take red from second pixel, and green and blue from third pixel */
			combined = ( blitLineBuffer[x+1] & 0x000F ) | ( blitLineBuffer[x+2] & 0x0FF0 );
			combineLineBuffer[plotX+1] = combined;
			x += 3;
			plotX += 2;
		}

		/* Do vertical scaling for a screen with 192 or 224 lines
           Lines 0-2 and 221-223 have no effect on the output on the GG screen.
           We will calculate the gamegear lines as follows:
           GG_0 = 1/6 * SMS_3 + 1/3 * SMS_4 + 1/3 * SMS_5 + 1/6 * SMS_6
           GG_1 = 1/6 * SMS_4 + 1/3 * SMS_5 + 1/3 * SMS_6 + 1/6 * SMS_7
           GG_2 = 1/6 * SMS_6 + 1/3 * SMS_7 + 1/3 * SMS_8 + 1/6 * SMS_9
           GG_3 = 1/6 * SMS_7 + 1/3 * SMS_8 + 1/3 * SMS_9 + 1/6 * SMS_10
           GG_4 = 1/6 * SMS_9 + 1/3 * SMS_10 + 1/3 * SMS_11 + 1/6 * SMS_12
           .....
           GG_142 = 1/6 * SMS_216 + 1/3 * SMS_217 + 1/3 * SMS_218 + 1/6 * SMS_219
           GG_143 = 1/6 * SMS_217 + 1/3 * SMS_218 + 1/3 * SMS_219 + 1/6 * SMS_220
        */
		{
			int	ggLine;
			int	myLine = pixelPlotY + line - ( TBORDER_START + NTSC_224_TBORDER_Y_PIXELS );
			int	*line1, *line2, *line3, *line4;

			/* First make sure there's enough data to draw anything */
			/* We need one more line of data if we're on line 8, 11, 14, 17, etc */
			if ( myLine < 6 || myLine > 220 || ( ( myLine - 8 ) % 3 == 0 ) )
			{
				return;
			}

			ggLine = ( ( myLine - 6 ) / 3 ) * 2;

			/* If we're on SMS line 7, 10, 13, etc we're on an odd GG line */
			if ( myLine % 3 )
			{
				ggLine++;
			}

			/* Calculate the line we will be drawing on */
			pixelPlotY = TBORDER_START + NTSC_192_TBORDER_Y_PIXELS + 24 + ggLine;

			/* Setup our source lines */
			line1 = smsvdp.line_buffer + ( ( ( myLine - 3 ) & 0x03 ) + 1 ) * 256;
			line2 = smsvdp.line_buffer + ( ( ( myLine - 2 ) & 0x03 ) + 1 ) * 256;
			line3 = smsvdp.line_buffer + ( ( ( myLine - 1 ) & 0x03 ) + 1 ) * 256;
			line4 = smsvdp.line_buffer + ( ( ( myLine - 0 ) & 0x03 ) + 1 ) * 256;

			for( x = 0+48; x < 160+48; x++ )
			{
				rgb_t	c1 = machine->pens[line1[x]];
				rgb_t	c2 = machine->pens[line2[x]];
				rgb_t	c3 = machine->pens[line3[x]];
				rgb_t	c4 = machine->pens[line4[x]];
				*BITMAP_ADDR32( bitmap, pixelPlotY, pixelOffsetX + x) =
					MAKE_RGB( ( RGB_RED(c1)/6 + RGB_RED(c2)/3 + RGB_RED(c3)/3 + RGB_RED(c4)/6 ),
						( RGB_GREEN(c1)/6 + RGB_GREEN(c2)/3 + RGB_GREEN(c3)/3 + RGB_GREEN(c4)/6 ),
						( RGB_BLUE(c1)/6 + RGB_BLUE(c2)/3 + RGB_BLUE(c3)/3 + RGB_BLUE(c4)/6 ) );
			}
			return;
		}
		blitLineBuffer = combineLineBuffer;
	}

	for( x = 0; x < 256; x++ )
	{
		*BITMAP_ADDR32( bitmap, pixelPlotY + line, pixelOffsetX + x) = machine->pens[blitLineBuffer[x]];
	}
}


static void sms_update_palette(void)
{
	int i;

	/* Exit if palette is has no changes */
	if (smsvdp.cram_dirty == 0)
	{
		return;
	}
	smsvdp.cram_dirty = 0;

	if ( smsvdp.vdp_mode != 4 && ! IS_GAMEGEAR_VDP )
	{
		for( i = 0; i < 16; i++ )
		{
			smsvdp.current_palette[i] = 64+i;
		}
		return;
	}

	if ( IS_GAMEGEAR_VDP )
	{
		if ( smsvdp.gg_sms_mode )
		{
			for ( i = 0; i < 32; i++ )
			{
				smsvdp.current_palette[i] = ( ( smsvdp.CRAM[i] & 0x30 ) << 6 ) | ( ( smsvdp.CRAM[i] & 0x0C ) << 4 ) | ( ( smsvdp.CRAM[i] & 0x03 ) << 2 );
			}
		}
		else
		{
			for ( i = 0; i < 32; i++ )
			{
				smsvdp.current_palette[i] = ( ( smsvdp.CRAM[i*2+1] << 8 ) | smsvdp.CRAM[i*2] ) & 0x0FFF;
			}
		}
	}
	else
	{
		for ( i = 0; i < 32; i++ )
		{
			smsvdp.current_palette[i] = smsvdp.CRAM[i] & 0x3F;
		}
	}
}


VIDEO_UPDATE(sms)
{
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);
	int x, y;

	if (smsvdp.prev_bitmap_saved)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				*BITMAP_ADDR32(bitmap, y, x) = (*BITMAP_ADDR32(tmpbitmap, y, x) + *BITMAP_ADDR32(smsvdp.prev_bitmap, y, x)) >> 2;
				logerror("%x %x %x\n", *BITMAP_ADDR32(tmpbitmap, y, x), *BITMAP_ADDR32(smsvdp.prev_bitmap, y, x), (*BITMAP_ADDR32(tmpbitmap, y, x) + *BITMAP_ADDR32(smsvdp.prev_bitmap, y, x)) >> 2);
			}
		}
	}
	else
	{
		copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect);
	}

	if (!smsvdp.prev_bitmap_saved)
	{
		copybitmap(smsvdp.prev_bitmap, tmpbitmap, 0, 0, 0, 0, cliprect);
	//smsvdp.prev_bitmap_saved = 1;
	}

	return 0;
}


