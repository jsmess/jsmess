/***************************************************************************

  video/odyssey2.c

***************************************************************************/

#include "emu.h"
#include "includes/odyssey2.h"


#define COLLISION_SPRITE_0			0x01
#define COLLISION_SPRITE_1			0x02
#define COLLISION_SPRITE_2			0x04
#define COLLISION_SPRITE_3			0x08
#define COLLISION_VERTICAL_GRID		0x10
#define COLLISION_HORIZ_GRID_DOTS	0x20
#define COLLISION_EXTERNAL_UNUSED	0x40
#define COLLISION_CHARACTERS		0x80

/* character sprite colors
   dark grey, red, green, orange, blue, violet, light grey, white
   dark back / grid colors
   black, dark blue, dark green, light green, red, violet, orange, light grey
   light back / grid colors
   black, blue, green, light green, red, violet, orange, light grey */

const UINT8 odyssey2_colors[] =
{
	/* Background,Grid Dim */
	0x00,0x00,0x00,
	0x00,0x00,0xFF,   /* Blue */
	0x00,0x80,0x00,   /* DK Green */
	0xff,0x9b,0x60,
	0xCC,0x00,0x00,   /* Red */
	0xa9,0x80,0xff,
	0x82,0xfd,0xdb,
	0xFF,0xFF,0xFF,

	/* Background,Grid Bright */
	0x80,0x80,0x80,
	0x50,0xAE,0xFF,   /* Blue */
	0x00,0xFF,0x00,   /* Dk Green */
	0x82,0xfb,0xdb,   /* Lt Grey */
	0xEC,0x02,0x60,   /* Red */
	0xa9,0x80,0xff,   /* Violet */
	0xff,0x9b,0x60,   /* Orange */
	0xFF,0xFF,0xFF,

	/* Character,Sprite colors */
	0x71,0x71,0x71,   /* Dark Grey */
	0xFF,0x80,0x80,   /* Red */
	0x00,0xC0,0x00,   /* Green */
	0xff,0x9b,0x60,   /* Orange */
	0x50,0xAE,0xFF,   /* Blue */
	0xa9,0x80,0xff,   /* Violet */
	0x82,0xfb,0xdb,   /* Lt Grey */
	0xff,0xff,0xff    /* White */
};

static const UINT8 o2_shape[0x40][8]={
    { 0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 }, // 0
    { 0x18,0x38,0x18,0x18,0x18,0x18,0x3C,0x00 },
    { 0x3C,0x66,0x0C,0x18,0x30,0x60,0x7E,0x00 },
    { 0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00 },
    { 0xCC,0xCC,0xCC,0xFE,0x0C,0x0C,0x0C,0x00 },
    { 0xFE,0xC0,0xC0,0x7C,0x06,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC0,0xFC,0xC6,0xC6,0x7C,0x00 },
    { 0xFE,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00 },
    { 0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC6,0x7E,0x06,0xC6,0x7C,0x00 },
    { 0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00 },
    { 0x18,0x7E,0x58,0x7E,0x1A,0x7E,0x18,0x00 },
    { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
    { 0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00 },
    { 0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00 },
    { 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 },
    { 0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00 },
    { 0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00 },
    { 0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00 },
    { 0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 },
    { 0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00 },
    { 0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x76,0x00 },
    { 0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00 },
    { 0xFC,0xC6,0xC6,0xC6,0xC6,0xC6,0xFC,0x00 },
    { 0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00 },
    { 0x7C,0xC6,0xC0,0xC0,0xCE,0xC6,0x7E,0x00 },
    { 0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00 },
    { 0x06,0x06,0x06,0x06,0x06,0xC6,0x7C,0x00 },
    { 0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00 },
    { 0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00 },
    { 0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00 },
    { 0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00 },
    { 0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00 },
    { 0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00 },
    { 0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00 },
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 },
    { 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 },
    { 0x00,0x66,0x3C,0x18,0x3C,0x66,0x00,0x00 },
    { 0x00,0x18,0x00,0x7E,0x00,0x18,0x00,0x00 },
    { 0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00 },
    { 0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 },
    { 0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00 },
    { 0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00 },
    { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00 },
    { 0xCE,0xDB,0xDB,0xDB,0xDB,0xDB,0xCE,0x00 },
    { 0x00,0x00,0x3C,0x7E,0x7E,0x7E,0x3C,0x00 },
    { 0x1C,0x1C,0x18,0x1E,0x18,0x18,0x1C,0x00 },
    { 0x1C,0x1C,0x18,0x1E,0x18,0x34,0x26,0x00 },
    { 0x38,0x38,0x18,0x78,0x18,0x2C,0x64,0x00 },
    { 0x38,0x38,0x18,0x78,0x18,0x18,0x38,0x00 },
    { 0x00,0x18,0x0C,0xFE,0x0C,0x18,0x00,0x00 },
    { 0x18,0x3C,0x7E,0xFF,0xFF,0x18,0x18,0x00 },
    { 0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF,0x00 },
    { 0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF,0x00 },
    { 0x38,0x38,0x12,0xFE,0xB8,0x28,0x6C,0x00 },
    { 0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,0x00 },
    { 0x00,0x00,0x0C,0x08,0x08,0x7F,0x3E,0x00 },
    { 0x00,0x03,0x63,0xFF,0xFF,0x18,0x08,0x00 },
    { 0x00,0x00,0x00,0x10,0x38,0xFF,0x7E,0x00 }
};

static union {
    UINT8 reg[0x100];
    struct {
	struct {
	    UINT8 y,x,color,res;
	} sprites[4];
	struct {
	    UINT8 y,x,ptr,color;
	} foreground[12];
	struct {
	    struct {
		UINT8 y,x,ptr,color;
	    } single[4];
	} quad[4];
	UINT8 shape[4][8];
	UINT8 control;
	UINT8 status;
	UINT8 collision;
	UINT8 color;
	UINT8 y;
	UINT8 x;
	UINT8 res;
	UINT8 shift1,shift2,shift3;
	UINT8 sound;
	UINT8 res2[5+0x10];
	UINT8 hgrid[2][0x10];
	UINT8 vgrid[0x10];
    } s;
} o2_vdc= { { 0 } };

static UINT32		o2_snd_shift[2];
static UINT8		x_beam_pos;
static UINT8		y_beam_pos;
static UINT8		control_status;
static UINT8		collision_status;
static int			iff;
static emu_timer	*i824x_line_timer;
static emu_timer	*i824x_hblank_timer;
static bitmap_t		*tmp_bitmap;
static int			start_vpos;
static int			start_vblank;
static UINT8		lum;		/* Output of P1 bit 7 influences the intensity of the background and grid colours */
static UINT16		lfsr;

static sound_stream *odyssey2_sh_channel;


PALETTE_INIT( odyssey2 )
{
	int i;

	for ( i = 0; i < 24; i++ )
	{
		palette_set_color_rgb( machine, i, odyssey2_colors[i*3], odyssey2_colors[i*3+1], odyssey2_colors[i*3+2] );
	}
}

READ8_HANDLER( odyssey2_video_r )
{
    UINT8 data = 0;

    switch (offset)
    {
        case 0xa1:
			data = control_status;
			iff = 0;
			cputag_set_input_line(space->machine, "maincpu", 0, CLEAR_LINE);
			control_status &= ~ 0x08;
			if ( space->machine->primary_screen->hpos() < I824X_START_ACTIVE_SCAN || space->machine->primary_screen->hpos() > I824X_END_ACTIVE_SCAN )
			{
				data |= 1;
			}

            break;

        case 0xa2:
			data = collision_status;
			collision_status = 0;

            break;

        case 0xa4:

            if ((o2_vdc.s.control & VDC_CONTROL_REG_STROBE_XY))
                y_beam_pos = space->machine->primary_screen->vpos() - start_vpos;

            data = y_beam_pos;

            break;


        case 0xa5:

            if ((o2_vdc.s.control & VDC_CONTROL_REG_STROBE_XY))
			{
                x_beam_pos = space->machine->primary_screen->hpos();
				if ( x_beam_pos < I824X_START_ACTIVE_SCAN )
				{
					x_beam_pos = x_beam_pos - I824X_START_ACTIVE_SCAN + I824X_LINE_CLOCKS;
				}
				else
				{
					x_beam_pos = x_beam_pos - I824X_START_ACTIVE_SCAN;
				}
			}

            data = x_beam_pos;

            break;

        default:
            data = o2_vdc.reg[offset];
    }

    return data;
}

WRITE8_HANDLER( odyssey2_video_w )
{
	/* Update the sound */
	if( offset >= 0xa7 && offset <= 0xaa )
		stream_update( odyssey2_sh_channel );

    if (offset == 0xa0)
    {
        if (    o2_vdc.s.control & VDC_CONTROL_REG_STROBE_XY
             && !(data & VDC_CONTROL_REG_STROBE_XY))
        {
            /* Toggling strobe bit, tuck away values */
            x_beam_pos = space->machine->primary_screen->hpos();
			if ( x_beam_pos < I824X_START_ACTIVE_SCAN )
			{
				x_beam_pos = x_beam_pos - I824X_START_ACTIVE_SCAN + 228;
			}
			else
			{
				x_beam_pos = x_beam_pos - I824X_START_ACTIVE_SCAN;
			}

            y_beam_pos = space->machine->primary_screen->vpos() - start_vpos;

            /* This is wrong but more games work with it, TODO: Figure
             * out correct change.  Maybe update the screen here??
             * It seems what happens is 0x0 is written to $A0 just before
             * VLBANK (video update) so no screen updates happen.
             */

            return;
        }
    }

    o2_vdc.reg[offset] = data;
}

WRITE8_HANDLER ( odyssey2_lum_w )
{
	lum = data;
}

READ8_HANDLER( odyssey2_t1_r )
{
	if ( space->machine->primary_screen->vpos() > start_vpos && space->machine->primary_screen->vpos() < start_vblank )
	{
		if ( space->machine->primary_screen->hpos() >= I824X_START_ACTIVE_SCAN && space->machine->primary_screen->hpos() < I824X_END_ACTIVE_SCAN )
		{
			return 1;
		}
	}
	return 0;
}

static TIMER_CALLBACK( i824x_scanline_callback )
{
	UINT8	collision_map[160];
	int		vpos = machine->primary_screen->vpos();

	if ( vpos < start_vpos )
		return;

	if ( vpos == start_vpos )
	{
		control_status &= ~0x08;
	}

	if ( vpos < start_vblank )
	{
		rectangle rect;
		//int   sprite_width[4] = { 8, 8, 8, 8 };
		int i;

		control_status &= ~ 0x01;

		/* Draw a line */
		rect.min_y = rect.max_y = vpos;
		rect.min_x = I824X_START_ACTIVE_SCAN;
		rect.max_x = I824X_END_ACTIVE_SCAN - 1;
		bitmap_fill( tmp_bitmap, &rect , ( (o2_vdc.s.color >> 3) & 0x7 ) | ( ( lum << 3 ) ^ 0x08 ));

		/* Clear collision map */
		memset( collision_map, 0, sizeof( collision_map ) );

		/* Display grid if enabled */
		if ( o2_vdc.s.control & 0x08 )
		{
			UINT16	color = ( o2_vdc.s.color & 7 ) | ( ( o2_vdc.s.color >> 3 ) & 0x08 ) | ( ( lum << 3 ) ^ 0x08 );
			int		x_grid_offset = 8;
			int 	y_grid_offset = 24;
			int		width = 16;
			int		height = 24;
			int		w = ( o2_vdc.s.control & 0x80 ) ? width : 2;
			int		j, k, y;

			/* Draw horizontal part of grid */
			for ( j = 1, y = 0; y < 9; y++, j <<= 1 )
			{
				if ( y_grid_offset + y * height <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y_grid_offset + y * height + 3 )
				{
					for ( i = 0; i < 9; i++ )
					{
						if ( ( o2_vdc.s.hgrid[0][i] & j ) || ( o2_vdc.s.hgrid[1][i] & ( j >> 8 ) ) )
						{
							for ( k = 0; k < width + 2; k++ )
							{
								int px = x_grid_offset + i * width + k;
								collision_map[ px ] |= COLLISION_HORIZ_GRID_DOTS;
								*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + px ) = color;
							}
						}
					}
				}
			}

			/* Draw vertical part of grid */
			for( j = 1, y = 0; y < 8; y++, j <<= 1 )
			{
				if ( y_grid_offset + y * height <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y_grid_offset + ( y + 1 ) * height )
				{
					for ( i = 0; i < 10; i++ )
					{
						if ( o2_vdc.s.vgrid[i] & j )
						{
							for ( k = 0; k < w; k++ )
							{
								int px = x_grid_offset + i * width + k;

								/* Check if we collide with an already drawn source object */
								if ( collision_map[ px ] & o2_vdc.s.collision )
								{
									collision_status |= COLLISION_VERTICAL_GRID;
								}
								/* Check if an already drawn object would collide with us */
								if ( COLLISION_VERTICAL_GRID & o2_vdc.s.collision && collision_map[ px ] )
								{
									collision_status |= collision_map[ px ];
								}
								collision_map[ px ] |= COLLISION_VERTICAL_GRID;
								*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + px ) = color;
							}
						}
					}
				}
			}
		}

		/* Display objects if enabled */
		if ( o2_vdc.s.control & 0x20 )
		{
			/* Regular foreground objects */
			for ( i = 0; i < ARRAY_LENGTH( o2_vdc.s.foreground ); i++ )
			{
				int	y = o2_vdc.s.foreground[i].y;
				int	height = 8 - ( ( ( y >> 1 ) + o2_vdc.s.foreground[i].ptr ) & 7 );

				if ( y <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y + height * 2 )
				{
					UINT16	color = 16 + ( ( o2_vdc.s.foreground[i].color & 0x0E ) >> 1 );
					int		offset = ( o2_vdc.s.foreground[i].ptr | ( ( o2_vdc.s.foreground[i].color & 0x01 ) << 8 ) ) + ( y >> 1 ) + ( ( vpos - start_vpos - y ) >> 1 );
					UINT8	chr = ((char*)o2_shape)[ offset & 0x1FF ];
					int		x = o2_vdc.s.foreground[i].x;
					UINT8	m;

					for ( m = 0x80; m > 0; m >>= 1, x++ )
					{
						if ( chr & m )
						{
							if ( x >= 0 && x < 160 )
							{
								/* Check if we collide with an already drawn source object */
								if ( collision_map[ x ] & o2_vdc.s.collision )
								{
									collision_status |= COLLISION_CHARACTERS;
								}
								/* Check if an already drawn object would collide with us */
								if ( COLLISION_CHARACTERS & o2_vdc.s.collision && collision_map[ x ] )
								{
									collision_status |= collision_map[ x ];
								}
								collision_map[ x ] |= COLLISION_CHARACTERS;
								*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + x ) = color;
							}
						}
					}
				}
			}

			/* Quad objects */
			for ( i = 0; i < ARRAY_LENGTH( o2_vdc.s.quad ); i++ )
			{
				int y = o2_vdc.s.quad[i].single[0].y;
				int height = 8;

				if ( y <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y + height * 2 )
				{
					int	x = o2_vdc.s.quad[i].single[0].x;
					int j;

					for ( j = 0; j < ARRAY_LENGTH( o2_vdc.s.quad[0].single ); j++, x += 8 )
					{
						int		char_height = 8 - ( ( ( y >> 1 ) + o2_vdc.s.quad[i].single[j].ptr ) & 7 );
						if ( y <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y + char_height * 2 )
						{
							UINT16 color = 16 + ( ( o2_vdc.s.quad[i].single[j].color & 0x0E ) >> 1 );
							int	offset = ( o2_vdc.s.quad[i].single[j].ptr | ( ( o2_vdc.s.quad[i].single[j].color & 0x01 ) << 8 ) ) + ( y >> 1 ) + ( ( vpos - start_vpos - y ) >> 1 );
							UINT8	chr = ((char*)o2_shape)[ offset & 0x1FF ];
							UINT8	m;
							for ( m = 0x80; m > 0; m >>= 1, x++ )
							{
								if ( chr & m )
								{
									if ( x >= 0 && x < 160 )
									{
										/* Check if we collide with an already drawn source object */
										if ( collision_map[ x ] & o2_vdc.s.collision )
										{
											collision_status |= COLLISION_CHARACTERS;
										}
										/* Check if an already drawn object would collide with us */
										if ( COLLISION_CHARACTERS & o2_vdc.s.collision && collision_map[ x ] )
										{
											collision_status |= collision_map[ x ];
										}
										collision_map[ x ] |= COLLISION_CHARACTERS;
										*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + x ) = color;
									}
								}
							}
						}
						else
						{
							x += 8;
						}
					}
				}
			}

			/* Sprites */
			for ( i = 0; i < ARRAY_LENGTH( o2_vdc.s.sprites ); i++ )
			{
				int y = o2_vdc.s.sprites[i].y;
				int height = 8;
				if ( o2_vdc.s.sprites[i].color & 4 )
				{
					/* Zoomed sprite */
					//sprite_width[i] = 16;
					if ( y <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y + height * 4 )
					{
						UINT16 color = 16 + ( ( o2_vdc.s.sprites[i].color >> 3 ) & 0x07 );
						UINT8	chr = o2_vdc.s.shape[i][ ( ( vpos - start_vpos - y ) >> 2 ) ];
						int		x = o2_vdc.s.sprites[i].x;
						UINT8	m;

						for ( m = 0x01; m > 0; m <<= 1, x += 2 )
						{
							if ( chr & m )
							{
								if ( x >= 0 && x < 160 )
								{
									/* Check if we collide with an already drawn source object */
									if ( collision_map[ x ] & o2_vdc.s.collision )
									{
										collision_status |= ( 1 << i );
									}
									/* Check if an already drawn object would collide with us */
									if ( ( 1 << i ) & o2_vdc.s.collision && collision_map[ x ] )
									{
										collision_status |= collision_map[ x ];
									}
									collision_map[ x ] |= ( 1 << i );
									*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + x ) = color;
								}
								if ( x >= -1 && x < 159 )
								{
									/* Check if we collide with an already drawn source object */
									if ( collision_map[ x ] & o2_vdc.s.collision )
									{
										collision_status |= ( 1 << i );
									}
									/* Check if an already drawn object would collide with us */
									if ( ( 1 << i ) & o2_vdc.s.collision && collision_map[ x ] )
									{
										collision_status |= collision_map[ x ];
									}
									collision_map[ x ] |= ( 1 << i );
									*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + x + 1 ) = color;
								}
							}
						}
					}
				}
				else
				{
					/* Regular sprite */
					if ( y <= ( vpos - start_vpos ) && ( vpos - start_vpos ) < y + height * 2 )
					{
						UINT16 color = 16 + ( ( o2_vdc.s.sprites[i].color >> 3 ) & 0x07 );
						UINT8	chr = o2_vdc.s.shape[i][ ( ( vpos - start_vpos - y ) >> 1 ) ];
						int		x = o2_vdc.s.sprites[i].x;
						UINT8	m;

						for ( m = 0x01; m > 0; m <<= 1, x++ )
						{
							if ( chr & m )
							{
								if ( x >= 0 && x < 160 )
								{
									/* Check if we collide with an already drawn source object */
									if ( collision_map[ x ] & o2_vdc.s.collision )
									{
										collision_status |= ( 1 << i );
									}
									/* Check if an already drawn object would collide with us */
									if ( ( 1 << i ) & o2_vdc.s.collision && collision_map[ x ] )
									{
										collision_status |= collision_map[ x ];
									}
									collision_map[ x ] |= ( 1 << i );
									*BITMAP_ADDR16( tmp_bitmap, vpos, I824X_START_ACTIVE_SCAN + x ) = color;
								}
							}
						}
					}
				}
			}
		}
	}

	/* Check for start of VBlank */
	if ( vpos == start_vblank )
	{
		control_status |= 0x08;
		if ( ! iff )
		{
			cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
			iff = 1;
		}
	}
}

static TIMER_CALLBACK( i824x_hblank_callback )
{
	int vpos = machine->primary_screen->vpos();

	if ( vpos < start_vpos - 1 )
		return;

	if ( vpos < start_vblank - 1 )
	{
		control_status |= 0x01;
	}
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( odyssey2 )
{
	screen_device *screen = screen_first(*machine);
	int width = screen->width();
	int height = screen->height();

	memset(o2_vdc.reg, 0, 0x100);

	o2_snd_shift[0] = o2_snd_shift[1] = 0;
	x_beam_pos = 0;
	y_beam_pos = 0;
	control_status = 0;
	collision_status = 0;
	iff = 0;
	start_vpos = 0;
	start_vblank = 0;
	lum = 0;
	lfsr = 0;

	o2_snd_shift[0] = machine->sample_rate / 983;
	o2_snd_shift[1] = machine->sample_rate / 3933;

	start_vpos = I824X_START_Y;
	start_vblank = I824X_START_Y + I824X_SCREEN_HEIGHT;
	control_status = 0;
	iff = 0;

	tmp_bitmap = auto_bitmap_alloc( machine, width, height, screen->format() );

	i824x_line_timer = timer_alloc(machine,  i824x_scanline_callback, NULL );
	timer_adjust_periodic( i824x_line_timer, machine->primary_screen->time_until_pos(1, I824X_START_ACTIVE_SCAN ), 0,  machine->primary_screen->scan_period() );

	i824x_hblank_timer = timer_alloc(machine,  i824x_hblank_callback, NULL );
	timer_adjust_periodic( i824x_hblank_timer, machine->primary_screen->time_until_pos(1, I824X_END_ACTIVE_SCAN + 18 ), 0, machine->primary_screen->scan_period() );
}

/***************************************************************************

  Refresh the video screen

***************************************************************************/

VIDEO_UPDATE( odyssey2 )
{
	copybitmap( bitmap, tmp_bitmap, 0, 0, 0, 0, cliprect );

	return 0;
}

static DEVICE_START( odyssey2_sound )
{
	odyssey2_sh_channel = stream_create(device, 0, 1, device->clock()/(I824X_LINE_CLOCKS*4), 0, odyssey2_sh_update );
}


DEVICE_GET_INFO( odyssey2_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(odyssey2_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "P8244/P8245");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}

STREAM_UPDATE( odyssey2_sh_update )
{
	static UINT32 signal;
	static UINT16 count = 0;
	int ii;
	int period;
	stream_sample_t *buffer = outputs[0];

	/* Generate the signal */
	signal = o2_vdc.s.shift3 | (o2_vdc.s.shift2 << 8) | (o2_vdc.s.shift1 << 16);

	if( o2_vdc.s.sound & 0x80 )	/* Sound is enabled */
	{
		for( ii = 0; ii < samples; ii++, buffer++ )
		{
			*buffer = 0;
			*buffer = signal & 0x1;
			period = (o2_vdc.s.sound & 0x20) ? 1 : 4;
			if( ++count >= period )
			{
				count = 0;
				signal >>= 1;
				/* Loop sound */
				if( o2_vdc.s.sound & 0x40 )
				{
					signal |= *buffer << 23;
				}
				/* Noise poly is : L=16 W=2 10000000000100001 */
				lfsr = ( lfsr >> 1 ) | ( ( ( lfsr & 0x01 ) ^ ( ( lfsr & 0x800 ) >> 11 ) ) << 15 );
				if ( ! lfsr )
				{
					lfsr = 0xFFFF;
				}
				/* Check if noise should be applied */
				if ( o2_vdc.s.sound & 0x10 )
				{
					*buffer |= ( lfsr & 0x01 );
				}
				o2_vdc.s.shift3 = signal & 0xFF;
				o2_vdc.s.shift2 = ( signal >> 8 ) & 0xFF;
				o2_vdc.s.shift1 = ( signal >> 16 ) & 0xFF;
			}

			/* Throw an interrupt if enabled */
			if( o2_vdc.s.control & 0x4 )
			{
				cputag_set_input_line(device->machine, "maincpu", 1, HOLD_LINE); /* Is this right? */
			}

			/* Adjust volume */
			*buffer *= o2_vdc.s.sound & 0xf;
			/* Pump the volume up */
			*buffer <<= 10;
		}
	}
	else
	{
		/* Sound disabled, so clear the buffer */
		for( ii = 0; ii < samples; ii++, buffer++ )
			*buffer = 0;
	}
}

/*
    Thomson EF9340/EF9341 extra chips in the g7400
 */

#ifdef UNUSED_FUNCTION
static struct {
	UINT8	X;
	UINT8	Y;
	UINT8	Y0;
	UINT8	R;
	UINT8	M;
	UINT8	TA;
	UINT8	TB;
	UINT8	busy;
	UINT8	ram[1024];
} ef9341;

INLINE UINT16 ef9341_get_c_addr( void )
{
	if ( ( ef9341.Y & 0x0C ) == 0x0C )
	{
		return 0x318 | ( ( ef9341.X & 0x38 ) << 2 ) | ( ef9341.X & 0x07 );
	}
	if ( ef9341.X & 0x20 )
	{
		return 0x300 | ( ( ef9341.Y & 0x07 ) << 5 ) | ( ef9341.Y & 0x18 ) | ( ef9341.X & 0x07 );
	}
	return ( ef9341.Y << 5 ) | ef9341.X;
}

INLINE void ef9341_inc_c( void )
{
	ef9341.X++;
	if ( ef9341.X >= 40 )
	{
		ef9341.Y = ( ef9341.Y + 1 ) % 24;
	}
}

void odyssey2_ef9341_w( int command, int b, UINT8 data )
{
	logerror("ef9341 %s write, t%s, data %02X\n", command ? "command" : "data", b ? "B" : "A", data );

	if ( command )
	{
		if ( b )
		{
			ef9341.TB = data;
			ef9341.busy = 0x80;
			switch( ef9341.TB & 0xE0 )
			{
			case 0x00:	/* Begin row */
				ef9341.X = 0;
				ef9341.Y = ef9341.TA & 0x1F;
				break;
			case 0x20:	/* Load Y */
				ef9341.Y = ef9341.TA & 0x1F;
				break;
			case 0x40:	/* Load X */
				ef9341.X = ef9341.TA & 0x3F;
				break;
			case 0x60:	/* INC C */
				ef9341_inc_c();
				break;
			case 0x80:	/* Load M */
				ef9341.M = ef9341.TA;
				break;
			case 0xA0:	/* Load R */
				ef9341.R = ef9341.TA;
				break;
			case 0xC0:	/* Load Y0 */
				ef9341.Y0 = ef9341.TA & 0x3F;
				break;
			}
			ef9341.busy = 0;
		}
		else
		{
			ef9341.TA = data;
		}
	}
	else
	{
		if ( b )
		{
			ef9341.TB = data;
			ef9341.busy = 0x80;
			switch ( ef9341.M & 0xE0 )
			{
			case 0x00:	/* Write */
				ef9341.ram[ ef9341_get_c_addr() ] = ef9341.TB;
				ef9341_inc_c();
				break;
			case 0x20:	/* Read */
				logerror("ef9341 unimplemented data action %02X\n", ef9341.M & 0xE0 );
				ef9341_inc_c();
				break;
			case 0x40:	/* Write without increment */
			case 0x60:	/* Read without increment */
			case 0x80:	/* Write slice */
			case 0xA0:	/* Read slice */
				logerror("ef9341 unimplemented data action %02X\n", ef9341.M & 0xE0 );
				break;
			}
			ef9341.busy = 0;
		}
		else
		{
			ef9341.TA = data;
		}
	}
}

UINT8 odyssey2_ef9341_r( int command, int b )
{
	UINT8	data = 0xFF;

	logerror("ef9341 %s read, t%s\n", command ? "command" : "data", b ? "B" : "A" );
	if ( command )
	{
		if ( b )
		{
			data = 0xFF;
		}
		else
		{
			data = ef9341.busy;
		}
	}
	else
	{
		if ( b )
		{
			data = ef9341.TB;
			ef9341.busy = 0x80;
		}
		else
		{
			data = ef9341.TA;
		}
	}
	return data;
}
#endif

DEFINE_LEGACY_SOUND_DEVICE(ODYSSEY2, odyssey2_sound);
