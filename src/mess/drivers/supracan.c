/***************************************************************************

Skeleton driver file for Super A'Can console.

The system unit contains a reset button.

Controllers:
- 4 directional buttons
- A, B, X, Y, buttons
- Start, select buttons
- L, R shoulder buttons

f00000 - bit 15 is probably vblank bit.

Known unemulated graphical effects:
- The green blob of the A'Can logo should slide out from beneath the word
  "A'Can";
- Unemulated 1bpp ROZ mode, used by the Super A'Can BIOS logo;
- Priorities;
- Boom Zoo: missing window effect applied on sprites;
- Sango Fighter: Missing rowscroll effect;
- Sango Fighter: sprites doesn't get drawn on the far right of the screen;
- Sango Fighter: sprites have some bad gaps of black;
- Sango Fighter: Missing black masking on the top-down edges of the screen on gameplay?
- Sango Fighter: intro looks bogus, dunno what's supposed to draw...
- Super Taiwanese Baseball League: Missing window effect applied on a tilemap;
- Super Taiwanese Baseball League: Unemulated paging mode;
- Super Dragon Force: sprite y masking limit looks wrong;
- Super Dragon Force: priority issues with the text;
- Super Dragon Force: wrong ysize on character select screen;

baseball game debug trick:
wpset e90020,1f,w
do pc=5ac40
...
do pc=5acd4
wpclear
bp 0269E4
[ff7be4] <- 0x269ec
bpclear

***************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "debugger.h"

typedef struct _acan_dma_regs_t acan_dma_regs_t;
struct _acan_dma_regs_t
{
	UINT32 source[2];
	UINT32 dest[2];
	UINT16 count[2];
	UINT16 control[2];
};

typedef struct _acan_sprdma_regs_t acan_sprdma_regs_t;
struct _acan_sprdma_regs_t
{
	UINT32 src;
	UINT32 dst;
	UINT16 count;
	UINT16 control;
};

class supracan_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, supracan_state(machine)); }

	supracan_state(running_machine &machine) { }

	acan_dma_regs_t acan_dma_regs;
	acan_sprdma_regs_t acan_sprdma_regs;

	UINT16 m6502_reset;
	UINT8 *soundram;
	UINT16 *soundram_16;
	emu_timer *video_timer;
	UINT16 *vram;
	UINT16 spr_limit;
	UINT32 spr_base_addr;
	UINT8 spr_flags;
	UINT32 tilemap_base_addr[4];
	int tilemap_scrollx[4],tilemap_scrolly[4];
	UINT16 video_flags;
	UINT16 tilemap_flags[4],tilemap_mode[4];
	UINT16 irq_mask;

	UINT32 roz_base_addr;
	UINT16 roz_mode;
	UINT32 roz_scrollx, roz_scrolly;
	UINT16 roz_tile_bank;

	UINT16 video_regs[256];
};

static READ16_HANDLER( supracan_unk1_r );
static WRITE16_HANDLER( supracan_soundram_w );
static WRITE16_HANDLER( supracan_sound_w );
static READ16_HANDLER( supracan_video_r );
static WRITE16_HANDLER( supracan_video_w );
static WRITE16_HANDLER( supracan_char_w );

#define VERBOSE_LEVEL	(99)

#define ENABLE_VERBOSE_LOG (1)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%06x: %s", cpu_get_pc(devtag_get_device(machine, "maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

static VIDEO_START( supracan )
{

}

static void draw_tilemap(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect,int layer)
{
	supracan_state *state = (supracan_state *)machine->driver_data;
	UINT16 *supracan_vram = state->vram;
	UINT32 count;
	int x,y;
	int scrollx,scrolly;
	int region;
	int gfx_mode;
	int xsize,ysize;
	UINT16 tile_bank,pal_bank;

	count = (state->tilemap_base_addr[layer]);

	switch(state->tilemap_flags[layer] & 0x0f00)
	{
		case 0x600: xsize = 64; ysize = 32; break;
		case 0xa00:	xsize = 128; ysize = 32; break;
		case 0xc00: xsize = 64; ysize = 64; break;
		default: xsize = 32; ysize = 32; break;
	}

	scrollx = (state->tilemap_scrollx[layer] & 0x800) ? ((state->tilemap_scrollx[layer] & (xsize*8-1)) - xsize*8) : (state->tilemap_scrollx[layer] & (xsize*8-1));
	scrolly = (state->tilemap_scrolly[layer] & 0x800) ? ((state->tilemap_scrolly[layer] & (ysize*8-1)) - ysize*8) : (state->tilemap_scrolly[layer] & (ysize*8-1));

	gfx_mode = (state->tilemap_mode[layer] & 0x7000) >> 12;

	switch(gfx_mode)
	{
		case 7:  region = 2; tile_bank = 0x1c00; pal_bank = 0x00; break;
		case 4:  region = 0; tile_bank = 0x400;  pal_bank = 0x00; break;
		case 2:  region = 1; tile_bank = 0x400;  pal_bank = 0x00; break;
		case 0:  region = 1; tile_bank = 0;      pal_bank = 0x00; break;
		default: region = 1; tile_bank = 0;      pal_bank = 0x00; break;
	}


	for (y=0;y<ysize;y++)
	{
		for (x=0;x<xsize;x++)
		{
			int tile, flipx, flipy, pal;
			tile = (supracan_vram[count] & 0x03ff) + tile_bank;
			flipx = (supracan_vram[count] & 0x0800) ? 1 : 0;
			flipy = (supracan_vram[count] & 0x0400) ? 1 : 0;
			pal = ((supracan_vram[count] & 0xf000) >> 12) + pal_bank;

			drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx,(y*8)-scrolly,0);
			if(state->tilemap_flags[layer] & 0x20) //wrap-around enable
			{
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx+xsize*8,(y*8)-scrolly,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx,(y*8)-scrolly+ysize*8,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx+xsize*8,(y*8)-scrolly+ysize*8,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx-xsize*8,(y*8)-scrolly,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx,(y*8)-scrolly-ysize*8,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx-xsize*8,(y*8)-scrolly-ysize*8,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx+xsize*8,(y*8)-scrolly-ysize*8,0);
				drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx-xsize*8,(y*8)-scrolly+ysize*8,0);
			}
			count++;
		}
	}
}

/* probably similar to the regular tilemap layers with extra features */
/* 0x0402 Boom Zoo */
/* 0x2603 / 0x6623 Super Dragon Force */
/* 0x4020 BIOS logo */
/* */
static void draw_roz(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	supracan_state *state = (supracan_state *)machine->driver_data;
	UINT16 *supracan_vram = state->vram;
	UINT32 roz_base_addr = state->roz_base_addr;
	UINT32 count;
	int x,y;
	int scrollx,scrolly;
	int region = 0;
//  int gfx_mode;
	int xsize,ysize;
	UINT16 tile_bank;

	switch(state->roz_mode & 3) //FIXME: fix gfx bpp order
	{
		case 0: return; //1bpp! Used on BIOS logo
		case 1: region = 2; break;
		case 2: region = 1; break;
		case 3: region = 0; break;
	}

	if((state->roz_mode & 0xe00) == 0xc00)
	{
		xsize = 64;
		ysize = 64;
	}
	else
	{
		xsize = ysize = state->roz_mode & 0x200 ? 64 : 32;
	}

	count = (0);
	scrollx = (state->roz_scrollx >> 8) & 0x1ff;
	scrolly = (state->roz_scrolly >> 8) & 0x1ff;

	//roz_mode & 0x20 enables roz capabilities

	tile_bank = (state->roz_tile_bank & 0xf000) >> 3; //FIXME: check this

	for (y=0;y<ysize;y++)
	{
		for (x=0;x<xsize;x++)
		{
			int tile, flipx, flipy, pal;
			tile = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x03ff) | tile_bank;
			flipx = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x0800) ? 1 : 0;
			flipy = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x0400) ? 1 : 0;
			pal = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0xf000) >> 12;

			drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx,(y*8)-scrolly,0);
			drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx+xsize*8,(y*8)-scrolly,0);
			drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx,(y*8)-scrolly+ysize*8,0);
			drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,pal,flipx,flipy,(x*8)-scrollx+xsize*8,(y*8)-scrolly+ysize*8,0);

			count++;
		}
	}
}

static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	supracan_state *state = (supracan_state *)machine->driver_data;
	UINT16 *supracan_vram = state->vram;
	int x, y;
	int xtile, ytile;
	int xsize, ysize;
	int bank;
	int spr_offs;
	int col;
	int i;
	int spr_base;
	int hflip,vflip;
	int region;
	//UINT8 *sprdata = (UINT8*)(&supracan_vram[0x1d000/2]);

	spr_base = state->spr_base_addr;
	region = (state->spr_flags & 1) ? 0 : 1; //8bpp : 4bpp

	/*
        [0]
        ---h hhh- ---- ---- Y size
        ---- ---- yyyy yyyy Y offset
        [1]
        bbbb ---- ---- ---- sprite offset bank
        ---- d--- ---- ---- horizontal direction
        ---- ---- ---- -www X size
        [2]
        zzz- ---- ---- ---- X shrinking
        ---- ---S xxxx xxxx X offset
        [3]
        oooo oooo oooo oooo sprite pointer
    */

	for(i=spr_base/2;i<(spr_base+(state->spr_limit*8))/2;i+=4)
	{
		x = supracan_vram[i+2] & 0x00ff;
		y = supracan_vram[i+0] & 0x00ff;
		spr_offs = supracan_vram[i+3] << 2;
		bank = (supracan_vram[i+1] & 0xf000) >> 12;
		hflip = (supracan_vram[i+1] & 0x0800) ? 1 : 0;
		vflip = (supracan_vram[i+1] & 0x0400) ? 1 : 0;

		/* FIXME: Sango Fighter doesn't like this, check why ... */
		if(supracan_vram[i+2] & 0x100)
			x-=256;

		if(supracan_vram[i+3] != 0)
		{
			xsize = 1 << (supracan_vram[i+1] & 7);
			ysize = ((supracan_vram[i+0] & 0x1e00) >> 9)+1;

			if(vflip)
			{
				for(ytile = (ysize - 1); ytile >= 0; ytile--)
				{
					if(hflip)
					{
						for(xtile = (xsize - 1); xtile >= 0; xtile--)
						{
							UINT16 data = supracan_vram[spr_offs/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
							int xflip = (data & 0x0800) ? 0 : 1;
							int yflip = (data & 0x0400) ? 0 : 1;
							col = (data & 0xf000) >> 12;
							drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,col,xflip,yflip,x+((xsize - 1) - xtile)*8,y+((ysize - 1) - ytile)*8,0);
						}
					}
					else
					{
						for(xtile = 0; xtile < xsize; xtile++)
						{
							UINT16 data = supracan_vram[spr_offs/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
							int xflip = (data & 0x0800) ? 1 : 0;
							int yflip = (data & 0x0400) ? 0 : 1;
							col = (data & 0xf000) >> 12;
							drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,col,xflip,yflip,x+xtile*8,y+((ysize - 1) - ytile)*8,0);
						}
					}
				}
			}
			else
			{
				for(ytile = 0; ytile < ysize; ytile++)
				{
					if(hflip)
					{
						for(xtile = (xsize - 1); xtile >= 0; xtile--)
						{
							UINT16 data = supracan_vram[spr_offs/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
							int xflip = (data & 0x0800) ? 0 : 1;
							int yflip = (data & 0x0400) ? 1 : 0;
							col = (data & 0xf000) >> 12;
							drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,col,xflip,yflip,x+((xsize - 1) - xtile)*8,y+ytile*8,0);
						}
					}
					else
					{
						for(xtile = 0; xtile < xsize; xtile++)
						{
							UINT16 data = supracan_vram[spr_offs/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
							int xflip = (data & 0x0800) ? 1 : 0;
							int yflip = (data & 0x0400) ? 1 : 0;
							col = (data & 0xf000) >> 12;
							drawgfx_transpen(bitmap,cliprect,machine->gfx[region],tile,col,xflip,yflip,x+xtile*8,y+ytile*8,0);
						}
					}
				}
			}
		}
	}
}

static VIDEO_UPDATE( supracan )
{
	supracan_state *state = (supracan_state *)screen->machine->driver_data;

	bitmap_fill(bitmap, cliprect, 0);

#if 0
	{
		count = 0x4400/2;

		for (y=0;y<64;y++)
		{
			for (x = 0; x < 64; x += 16)
			{
				static UINT16 dot_data;

				dot_data = supracan_vram[count] ^ 0xffff;

				*BITMAP_ADDR16(bitmap, y, 128+x+ 0) = ((dot_data >> 15) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 1) = ((dot_data >> 14) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 2) = ((dot_data >> 13) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 3) = ((dot_data >> 12) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 4) = ((dot_data >> 11) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 5) = ((dot_data >> 10) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 6) = ((dot_data >>  9) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 7) = ((dot_data >>  8) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 8) = ((dot_data >>  7) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+ 9) = ((dot_data >>  6) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+10) = ((dot_data >>  5) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+11) = ((dot_data >>  4) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+12) = ((dot_data >>  3) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+13) = ((dot_data >>  2) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+14) = ((dot_data >>  1) & 0x0001) + 0x20;
				*BITMAP_ADDR16(bitmap, y, 128+x+15) = ((dot_data >>  0) & 0x0001) + 0x20;
				count++;
			}
		}
	}
#endif

#if 0
	count = 0x4200/2;

	for (y=0;y<16;y++)
	{
		for (x=0;x<16;x++)
		{
			int tile;

			tile = (supracan_vram[count] & 0x01ff);
			drawgfx_transpen(bitmap,cliprect,screen->machine->gfx[0],tile,0,0,0,x*8,y*8,0);
			count++;
		}
	}
#endif

	if(state->video_flags & 0x20) //guess, not tested
		draw_tilemap(screen->machine,bitmap,cliprect,2);

	if((state->tilemap_flags[1] & 0x7000) < (state->tilemap_flags[0] & 0x7000)) //pri number?
	{
		if(state->video_flags & 0x80)
			draw_tilemap(screen->machine,bitmap,cliprect,0);
		if(state->video_flags & 0x40)
			draw_tilemap(screen->machine,bitmap,cliprect,1);
	}
	else
	{
		if(state->video_flags & 0x40)
			draw_tilemap(screen->machine,bitmap,cliprect,1);
		if(state->video_flags & 0x80)
			draw_tilemap(screen->machine,bitmap,cliprect,0);
	}
	if(state->video_flags & 8)
		draw_sprites(screen->machine,bitmap,cliprect);

	if(state->video_flags & 4)
		draw_roz(screen->machine,bitmap,cliprect);

	return 0;
}


static WRITE16_HANDLER( supracan_dma_w )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;
	acan_dma_regs_t *acan_dma_regs = &state->acan_dma_regs;
	int i;
	int ch;

	ch = (offset < 0x10/2) ? 0 : 1;

	switch(offset)
	{
		case 0x00/2: // Source address MSW
		case 0x10/2:
			acan_dma_regs->source[ch] &= 0x0000ffff;
			acan_dma_regs->source[ch] |= data << 16;
			break;
		case 0x02/2: // Source address LSW
		case 0x12/2:
			acan_dma_regs->source[ch] &= 0xffff0000;
			acan_dma_regs->source[ch] |= data;
			break;
		case 0x04/2: // Destination address MSW
		case 0x14/2:
			acan_dma_regs->dest[ch] &= 0x0000ffff;
			acan_dma_regs->dest[ch] |= data << 16;
			break;
		case 0x06/2: // Destination address LSW
		case 0x16/2:
			acan_dma_regs->dest[ch] &= 0xffff0000;
			acan_dma_regs->dest[ch] |= data;
			break;
		case 0x08/2: // Byte count
		case 0x18/2:
			acan_dma_regs->count[ch] = data;
			break;
		case 0x0a/2: // Control
		case 0x1a/2:
			//if(acan_dma_regs.dest != 0xf00200)
			printf("%08x %08x %02x %04x\n",acan_dma_regs->source[ch],acan_dma_regs->dest[ch],acan_dma_regs->count[ch] + 1,data);
			if(data & 0x8800)
			{
				//if(data != 0x9800 && data != 0x8800)
				//{
				//  fatalerror("%04x",data);
				//}
//              if(data & 0x2000)
//                  acan_dma_regs->source-=2;
				verboselog(space->machine, 0, "supracan_dma_w: Kicking off a DMA from %08x to %08x, %d bytes (%04x)\n", acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1, data);

				for(i = 0; i <= acan_dma_regs->count[ch]; i++)
				{
					if(data & 0x1000)
					{
						memory_write_word(space, acan_dma_regs->dest[ch], memory_read_word(space, acan_dma_regs->source[ch]));
						acan_dma_regs->dest[ch]+=2;
						acan_dma_regs->source[ch]+=2;
						if(data & 0x0100)
							if((acan_dma_regs->dest[ch] & 0xf) == 0)
								acan_dma_regs->dest[ch]-=0x10;
					}
					else
					{
						memory_write_byte(space, acan_dma_regs->dest[ch], memory_read_byte(space, acan_dma_regs->source[ch]));
						acan_dma_regs->dest[ch]++;
						acan_dma_regs->source[ch]++;
					}
				}
			}
			else
			{
				verboselog(space->machine, 0, "supracan_dma_w: Unknown DMA kickoff value of %04x (other regs %08x, %08x, %d)\n", data, acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1);
				fatalerror("supracan_dma_w: Unknown DMA kickoff value of %04x (other regs %08x, %08x, %d)",data, acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1);
			}
			break;
	}
}

static ADDRESS_MAP_START( supracan_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x3fffff ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xe80200, 0xe80201 ) AM_READ_PORT("P1")
	AM_RANGE( 0xe80202, 0xe80203 ) AM_READ_PORT("P2")
//  AM_RANGE( 0xe80204, 0xe80205 ) AM_READ( supracan_unk1_r ) //or'ed with player 2 inputs, why?
	AM_RANGE( 0xe80208, 0xe80209 ) AM_READ_PORT("P3")
	AM_RANGE( 0xe8020c, 0xe8020d ) AM_READ_PORT("P4")
	AM_RANGE( 0xe80300, 0xe80301 ) AM_READ( supracan_unk1_r )
	AM_RANGE( 0xe81000, 0xe8ffff ) AM_RAM_WRITE( supracan_soundram_w ) AM_BASE_MEMBER(supracan_state,soundram_16)
	AM_RANGE( 0xe90000, 0xe9001f ) AM_WRITE( supracan_sound_w )
	AM_RANGE( 0xe90020, 0xe9003f ) AM_WRITE( supracan_dma_w )
	AM_RANGE( 0xf00000, 0xf001ff ) AM_READWRITE( supracan_video_r, supracan_video_w )
	AM_RANGE( 0xf00200, 0xf003ff ) AM_RAM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE_GENERIC(paletteram)
	AM_RANGE( 0xf40000, 0xf5ffff ) AM_RAM_WRITE(supracan_char_w) AM_BASE_MEMBER(supracan_state,vram)	/* Unknown, some data gets copied here during boot */
//  AM_RANGE( 0xf44000, 0xf441ff ) AM_RAM AM_BASE_MEMBER(supracan_state,rozpal) /* clut data for the ROZ? */
//  AM_RANGE( 0xf44600, 0xf44fff ) AM_RAM   /* Unknown, stuff gets written here. Zoom table?? */
	AM_RANGE( 0xfc0000, 0xfcffff ) AM_MIRROR(0x30000) AM_RAM /* System work ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( supracan_sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x03ff ) AM_RAM
	AM_RANGE( 0x0411, 0x0411 ) AM_READ(soundlatch_r)
	AM_RANGE( 0x0420, 0x0420 ) AM_READNOP //AM_READ(status)
	AM_RANGE( 0x0420, 0x0420 ) AM_WRITENOP //AM_WRITE(data)
	AM_RANGE( 0x0422, 0x0422 ) AM_WRITENOP //AM_WRITE(address)
	AM_RANGE( 0x1000, 0xffff ) AM_RAM AM_BASE_MEMBER(supracan_state,soundram )
ADDRESS_MAP_END

static WRITE16_HANDLER( supracan_char_w )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;
	UINT16 *supracan_vram = state->vram;

	COMBINE_DATA(&supracan_vram[offset]);

	{
		UINT8 *gfx = memory_region(space->machine, "ram_gfx");

		gfx[offset*2+0] = (supracan_vram[offset] & 0xff00) >> 8;
		gfx[offset*2+1] = (supracan_vram[offset] & 0x00ff) >> 0;

		gfx_element_mark_dirty(space->machine->gfx[0], offset/32);
		gfx_element_mark_dirty(space->machine->gfx[1], offset/16);
		gfx_element_mark_dirty(space->machine->gfx[2], offset/8);
	}
}

static INPUT_PORTS_START( supracan )
	PORT_START("P1")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Button A")

	PORT_START("P2")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Button A")

	PORT_START("P3")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(3) PORT_NAME("P3 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(3) PORT_NAME("P3 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(3) PORT_NAME("P3 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3) PORT_NAME("P3 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3) PORT_NAME("P3 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3) PORT_NAME("P3 Button A")

	PORT_START("P4")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(4) PORT_NAME("P4 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(4) PORT_NAME("P4 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(4) PORT_NAME("P4 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(4) PORT_NAME("P4 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(4) PORT_NAME("P4 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(4) PORT_NAME("P4 Button A")
INPUT_PORTS_END


static PALETTE_INIT( supracan )
{
	// Used for debugging purposes for now
	//#if 0
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1f) << 3;
		g = ((i >> 5) & 0x1f) << 3;
		b = ((i >> 10) & 0x1f) << 3;
		palette_set_color_rgb( machine, i, r, g, b );
	}
	//#endif
}


static WRITE16_HANDLER( supracan_soundram_w )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;

	state->soundram_16[offset] = data;

	state->soundram[offset*2 + 1] = data & 0xff;
	state->soundram[offset*2 + 0] = data >> 8;
}


static READ16_HANDLER( supracan_unk1_r )
{
	/* No idea what hardware this is connected to. */
	return 0xffff;
}


static WRITE16_HANDLER( supracan_sound_w )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;

	switch ( offset )
	{
		case 0x000a/2:
			//soundlatch_w(space, 0, (data & 0xff00) >> 8);
			//cputag_set_input_line(space->machine, "soundcpu", 0, HOLD_LINE);
			break;
		case 0x001c/2:	/* Sound cpu control. Bit 0 tied to sound cpu RESET line */
			if ( data & 0x01 )
			{
				if ( ! state->m6502_reset )
				{
					/* Reset and enable the sound cpu */
					//cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, CLEAR_LINE);
					//devtag_get_device(space->machine, "soundcpu")->reset();
				}
			}
			else
			{
				/* Halt the sound cpu */
				cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
			}
			state->m6502_reset = data & 0x01;
			break;
		default:
			//printf("supracan_sound_w: writing %04x to unknown address %08x\n", data, 0xe90000 + offset * 2 );
			break;
	}
}


static READ16_HANDLER( supracan_video_r )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;
	UINT16 data = state->video_regs[offset];

	switch(offset)
	{
		case 0: // VBlank flag
			break;
		default:
			verboselog(space->machine, 0, "supracan_video_r: Unknown register: %08x (%04x & %04x)\n", 0xf00000 + (offset << 1), data, mem_mask);
			break;
	}

	return data;
}


static TIMER_CALLBACK( supracan_video_callback )
{
	supracan_state *state = (supracan_state *)machine->driver_data;
	int vpos = video_screen_get_vpos(machine->primary_screen);

	switch( vpos )
	{
	case 0:
		state->video_regs[0] &= 0x7fff;
		break;
	case 240:
		state->video_regs[0] |= 0x8000;
		break;
	}

	timer_adjust_oneshot( state->video_timer, video_screen_get_time_until_pos( machine->primary_screen, ( vpos + 1 ) & 0xff, 0 ), 0 );
}

/*
0x188/0x18a is global x/y offset for the roz?

f00018 is src
f00012 is dst
f0001c / f00016 are flags of some sort
f00010 is the size
f0001e is the trigger
*/
static WRITE16_HANDLER( supracan_video_w )
{
	supracan_state *state = (supracan_state *)space->machine->driver_data;
	acan_sprdma_regs_t *acan_sprdma_regs = &state->acan_sprdma_regs;
	int i;

	switch(offset)
	{
		case 0x18/2: // Source address MSW
			acan_sprdma_regs->src &= 0x0000ffff;
			acan_sprdma_regs->src |= data << 16;
			break;
		case 0x1a/2: // Source address LSW
			acan_sprdma_regs->src &= 0xffff0000;
			acan_sprdma_regs->src |= data;
			break;
		case 0x12/2: // Destination address MSW
			acan_sprdma_regs->dst &= 0x0000ffff;
			acan_sprdma_regs->dst |= data << 16;
			break;
		case 0x14/2: // Destination address LSW
			acan_sprdma_regs->dst &= 0xffff0000;
			acan_sprdma_regs->dst |= data;
			break;
		case 0x10/2: // Byte count
			acan_sprdma_regs->count = data;
			break;
		case 0x1e/2:
			printf("* %08x %08x %04x %04x\n",acan_sprdma_regs->src,acan_sprdma_regs->dst,acan_sprdma_regs->count,data);
			verboselog(space->machine, 0, "supracan_dma_w: Kicking off a DMA from %08x to %08x, %d bytes (%04x)\n", acan_sprdma_regs->src, acan_sprdma_regs->dst, acan_sprdma_regs->count + 1, data);

			/* TODO: what's 0x2000 and 0x4000 for? */
			if(data & 0x8000)
			{
				if(data & 0x2000 || data & 0x4000)
					acan_sprdma_regs->dst |= 0xf40000;

				for(i = 0; i <= acan_sprdma_regs->count; i++)
				{
					if(data & 0x0100) //dma 0x00 fill (or fixed value?)
					{
						memory_write_word(space, acan_sprdma_regs->dst, 0);
						acan_sprdma_regs->dst+=2;
						//memset(supracan_vram,0x00,0x020000);
					}
					else
					{
						memory_write_word(space, acan_sprdma_regs->dst, memory_read_word(space, acan_sprdma_regs->src));
						acan_sprdma_regs->dst+=2;
						acan_sprdma_regs->src+=2;
					}
				}
			}
			else
			{
				// ...
			}
			break;
		case 0x08/2:
			state->video_flags = data;
			{
				rectangle visarea = *video_screen_get_visible_area(space->machine->primary_screen);

				visarea.min_x = visarea.min_y = 0;
				visarea.max_y = 240 - 1;
				visarea.max_x = ((state->video_flags & 0x100) ? 320 : 256) - 1;
				video_screen_configure(space->machine->primary_screen, 348, 256, &visarea, video_screen_get_frame_period(space->machine->primary_screen).attoseconds);
			}
			break;
		case 0x20/2: state->spr_base_addr = data << 2; break;
		case 0x22/2: state->spr_limit = data+1; break;
		case 0x26/2: state->spr_flags = data; break;
		case 0x100/2: state->tilemap_flags[0] = data; break;
		case 0x104/2: state->tilemap_scrollx[0] = data; break;
		case 0x106/2: state->tilemap_scrolly[0] = data; break;
		case 0x108/2: state->tilemap_base_addr[0] = (data) << 1; break;
		case 0x10a/2: state->tilemap_mode[0] = data; break;
		case 0x120/2: state->tilemap_flags[1] = data; break;
		case 0x124/2: state->tilemap_scrollx[1] = data; break;
		case 0x126/2: state->tilemap_scrolly[1] = data; break;
		case 0x128/2: state->tilemap_base_addr[1] = (data) << 1; break;
		case 0x12a/2: state->tilemap_mode[1] = data; break;
		case 0x140/2: state->tilemap_flags[2] = data; break;
		case 0x144/2: state->tilemap_scrollx[2] = data; break;
		case 0x146/2: state->tilemap_scrolly[2] = data; break;
		case 0x148/2: state->tilemap_base_addr[2] = (data) << 1; break;
		case 0x14a/2: state->tilemap_mode[2] = data; break;

		/* 0x180-0x19f are roz tilemap related regs */
		case 0x180/2: state->roz_mode = data; break;
		case 0x184/2: state->roz_scrollx = (data << 16) | (state->roz_scrollx & 0xffff); break;
		case 0x186/2: state->roz_scrollx = (data) | (state->roz_scrollx & 0xffff0000); break;
		case 0x188/2: state->roz_scrolly = (data << 16) | (state->roz_scrolly & 0xffff); break;
		case 0x18a/2: state->roz_scrolly = (data) | (state->roz_scrolly & 0xffff0000); break;
		case 0x194/2: state->roz_base_addr = (data) << 1; break;
		case 0x196/2: state->roz_tile_bank = data; break; //tile bank

		case 0x1f0/2: //FIXME: this register is mostly not understood
			state->irq_mask = (data & 8) ? 0 : 1;
			break;
		default:
			verboselog(space->machine, 0, "supracan_video_w: Unknown register: %08x = %04x & %04x\n", 0xf00000 + (offset << 1), data, mem_mask);
			break;
	}
	state->video_regs[offset] = data;
}


static DEVICE_IMAGE_LOAD( supracan_cart )
{
	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if ( size > 0x400000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	return INIT_PASS;
}


static MACHINE_START( supracan )
{
	supracan_state *state = (supracan_state *)machine->driver_data;

	state->video_timer = timer_alloc( machine, supracan_video_callback, NULL );
}


static MACHINE_RESET( supracan )
{
	supracan_state *state = (supracan_state *)machine->driver_data;

	cputag_set_input_line(machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
	timer_adjust_oneshot( state->video_timer, video_screen_get_time_until_pos( machine->primary_screen, 0, 0 ), 0 );
	state->irq_mask = 0;
}

static const gfx_layout supracan_gfx8bpp =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

static const gfx_layout supracan_gfx4bpp =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static const gfx_layout supracan_gfx2bpp =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0,1 },
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static GFXDECODE_START( supracan )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx8bpp,   0, 1 )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx4bpp,   0, 0x10 )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx2bpp,   0, 0x40 )
GFXDECODE_END

static INTERRUPT_GEN( supracan_irq )
{
	supracan_state *state = (supracan_state *)device->machine->driver_data;

	if(state->irq_mask)
		cpu_set_input_line(device, 7, HOLD_LINE);
}

static MACHINE_DRIVER_START( supracan )

	MDRV_DRIVER_DATA( supracan_state )

	MDRV_CPU_ADD( "maincpu", M68000, XTAL_10_738635MHz )		/* Correct frequency unknown */
	MDRV_CPU_PROGRAM_MAP( supracan_mem )
	MDRV_CPU_VBLANK_INT("screen", supracan_irq)

	MDRV_CPU_ADD( "soundcpu", M6502, XTAL_3_579545MHz )		/* TODO: Verfiy actual clock */
	MDRV_CPU_PROGRAM_MAP( supracan_sound_mem )

	MDRV_MACHINE_START( supracan )
	MDRV_MACHINE_RESET( supracan )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS(XTAL_10_738635MHz/2, 348, 0, 256, 256, 0, 240 )	/* No idea if this is correct */

	MDRV_PALETTE_LENGTH( 32768 )
	MDRV_PALETTE_INIT( supracan )
	MDRV_GFXDECODE( supracan )

	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( supracan_cart )

	MDRV_VIDEO_START( supracan )
	MDRV_VIDEO_UPDATE( supracan )
MACHINE_DRIVER_END


ROM_START( supracan )
	ROM_REGION( 0x400000, "cart", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "ram_gfx", ROMREGION_ERASEFF )
ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT    COMPANY                  FULLNAME        FLAGS */
CONS( 1995, supracan,   0,      0,      supracan,   supracan, 0,      "Funtech Entertainment", "Super A'Can",  GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )


