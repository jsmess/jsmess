#include "emu.h"
#include "includes/intv.h"

#define FOREGROUND_BIT 0x0010

/* initialized to non-zero, because we divide by it */

INLINE void intv_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (color<<1)+1;
}

VIDEO_START( intv )
{
	intv_state *state = machine.driver_data<intv_state>();
	//int i,j,k;

	state->m_tms9927_num_rows = 25;
	VIDEO_START_CALL(generic_bitmapped);

#if 0
    for (i = 0; i < 8; i++) {
        intv_sprite_type* s = &state->m_sprite[i];
        s->visible = 0;
        s->xpos = 0;
        s->ypos = 0;
        s->coll = 0;
        s->collision = 0;
        s->doublex = 0;
        s->doubley = 0;
        s->quady = 0;
        s->xflip = 0;
        s->yflip = 0;
        s->behind_foreground = 0;
        s->grom = 0;
        s->card = 0;
        s->color = 0;
        s->doubleyres = 0;
        s->dirty = 1;
        for (j = 0; j < 16; j++) {
            for (k = 0; k < 128; k++) {
                state->m_sprite_buffers[i][j][k] = 0;
            }
        }
        state->m_collision_registers[i] = 0;
    }
    for(i = 0; i < 4; i++) {
        state->m_color_stack[i] = 0;
    }
    state->m_color_stack_mode = 0;
    state->m_color_stack_offset = 0;
    state->m_stic_handshake = 0;
    state->m_border_color = 0;
    state->m_col_delay = 0;
    state->m_row_delay = 0;
    state->m_left_edge_inhibit = 0;
    state->m_top_edge_inhibit = 0;

    state->m_gramdirty = 1;
    for(i=0;i<64;i++) {
        state->m_gram[i] = 0;
        state->m_gramdirtybytes[i] = 1;
    }
#endif
}


static int sprites_collide(intv_state *state, int spriteNum0, int spriteNum1) {
    INT16 x1, x2, y1, y2, w1, w2, h1, h2, x0, y0, r0y, r1y,
          width, height, x, y;

    intv_sprite_type* s1 = &state->m_sprite[spriteNum0];
    intv_sprite_type* s2 = &state->m_sprite[spriteNum1];

    x1 = s1->xpos-8; x2 = s2->xpos-8;
    y1 = (s1->ypos-8)<<1; y2 = (s2->ypos-8)<<1;
    w1 = (s1->doublex ? 2 : 1)<<3;
    w2 = (s2->doublex ? 2 : 1)<<3;
    h1 = (s1->quady ? 4 : 1) * (s1->doubley ? 2 : 1) * (s1->doubleyres ? 2 : 1)<<3;
    h2 = (s2->quady ? 4 : 1) * (s2->doubley ? 2 : 1) * (s2->doubleyres ? 2 : 1)<<3;

    if ((x1 + w1 <= x2) || (y1 + h1 <= y2) ||
            (x1 >= x2 + w2) || (y1 >= y2 + h2))
        return FALSE;

    //iterate over the intersecting bits to see if any touch
    x0 = MAX(x1, x2);
    y0 = MAX(y1, y2);
    r0y = y0-y1;
    r1y = y0-y2;
    width = MIN(x1+w1, x2+w2) - x0;
    height = MIN(y1+h1, y2+h2) - y0;
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            if (state->m_sprite_buffers[spriteNum0][x0-x1+x][r0y+y] &&
                    state->m_sprite_buffers[spriteNum1][x0-x2+x][r1y+y])
                return TRUE;
        }
    }

    return FALSE;
}

static void determine_sprite_collisions(intv_state *state)
{
    //check sprite to sprite collisions
    int i, j;
    for (i = 0; i < 7; i++) {
        intv_sprite_type* s1 = &state->m_sprite[i];
        if (s1->xpos == 0 || !s1->coll)
            continue;

        for (j = i+1; j < 8; j++) {
            intv_sprite_type* s2 = &state->m_sprite[j];
            if (s2->xpos == 0 || !s2->coll)
                continue;

            if (sprites_collide(state, i, j)) {
                s1->collision |= (1 << j);
                s2->collision |= (1 << i);
            }
        }
    }
}

static void render_sprites(running_machine &machine)
{
	intv_state *state = machine.driver_data<intv_state>();
    INT32 cardMemoryLocation, pixelSize;
    INT32 spritePixelHeight;
    INT32 nextMemoryLocation;
    INT32 nextData;
    INT32 nextX;
    INT32 nextY;
    INT32 xInc;
    INT32 i, j;

    UINT8* memory = machine.region("maincpu")->base();

    for (i = 0; i < 8; i++) {
		intv_sprite_type* s = &state->m_sprite[i];

        if (s->grom)
            cardMemoryLocation = (s->card << 3);
        else
            cardMemoryLocation = ((s->card & 0x003F) << 3);

        pixelSize = (s->quady ? 4 : 1) * (s->doubley ? 2 : 1);
        spritePixelHeight = 8 * pixelSize * (s->doubleyres ? 2 : 1);

        for (j = 0; j < spritePixelHeight; j++) {
            nextMemoryLocation = (cardMemoryLocation + (j/pixelSize));
            if (s->grom)
                nextData = memory[(0x3000+nextMemoryLocation)<<1];
            else if (nextMemoryLocation < 0x200)
                nextData = state->m_gram[nextMemoryLocation];
            else
                nextData = 0xFFFF;
            nextX = (s->xflip ? (s->doublex ? 15 : 7) : 0);
            nextY = (s->yflip ? (spritePixelHeight - j - 1) : j);
            xInc = (s->xflip ? -1: 1);
            state->m_sprite_buffers[i][nextX][nextY] = ((nextData & 0x0080) != 0);
            state->m_sprite_buffers[i][nextX + xInc][nextY] = (s->doublex
                    ? ((nextData & 0x0080) != 0)
                    : ((nextData & 0x0040) != 0));
            state->m_sprite_buffers[i][nextX + (2*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0040) != 0)
                    : ((nextData & 0x0020) != 0));
            state->m_sprite_buffers[i][nextX + (3*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0040) != 0)
                    : ((nextData & 0x0010) != 0));
            state->m_sprite_buffers[i][nextX + (4*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0020) != 0)
                    : ((nextData & 0x0008) != 0));
            state->m_sprite_buffers[i][nextX + (5*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0020) != 0)
                    : ((nextData & 0x0004) != 0));
            state->m_sprite_buffers[i][nextX + (6*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0010) != 0)
                    : ((nextData & 0x0002) != 0));
            state->m_sprite_buffers[i][nextX + (7*xInc)][nextY] = (s->doublex
                    ? ((nextData & 0x0010) != 0)
                    : ((nextData & 0x0001) != 0));
            if (!s->doublex)
                continue;

            state->m_sprite_buffers[i][nextX + (8*xInc)][nextY] =
                    ((nextData & 0x0008) != 0);
            state->m_sprite_buffers[i][nextX + (9*xInc)][nextY] =
                    ((nextData & 0x0008) != 0);
            state->m_sprite_buffers[i][nextX + (10*xInc)][nextY] =
                    ((nextData & 0x0004) != 0);
            state->m_sprite_buffers[i][nextX + (11*xInc)][nextY] =
                    ((nextData & 0x0004) != 0);
            state->m_sprite_buffers[i][nextX + (12*xInc)][nextY] =
                    ((nextData & 0x0002) != 0);
            state->m_sprite_buffers[i][nextX + (13*xInc)][nextY] =
                    ((nextData & 0x0002) != 0);
            state->m_sprite_buffers[i][nextX + (14*xInc)][nextY] =
                    ((nextData & 0x0001) != 0);
            state->m_sprite_buffers[i][nextX + (15*xInc)][nextY] =
                    ((nextData & 0x0001) != 0);
        }

    }
}

static void render_line(running_machine &machine, bitmap_t *bitmap,
	UINT8 nextByte, UINT16 x, UINT16 y, UINT8 fgcolor, UINT8 bgcolor)
{
    UINT32 color = (nextByte & 0x80 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x, y, color);
    intv_plot_pixel(bitmap, x+1, y, color);
    intv_plot_pixel(bitmap, x, y+1, color);
    intv_plot_pixel(bitmap, x+1, y+1, color);

    color = (nextByte & 0x40 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+2, y, color);
    intv_plot_pixel(bitmap, x+3, y, color);
    intv_plot_pixel(bitmap, x+2, y+1, color);
    intv_plot_pixel(bitmap, x+3, y+1, color);

    color = (nextByte & 0x20 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+4, y, color);
    intv_plot_pixel(bitmap, x+5, y, color);
    intv_plot_pixel(bitmap, x+4, y+1, color);
    intv_plot_pixel(bitmap, x+5, y+1, color);

    color = (nextByte & 0x10 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+6, y, color);
    intv_plot_pixel(bitmap, x+7, y, color);
    intv_plot_pixel(bitmap, x+6, y+1, color);
    intv_plot_pixel(bitmap, x+7, y+1, color);

    color = (nextByte & 0x08 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+8, y, color);
    intv_plot_pixel(bitmap, x+9, y, color);
    intv_plot_pixel(bitmap, x+8, y+1, color);
    intv_plot_pixel(bitmap, x+9, y+1, color);

    color = (nextByte & 0x04 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+10, y, color);
    intv_plot_pixel(bitmap, x+11, y, color);
    intv_plot_pixel(bitmap, x+10, y+1, color);
    intv_plot_pixel(bitmap, x+11, y+1, color);

    color = (nextByte & 0x02 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+12, y, color);
    intv_plot_pixel(bitmap, x+13, y, color);
    intv_plot_pixel(bitmap, x+12, y+1, color);
    intv_plot_pixel(bitmap, x+13, y+1, color);

    color = (nextByte & 0x01 ? fgcolor : bgcolor);
    intv_plot_pixel(bitmap, x+14, y, color);
    intv_plot_pixel(bitmap, x+15, y, color);
    intv_plot_pixel(bitmap, x+14, y+1, color);
    intv_plot_pixel(bitmap, x+15, y+1, color);
}

static void render_colored_squares(running_machine &machine, bitmap_t *bitmap,
	UINT16 x, UINT16 y, UINT8 color0, UINT8 color1, UINT8 color2, UINT8 color3)
{
    plot_box(bitmap, x, y, 8, 8, (color0<<1)+1);
    plot_box(bitmap, x+8, y, 8, 8, (color1<<1)+1);
    plot_box(bitmap, x, y+8, 8, 8, (color2<<1)+1);
    plot_box(bitmap, x+8, y+8, 8, 8, (color3<<1)+1);
}

static void render_color_stack_mode(running_machine &machine, bitmap_t *bitmap)
{
	intv_state *state = machine.driver_data<intv_state>();
    UINT8 h, csPtr = 0, nexty = 0;
    UINT16 nextCard, nextx = 0;
    UINT8 *ram = machine.region("maincpu")->base();

    for (h = 0; h < 240; h++) {
        nextCard = state->m_ram16[h];

        //colored squares mode
        if ((nextCard & 0x1800) == 0x1000) {
            UINT8 csColor = state->m_color_stack[csPtr];
            UINT8 color0 = nextCard & 0x0007;
            UINT8 color1 = (nextCard & 0x0038) >> 3;
            UINT8 color2 = (nextCard & 0x01C0) >> 6;
            UINT8 color3 = ((nextCard & 0x2000) >> 11) |
                    ((nextCard & 0x0600) >> 9);
            render_colored_squares(machine, bitmap, nextx, nexty,
                    (color0 == 7 ? csColor : (color0 | FOREGROUND_BIT)),
                    (color1 == 7 ? csColor : (color1 | FOREGROUND_BIT)),
                    (color2 == 7 ? csColor : (color2 | FOREGROUND_BIT)),
                    (color3 == 7 ? csColor : (color3 | FOREGROUND_BIT)));
        }
        //color stack mode
        else {
            UINT8 isGrom, j;
            UINT16 memoryLocation, fgcolor, bgcolor;
            UINT8* memory;

            //advance the color pointer, if necessary
            if (nextCard & 0x2000)
                csPtr = (csPtr+1) & 0x03;

            fgcolor = ((nextCard & 0x1000) >> 9) |
                    (nextCard & 0x0007) | FOREGROUND_BIT;
            bgcolor = state->m_color_stack[csPtr] & 0x0F;

            isGrom = !(nextCard & 0x0800);
            if (isGrom) {
                memoryLocation = 0x3000 + (nextCard & 0x07F8);
                memory = ram;
                for (j = 0; j < 16; j+=2)
                    render_line(machine, bitmap, memory[(memoryLocation<<1)+j],
                            nextx, nexty+j, fgcolor, bgcolor);
            }
            else {
                memoryLocation = (nextCard & 0x01F8);
                memory = state->m_gram;
                for (j = 0; j < 16; j+=2)
                    render_line(machine, bitmap, memory[memoryLocation+(j>>1)],
                            nextx, nexty+j, fgcolor, bgcolor);
            }
        }
        nextx += 16;
        if (nextx == 320) {
            nextx = 0;
            nexty += 16;
        }
    }
}

static void render_fg_bg_mode(running_machine &machine, bitmap_t *bitmap)
{
	intv_state *state = machine.driver_data<intv_state>();
    UINT8 i, j, isGrom, fgcolor, bgcolor, nexty = 0;
    UINT16 nextCard, memoryLocation, nextx = 0;
    UINT8* memory;
    UINT8* ram = machine.region("maincpu")->base();

    for (i = 0; i < 240; i++) {
        nextCard = state->m_ram16[i];
        fgcolor = (nextCard & 0x0007) | FOREGROUND_BIT;
        bgcolor = ((nextCard & 0x2000) >> 11) |
                ((nextCard & 0x1600) >> 9);

        isGrom = !(nextCard & 0x0800);
        if (isGrom) {
            memoryLocation = 0x3000 + (nextCard & 0x01F8);
            memory = ram;
            for (j = 0; j < 16; j+=2)
                render_line(machine, bitmap, memory[(memoryLocation<<1)+j],
                        nextx, nexty+j, fgcolor, bgcolor);
        }
        else {
            memoryLocation = (nextCard & 0x01F8);
            memory = state->m_gram;
            for (j = 0; j < 16; j+=2)
                render_line(machine, bitmap, memory[memoryLocation+(j>>1)],
                        nextx, nexty+j, fgcolor, bgcolor);
        }

        nextx += 16;
        if (nextx == 320) {
            nextx = 0;
            nexty += 16;
        }
    }
}

static void copy_sprites_to_background(running_machine &machine, bitmap_t *bitmap)
{
	intv_state *state = machine.driver_data<intv_state>();
    UINT8 width, currentPixel;
    UINT8 borderCollision, foregroundCollision;
    UINT8 spritePixelHeight, x, y;
    INT16 leftX, nextY, i;
    INT32 nextX;

    for (i = 7; i >= 0; i--) {
		intv_sprite_type *s = &state->m_sprite[i];
        if (s->xpos == 0 ||
                (!s->coll && !s->visible))
            continue;

        borderCollision = FALSE;
        foregroundCollision = FALSE;

        spritePixelHeight = (s->quady ? 4 : 1) *
                (s->doubley ? 2 : 1) * (s->doubleyres ? 2 : 1)<<3;
        width = (s->doublex ? 2 : 1)<<3;

        leftX = (s->xpos-8)<<1;
        nextY = (s->ypos-8)<<1;

        for (y = 0; y < spritePixelHeight; y++) {
            for (x = 0; x < width; x++) {
                //if this sprite pixel is not on, then don't paint it
                if (!state->m_sprite_buffers[i][x][y])
                    continue;

                nextX = leftX + (x<<1);
                //if the next pixel location is on the border, then we
                //have a border collision and we can ignore painting it
                if (nextX < (state->m_row_delay ? 8 : 0) || nextX > 317 ||
                        nextY < (state->m_col_delay ? 16 : 0) || nextY > 191)
                {
                    borderCollision = TRUE;
                    continue;
                }

                currentPixel = (*BITMAP_ADDR16(bitmap, nextY, nextX) -1) >> 1;

                //check for foreground collision
                if (currentPixel & FOREGROUND_BIT) {
                    foregroundCollision = TRUE;
                    if (s->behind_foreground)
                        continue;
                }

                if (s->visible) {
                    intv_plot_pixel(bitmap, nextX, nextY, s->color | (currentPixel & FOREGROUND_BIT));
                    intv_plot_pixel(bitmap, nextX+1, nextY, s->color | (currentPixel & FOREGROUND_BIT));
                }
            }
            nextY++;
        }

        //update the collision bits
        if (s->coll) {
            if (foregroundCollision)
                s->collision |= 0x0100;
            if (borderCollision)
                s->collision |= 0x0200;
        }
    }
}

static void render_background(running_machine &machine, bitmap_t *bitmap)
{
	intv_state *state = machine.driver_data<intv_state>();
	if (state->m_color_stack_mode)
        render_color_stack_mode(machine, bitmap);
    else
        render_fg_bg_mode(machine, bitmap);
}

#ifdef UNUSED_CODE
static void draw_background(running_machine &machine, bitmap_t *bitmap, int transparency)
{
	intv_state *state = machine.driver_data<intv_state>();
	// First, draw the background
	int offs = 0;
	int value = 0;
	int row,col;
	int fgcolor,bgcolor = 0;
	int code;

	int colora, colorb, colorc, colord;

	int n_bit;
	int p_bit;
	int g_bit;

	int j;

	if (state->m_color_stack_mode == 1)
	{
		state->m_color_stack_offset = 0;
		for(row = 0; row < 12; row++)
		{
			for(col = 0; col < 20; col++)
			{
				value = state->m_ram16[offs];

				n_bit = value & 0x2000;
				p_bit = value & 0x1000;
				g_bit = value & 0x0800;

				if (p_bit && (!g_bit)) // colored squares mode
				{
					colora = value&0x7;
					colorb = (value>>3)&0x7;
					colorc = (value>>6)&0x7;
					colord = ((n_bit>>11)&0x4) + ((value>>9)&0x3);
					// color 7 if the top of the color stack in this mode
					if (colora == 7) colora = state->m_color_stack[3];
					if (colorb == 7) colorb = state->m_color_stack[3];
					if (colorc == 7) colorc = state->m_color_stack[3];
					if (colord == 7) colord = state->m_color_stack[3];
					plot_box(bitmap,col*16,row*16,8,8,(colora<<1)+1);
					plot_box(bitmap,col*16+8,row*16,8,8,(colorb<<1)+1);
					plot_box(bitmap,col*16,row*16+8,8,8,(colorc<<1)+1);
					plot_box(bitmap,col*16+8,row*16+8,8,8,(colord<<1)+1);
				}
				else // normal color stack mode
				{
					if (n_bit) // next color
					{
						state->m_color_stack_offset += 1;
						state->m_color_stack_offset &= 3;
					}

					if (p_bit) // pastel color set
						fgcolor = (value&0x7) + 8;
					else
						fgcolor = value&0x7;

					bgcolor = state->m_color_stack[state->m_color_stack_offset];
					code = (value>>3)&0xff;

					if (g_bit) // read from gram
					{
						code %= 64;  // keep from going outside the array
						//if (state->m_gramdirtybytes[code] == 1)
						{
							decodechar(machine.gfx[1],
								   code,
								   state->m_gram,
								   machine.config()->gfxdecodeinfo[1].gfxlayout);
							state->m_gramdirtybytes[code] = 0;
						}
						// Draw GRAM char
						drawgfx(bitmap,machine.gfx[1],
							code,
							bgcolor*16+fgcolor,
							0,0,col*16,row*16,
							0,transparency,bgcolor);

						for(j=0;j<8;j++)
						{
							//intv_plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, 1);
							//intv_plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, 1);
						}

					}
					else // read from grom
					{
						drawgfx(bitmap,machine.gfx[0],
							code,
							bgcolor*16+fgcolor,
							0,0,col*16,row*16,
							0,transparency,bgcolor);

						for(j=0;j<8;j++)
						{
							//intv_plot_pixel(bitmap, col*16+j*2, row*16+7*2+1, 2);
							//intv_plot_pixel(bitmap, col*16+j*2+1, row*16+7*2+1, 2);
						}
					}
				}
				offs++;
			} // next col
		} // next row
	}
	else
	{
		// fg/bg mode goes here
		for(row = 0; row < 12; row++)
		{
			for(col = 0; col < 20; col++)
			{
				value = state->m_ram16[offs];
				fgcolor = value & 0x07;
				bgcolor = ((value & 0x2000)>>11)+((value & 0x1600)>>9);
				code = (value & 0x01f8)>>3;

				if (value & 0x0800) // read for GRAM
				{
					//if (state->m_gramdirtybytes[code] == 1)
					{
						decodechar(machine.gfx[1],
							   code,
							   state->m_gram,
							   machine.config()->gfxdecodeinfo[1].gfxlayout);
						state->m_gramdirtybytes[code] = 0;
					}
					// Draw GRAM char
					drawgfx(bitmap,machine.gfx[1],
						code,
						bgcolor*16+fgcolor,
						0,0,col*16,row*16,
						0,transparency,bgcolor);
				}
				else // read from GROM
				{
					drawgfx(bitmap,machine.gfx[0],
						code,
						bgcolor*16+fgcolor,
						0,0,col*16,row*16,
						0,transparency,bgcolor);
				}
				offs++;
			} // next col
		} // next row
	}
}
#endif

/* TBD: need to handle sprites behind foreground? */
#ifdef UNUSED_FUNCTION
static void draw_sprites(running_machine &machine, bitmap_t *bitmap, int behind_foreground)
{
	intv_state *state = machine.driver_data<intv_state>();
    int i;
    int code;

    for(i=7;i>=0;--i)
    {
        intv_sprite_type *s = &state->m_sprite[i];
        if (s->visible && (s->behind_foreground == behind_foreground))
        {
            code = s->card;
            if (!s->grom)
            {
                code %= 64;  // keep from going outside the array
                if (s->yres == 1)
                {
                    //if (state->m_gramdirtybytes[code] == 1)
                    {
                        decodechar(machine.gfx[1],
                           code,
                           state->m_gram,
                           machine.config()->gfxdecodeinfo[1].gfxlayout);
                        state->m_gramdirtybytes[code] = 0;
                    }
                    // Draw GRAM char
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[1],
                        code,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                }
                else
                {
                    //if ((state->m_gramdirtybytes[code] == 1) || (state->m_gramdirtybytes[code+1] == 1))
                    {
                        decodechar(machine.gfx[1],
                           code,
                           state->m_gram,
                           machine.config()->gfxdecodeinfo[1].gfxlayout);
                        decodechar(machine.gfx[1],
                           code+1,
                           state->m_gram,
                           machine.config()->gfxdecodeinfo[1].gfxlayout);
                        state->m_gramdirtybytes[code] = 0;
                        state->m_gramdirtybytes[code+1] = 0;
                    }
                    // Draw GRAM char
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[1],
                        code,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16+(s->yflip)*s->ysize*8,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[1],
                        code+1,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16+(1-s->yflip)*s->ysize*8,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                }
            }
            else
            {
                if (s->yres == 1)
                {
                    // Draw GROM char
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[0],
                        code,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                }
                else
                {
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[0],
                        code,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16+(s->yflip)*s->ysize*8,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                    drawgfxzoom_transpen(bitmap,&machine.screen[0].visarea,machine.gfx[0],
                        code+1,
                        s->color,
                        s->xflip,s->yflip,
                        s->xpos*2-16,s->ypos*2-16+(1-s->yflip)*s->ysize*8,
                        0x8000*s->xsize, 0x8000*s->ysize,0);
                }
            }
        }
    }
}
#endif

static void draw_borders(running_machine &machine, bitmap_t *bm)
{
	intv_state *state = machine.driver_data<intv_state>();
	if (state->m_left_edge_inhibit)
		plot_box(bm, 0, 0, 16-state->m_col_delay*2, 16*12, (state->m_border_color<<1)+1);

	if (state->m_top_edge_inhibit)
		plot_box(bm, 0, 0, 16*20, 16-state->m_row_delay*2, (state->m_border_color<<1)+1);
}

void intv_stic_screenrefresh(running_machine &machine)
{
	intv_state *state = machine.driver_data<intv_state>();
	int i;

	if (state->m_stic_handshake != 0)
	{
		state->m_stic_handshake = 0;
		// Render the background
		render_background(machine, machine.generic.tmpbitmap);
		// Render the sprites into their buffers
		render_sprites(machine);
		for (i = 0; i < 8; i++) state->m_sprite[i].collision = 0;
		// Copy the sprites to the background
		copy_sprites_to_background(machine, machine.generic.tmpbitmap);
		determine_sprite_collisions(state);
		for (i = 0; i < 8; i++) state->m_collision_registers[i] |= state->m_sprite[i].collision;
		/* draw the screen borders if enabled */
		draw_borders(machine, machine.generic.tmpbitmap);
	}
	else
	{
		/* STIC disabled, just fill with border color */
		bitmap_fill(machine.generic.tmpbitmap, NULL, (state->m_border_color<<1)+1);
	}
}


/* very rudimentary support for the tms9927 character generator IC */


 READ8_HANDLER( intvkbd_tms9927_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	UINT8 rv;
	switch (offset)
	{
		case 8:
			rv = state->m_tms9927_cursor_row;
			break;
		case 9:
			/* note: this is 1-based */
			rv = state->m_tms9927_cursor_col;
			break;
		case 11:
			state->m_tms9927_last_row = (state->m_tms9927_last_row + 1) % state->m_tms9927_num_rows;
			rv = state->m_tms9927_last_row;
			break;
		default:
			rv = 0;
	}
	return rv;
}

WRITE8_HANDLER( intvkbd_tms9927_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	switch (offset)
	{
		case 3:
			state->m_tms9927_num_rows = (data & 0x3f) + 1;
			break;
		case 6:
			state->m_tms9927_last_row = data;
			break;
		case 11:
			state->m_tms9927_last_row = (state->m_tms9927_last_row + 1) % state->m_tms9927_num_rows;
			break;
		case 12:
			/* note: this is 1-based */
			state->m_tms9927_cursor_col = data;
			break;
		case 13:
			state->m_tms9927_cursor_row = data;
			break;
	}
}

SCREEN_UPDATE( intvkbd )
{
	intv_state *state = screen->machine().driver_data<intv_state>();
	UINT8 *videoram = state->m_videoram;
	int x,y,offs;
	int current_row;
//  char c;

	/* Draw the underlying INTV screen first */
	SCREEN_UPDATE_CALL(generic_bitmapped);

	/* if the intvkbd text is not blanked, overlay it */
	if (!state->m_intvkbd_text_blanked)
	{
		current_row = (state->m_tms9927_last_row + 1) % state->m_tms9927_num_rows;
		for(y=0;y<24;y++)
		{
			for(x=0;x<40;x++)
			{
				offs = current_row*64+x;
				drawgfx_transpen(bitmap, NULL,
					screen->machine().gfx[1],
					videoram[offs],
					7, /* white */
					0,0,
					x<<3,y<<3, 0);
			}
			if (current_row == state->m_tms9927_cursor_row)
			{
				/* draw the cursor as a solid white block */
				/* (should use a filled rect here!) */
				drawgfx_transpen(bitmap, NULL,
					screen->machine().gfx[1],
					191, /* a block */
					7,   /* white   */
					0,0,
					(state->m_tms9927_cursor_col-1)<<3,y<<3, 0);
			}
			current_row = (current_row + 1) % state->m_tms9927_num_rows;
		}
	}

#if 0
	// debugging
	c = tape_motor_mode_desc[state->m_tape_motor_mode][0];
	drawgfx_transpen(bitmap,&machine.screen[0].visarea, machine.gfx[1],
		c,
		1,
		0,0,
		0*8,0*8, 0);
	for(y=0;y<5;y++)
	{
		drawgfx_transpen(bitmap,&machine.screen[0].visarea, machine.gfx[1],
			state->m_tape_unknown_write[y]+'0',
			1,
			0,0,
			0*8,(y+2)*8, 0);
	}
	drawgfx_transpen(bitmap,&machine.screen[0].visarea, machine.gfx[1],
			state->m_tape_unknown_write[5]+'0',
			1,
			0,0,
			0*8,8*8, 0);
	drawgfx_transpen(bitmap,&machine.screen[0].visarea, machine.gfx[1],
			state->m_tape_interrupts_enabled+'0',
			1,
			0,0,
			0*8,10*8, 0);
#endif
	return 0;
}
