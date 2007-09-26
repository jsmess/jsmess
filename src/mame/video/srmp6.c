/*
    Super Real Mahjong P6 (JPN Ver.)
    (c)1996 Seta

WIP driver by Sebastien Volpe

according prg ROM (offset $0fff80):

    S12 SYSTEM
    SUPER REAL MAJAN P6
    SETA CO.,LTD
    19960410
    V1.00

TODO:
 - gfx decoding / emulation
 - sound emulation (appears to be really close to st0016)

Are there other games on this 'System S12' hardware ???

---------------- dump infos ----------------

[Jun/15/2000]

Super Real Mahjong P6 (JPN Ver.)
(c)1996 Seta

SX011
E47-REV01B

CPU:    68000-16
Sound:  NiLe
OSC:    16.0000MHz
        42.9545MHz
        56.0000MHz

Chips:  ST-0026 NiLe (video, sound)
        ST-0017


SX011-01.22  chr, samples (?)
SX011-02.21
SX011-03.20
SX011-04.19
SX011-05.18
SX011-06.17
SX011-07.16
SX011-08.15

SX011-09.10  68000 data

SX011-10.4   68000 prg.
SX011-11.5


Dumped 06/15/2000
*/

#include "driver.h"
#include "srmp6.h"

/***************************************************************************
    vidhrdw - VERY preliminary
***************************************************************************/

/* sprite RAM format: 8 words, 1st word = 0x8000 means end of list

Offset:         Format:                     Value:

0000.w
                    f--- ---- ---- ----     End of list
                    -??? ???? ???? ???-
                    ---- ---- ---- ---0     Sprite visible?

0002.w                                      Code
0004.w                                      X Position
0006.w                                      Y Position

0008.w                                      attributes/color?
000a.w                                      attributes/color?

000c.l                                      used ?

cpu #0 (PC=00011B2A): unmapped program memory word write to 00400490 = 0001 & FFFF - sprite visible ?
cpu #0 (PC=00011B2E): unmapped program memory word write to 00400492 = 0233 & FFFF - code
cpu #0 (PC=00011B32): unmapped program memory word write to 00400494 = 00D8 & FFFF - x
cpu #0 (PC=00011B36): unmapped program memory word write to 00400496 = 0080 & FFFF - y
cpu #0 (PC=00011B3A): unmapped program memory word write to 00400498 = E000 & FFFF - attributes/color?
cpu #0 (PC=00011B3E): unmapped program memory word write to 0040049A = 0000 & FFFF - attributes/color?
cpu #0 (PC=00011B40): unmapped program memory word write to 0040049C = 0000 & FFFF -\
cpu #0 (PC=00011B40): unmapped program memory word write to 0040049E = 0000 & FFFF -/ used ?

cpu #0 (PC=00011B4E): unmapped program memory word write to 004004B0 = 8000 & FFFF - end of list
*/

/* tile RAM format: where's color ?

cpu #0 (PC=000118E8): unmapped program memory word write to 00412002 = 0005 & FFFF - kind of init, interleaved with 'meaningful' words
cpu #0 (PC=000118EE): unmapped program memory word write to 00412008 = 0000 & FFFF
cpu #0 (PC=000118F4): unmapped program memory word write to 0041200A = 0000 & FFFF
...
cpu #0 (PC=000118E8): unmapped program memory word write to 00416EB2 = 0005 & FFFF
cpu #0 (PC=000118EE): unmapped program memory word write to 00416EB8 = 0000 & FFFF
cpu #0 (PC=000118F4): unmapped program memory word write to 00416EBA = 0000 & FFFF


cpu #0 (PC=0001170A): unmapped program memory word write to 00412000 = 4004 & FFFF
cpu #0 (PC=00011712): unmapped program memory word write to 00412004 = 0000 & FFFF
cpu #0 (PC=0001171A): unmapped program memory word write to 00412006 = 0000 & FFFF
...
cpu #0 (PC=0001170A): unmapped program memory word write to 004133A0 = 4004 & FFFF - tile code [*]
cpu #0 (PC=00011712): unmapped program memory word write to 004133A4 = 0140 & FFFF - x (00..140, step 16d) -> res = 320d
cpu #0 (PC=0001171A): unmapped program memory word write to 004133A6 = 00E0 & FFFF - y (00..F0, step 16d)  -> res = 240d

[*] tile code is 'incremented by 4', suggesting there are 4 8x8 tiles arranged to make a 16x16 one ???
*/

UINT16 *sprite_ram; // Sprite RAM 0x400000-0x47dfff
UINT16 *sprite_ram_old; // Sprite RAM(previous frame)
UINT16 *dec_regs;   //graphics decode register
UINT16 *video_regs, *sound_regs;

int brightness;	    //fade in/out

/***************************************************************************
    Graphics definitions
***************************************************************************/

gfx_layout srmp6_layout_8x8 =
{
    8,8,
    0x8000 / 4,
    8,
    { STEP8(0,1)},
    { STEP8(0,8)},
    { STEP8(0,8*8)},
    8*8*8
};

//fade in/out
void update_palette(void)
{
	char r, g ,b;
	int brg = brightness - 0x60;
	int i;

	for(i = 0; i < 0x800; i++)
	{
		r = paletteram16[i] >>  0 & 0x1F;
		g = paletteram16[i] >>  5 & 0x1F;
		b = paletteram16[i] >> 10 & 0x1F;

		if(brg < 0) {
			r += (r * brg) >> 5;
			if(r < 0) r = 0;
			g += (g * brg) >> 5;
			if(g < 0) g = 0;
			b += (b * brg) >> 5;
			if(b < 0) b = 0;
		}
		else if(brg > 0) {
			r += ((0x1F - r) * brg) >> 5;
			if(r > 0x1F) r = 0x1F;
			g += ((0x1F - g) * brg) >> 5;
			if(g > 0x1F) g = 0x1F;
			b += ((0x1F - b) * brg) >> 5;
			if(b > 0x1F) b = 0x1F;
		}
		palette_set_color(Machine, i, MAKE_RGB(r << 3, g << 3, b << 3));
	}
}

VIDEO_START(srmp6)
{
	int i;

	sprite_ram_old = auto_malloc(0x7E000);
	memset(sprite_ram_old, 0x00, 0x7E000);

	brightness = 0x60;

	for(i=0;i<4;i++)
	{
		if(Machine->gfx[i])
			freegfx(Machine->gfx[i]);
		Machine->gfx[i] = allocgfx(&srmp6_layout_8x8);
		if (!Machine->gfx[i])
			return;
		decodegfx(Machine->gfx[i], memory_region(REGION_GFX1) + (i << 19), 0, Machine->gfx[i]->total_elements);
		Machine->gfx[i]->color_base = 0;
		Machine->gfx[i]->total_colors = 0x800;
	}
}

VIDEO_UPDATE(srmp6)
{
	int trans = TRANSPARENCY_PEN;
	UINT16 *sprite_list = sprite_ram_old;
	UINT16 *finish = sprite_list+0x1000;

	fillbitmap(bitmap, get_black_pen(machine), cliprect);

	// parse sprite list
	while( sprite_list < finish && sprite_list[0] != 0x8000)
	{
		UINT16 *char_list = sprite_ram_old + sprite_list[1] * 0x8;
		short char_count = sprite_list[0];
		short xpos = sprite_list[2];
		short ypos = sprite_list[3];
		int color = sprite_list[4]&0x07;

		if((sprite_list[5] & 0x700) == 0x700)
		{
			trans = TRANSPARENCY_ALPHA;
			alpha_set_level((sprite_list[5] & 0x1F) << 3);
		}
		else
		{
			trans = TRANSPARENCY_PEN;
			alpha_set_level(255);
		}

		while(char_count > 0)
		{
			int char_code, size_x, size_y, ax, ay;
			int x_offset, y_offset, flipx, flipy;

			char_code =  char_list[0];
			size_x = 1 << (char_list[1] & 0x03);
			size_y = 1 << (char_list[1] >> 2 & 0x03);
			flipx = (char_list[1] >> 8) & 1;
			flipy = (char_list[1] >> 9) & 1;

			x_offset = char_list[2];
			y_offset = char_list[3]-size_y*8;
			for(ax=0;ax<size_x;ax++) {
				for(ay=0;ay<size_y;ay++) {
					int bx,by;

					if(!flipx)
						bx = xpos + ax*8 + x_offset;
					else
						bx = xpos + (size_x-ax-1)*8 + x_offset;
					if(!flipy)
						by = ypos + ay*8 + y_offset;
					else
						by = ypos + (size_y-ay-1)*8 + y_offset;

					drawgfx(
						bitmap,
						Machine->gfx[char_code >> 13],
						char_code & 0x1fff,
						color,
						flipx,flipy,
						bx,by,
						cliprect,
						trans,0
					);
					char_code++;
				}
			}
			char_list += 8;
			char_count--;
		}
		sprite_list += 0x8;
	}

	memcpy(sprite_ram_old, sprite_ram, 0x7E000);
	return 0;
}

WRITE16_HANDLER( video_regs_w )
{
	switch(offset)
	{
		case 0x5e/2: // bank switch, used by ROM check
			logerror("Bankswitch %02X\n", data&0x0f);
			memory_set_bankptr(1,(UINT16 *)(memory_region(REGION_USER2) + (data & 0x0f)*0x200000));
			break;


		// set by IT4
		case 0x5c/2: // either 0x40 explicitely in many places, or according $2083b0 (IT4)
			//fade in/out (0x40(dark)-0x60(normal)-0x7e?(bright) reset by 0x00?
			data = (!data)?0x60:(data == 0x5e)?0x60:data;
			if(brightness != data) {
				brightness = data;
				update_palette();
			}
			break;
		/* unknown registers - there are others */

		// set by IT4 (jsr $b3c), according flip screen dsw
		case 0x48/2: //     0 /  0xb0 if flipscreen
		case 0x52/2: //     0 / 0x2ef if flipscreen
		case 0x54/2: // 0x152 / 0x15e if flipscreen

		// set by IT4 ($82e-$846)
		case 0x56/2: // written 8,9,8,9 successively
			break;
		case 0x5a/2:
			// graphics decode register write enable/disable flag?
			/* disable by 0x00, enable by 0x01 */
			/* the game program writes 0x02 to 0x4c000c */
			/* to disable register write while decoding*/
			/* currently it's meaningless as we decode frame by frame */
			video_regs[0x0c/2] = data <<1;
			break;
		default:
//          logerror("video_regs_w (PC=%06X): %04x = %04x & %04x\n", activecpu_get_previouspc(), offset*2, data, mem_mask);
			break;
	}
	COMBINE_DATA(&video_regs[offset]);
}

READ16_HANDLER( video_regs_r )
{
	return video_regs[offset];
}

//decode the graphic data
void decode_gfx( UINT32 table_address, UINT32 data_address, UINT32 gfx_size, UINT32 mem_offset)
{
	UINT8  *src   = memory_region(REGION_USER3) , *dst = memory_region(REGION_GFX1) + mem_offset;
	UINT16 *table = (UINT16 *)(src + table_address);
	UINT8  *data  = src + data_address;
	UINT8  *end   = dst + gfx_size;

	UINT8 loop_count, block_size, block_count, block[0x10];
	UINT32 pal, prev_pal=~0, table_flag, pal_flag=0;

	while(1)
	{
		block_size=0;

		table_flag = *(data++) | 0x100;
		while(!(table_flag & 0x10000)) {
			if(table_flag & 0x80) {
				*((UINT16 *)&block[block_size]) = table[*(data++)];
				block_size+=2;
			}
			else {
				block[block_size++]= *(data++);
			}
			table_flag<<=1;
		}

		block_count = 0;
		while(block_count != block_size) {
			pal = block[block_count++];
			if(pal_flag) {
				loop_count = pal + 1;
				pal = prev_pal;
				while(loop_count) {
					*(dst++) = pal; loop_count--;
					if(dst == end) return;
				}
				pal_flag = 0;
			}
			else {
				*(dst++) = pal;
				if(dst == end) return;
				if(pal == prev_pal) {
					if(block_count < block_size) {
						loop_count = block[block_count++] + 1;
						while(loop_count) {
							*(dst++) = pal; loop_count--;
							if(dst == end) return;
						}
					}
					else
						pal_flag=1;
				}
				prev_pal = pal;
			}
		}
	}
}

WRITE16_HANDLER( gfx_decode_w )
{
	int i;
	int table_address, table_size, data_address, gfx_size, page_index;
	COMBINE_DATA(&dec_regs[offset]);

	// check if write protection of graphic decode memory is disabled
	if(offset == 0x0d && video_regs[0x5a/2] == 0x0001) {
		table_size    = dec_regs[0] + 1;
		table_address = ((dec_regs[5] << 16) | dec_regs[4]) << 1;
		gfx_size      = (((dec_regs[7] & 3) << 16) | (dec_regs[6]+1)) << 2;
		page_index    = dec_regs[9] >> 1;
		data_address  = ((dec_regs[11] << 16) | dec_regs[10]) << 1;

		if(( table_address >= 0xc00000 && table_address < 0x2000000) &&
		   ( data_address  >= 0xc00000 && data_address  < 0x2000000) &&
		   ( table_size    >  0x000000 && table_size    < 0x81))
		{
/*
            logerror("number of word table?@?@?@?@?@?@%08X\n",   table_size);
            logerror("word table address?@?@?@%08X\n",   table_address);
            logerror("decode data size?@?@?@?@%08X\n",   gfx_size);
            logerror("decode page index?@%08X\n",   page_index);
            logerror("decode data address?@?@?@%08X\n\n", data_address);
*/
			decode_gfx( table_address - 0xc00000, data_address - 0xc00000, gfx_size, page_index << 19);

			for(i=0; gfx_size >= 0; gfx_size -= 0x80000, i++)
			{
				freegfx(Machine->gfx[page_index+i]);
				Machine->gfx[page_index+i] = allocgfx(&srmp6_layout_8x8);
				if (!Machine->gfx[page_index+i])
					return;
				decodegfx(Machine->gfx[page_index+i], memory_region(REGION_GFX1) + ((page_index+i) << 19), 0, Machine->gfx[page_index+i]->total_elements);
				Machine->gfx[page_index+i]->color_base = 0;
				Machine->gfx[page_index+i]->total_colors = 0x800;
			}
		}
	}
}

READ16_HANDLER( srmp6_sprite_r )
{
	return sprite_ram[offset];
}

WRITE16_HANDLER( srmp6_sprite_w )
{
	COMBINE_DATA(&sprite_ram[offset]);
}

WRITE16_HANDLER(paletteram_w)
{
	char r, g, b;
	int brg = brightness - 0x60;

	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset, data, mem_mask);

	if(brg)
	{
		r = data >>  0 & 0x1F;
		g = data >>  5 & 0x1F;
		b = data >> 10 & 0x1F;

		if(brg < 0) {
			r += (r * brg) >> 5;
			if(r < 0) r = 0;
			g += (g * brg) >> 5;
			if(g < 0) g = 0;
			b += (b * brg) >> 5;
			if(b < 0) b = 0;
		}
		else if(brg > 0) {
			r += ((0x1F - r) * brg) >> 5;
			if(r > 0x1F) r = 0x1F;
			g += ((0x1F - g) * brg) >> 5;
			if(g > 0x1F) g = 0x1F;
			b += ((0x1F - b) * brg) >> 5;
			if(b > 0x1F) b = 0x1F;
		}

		palette_set_color(Machine, offset, MAKE_RGB(r << 3, g << 3, b << 3));
	}
}

