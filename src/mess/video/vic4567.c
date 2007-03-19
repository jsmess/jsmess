// don't include this into the makefile
// it is included in vic6567.c yet

#define OPTIMIZE

#define VIC3_BITPLANES_MASK (vic2.reg[0x32])
/* bit 0, 4 not used !?*/
/* I think hinibbles contains the banknumbers for interlaced modes */
/* if hinibble set then x&1==0 should be in bank1 (0x10000), x&1==1 in bank 0 */
#define VIC3_BITPLANE_ADDR_HELPER(x)  ( (vic2.reg[0x33+x]&0xf) <<12)
#define VIC3_BITPLANE_ADDR(x) ( x&1 ? VIC3_BITPLANE_ADDR_HELPER(x)+0x10000 \
								: VIC3_BITPLANE_ADDR_HELPER(x) )
#define VIC3_BITPLANE_IADDR_HELPER(x)  ( (vic2.reg[0x33+x]&0xf0) <<8)
#define VIC3_BITPLANE_IADDR(x) ( x&1 ? VIC3_BITPLANE_IADDR_HELPER(x)+0x10000 \
								: VIC3_BITPLANE_IADDR_HELPER(x) )

unsigned char vic3_palette[0x100*3]={0};

static struct {
	struct {
		UINT8 red, green, blue;
	} palette[0x100];
	int palette_dirty;
} vic3;

WRITE8_HANDLER( vic3_palette_w )
{
	if (offset<0x100) vic3.palette[offset].red=data;
	else if (offset<0x200) vic3.palette[offset&0xff].green=data;
	else vic3.palette[offset&0xff].blue=data;
	vic3.palette_dirty=1;
}

void vic4567_init (int pal, int (*dma_read) (int),
						  int (*dma_read_color) (int), void (*irq) (int),
						  void (*param_port_changed)(int))
{
	memset(&vic2, 0, sizeof(vic2));

	vic2.lines = VIC2_LINES;
	vic2.vic3 = TRUE;

	vic2.dma_read = dma_read;
	vic2.dma_read_color = dma_read_color;
	vic2.interrupt = irq;
	vic2.pal = pal;
	vic2.port_changed = param_port_changed;
	vic2.on = TRUE;

	state_save_register_global_array(vic2.reg);
}

WRITE8_HANDLER ( vic3_port_w )
{
	DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
	offset &= 0x7f;
	switch (offset)
	{
	case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e:
		vic2_port_w(offset,data);
		break;
	case 0x2f:
		DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
		vic2.reg[offset] = data;
		break;
	case 0x30:
		vic2.reg[offset] = data;
		if (vic2.port_changed!=NULL) {
			DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
			vic2.reg[offset] = data;
			vic2.port_changed(data);
		}
		break;
	case 0x31:
		vic2.reg[offset] = data;
		if (data&0x40) cpunum_set_clockscale(0,1.0);
		else cpunum_set_clockscale(0, 1.0/3.5);
		break;
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:
		vic2.reg[offset] = data;
		DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
		DBG_LOG (2, "vic plane write", ("%.2x:%.2x\n", offset, data));
		break;
	default:
		vic2.reg[offset] = data;
		break;
	}
}

 READ8_HANDLER ( vic3_port_r )
{
	int val = 0;
	offset &= 0x7f;
	switch (offset)
	{
	case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e:
		return vic2_port_r(offset);
	case 0x2f:
	case 0x30:
		val = vic2.reg[offset];
		DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
		break;
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:						   /* not used */
		val = vic2.reg[offset];
		DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
		DBG_LOG (2, "vic3 plane read", ("%.2x:%.2x\n", offset, val));
		break;
	default:
		val = vic2.reg[offset];
	}
	return val;
}

static void vic3_drawlines (int first, int last)
{
	int line, vline, end;
	int attr, ch, ecm;
	int syend;
	int offs, yoff, xoff, ybegin, yend, xbegin, xend;
	int x_end2;
	int i, j;

	if (first == last)
		return;
	vic2.lastline = last;
	if (video_skip_this_frame ())
		return;

	/* top part of display not rastered */
	first -= VIC2_YPOS - YPOS;
	last -= VIC2_YPOS - YPOS;
	if ((first >= last) || (last <= 0))
	{
		for (i = 0; i < 8; i++)
			vic2.sprites[i].repeat = vic2.sprites[i].line = 0;
		return;
	}
	if (first < 0)
		first = 0;

	if (!SCREENON)
	{
		for (line = first; (line < last) && (line < vic2.bitmap->height); line++)
			memset16 (BITMAP_ADDR16(vic2.bitmap, line, 0), Machine->pens[0], vic2.bitmap->width);
		return;
	}
	if (COLUMNS40)
		xbegin = XPOS, xend = xbegin + 640;
	else
		xbegin = XPOS + 7, xend = xbegin + 624;

	if (last < vic2.y_begin)
		end = last;
	else
		end = vic2.y_begin + YPOS;

	for (line = first; line < end; line++)
	{
		memset16 (BITMAP_ADDR16(vic2.bitmap, line, 0), Machine->pens[FRAMECOLOR],
			vic2.bitmap->width);
	}

	if (LINES25)
	{
		vline = line - vic2.y_begin - YPOS;
	}
	else
	{
		vline = line - vic2.y_begin - YPOS + 8 - VERTICALPOS;
	}
	if (last < vic2.y_end + YPOS)
		end = last;
	else
		end = vic2.y_end + YPOS;
	x_end2=vic2.x_end*2;
	for (; line < end; vline = (vline + 8) & ~7, line = line + 1 + yend - ybegin)
	{
		offs = (vline >> 3) * 80;
		ybegin = vline & 7;
		yoff = line - ybegin;
		yend = (yoff + 7 < end) ? 7 : (end - yoff - 1);
		/* rendering 39 characters */
		/* left and right borders are overwritten later */
		vic2.shift[line] = HORICONTALPOS;

		for (xoff = vic2.x_begin + XPOS; xoff < x_end2 + XPOS; xoff += 8, offs++)
		{
			ch = vic2.dma_read (vic2.videoaddr + offs);
			attr = vic2.dma_read_color (vic2.videoaddr + offs);
			if (HIRESON)
			{
				vic2.bitmapmulti[1] = vic2.c64_bitmap[1] = Machine->pens[ch >> 4];
				vic2.bitmapmulti[2] = vic2.c64_bitmap[0] = Machine->pens[ch & 0xf];
				if (MULTICOLORON)
				{
					vic2.bitmapmulti[3] = Machine->pens[attr];
					vic2_draw_bitmap_multi (ybegin, yend, offs, yoff, xoff);
				}
				else
				{
					vic2_draw_bitmap (ybegin, yend, offs, yoff, xoff);
				}
			}
			else if (ECMON)
			{
				ecm = ch >> 6;
				vic2.ecmcolor[0] = vic2.colors[ecm];
				vic2.ecmcolor[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch & ~0xC0, yoff, xoff, vic2.ecmcolor);
			}
			else if (MULTICOLORON && (attr & 8))
			{
				vic2.multi[3] = Machine->pens[attr & 7];
				vic2_draw_character_multi (ybegin, yend, ch, yoff, xoff);
			}
			else
			{
				vic2.mono[1] = Machine->pens[attr];
				vic2_draw_character (ybegin, yend, ch, yoff, xoff, vic2.mono);
			}
		}
		/* sprite priority, sprite overwrites lowerprior pixels */
		for (i = 7; i >= 0; i--)
		{
			if (vic2.sprites[i].line || vic2.sprites[i].repeat)
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if ((21 - vic2.sprites[i].line) * 2 - vic2.sprites[i].repeat < yend - ybegin + 1)
						syend = ybegin + (21 - vic2.sprites[i].line) * 2 - vic2.sprites[i].repeat - 1;
				}
				else
				{
					if (vic2.sprites[i].line + yend - ybegin + 1 > 20)
						syend = ybegin + 20 - vic2.sprites[i].line;
				}
				if (yoff + syend > YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, ybegin, syend);
				else
					vic2_draw_sprite (i, yoff, ybegin, syend);
				if ((syend != yend) || (vic2.sprites[i].line > 20))
				{
					vic2.sprites[i].line = vic2.sprites[i].repeat = 0;
					for (j = syend; j <= yend; j++)
						vic2.sprites[i].paintedline[j] = 0;
				}
			}
			else if (SPRITEON (i) && (yoff + ybegin <= SPRITE_Y_POS (i))
					 && (yoff + yend >= SPRITE_Y_POS (i)))
			{
				syend = yend;
				if (SPRITE_Y_EXPAND (i))
				{
					if (21 * 2 < yend - ybegin + 1)
						syend = ybegin + 21 * 2 - 1;
				}
				else
				{
					if (yend - ybegin + 1 > 21)
						syend = ybegin + 21 - 1;
				}
				if (yoff + syend >= YPOS + 200)
					syend = YPOS + 200 - yoff - 1;
				for (j = 0; j < SPRITE_Y_POS (i) - yoff; j++)
					vic2.sprites[i].paintedline[j] = 0;
				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				else
					vic2_draw_sprite (i, yoff, SPRITE_Y_POS (i) - yoff, syend);
				if ((syend != yend) || (vic2.sprites[i].line > 20))
				{
					for (j = syend; j <= yend; j++)
						vic2.sprites[i].paintedline[j] = 0;
					vic2.sprites[i].line = vic2.sprites[i].repeat = 0;
				}
			}
			else
			{
				memset (vic2.sprites[i].paintedline, 0, sizeof (vic2.sprites[i].paintedline));
			}
		}

		for (i = ybegin; i <= yend; i++)
		{
			memset16 (BITMAP_ADDR16(vic2.bitmap, yoff + 1, 0),
						Machine->pens[FRAMECOLOR], xbegin);
			memset16 (BITMAP_ADDR16(vic2.bitmap, yoff + 1, xend),
						Machine->pens[FRAMECOLOR], vic2.bitmap->width - xend);
		}
	}
	if (last < vic2.bitmap->height)
		end = last;
	else
		end = vic2.bitmap->height;

	for (; line < end; line++)
	{
		memset16 (BITMAP_ADDR16(vic2.bitmap, line, 0), Machine->pens[FRAMECOLOR],
			vic2.bitmap->width);
	}
}

#ifndef OPTIMIZE
INLINE UINT8 vic3_bitplane_to_packed(UINT8 *latch, int mask, int number)
{
	UINT8 color=0;
	if ( (mask&1)&&(latch[0]&(1<<number)) ) color|=1;
	if ( (mask&2)&&(latch[1]&(1<<number)) ) color|=2;
	if ( (mask&4)&&(latch[2]&(1<<number)) ) color|=4;
	if ( (mask&8)&&(latch[3]&(1<<number)) ) color|=8;
	if ( (mask&0x10)&&(latch[4]&(1<<number)) ) color|=0x10;
	if ( (mask&0x20)&&(latch[5]&(1<<number)) ) color|=0x20;
	if ( (mask&0x40)&&(latch[6]&(1<<number)) ) color|=0x40;
	if ( (mask&0x80)&&(latch[7]&(1<<number)) ) color|=0x80;
	return color;
}


INLINE void vic3_block_2_color(int offset, UINT8 colors[8])
{
	if (VIC3_BITPLANES_MASK&1) {
		colors[0]=c64_memory[VIC3_BITPLANE_ADDR(0)+offset];
	}
	if (VIC3_BITPLANES_MASK&2) {
		colors[1]=c64_memory[VIC3_BITPLANE_ADDR(1)+offset];
	}
	if (VIC3_BITPLANES_MASK&4) {
		colors[2]=c64_memory[VIC3_BITPLANE_ADDR(2)+offset];
	}
	if (VIC3_BITPLANES_MASK&8) {
		colors[3]=c64_memory[VIC3_BITPLANE_ADDR(3)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x10) {
		colors[4]=c64_memory[VIC3_BITPLANE_ADDR(4)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x20) {
		colors[5]=c64_memory[VIC3_BITPLANE_ADDR(5)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x40) {
		colors[6]=c64_memory[VIC3_BITPLANE_ADDR(6)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x80) {
		colors[7]=c64_memory[VIC3_BITPLANE_ADDR(7)+offset];
	}
}

INLINE void vic3_interlace_block_2_color(int offset, UINT8 colors[8])
{

	if (VIC3_BITPLANES_MASK&1) {
		colors[0]=c64_memory[VIC3_BITPLANE_IADDR(0)+offset];
	}
	if (VIC3_BITPLANES_MASK&2) {
		colors[1]=c64_memory[VIC3_BITPLANE_IADDR(1)+offset];
	}
	if (VIC3_BITPLANES_MASK&4) {
		colors[2]=c64_memory[VIC3_BITPLANE_IADDR(2)+offset];
	}
	if (VIC3_BITPLANES_MASK&8) {
		colors[3]=c64_memory[VIC3_BITPLANE_IADDR(3)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x10) {
		colors[4]=c64_memory[VIC3_BITPLANE_IADDR(4)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x20) {
		colors[5]=c64_memory[VIC3_BITPLANE_IADDR(5)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x40) {
		colors[6]=c64_memory[VIC3_BITPLANE_IADDR(6)+offset];
	}
	if (VIC3_BITPLANES_MASK&0x80) {
		colors[7]=c64_memory[VIC3_BITPLANE_IADDR(7)+offset];
	}
}

INLINE void vic3_draw_block(int x, int y, UINT8 colors[8])
{
	vic2.bitmap->line[YPOS+y][XPOS+x]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 7)];
	vic2.bitmap->line[YPOS+y][XPOS+x+1]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 6)];
	vic2.bitmap->line[YPOS+y][XPOS+x+2]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 5)];
	vic2.bitmap->line[YPOS+y][XPOS+x+3]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 4)];
	vic2.bitmap->line[YPOS+y][XPOS+x+4]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 3)];
	vic2.bitmap->line[YPOS+y][XPOS+x+5]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 2)];
	vic2.bitmap->line[YPOS+y][XPOS+x+6]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 1)];
	vic2.bitmap->line[YPOS+y][XPOS+x+7]=
		Machine->pens[vic3_bitplane_to_packed(colors, vic2.reg[0x32], 0)];
}

#else
/* very recognizable SPEED improvement */
#define VIC3_MASK(M)                                            \
    if (M)                                                      \
    {                                                           \
        if (M & 0x01)                                    \
            colors[0] = c64_memory[VIC3_ADDR(0)+offset];        \
        if (M & 0x02)                                           \
            colors[1] = c64_memory[VIC3_ADDR(1)+offset]<<1;     \
        if (M & 0x04)                                    \
            colors[2] = c64_memory[VIC3_ADDR(2)+offset]<<2;     \
        if (M & 0x08)                                           \
            colors[3] = c64_memory[VIC3_ADDR(3)+offset]<<3;     \
        if (M & 0x10)                                           \
            colors[4] = c64_memory[VIC3_ADDR(4)+offset]<<4;     \
        if (M & 0x20)                                           \
            colors[5] = c64_memory[VIC3_ADDR(5)+offset]<<5;     \
        if (M & 0x40)                                           \
            colors[6] = c64_memory[VIC3_ADDR(6)+offset]<<6;     \
        if (M & 0x80)                                           \
            colors[7] = c64_memory[VIC3_ADDR(7)+offset]<<7;     \
        for (i=7; i >= 0; i--)                                  \
        {                                                       \
            p = 0;                                              \
            if (M & 0x01)                                \
            {                                                   \
                p = colors[0] & 0x01;                           \
                colors[0] >>= 1;                                \
            }                                                   \
            if (M & 0x02)                                       \
            {                                                   \
                p |= colors[1] & 0x02;                      \
                colors[1] >>= 1;                                \
            }                                                   \
            if (M & 0x04)                                       \
            {                                                   \
                p |= colors[2] & 0x04;                      \
                colors[2] >>= 1;                                \
            }                                                   \
            if (M & 0x08)                                       \
            {                                                   \
                p |= colors[3] & 0x08;                      \
                colors[3] >>= 1;                                \
            }                                                   \
            if (M & 0x10)                                       \
            {                                                   \
                p |= colors[4] & 0x10;                      \
                colors[4] >>= 1;                                \
            }                                                   \
            if (M & 0x20)                                       \
            {                                                   \
                p |= colors[5] & 0x20;                      \
                colors[5] >>= 1;                                \
            }                                                   \
            if (M & 0x40)                                       \
            {                                                   \
                p |= colors[6] & 0x40;                      \
                colors[6] >>= 1;                                \
            }                                                   \
            if (M & 0x80)                                       \
            {                                                   \
                p |= colors[7] & 0x80;                      \
                colors[7] >>= 1;                                \
            }                                                   \
            plot_pixel(vic2.bitmap, XPOS+x+i, YPOS+y, Machine->pens[p]);  \
        }                                                       \
    }

#define VIC3_ADDR(a) VIC3_BITPLANE_IADDR(a)
static void vic3_interlace_draw_block(int x, int y, int offset)
{
	int colors[8]={0};
	int i, p;

	switch (VIC3_BITPLANES_MASK) {
	case 0x05:
		VIC3_MASK(0x05)
		break;
    case 0x07:
		VIC3_MASK(0x07)
		break;
    case 0x0f:
		VIC3_MASK(0x0f)
		break;
    case 0x1f:
		VIC3_MASK(0x1f)
		break;
    case 0x7f:
		VIC3_MASK(0x7f)
		break;
    case 0xff:
		VIC3_MASK(0xff)
		break;
	default:
		if (VIC3_BITPLANES_MASK&1) {
			colors[0]=c64_memory[VIC3_BITPLANE_IADDR(0)+offset];
		}
		if (VIC3_BITPLANES_MASK&2) {
			colors[1]=c64_memory[VIC3_BITPLANE_IADDR(1)+offset]<<1;
		}
		if (VIC3_BITPLANES_MASK&4) {
			colors[2]=c64_memory[VIC3_BITPLANE_IADDR(2)+offset]<<2;
		}
		if (VIC3_BITPLANES_MASK&8) {
			colors[3]=c64_memory[VIC3_BITPLANE_IADDR(3)+offset]<<3;
		}
		if (VIC3_BITPLANES_MASK&0x10) {
			colors[4]=c64_memory[VIC3_BITPLANE_IADDR(4)+offset]<<4;
		}
		if (VIC3_BITPLANES_MASK&0x20) {
			colors[5]=c64_memory[VIC3_BITPLANE_IADDR(5)+offset]<<5;
		}
		if (VIC3_BITPLANES_MASK&0x40) {
			colors[6]=c64_memory[VIC3_BITPLANE_IADDR(6)+offset]<<6;
		}
		if (VIC3_BITPLANES_MASK&0x80) {
			colors[7]=c64_memory[VIC3_BITPLANE_IADDR(7)+offset]<<7;
		}
		for (i=7;i>=0;i--) {
			plot_pixel(vic2.bitmap, XPOS+x+i, YPOS+y,
				Machine->pens[(colors[0]&1)|(colors[1]&2)
							 |(colors[2]&4)|(colors[3]&8)
							 |(colors[4]&0x10)|(colors[5]&0x20)
							 |(colors[6]&0x40)|(colors[7]&0x80)]);
			colors[0]>>=1;
			colors[1]>>=1;
			colors[2]>>=1;
			colors[3]>>=1;
			colors[4]>>=1;
			colors[5]>>=1;
			colors[6]>>=1;
			colors[7]>>=1;
		}
	}
}

#undef VIC3_ADDR
#define VIC3_ADDR(a) VIC3_BITPLANE_ADDR(a)
static void vic3_draw_block(int x, int y, int offset)
{
	int colors[8]={0};
	int i, p;

	switch (VIC3_BITPLANES_MASK) {
	case 5:
		VIC3_MASK(0x05)
		break;
	case 7:
		VIC3_MASK(0x07)
        break;
	case 0xf:
		VIC3_MASK(0x0f)
        break;
	case 0x1f:
		VIC3_MASK(0x1f)
        break;
	case 0x7f:
		VIC3_MASK(0x7f)
        break;
	case 0xff:
		VIC3_MASK(0xff)
        break;
	default:
		if (VIC3_BITPLANES_MASK&1) {
			colors[0]=c64_memory[VIC3_BITPLANE_ADDR(0)+offset];
		}
		if (VIC3_BITPLANES_MASK&2) {
			colors[1]=c64_memory[VIC3_BITPLANE_ADDR(1)+offset]<<1;
		}
		if (VIC3_BITPLANES_MASK&4) {
			colors[2]=c64_memory[VIC3_BITPLANE_ADDR(2)+offset]<<2;
		}
		if (VIC3_BITPLANES_MASK&8) {
			colors[3]=c64_memory[VIC3_BITPLANE_ADDR(3)+offset]<<3;
		}
		if (VIC3_BITPLANES_MASK&0x10) {
			colors[4]=c64_memory[VIC3_BITPLANE_ADDR(4)+offset]<<4;
		}
		if (VIC3_BITPLANES_MASK&0x20) {
			colors[5]=c64_memory[VIC3_BITPLANE_ADDR(5)+offset]<<5;
		}
		if (VIC3_BITPLANES_MASK&0x40) {
			colors[6]=c64_memory[VIC3_BITPLANE_ADDR(6)+offset]<<6;
		}
		if (VIC3_BITPLANES_MASK&0x80) {
			colors[7]=c64_memory[VIC3_BITPLANE_ADDR(7)+offset]<<7;
		}
		for (i=7;i>=0;i--) {
			plot_pixel(vic2.bitmap, XPOS+x+i, YPOS+y,
				Machine->pens[(colors[0]&1)|(colors[1]&2)
							 |(colors[2]&4)|(colors[3]&8)
							 |(colors[4]&0x10)|(colors[5]&0x20)
							 |(colors[6]&0x40)|(colors[7]&0x80)]);
			colors[0]>>=1;
			colors[1]>>=1;
			colors[2]>>=1;
			colors[3]>>=1;
			colors[4]>>=1;
			colors[5]>>=1;
			colors[6]>>=1;
			colors[7]>>=1;
		}
	}
}
#endif

static void vic3_draw_bitplanes(void)
{
	static int interlace=0;
	int x, y, y1s, offset;
#ifndef OPTIMIZE
	UINT8 colors[8];
#endif
	rectangle vis;

	if (VIC3_LINES==400) { /* interlaced! */
		for ( y1s=0, offset=0; y1s<400; y1s+=16) {
			for (x=0; x<VIC3_BITPLANES_WIDTH; x+=8) {
				for ( y=y1s; y<y1s+16; y+=2, offset++) {
#ifndef OPTIMIZE
					if (interlace) {
						vic3_block_2_color(offset,colors);
						vic3_draw_block(x,y,colors);
					} else {
						vic3_interlace_block_2_color(offset,colors);
						vic3_draw_block(x,y+1,colors);
					}
#else
					if (interlace)
						vic3_draw_block(x,y,offset);
					else
						vic3_interlace_draw_block(x,y+1,offset);
#endif
				}
			}
		}
		interlace^=1;
	} else {
		for ( y1s=0, offset=0; y1s<200; y1s+=8) {
			for (x=0; x<VIC3_BITPLANES_WIDTH; x+=8) {
				for ( y=y1s; y<y1s+8; y++, offset++) {
#ifndef OPTIMIZE
					vic3_block_2_color(offset,colors);
					vic3_draw_block(x,y,colors);
#else
					vic3_draw_block(x,y,offset);
#endif
				}
			}
		}
	}
	if (XPOS>0) {
		vis.min_x = 0;
		vis.max_x = XPOS-1;
		vis.min_y = 0;
		vis.max_y = Machine->screen[0].visarea.max_y;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
	if (XPOS+VIC3_BITPLANES_WIDTH<Machine->screen[0].visarea.max_x) {
		vis.min_x = XPOS+VIC3_BITPLANES_WIDTH;
		vis.max_x = Machine->screen[0].visarea.max_x;
		vis.min_y = 0;
		vis.max_y = Machine->screen[0].visarea.max_y;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
	if (YPOS>0)
	{
		vis.min_y = 0;
		vis.max_y = YPOS-1;
		vis.min_x = 0;
		vis.max_x = Machine->screen[0].visarea.max_x;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
	if (YPOS+VIC3_LINES<Machine->screen[0].visarea.max_y) {
		vis.min_y = YPOS+VIC3_LINES;
		vis.max_y = Machine->screen[0].visarea.max_y;
		vis.min_x = 0;
		vis.max_x = Machine->screen[0].visarea.max_x;
		fillbitmap(vic2.bitmap, Machine->pens[FRAMECOLOR],&vis);
	}
}

INTERRUPT_GEN( vic3_raster_irq )
{
	static int columns=640, raws=200;
	int new_columns, new_raws;
	int i;

	vic2.rasterline++;
	if (vic2.rasterline >= vic2.lines)
	{
		vic2.rasterline = 0;
		if (vic3.palette_dirty) {
			for (i=0; i<256; i++) {
				palette_set_color(Machine, i,vic3.palette[i].red<<4,
									 vic3.palette[i].green<<4,
									 vic3.palette[i].blue<<4);
			}
		}
		if (vic3.palette_dirty) {
			vic2.spritemulti[1] = Machine->pens[SPRITE_MULTICOLOR1];
			vic2.spritemulti[3] = Machine->pens[SPRITE_MULTICOLOR2];
			vic2.mono[0] = vic2.bitmapmulti[0] = vic2.multi[0] =
				vic2.colors[0] = Machine->pens[BACKGROUNDCOLOR];
			vic2.multi[1] = vic2.colors[1] = Machine->pens[MULTICOLOR1];
			vic2.multi[2] = vic2.colors[2] = Machine->pens[MULTICOLOR2];
			vic2.colors[3] = Machine->pens[FOREGROUNDCOLOR];
			vic3.palette_dirty=0;
		}
		new_raws=200;
		if (VIC3_BITPLANES) {
			new_columns=VIC3_BITPLANES_WIDTH;
			if (new_columns<320) new_columns=320; /*sprites resolution about 320x200 */
			new_raws=VIC3_LINES;
		} else if (VIC3_80COLUMNS) {
			new_columns=640;
		} else {
			new_columns=320;
		}
		if ((new_columns!=columns)||(new_raws!=raws)) {
			raws=new_raws;
			columns=new_columns;
			video_screen_set_visarea(0, 0,columns+16-1,0, raws+16-1);
		}
		if (VIC3_BITPLANES) {
			if (!video_skip_this_frame ()) vic3_draw_bitplanes();
		} else {
			if (vic2.on) vic2_drawlines (vic2.lastline, vic2.lines);
		}
		for (i = 0; i < 8; i++)
			vic2.sprites[i].repeat = vic2.sprites[i].line = 0;
		vic2.lastline = 0;
		if (LIGHTPEN_BUTTON)
		{
			double tme = 0.0;

			/* lightpen timer starten */
			timer_set (tme, 1, vic2_timer_timeout);
		}
		//state_display(vic2.bitmap);
	}
	if (vic2.rasterline == C64_2_RASTERLINE (RASTERLINE))
	{
		if (vic2.on)
			vic2_drawlines (vic2.lastline, vic2.rasterline);
		vic2_set_interrupt (1);
	}
}

