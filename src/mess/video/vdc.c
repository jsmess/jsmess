#include <assert.h>

#include "driver.h"
#include "video/generic.h"
#include "video/vdc.h"

/* Our VDP context */

VDC vdc;

/* Function prototypes */

void pce_refresh_sprites(int bitmap_line, int line);
void vdc_do_dma(void);

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
/* don't know which pen is really black, so it currently uses BG pen 0*/
	int i;

    /* our line buffer */ 
    UINT16 *line_buffer = BITMAP_ADDR16( vdc.bmp, line, 0 );
	
	for( i=0; i< Machine->screen[0].width; i++ )
		line_buffer[i] = Machine->pens[0];
}

void draw_overscan_line(int line)
{
	int i;

    /* our line buffer */ 
    UINT16 *line_buffer = BITMAP_ADDR16( vdc.bmp, line, 0 );
	
	for( i=0; i< Machine->screen[0].width; i++ )
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
        case 0x00: /* VDC register select */
             vdc.vdc_register = (data & 0x1F);
             break;

        case 0x02: /* VDC data (LSB) */
             vdc.vdc_data[vdc.vdc_register].b.l = data;
             switch(vdc.vdc_register)
             {
                case VxR: /* LSB of data to write to VRAM */
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
//                   logerror("LENR LSB = %02X\n", data);
                     break;
                case SOUR:
//                   logerror("SOUR LSB = %02X\n", data);
                     break;
                case DESR:
//                   logerror("DESR LSB = %02X\n", data);
                     break;
             }
             break;

        case 0x03: /* VDC data (MSB) */
             vdc.vdc_data[vdc.vdc_register].b.h = data;
             switch(vdc.vdc_register)
             {
                case VxR: /* MSB of data to write to VRAM */
                     //vdc.vdc_data[MAWR].w &= 0x7FFF;
                     vram_write(vdc.vdc_data[MAWR].w*2+0, vdc.vdc_latch);
                     vram_write(vdc.vdc_data[MAWR].w*2+1, data);
                     vdc.vdc_data[MAWR].w += vdc.inc;
                     //vdc.vdc_data[MAWR].w &= 0x7FFF;
                     //vdc.vdc_latch = 0;
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
//                   logerror("SOUR MSB = %02X\n", data);
                     break;
                case DESR:
//                   logerror("DESR MSB = %02X\n", data);
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
    int temp = 0;
    switch(offset & 7)
    {
        case 0x04: /* color table data (LSB) */
             temp = vdc.vce_data[vdc.vce_address.w].b.l;
             break;

        case 0x05: /* color table data (MSB) */
             temp = vdc.vce_data[vdc.vce_address.w].b.h;
             vdc.vce_address.w = (vdc.vce_address.w + 1) & 0x01FF;
             break;
    }
    return (temp);
}


WRITE8_HANDLER ( vce_w )
{
    switch(offset & 7)
    {
        case 0x00: /* control reg. */
             break;

        case 0x02: /* color table address (LSB) */
             vdc.vce_address.b.l = data;
             vdc.vce_address.w &= 0x1FF;
             break;

        case 0x03: /* color table address (MSB) */
             vdc.vce_address.b.h = data;
             vdc.vce_address.w &= 0x1FF;
             break;

        case 0x04: /* color table data (LSB) */
             vdc.vce_data[vdc.vce_address.w].b.l = data;
             break;

        case 0x05: /* color table data (MSB) */
             vdc.vce_data[vdc.vce_address.w].b.h = data;

             /* set color */
             {
                int /*c,*/ i, r, g, b;
                /*pair f;*/

                /* mirror entry 0 of palette 0 */
                for(i=1;i<32;i++)
                {
                    vdc.vce_data[i << 4].w = vdc.vce_data[0].w;
                }

                i = vdc.vce_address.w;
                r = ((vdc.vce_data[i].w >> 3) & 7) << 5;
                g = ((vdc.vce_data[i].w >> 6) & 7) << 5;
                b = ((vdc.vce_data[i].w >> 0) & 7) << 5;
                palette_set_color_rgb(Machine, i, r, g, b);
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
		for(i=0;i<(vdc.physical_width >> 3);i++)
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

				line_buffer[ pixel++ ] = Machine->pens[c];
				if ( vdc.physical_width != 512 ) {
					if ( pixel < ( ( ( i * 8 + x + 1 ) * 512 ) / vdc.physical_width ) ) {
						line_buffer[ pixel++ ] = Machine->pens[c];
					}
				}
			}
		}
	}
	/* Sprite rendering is independant of BG drawing! */
	if(vdc.vdc_data[CR].w & CR_SB)
	{
		pce_refresh_sprites(bitmap_line, line);
	}
	vdc.y_scroll++;
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
    static int cgx_table[] = {16, 32};
    static int cgy_table[] = {16, 32, 64, 64};

    int obj_x, obj_y, obj_i, obj_a;
    int obj_w, obj_h, obj_l, cgypos;
    int hf, vf;
    int cgx, cgy, palette;
	int priority;
    int i, x, c /*, b*/;
    char buf[16];

	UINT8 sprites_drawn=0;
	UINT16 *line_buffer= BITMAP_ADDR16( vdc.bmp, bitmap_line, 86 );

	/* 0 -> no sprite pixels drawn, otherwise is sprite #+1 */
	UINT8 drawn[Machine->screen[0].width];

	//clear our sprite-to-sprite clipping buffer.
	memset(drawn, 0, sizeof(drawn));
	/* count up: Highest priority is Sprite 0 */ 
	for(i=0; i<64; i++)
	{
		obj_y = (vdc.sprite_ram[(i<<2)+0] & 0x03FF) - 64;
		obj_x = (vdc.sprite_ram[(i<<2)+1] & 0x03FF) - 32;

		if ( vdc.vdc_data[MWR].w & 0x0040 ) {
			obj_y -= ( vdc.vdc_data[BYR].w & 0x1FF );
		}

		if ((obj_y == -64) || (obj_y > line)) continue;
		if ((obj_x == -32) || (obj_x > vdc.physical_width)) continue;


		obj_a = (vdc.sprite_ram[(i<<2)+3]);

//      if ((obj_a & 0x80) == 0) continue;

		cgx   = (obj_a >> 8) & 1;   /* sprite width */ 
		cgy   = (obj_a >> 12) & 3;  /* sprite height */ 
		hf    = (obj_a >> 11) & 1;  /* horizontal flip */ 
		vf    = (obj_a >> 15) & 1;  /* vertical flip */ 
		palette = (obj_a & 0x000F);
		priority = (obj_a >> 7) & 1;   /* why was this not emulated? */ 

		obj_i = (vdc.sprite_ram[(i<<2)+2] & 0x07FE);

		obj_w = cgx_table[cgx];
		obj_h = cgy_table[cgy];
		obj_l = (line - obj_y);

		//no need to draw an object that's ABOVE where we are.
		if((obj_y + obj_h)<line) continue;

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
				int pixel_x = ( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2), obj_l, hf, vf, buf);

				for(x=0;x<16;x++)
				{
					if(((obj_x + x)<(vdc.physical_width))&&((obj_x + x)>=0))
					{
						c = buf[x];
						if(c)
						{
							if(!drawn[obj_x+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + c];
									if ( vdc.physical_width != 512 ) { 
										if ( pixel_x + 1 < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) { 
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + c];
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
				int pixel_x = ( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2) + (hf ? 2 : 0), obj_l, hf, vf, buf);

				for(x=0;x<16;x++)
				{
					if(((obj_x + x)<(vdc.physical_width))&&((obj_x + x)>=0))
					{
						c = buf[x];
						if(c)
						{
							if(!drawn[obj_x+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + c];
									if ( vdc.physical_width != 512 ) {
										if ( pixel_x + 1 < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) ) {
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + c];
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
						c = buf[x];
						if(c)
						{
							if(!drawn[obj_x+0x10+x])
							{
								if(priority || (line_buffer[pixel_x] == Machine->pens[0])) {
									line_buffer[pixel_x] = Machine->pens[0x100 + (palette << 4) + c];
									if ( vdc.physical_width != 512 ) {
										if ( pixel_x + 1 < ( ( ( obj_x + x + 17 ) * 512 ) / vdc.physical_width ) ) {
											line_buffer[pixel_x + 1] = Machine->pens[0x100 + (palette << 4) + c];
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
