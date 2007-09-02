#include "driver.h"
#include "video/vdc.h"

/* VDC segments */
#define STATE_VSW		0
#define STATE_VDS		1
#define STATE_VDW		2
#define STATE_VCR		3

/* todo: replace this with the PAIR structure from 'osd_cpu.h' */
typedef union
{
#ifdef LSB_FIRST
	struct { unsigned char l,h; } b;
#else
	struct { unsigned char h,l; } b;
#endif
	unsigned short int w;
}pair;

/* the VDC context */

typedef struct
{
	int dvssr_write;			/* Set when the DVSSR register has been written to */
	int physical_width;			/* Width of the display */
	int physical_height;		/* Height of the display */
	UINT8 vce_control;			/* VCE control register */
	pair vce_address;			/* Current address in the palette */
	pair vce_data[512];			/* Palette data */
	UINT16 sprite_ram[64*4];	/* Sprite RAM */
	int curline;				/* the current scanline we're on */
	int current_bitmap_line;	/* the current line in the display we are on */
	int current_segment;		/* current segment of display */
	int current_segment_line;	/* current line inside a segment of display */
	int vblank_triggered;		/* to indicate whether vblank has been triggered */
	int raster_count;			/* counter to compare RCR against */
	UINT8 *vram;
	UINT8   inc;
	UINT8 vdc_register;
	UINT8 vdc_latch;
	pair vdc_data[32];
	int status;
	mame_bitmap *bmp;
	int y_scroll;
}VDC;

static VDC vdc;

/* Function prototypes */

void draw_black_line(int line);
void draw_overscan_line(int line);
void pce_refresh_line(int bitmap_line, int line);
void pce_refresh_sprites(int bitmap_line, int line);
void vdc_do_dma(void);

INTERRUPT_GEN( pce_interrupt )
{
	int ret = 0;

	/* Draw the last scanline */
	if ( vdc.current_bitmap_line >= 14 && vdc.current_bitmap_line < 14 + 242 ) {
		/* We are in the active display area */
		if ( vdc.current_segment == STATE_VDW ) {
			pce_refresh_line( vdc.current_bitmap_line, vdc.current_segment_line );
		} else {
			draw_overscan_line( vdc.current_bitmap_line );
		}
    } else {
		/* We are in one of the blanking areas */
		draw_black_line( vdc.current_bitmap_line );
	}

	/* bump current scanline */
	vdc.current_bitmap_line = ( vdc.current_bitmap_line + 1 ) % VDC_LPF;
	vdc.curline += 1;
	vdc.current_segment_line += 1;
	vdc.raster_count += 1;
	vdc.y_scroll++;

	if ( vdc.current_bitmap_line == 0 ) {
		vdc.current_segment = STATE_VSW;
		vdc.current_segment_line = 0;
		vdc.vblank_triggered = 0;
		vdc.curline = 0;
	}

	if ( STATE_VSW == vdc.current_segment && vdc.current_segment_line >= ( vdc.vdc_data[VPR].b.l & 0x1F ) ) {
		vdc.current_segment = STATE_VDS;
		vdc.current_segment_line = 0;
	}

	if ( STATE_VDS == vdc.current_segment && vdc.current_segment_line >= vdc.vdc_data[VPR].b.h ) {
		vdc.current_segment = STATE_VDW;
		vdc.current_segment_line = 0;
		vdc.raster_count = 0x40;
		vdc.y_scroll = vdc.vdc_data[BYR].w;
	}

	if ( STATE_VDW == vdc.current_segment && vdc.current_segment_line > ( vdc.vdc_data[VDW].w & 0x01FF ) ) {
		vdc.current_segment = STATE_VCR;
		vdc.current_segment_line = 0;

		/* Generate VBlank interrupt, sprite DMA */
		vdc.status |= VDC_VD;
		vdc.vblank_triggered = 1;
		if ( vdc.vdc_data[CR].w & CR_VR )
			ret = 1;

		/* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
		if( ( vdc.vdc_data[DCR].w & DCR_DSR ) || vdc.dvssr_write ) {
			int i;

			vdc.dvssr_write = 0;

			for( i = 0; i < 256; i++ ) {
				vdc.sprite_ram[i] = ( vdc.vram[ ( vdc.vdc_data[DVSSR].w << 1 ) + i * 2 + 1 ] << 8 ) | vdc.vram[ ( vdc.vdc_data[DVSSR].w << 1 ) + i * 2 ];
			}

			vdc.status |= VDC_DS;	/* set satb done flag */

			/* generate interrupt if needed */
			if ( vdc.vdc_data[DCR].w & DCR_DSC )
				ret = 1;
		}
	}

	if ( STATE_VCR == vdc.current_segment ) {
		if ( vdc.current_segment_line >= 3 && vdc.current_segment_line >= vdc.vdc_data[VCR].b.l ) {
			vdc.current_segment = STATE_VSW;
			vdc.current_segment_line = 0;
		}
	}

	/* generate interrupt on line compare if necessary */
	if ( vdc.raster_count == vdc.vdc_data[RCR].w && vdc.vdc_data[CR].w & CR_RC && vdc.vdc_data[RCR].w < 0x0147 ) {
		vdc.status |= VDC_RR;
		ret = 1;
	}

	/* handle frame events */
	if(vdc.curline == 261 && ! vdc.vblank_triggered ) {

		vdc.status |= VDC_VD;	/* set vblank flag */
		vdc.vblank_triggered = 1;
        if(vdc.vdc_data[CR].w & CR_VR)	/* generate IRQ1 if enabled */
			ret = 1;

		/* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
		if ( ( vdc.vdc_data[DCR].w & DCR_DSR ) || vdc.dvssr_write ) {
			int i;

			vdc.dvssr_write = 0;
#ifdef MAME_DEBUG
			assert(((vdc.vdc_data[DVSSR].w<<1) + 512) <= 0x10000);
#endif
			for( i = 0; i < 256; i++ ) {
				vdc.sprite_ram[i] = ( vdc.vram[ ( vdc.vdc_data[DVSSR].w << 1 ) + i * 2 + 1 ] << 8 ) | vdc.vram[ ( vdc.vdc_data[DVSSR].w << 1 ) + i * 2 ];
			}

			vdc.status |= VDC_DS;	/* set satb done flag */

			/* generate interrupt if needed */
			if(vdc.vdc_data[DCR].w & DCR_DSC)
				ret = 1;
		}
	}

	if (ret)
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

VIDEO_START( pce )
{
	logerror("*** pce_vh_start\n");

	/* clear context */
	memset(&vdc, 0, sizeof(vdc));

	/* allocate VRAM */
	vdc.vram = auto_malloc(0x10000);
	memset(vdc.vram, 0, 0x10000);

	/* create display bitmap */
	vdc.bmp = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, machine->screen[0].format);
}


VIDEO_UPDATE( pce )
{
    /* copy our rendering buffer to the display */
    copybitmap (bitmap,vdc.bmp,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
	return 0;
}

void draw_black_line(int line)
{
	int i;

    /* our line buffer */ 
    UINT16 *line_buffer = BITMAP_ADDR16( vdc.bmp, line, 0 );
	
	for( i=0; i< VDC_WPF; i++ )
		line_buffer[i] = get_black_pen( Machine );
}

void draw_overscan_line(int line)
{
	int i;

    /* our line buffer */ 
    UINT16 *line_buffer = BITMAP_ADDR16( vdc.bmp, line, 0 );
	
	for ( i = 0; i < VDC_WPF; i++ )
		line_buffer[i] = Machine->pens[0x100];

}

void vram_write(offs_t offset, UINT8 data)
{
	if(offset & 0x10000)
	{
		logerror("Write to VRAM offset %05X\n", offset);
		return;
	}
	else
	{
		vdc.vram[offset] = data;
	}
}

UINT8 vram_read(offs_t offset)
{
	UINT8 temp;

	if(offset & 0x10000)
	{
		temp = 0x00;
	}
	else
	{
		temp = vdc.vram[offset];
	}

	return temp;
}


WRITE8_HANDLER ( vdc_w )
{
	switch(offset&3)
	{
		case 0x00:	/* VDC register select */
			vdc.vdc_register = (data & 0x1F);
			break;

		case 0x02:	/* VDC data (LSB) */
			vdc.vdc_data[vdc.vdc_register].b.l = data;
			switch(vdc.vdc_register)
			{
				case VxR:	/* LSB of data to write to VRAM */
					vdc.vdc_latch = data;
					break;

			    case BYR:
					vdc.y_scroll=vdc.vdc_data[BYR].w;
					break;

				case HDR:
					vdc.physical_width = ((data & 0x003F) + 1) << 3;
					break;

				case VDW:
					vdc.physical_height &= 0xFF00;
					vdc.physical_height |= (data & 0xFF);
					vdc.physical_height &= 0x01FF;
					break;

				case LENR:
//					logerror("LENR LSB = %02X\n", data);
					break;
				case SOUR:
//					logerror("SOUR LSB = %02X\n", data);
					break;
				case DESR:
//					logerror("DESR LSB = %02X\n", data);
					break;
			}
			break;

		case 0x03:	/* VDC data (MSB) */
			vdc.vdc_data[vdc.vdc_register].b.h = data;
			switch(vdc.vdc_register)
			{
				case VxR:	/* MSB of data to write to VRAM */
					vram_write(vdc.vdc_data[MAWR].w*2+0, vdc.vdc_latch);
					vram_write(vdc.vdc_data[MAWR].w*2+1, data);
					vdc.vdc_data[MAWR].w += vdc.inc;
					break;

				case CR:
					{
						static unsigned char inctab[] = {1, 32, 64, 128};
						vdc.inc = inctab[(data >> 3) & 3];
					}
					break;

				case VDW:
					vdc.physical_height &= 0x00FF;
					vdc.physical_height |= (data << 8);
					vdc.physical_height &= 0x01FF;
					break;

				case DVSSR:
					/* Force VRAM <> SATB DMA for this frame */
					vdc.dvssr_write = 1;
					break;

				case BYR:
					vdc.y_scroll=vdc.vdc_data[BYR].w;
					break;

				case LENR:
					vdc_do_dma();
					break;
				case SOUR:
//					logerror("SOUR MSB = %02X\n", data);
					break;
				case DESR:
//					logerror("DESR MSB = %02X\n", data);
					break;
			}
			break;
	}
}


 READ8_HANDLER ( vdc_r )
{
	int temp = 0;
	switch(offset&3)
	{
		case 0x00:
			temp = vdc.status;
			vdc.status &= ~(VDC_VD | VDC_RR | VDC_CR | VDC_OR | VDC_DS);
			cpunum_set_input_line(0,0,CLEAR_LINE);
			break;

		case 0x02:
			switch(vdc.vdc_register)
			{
				case VxR:
					temp = vram_read(vdc.vdc_data[MARR].w*2+0);
					break;
			}
			break;

		case 0x03:
			switch(vdc.vdc_register)
			{
				case VxR:
					temp = vram_read(vdc.vdc_data[MARR].w*2+1);
					vdc.vdc_data[MARR].w += vdc.inc;
					break;
			}
			break;
	}
	return (temp);
}


 READ8_HANDLER ( vce_r )
{
	int temp = 0xFF;
	switch(offset & 7)
	{
		case 0x04:	/* color table data (LSB) */
			temp = vdc.vce_data[vdc.vce_address.w].b.l;
			break;

		case 0x05:	/* color table data (MSB) */
			temp = vdc.vce_data[vdc.vce_address.w].b.h;
			temp |= 0xFE;
			vdc.vce_address.w = (vdc.vce_address.w + 1) & 0x01FF;
			break;
	}
	return (temp);
}


WRITE8_HANDLER ( vce_w )
{
	switch(offset & 7)
	{
		case 0x00:	/* control reg. */
			vdc.vce_control = data;
			break;

		case 0x02:	/* color table address (LSB) */
			vdc.vce_address.b.l = data;
			vdc.vce_address.w &= 0x1FF;
			break;

		case 0x03:	/* color table address (MSB) */
			vdc.vce_address.b.h = data;
			vdc.vce_address.w &= 0x1FF;
			break;

		case 0x04:	/* color table data (LSB) */
			vdc.vce_data[vdc.vce_address.w].b.l = data;
			/* set color */
			{
				int r, g, b;

				r = ((vdc.vce_data[vdc.vce_address.w].w >> 3) & 7) << 5;
				g = ((vdc.vce_data[vdc.vce_address.w].w >> 6) & 7) << 5;
				b = ((vdc.vce_data[vdc.vce_address.w].w >> 0) & 7) << 5;
				palette_set_color_rgb(Machine, vdc.vce_address.w, r, g, b);
			}
			break;

		case 0x05:	/* color table data (MSB) */
			vdc.vce_data[vdc.vce_address.w].b.h = data;

			/* set color */
			{
				int r, g, b;

				r = ((vdc.vce_data[vdc.vce_address.w].w >> 3) & 7) << 5;
				g = ((vdc.vce_data[vdc.vce_address.w].w >> 6) & 7) << 5;
				b = ((vdc.vce_data[vdc.vce_address.w].w >> 0) & 7) << 5;
				palette_set_color_rgb(Machine, vdc.vce_address.w, r, g, b);
			}

			/* bump internal address */
			vdc.vce_address.w = (vdc.vce_address.w + 1) & 0x01FF;
			break;
    }
}


void pce_refresh_line(int bitmap_line, int line)
{
    static int width_table[4] = {5, 6, 7, 7};

    int scroll_y = ( vdc.y_scroll & 0x01FF);
    int scroll_x = (vdc.vdc_data[BXR].w & 0x03FF);
    int nt_index;

    /* is virtual map 32 or 64 characters tall ? (256 or 512 pixels) */
    int v_line = (scroll_y) & (vdc.vdc_data[MWR].w & 0x0040 ? 0x1FF : 0x0FF);

    /* row within character */
    int v_row = (v_line & 7);

    /* row of characters in BAT */
    int nt_row = (v_line >> 3);

    /* virtual X size (# bits to shift) */
    int v_width =        width_table[(vdc.vdc_data[MWR].w >> 4) & 3];

    /* our line buffer */
    UINT16 *line_buffer = BITMAP_ADDR16( vdc.bmp, bitmap_line, 86 );
#ifdef MAME_DEBUG
	int line_buffer_size;
#endif

    /* pointer to the name table (Background Attribute Table) in VRAM */
    UINT8 *bat = &(vdc.vram[nt_row << (v_width+1)]);

    int b0, b1, b2, b3;
    int i0, i1, i2, i3;
    int cell_pattern_index;
    int cell_palette;
    int x, c, i;

#ifdef MAME_DEBUG
	line_buffer_size = vdc.physical_width + 8;
//	this line no longer funcrions correctly because line_buffer is now "just a pointer"
//	assert(line_buffer_size <= sizeof(line_buffer));
#endif

    /* character blanking bit */
    if(!(vdc.vdc_data[CR].w & CR_BB))
    {
	  draw_black_line(bitmap_line);
    }
	else
	{
		int	pixel = 0;
		int phys_x = - ( scroll_x & 0x07 );

		/* First fill line with overscan colour */
		for ( i = -86; i < VDC_WPF - 86; i++ )
			line_buffer[i] = Machine->pens[0x100];

		for(i=0;i<(vdc.physical_width >> 3) + 1;i++)
		{
			nt_index = (i + (scroll_x >> 3)) & ((2 << (v_width-1))-1);
			nt_index *= 2;

			/* get name table data: */

			/* palette # = index from 0-15 */
			cell_palette = ( bat[nt_index + 1] >> 4 ) & 0x0F;

			/* This is the 'character number', from 0-0x0FFF         */
			/* then it is shifted left 4 bits to form a VRAM address */
			/* and one more bit to convert VRAM word offset to a     */
			/* byte-offset within the VRAM space                     */
			cell_pattern_index = ( ( ( bat[nt_index + 1] << 8 ) | bat[nt_index] ) & 0x0FFF) << 5;

			b0 = vram_read((cell_pattern_index) + (v_row << 1) + 0x00);
			b1 = vram_read((cell_pattern_index) + (v_row << 1) + 0x01);
			b2 = vram_read((cell_pattern_index) + (v_row << 1) + 0x10);
			b3 = vram_read((cell_pattern_index) + (v_row << 1) + 0x11);

			for(x=0;x<8;x++)
			{
				i0 = (b0 >> (7-x)) & 1;
				i1 = (b1 >> (7-x)) & 1;
				i2 = (b2 >> (7-x)) & 1;
				i3 = (b3 >> (7-x)) & 1;
				c = (cell_palette << 4 | i3 << 3 | i2 << 2 | i1 << 1 | i0);

				/* colour #0 always comes from palette #0 */
				if ( ! ( c & 0x0F ) )
					c &= 0x0F;

				if ( phys_x >= 0 && phys_x < vdc.physical_width ) {
					line_buffer[ pixel ] = Machine->pens[c];
					pixel++;
					if ( vdc.physical_width != 512 ) {
						if ( pixel < ( ( ( phys_x + 1 ) * 512 ) / vdc.physical_width ) ) {
							line_buffer[ pixel ] = Machine->pens[c];
							pixel++;
						}
					}
				}
				phys_x += 1;
			}
		}
	}
	/* Sprite rendering is independant of BG drawing! */
	if(vdc.vdc_data[CR].w & CR_SB)
	{
		pce_refresh_sprites(bitmap_line, line);
	}
}



static void conv_obj(int i, int l, int hf, int vf, char *buf)
{
    int b0, b1, b2, b3, i0, i1, i2, i3, x;
    int xi;
	int tmp;


    l &= 0x0F;
    if(vf) l = (15 - l);

	tmp = l + ( i << 5);

    b0 = vram_read((tmp + 0x00)<<1);
    b0 |= vram_read(((tmp + 0x00)<<1)+1)<<8;
	b1 = vram_read((tmp + 0x10)<<1);
	b1 |= vram_read(((tmp + 0x10)<<1)+1)<<8;
    b2 = vram_read((tmp + 0x20)<<1);
    b2 |= vram_read(((tmp + 0x20)<<1)+1)<<8;
	b3 = vram_read((tmp + 0x30)<<1);
	b3 |= vram_read(((tmp + 0x30)<<1)+1)<<8;

    for(x=0;x<16;x++)
    {
        if(hf) xi = x; else xi = (15 - x);
        i0 = (b0 >> xi) & 1;
        i1 = (b1 >> xi) & 1;
        i2 = (b2 >> xi) & 1;
        i3 = (b3 >> xi) & 1;
        buf[x] = (i3 << 3 | i2 << 2 | i1 << 1 | i0);
    }
}

void pce_refresh_sprites(int bitmap_line, int line)
{
    int i;
	UINT8 sprites_drawn=0;
	UINT16 *line_buffer= BITMAP_ADDR16( vdc.bmp, bitmap_line, 86 );
	/* 0 -> no sprite pixels drawn, otherwise is sprite #+1 */
	UINT8 drawn[VDC_WPF];

	/* clear our sprite-to-sprite clipping buffer. */
	memset(drawn, 0, VDC_WPF);
	/* count up: Highest priority is Sprite 0 */ 
	for(i=0; i<64; i++)
	{
		static const int cgy_table[] = {16, 32, 64, 64};

		int obj_y = (vdc.sprite_ram[(i<<2)+0] & 0x03FF) - 64;
		int obj_x = (vdc.sprite_ram[(i<<2)+1] & 0x03FF) - 32;
		int obj_i = (vdc.sprite_ram[(i<<2)+2] & 0x07FE);
		int obj_a = (vdc.sprite_ram[(i<<2)+3]);
		int cgx   = (obj_a >> 8) & 1;   /* sprite width */
		int cgy   = (obj_a >> 12) & 3;  /* sprite height */
		int hf    = (obj_a >> 11) & 1;  /* horizontal flip */
		int vf    = (obj_a >> 15) & 1;  /* vertical flip */
		int palette = (obj_a & 0x000F);
		int priority = (obj_a >> 7) & 1;
		int obj_h = cgy_table[cgy];
		int obj_l = (line - obj_y);
		int cgypos;
		char buf[16];

		if ((obj_y == -64) || (obj_y > line)) continue;
		if ((obj_x == -32) || (obj_x > vdc.physical_width)) continue;

		/* no need to draw an object that's ABOVE where we are. */
		if((obj_y + obj_h)<line) continue;

		/* If CGX is set, bit 0 of sprite pattern index is forced to 0 */
		if ( cgx )
			obj_i &= ~1;

		/* If CGY is set to 1, bit 1 of the sprite pattern index is forced to 0. */
		if ( cgy & 1 )
			obj_i &= ~2;

		/* If CGY is set to 2 or 3, bit 1 and 2 of the sprite pattern index are forced to 0. */
		if ( cgy & 2 )
			obj_i &= ~3;

		sprites_drawn++;
		if(sprites_drawn > 16)
		{
			vdc.status |= VDC_OR;
			if(vdc.vdc_data[CR].w&CR_OV)
				cpunum_set_input_line(0, 0, ASSERT_LINE);
			continue;  /* Should cause an interrupt */ 
		}

		if (obj_l < obj_h)
		{
			cgypos = (obj_l >> 4);
			if(vf) cgypos = ((obj_h - 1) >> 4) - cgypos;

			if(cgx == 0)
			{
				int x;
				int pixel_x = ( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2), obj_l, hf, vf, buf);

				for(x=0;x<16;x++)
				{
					if(((obj_x + x)<(vdc.physical_width))&&((obj_x + x)>=0))
					{
						if ( buf[x] )
						{
							if(!drawn[obj_x+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + buf[x]];
									if ( vdc.physical_width != 512 ) { 
										if ( pixel_x + 1 < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) { 
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + buf[x]];
										}
									}
									drawn[obj_x+x]=i+1;
								}
							}
							else if (drawn[obj_x+x]==1)
							{
								if(vdc.vdc_data[CR].w&CR_CC)
									cpunum_set_input_line(0, 0, ASSERT_LINE);
								vdc.status|=VDC_CR;
							}
						}
					}
					pixel_x += 1;
					if ( vdc.physical_width != 512 ) {
						if ( pixel_x < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) {
							pixel_x += 1;
						}
					}
				}
			}
			else
			{
				int x;
				int pixel_x = ( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2) + (hf ? 2 : 0), obj_l, hf, vf, buf);

				for(x=0;x<16;x++)
				{
					if(((obj_x + x)<(vdc.physical_width))&&((obj_x + x)>=0))
					{
						if ( buf[x] )
						{
							if(!drawn[obj_x+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + buf[x]];
									if ( vdc.physical_width != 512 ) {
										if ( pixel_x + 1 < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) {
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + buf[x]];
										}
									}
									drawn[obj_x + x]=i+1;
								}
							}
							else if (drawn[obj_x+x]==1)
							{
								if(vdc.vdc_data[CR].w&CR_CC)
									cpunum_set_input_line(0, 0, ASSERT_LINE);
								vdc.status|=VDC_CR;


							}
						}
					}
					pixel_x += 1;
					if ( vdc.physical_width != 512 ) {
						if ( pixel_x < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) {
							pixel_x += 1;
						}
					}
				}

				conv_obj(obj_i + (cgypos << 2) + (hf ? 0 : 2), obj_l, hf, vf, buf);
				for(x=0;x<16;x++)
				{
					if(((obj_x + 0x10 + x)<(vdc.physical_width))&&((obj_x + 0x10 + x)>=0))
					{
						if ( buf[x] )
						{
							if(!drawn[obj_x+0x10+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + buf[x]];
									if ( vdc.physical_width != 512 ) {
										if ( pixel_x + 1 < ( ( ( obj_x + x + 17 ) * 512 ) / vdc.physical_width ) ) {
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + buf[x]];
										}
									}                                   
									drawn[obj_x + 0x10 + x]=i+1;
								}
							}
							else if (drawn[obj_x+0x10+x]==1)
							{
								if(vdc.vdc_data[CR].w&CR_CC)
									cpunum_set_input_line(0, 0, ASSERT_LINE);
								vdc.status|=VDC_CR;
							}
						}
					}
					pixel_x += 1;
					if ( vdc.physical_width != 512 ) {
						if ( pixel_x < ( ( ( obj_x + x + 17 ) * 512 ) / vdc.physical_width ) ) {
							pixel_x += 1;
						}
					}
				}
			}
		}
	}
}

void vdc_do_dma(void)
{
	int src = vdc.vdc_data[SOUR].w;
	int dst = vdc.vdc_data[DESR].w;
	int len = vdc.vdc_data[LENR].w;
	
	int did = (vdc.vdc_data[DCR].w >> 3) & 1;
	int sid = (vdc.vdc_data[DCR].w >> 2) & 1;
	int dvc = (vdc.vdc_data[DCR].w >> 1) & 1;
	
	do {
		UINT8 l, h;

		l = vram_read(src<<1);
		h = vram_read((src<<1) + 1);

		vram_write(dst<<1,l);
		vram_write(1+(dst<<1),h);

		if(sid) src = (src - 1) & 0xFFFF;
		else	src = (src + 1) & 0xFFFF;
		
		if(did) dst = (dst - 1) & 0xFFFF;
		else	dst = (dst + 1) & 0xFFFF;
		
		len = (len - 1) & 0xFFFF;
		
	} while (len != 0xFFFF);
	
	vdc.status |= VDC_DV;
	vdc.vdc_data[SOUR].w = src;
	vdc.vdc_data[DESR].w = dst;
	vdc.vdc_data[LENR].w = len;
	if(dvc)
	{
		cpunum_set_input_line(0, 0, ASSERT_LINE);
	}
	
}
