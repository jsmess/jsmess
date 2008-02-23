/* Sega Megadrive / Genesis */

/*

Highlight calculation is wrong

Horizontal window not supported

various raster glitches eg road rash road gets corrupted - gunstar heroes - toejam and earl - contra hardcorps - lemmings

needs serious cleanups

based on cgfm docs

no battery ram etc.

vram fill not done

io bug? controls don't work in decap attack

*/

#include "driver.h"
#include "deprecat.h"
#include "includes/genesis.h"
#include "sound/2612intf.h"
#include "sound/sn76496.h"

static int oldscreenmode;

typedef struct
{
	UINT8   genesis_vdp_command_pending;

	UINT8   genesis_vdp_cmdmode;
	UINT32  genesis_vdp_cmdaddr;

	UINT16*  genesis_vdp_vram;
	UINT8*  genesis_vdp_vram_is_dirty;

	UINT16*  genesis_vdp_cram;
	UINT16*  genesis_vdp_vsram;
	UINT8*   genesis_vdp_regs;

	UINT8 genesis_vdp_index;
	UINT8 genesis_vdp_fill_mode;

	UINT8 dma_transfer_type;
	UINT32 dma_transfer_count;
	UINT32 dma_transfer_start;

	int irq4_counter;

	UINT8*   genesis_vdp_bg_render_buffer;
	UINT16*   genesis_vdp_sp_render_buffer;

	int sline;


} genvdp;

UINT16 *genesis_cartridge;
UINT16 *genesis_mainram;
UINT8 *genesis_z80ram;

int genesis_z80_bank_step;
UINT32 genesis_z80_bank_addr;
UINT32 genesis_z80_bank_partial;

int genesis_z80_is_reset;
int genesis_68k_has_z80_bus;

static genvdp genesis_vdp;

static const gfx_layout charlayout =
{
	8,8,
	0x800,
	4,
	{ 0, 1, 2, 3 },
	{  2*4, 3*4, 0*4, 1*4, 6*4, 7*4,4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};


void genesis_vdp_start (genvdp *current_vdp)
{
	int gfx_index=0;

	current_vdp -> genesis_vdp_vram = auto_malloc (0x10000);
	current_vdp -> genesis_vdp_vram_is_dirty = auto_malloc (0x800);
	current_vdp -> genesis_vdp_cram = auto_malloc (0x100);
	current_vdp -> genesis_vdp_vsram= auto_malloc (0x100);
	current_vdp -> genesis_vdp_regs = auto_malloc (0x20);

	current_vdp -> genesis_vdp_bg_render_buffer= auto_malloc (320);
	current_vdp -> genesis_vdp_sp_render_buffer = auto_malloc (2048);





	/* reset to some default states */
	current_vdp -> genesis_vdp_command_pending = 0;
	current_vdp -> genesis_vdp_cmdmode = 0;
	current_vdp -> genesis_vdp_cmdaddr = 0;

	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;

//	if (gfx_index == MAX_GFX_ELEMENTS)
//		return 1;

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[gfx_index] = allocgfx(&charlayout);
	decodegfx(Machine->gfx[gfx_index] , (UINT8 *)&current_vdp->genesis_vdp_vram, 0, 1);

	/* set the color information */
	Machine->gfx[gfx_index]->total_colors = 0x4;
	current_vdp -> genesis_vdp_index = gfx_index;

}


void genesis_vdp_draw_scanline (genvdp *current_vdp, int line)
{
	UINT32 *destline;
	int i;
	mame_bitmap *bitmap = tmpbitmap;

	destline = BITMAP_ADDR32(bitmap, line, 0);

	for (i = 0; i < 320; i++)
	{
		UINT8 pixel = (UINT16)current_vdp->genesis_vdp_bg_render_buffer[i];
		int r,g,b;
		int paldat;

		paldat = genesis_vdp.genesis_vdp_cram[pixel&0x3f];

		r = (paldat & 0x000e) <<1;
		g = (paldat & 0x00e0) >>3;
		b = (paldat & 0x0e00) >>7;

		if (pixel & 0x40)
		{
			r >>=1;
			g >>=1;
			b >>=1;
		}

		if (pixel & 0x80)
		{
		//	r >>=1;
		//	g >>=1;
		//	b >>=1;
		//	r |=0x10;
		//	g |=0x10;
		//	b |=0x10
			r <<=1;
			g <<=1;
			b <<=1;
			r&=0x1f;
			g&=0x1f;
			b&=0x1f;
		}

		destline[0] = (r<<(16+3))|(g<<(8+3))|(b<<(0+3));

	//	;
		destline++;
	}



}


void genesis_vdp_spritebuffer_scanline (genvdp *current_vdp, int line, int reqpri)
{
	const gfx_element *gfx = Machine->gfx[0];
	UINT8 *srcgfx = gfx->gfxdata;
	int shadowmode = genesis_vdp.genesis_vdp_regs[0x0c]&0x08;
	int sbr = genesis_vdp.genesis_vdp_regs[0x05]<<9;
	int spriteno = 0;
	UINT16* spritebase = &genesis_vdp.genesis_vdp_vram[sbr>>1];

//	Index + 0  :   ------yy yyyyyyyy
//  Index + 2  :   ----hhvv -lllllll
//	Index + 4  :   pccvhnnn nnnnnnnn
//  Index + 6  :   ------xx xxxxxxxx

	memset(genesis_vdp.genesis_vdp_sp_render_buffer, 0xffff, 1024);


	while (spriteno < 100)
	{
		int ypos = spritebase[0] & 0x01ff;
		int ysiz =(spritebase[1] & 0x0300)>>8;
		int xsiz =(spritebase[1] & 0x0c00)>>10;
		int link =(spritebase[1] & 0x007f)>>0;
		int tnum =(spritebase[2] & 0x07ff)>>0;
		int xflp =(spritebase[2] & 0x0800)>>11;
		int yflp =(spritebase[2] & 0x1000)>>12;
		int col  =(spritebase[2] & 0x6000)>>13;
		int pri  =(spritebase[2] & 0x8000)>>15;
		int xpos =(spritebase[3] & 0x01ff)>>0;

		xsiz++;
		ysiz++;

		ypos-=128;


		if ((line>=ypos) && (line<(ypos+ysiz*8)))
		{
			int xct;

			if (xpos == 0)
			{
			//	popmessage("xpos0!");
				return;
			}

			for (xct = 0; xct < xsiz*8; xct++)
			{
				int xp, chk;
				xp = (xpos)+xct;


				chk = current_vdp->genesis_vdp_sp_render_buffer[xp];

				if (chk == 0xffff)
				{

					xp-=128;

					if ((xp >=0) && (xp <320))
					{
						int dat; int sprline;
						int xof;

						sprline = line-ypos;
						if (yflp) sprline = (ysiz*8)-sprline-1;
						xof = xct;

						if (xflp) xof = (xsiz*8)-xct-1;


						dat = srcgfx[tnum*64+(xof&7)+((sprline&0x1f)*8)+  (((xof&0x18)>>3)*(ysiz*64)) ];

						if (dat)
						{
							if (pri==reqpri)
							{
								if (!shadowmode)
								{
									current_vdp->genesis_vdp_bg_render_buffer[xp] = (dat & 0x0f)|col<<4;
								}
								else
								{
									if (!pri) // low priority sprites in shadow mode
									{
										int coldat;
										coldat = (dat & 0x0f)|col<<4;

										if (coldat == 0x3e)
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = current_vdp->genesis_vdp_bg_render_buffer[xp] | 0x80;
										}
										else if (coldat == 0x3f)
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = current_vdp->genesis_vdp_bg_render_buffer[xp] | 0x40;
										}
										else if ((coldat == 0x0e) || (coldat == 0x1e) || (coldat == 0x2e))
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = coldat;
										}
										else
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = coldat|0x40;
										}

									}
									else // high priority sprites in shadow mode
									{
										int coldat;
										coldat = (dat & 0x0f)|col<<4;

										if (coldat == 0x3e)
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = current_vdp->genesis_vdp_bg_render_buffer[xp] | 0x80;
										}
										else if (coldat == 0x3f)
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = current_vdp->genesis_vdp_bg_render_buffer[xp] | 0x40;
										}
										else
										{
											current_vdp->genesis_vdp_bg_render_buffer[xp] = coldat;
										}


									}

								}
							}

							current_vdp->genesis_vdp_sp_render_buffer[xp+128]=spriteno;

						}
					}
				}

			}

		}

		if (link==0)
		{
			return;
		}


		spritebase = &genesis_vdp.genesis_vdp_vram[(sbr>>1)+link*4];

		spriteno++;
	}

}

/* this will render a scanline to a buffer using raw pen values
00-3f are normal pens
40-7f are shadow pens
80-bf are highlight pens
c0-ff are unused
*/

void genesis_vdp_render_scanline (genvdp *current_vdp, int line)
{
	const gfx_element *gfx = Machine->gfx[0];

	int _2tileblock;
	UINT8 *srcgfx = gfx->gfxdata;
	UINT8* renderto ;
	int scroll_xsize = genesis_vdp.genesis_vdp_regs[0x10]&0x3;
	int scroll_ysize = (genesis_vdp.genesis_vdp_regs[0x10]&0x30)>>4;

	int hscroll_base = genesis_vdp.genesis_vdp_regs[0x0d]<<10;
	UINT16 hscroll_line;
	UINT16 hscroll_tile;
	UINT16 vscroll_cols;
	UINT16 vscroll_tile;

	int scrolla_base = genesis_vdp.genesis_vdp_regs[0x02]<<10;
	int window_base = genesis_vdp.genesis_vdp_regs[0x03]<<10;
	int scrollb_base = genesis_vdp.genesis_vdp_regs[0x04]<<13;
	int shadowmode = genesis_vdp.genesis_vdp_regs[0x0c]&0x08;

	int window_size = 32;
	int scrwidth = 32;
	int bgcol;

	int win_down, win_right;
	int win_vpos, win_hpos;
	int block_is_not_vwindow;
	int block_is_not_hwindow;

	switch (genesis_vdp.genesis_vdp_regs[0x0c]&0x81)
	{
		case 0x00: // 32 cell
			scrwidth=32;
			window_size=32;
		break;
		case 0x01: // 40 cell corrupted
			scrwidth=40;
			window_size=64;
		break;
		case 0x80: // illegal!
			scrwidth=40;
			window_size=64;
		break;
		case 0x81: // 40 cell
			scrwidth=40;
			window_size=64;

		break;

	}


	switch (scroll_xsize)
	{
		case 0:scroll_xsize = 32;break;
		case 1:scroll_xsize = 64;break;
		case 2:scroll_xsize = 64;break;
		case 3:scroll_xsize = 128;break;
	}

	switch (scroll_ysize)
	{
		case 0:scroll_ysize = 32;break;
		case 1:scroll_ysize = 64;break;
		case 2:scroll_ysize = 64;break;
		case 3:scroll_ysize = 128;break;
	}

	/* Backdrop */
	bgcol = genesis_vdp.genesis_vdp_regs[0x07]&0x3f;

	if (shadowmode) bgcol |= 0x40;

	memset(genesis_vdp.genesis_vdp_bg_render_buffer, bgcol, 320);

	if (!(genesis_vdp.genesis_vdp_regs[0x01]&0x40)) return;

	/* Scroll B Low */
	renderto = current_vdp->genesis_vdp_bg_render_buffer;
	hscroll_line=0;
	switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x03) // linescroll mode
	{
		case 0: // full screen scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1];
			break;
		case 1: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + (line *2)];
			break;
		case 2: // cell scroll
//			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + ((line>>4) *2)];
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + ((line&0xf8) *2)];

			break;
		case 3: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + (line *2)];
			break;
	}

	hscroll_tile = hscroll_line>>3;
	hscroll_line %= 8;
	hscroll_tile &= scroll_xsize-1;

	for (_2tileblock = 0; _2tileblock < scrwidth; _2tileblock++)
	{
		int tiledat;
		int tileno,flipx,flipy,col,pri;
		UINT8* tileloc;
		int x;

		vscroll_cols = 0;
		switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x04) /* vscroll for this block */
		{
			case 0x00:
				vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+1]+line;
				break;
			case 0x04:
				vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+1+((_2tileblock>>1)*2)]+line;
				break;
		}

		vscroll_tile = vscroll_cols >>3;
		vscroll_cols %= 8;
		vscroll_tile &= scroll_ysize-1;


		/* first tile */
		tiledat =  genesis_vdp.genesis_vdp_vram[(scrollb_base/2)+  ((_2tileblock+hscroll_tile)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
		pri    = (tiledat&0x8000)>>15;

		if (!pri)
		{
			tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
			tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

			x=hscroll_line;
			while (x<8)
			{
				int dat;
				dat = tileloc[flipx?7-x:x];
				if (dat)
				{
					dat|=(col<<4);
					if (shadowmode) dat|=0x40;
					*renderto = dat;
				}
				x++;
				renderto++;
			}
		}
		else
		{
			renderto+= 8-hscroll_line;
		}

		/* second tile */
		tiledat =  genesis_vdp.genesis_vdp_vram[(scrollb_base/2)+  ((_2tileblock+hscroll_tile+1)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
		pri    = (tiledat&0x8000)>>15;

		if (!pri)
		{
			tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
			tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;
			x = 0;
			while (x<hscroll_line)
			{
				int dat;
				dat = tileloc[flipx?7-x:x];
				if (dat)
				{
					dat|=(col<<4);
					if (shadowmode) dat|=0x40;
					*renderto = dat;
				}
				x++;
				renderto++;
			}
		}
		else
		{
			renderto+= hscroll_line;
		}
	}

/*

 $11 - Window H Position
 -----------------------
 d7 - RIGT
 d6 - 0 (No effect)
 d5 - 0 (No effect)
 d4 - WHP5
 d3 - WHP4
 d2 - WHP3
 d1 - WHP2
 d0 - WHP1

 $12 - Window V Position
 -----------------------

 d7 - DOWN
 d6 - 0 (No effect)
 d5 - 0 (No effect)
 d4 - WVP4
 d3 - WVP3
 d2 - WVP2
 d1 - WVP1
 d0 - WVP0

 */

//	int windowlpos;
//	int windowrpos;

//	int windowtpos;
//	int windowbpos;
//	int windowwide=32;

//	if (genesis_vdp.genesis_vdp_regs[0x12]&0x80)
//	{
//		windowtpos = (genesis_vdp.genesis_vdp_regs[0x12]&0x1f)*8;
//		windowbpos = 224;
//	}
//	else
//	{
//		windowtpos = 0;
//		windowbpos = (genesis_vdp.genesis_vdp_regs[0x12]&0x1f)*8;
//	}

	win_down = (genesis_vdp.genesis_vdp_regs[0x12]&0x80);
	win_vpos = (genesis_vdp.genesis_vdp_regs[0x12]&0x1f)*8;


	win_right = (genesis_vdp.genesis_vdp_regs[0x11]&0x80);
	win_hpos  = (genesis_vdp.genesis_vdp_regs[0x11]&0x1f)*2;



	/* Scroll A Low */
	renderto = current_vdp->genesis_vdp_bg_render_buffer;
	hscroll_line=0;
	switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x03) // linescroll mode
	{
		case 0: // full screen scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)];
			break;
		case 1: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + (line *2)];
			break;
		case 2: // cell scroll
//			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + ((line>>4) *2)];
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + ((line&0xf8) *2)];
			break;
		case 3: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + (line *2)];
			break;
	}

	hscroll_tile = hscroll_line>>3;
	hscroll_line %= 8;
	hscroll_tile &= scroll_xsize-1;

	for (_2tileblock = 0; _2tileblock < scrwidth; _2tileblock++)
	{
		int tiledat;
		int tileno,flipx,flipy,col,pri;
		UINT8* tileloc;
		int x;

		block_is_not_vwindow = (
			((!win_down)  && (line>=win_vpos)) ||
			(( win_down)  && (line< win_vpos))
			);

		block_is_not_hwindow = (
			((!win_right)  && (_2tileblock>=win_hpos)) ||
			(( win_right)  && (_2tileblock< win_hpos))
			);

//		if ((line<=windowtpos) && (line>=windowtpos))
		if (block_is_not_vwindow && block_is_not_hwindow)
		{
			vscroll_cols = 0;
			switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x04) /* vscroll for this block */
			{
				case 0x00:
					vscroll_cols = genesis_vdp.genesis_vdp_vsram[0]+line;
					break;
				case 0x04:
					vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+((_2tileblock>>1)*2)]+line;
					break;
			}
			vscroll_tile = vscroll_cols >>3;
			vscroll_cols %= 8;
			vscroll_tile &= scroll_ysize-1;


			/* first tile */
			tiledat =  genesis_vdp.genesis_vdp_vram[(scrolla_base/2)+  ((_2tileblock+hscroll_tile)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
			pri    = (tiledat&0x8000)>>15;

			if (!pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

				x=hscroll_line;
				while (x<8)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						dat|=(col<<4);
						if (shadowmode) dat|=0x40;
						*renderto = dat;
					}
					x++;
					renderto++;
				}
			}
			else
			{
				renderto+= 8-hscroll_line;
			}

			/* second tile */
			tiledat =  genesis_vdp.genesis_vdp_vram[(scrolla_base/2)+  ((_2tileblock+hscroll_tile+1)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
			pri    = (tiledat&0x8000)>>15;

			if (!pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;
				x = 0;
				while (x<hscroll_line)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						dat|=(col<<4);
						if (shadowmode) dat|=0x40;
						*renderto = dat;
					}
					x++;
					renderto++;
				}
			}
			else
			{
				renderto+= hscroll_line;
			}
		}
		else // window !*!
		{
			vscroll_cols = line;
			vscroll_tile = vscroll_cols >>3;
			vscroll_cols %= 8;

			/* only tile (window has no scrolls) */
			tiledat =  genesis_vdp.genesis_vdp_vram[(window_base/2)+ _2tileblock  +   (vscroll_tile*window_size)];
			pri    = (tiledat&0x8000)>>15;


			if (!pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

				x=0;
				while (x<8)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						dat|=(col<<4);
						if (shadowmode) dat|=0x40;
						*renderto = dat;
					}
					x++;
					renderto++;
				}
			}
			else
			{
				renderto+= 8;
			}

//			x=0;
//			while (x<8)
//			{
//				*renderto = mame_rand()&0x3f;
//				x++;renderto++;
//			}
		}
	}

	genesis_vdp_spritebuffer_scanline(current_vdp, line, 0);

	/* Scroll B High */
	renderto = current_vdp->genesis_vdp_bg_render_buffer;
	hscroll_line=0;
	switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x03) // linescroll mode
	{
		case 0: // full screen scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1];
			break;
		case 1: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + (line *2)];
			break;
		case 2: // cell scroll
//			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + ((line>>4) *2)];
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + ((line&0xf8) *2)];
			break;
		case 3: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)+1 + (line *2)];
			break;
	}

	hscroll_tile = hscroll_line>>3;
	hscroll_line %= 8;
	hscroll_tile &= scroll_xsize-1;

	for (_2tileblock = 0; _2tileblock < scrwidth; _2tileblock++)
	{
		int tiledat;
		int tileno,flipx,flipy,col,pri;
		UINT8* tileloc;
		int x;

		vscroll_cols = 0;
		switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x04) /* vscroll for this block */
		{
			case 0x00:
				vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+1]+line;
				break;
			case 0x04:
				vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+1+((_2tileblock>>1)*2)]+line;
				break;
		}

		vscroll_tile = vscroll_cols >>3;
		vscroll_cols %= 8;
		vscroll_tile &= scroll_ysize-1;


		/* first tile */
		tiledat =  genesis_vdp.genesis_vdp_vram[(scrollb_base/2)+  ((_2tileblock+hscroll_tile)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
		pri    = (tiledat&0x8000)>>15;

		if (pri)
		{
			tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
			tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

			x=hscroll_line;
			while (x<8)
			{
				int dat;
				dat = tileloc[flipx?7-x:x];
				if (dat)
				{
					*renderto = dat|(col<<4);
				}
				else if (shadowmode)
				{
					*renderto &=0x3f;
				}
				x++;
				renderto++;
			}
		}
		else
		{
			renderto+= 8-hscroll_line;
		}

		/* second tile */
		tiledat =  genesis_vdp.genesis_vdp_vram[(scrollb_base/2)+  ((_2tileblock+hscroll_tile+1)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
		pri    = (tiledat&0x8000)>>15;

		if (pri)
		{
			tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
			tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;
			x = 0;
			while (x<hscroll_line)
			{
				int dat;
				dat = tileloc[flipx?7-x:x];
				if (dat)
				{
					*renderto = dat|(col<<4);
				}
				else if (shadowmode)
				{
					*renderto &=0x3f;
				}

				x++;
				renderto++;
			}
		}
		else
		{
			renderto+= hscroll_line;
		}
	}

	/* Scroll A High */
	renderto = current_vdp->genesis_vdp_bg_render_buffer;
	hscroll_line=0;
	switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x03) // linescroll mode
	{
		case 0: // full screen scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1)];
			break;
		case 1: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + (line *2)];
			break;
		case 2: // cell scroll
//			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + ((line>>4) *2)];
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + ((line&0xf8) *2)];
			break;
		case 3: // line scroll
			hscroll_line = -genesis_vdp.genesis_vdp_vram[(hscroll_base>>1) + (line *2)];
			break;
	}

	hscroll_tile = hscroll_line>>3;
	hscroll_line %= 8;
	hscroll_tile &= scroll_xsize-1;

	for (_2tileblock = 0; _2tileblock < scrwidth; _2tileblock++)
	{
		int tiledat;
		int tileno,flipx,flipy,col,pri;
		UINT8* tileloc;
		int x;
		block_is_not_vwindow = (
			((!win_down)  && (line>=win_vpos)) ||
			(( win_down)  && (line< win_vpos))
			);

		block_is_not_hwindow = (
			((!win_right)  && (_2tileblock>=win_hpos)) ||
			(( win_right)  && (_2tileblock< win_hpos))
			);

	//	if () block_is_not_vwindow = 0;
//		if ((line<=windowtpos) && (line>=windowtpos))
		if (block_is_not_vwindow && block_is_not_hwindow)
		{

			vscroll_cols = 0;
			switch (genesis_vdp.genesis_vdp_regs[0x0b]&0x04) /* vscroll for this block */
			{
				case 0x00:
					vscroll_cols = genesis_vdp.genesis_vdp_vsram[0]+line;
					break;
				case 0x04:
					vscroll_cols = genesis_vdp.genesis_vdp_vsram[0+((_2tileblock>>1)*2)]+line;
					break;
			}

			vscroll_tile = vscroll_cols >>3;
			vscroll_cols %= 8;
			vscroll_tile &= scroll_ysize-1;


			/* first tile */
			tiledat =  genesis_vdp.genesis_vdp_vram[(scrolla_base/2)+  ((_2tileblock+hscroll_tile)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
			pri    = (tiledat&0x8000)>>15;

			if (pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

				x=hscroll_line;
				while (x<8)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						*renderto = dat|(col<<4);
					}
					else if (shadowmode)
					{
						*renderto &=0x3f;
					}				x++;
					renderto++;
				}
			}
			else
			{
				renderto+= 8-hscroll_line;
			}

			/* second tile */
			tiledat =  genesis_vdp.genesis_vdp_vram[(scrolla_base/2)+  ((_2tileblock+hscroll_tile+1)&(scroll_xsize-1))  +   (((vscroll_tile&(scroll_ysize-1))*scroll_xsize))];
			pri    = (tiledat&0x8000)>>15;

			if (pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;
				x = 0;
				while (x<hscroll_line)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						*renderto = dat|(col<<4);
					}
					else if (shadowmode)
					{
						*renderto &=0x3f;
					}				x++;
					renderto++;
				}
			}
			else
			{
				renderto+= hscroll_line;
			}
		}
		else // window !*!
		{
			vscroll_cols = line;
			vscroll_tile = vscroll_cols >>3;
			vscroll_cols %= 8;

			/* only tile (window has no scrolls) */
			tiledat =  genesis_vdp.genesis_vdp_vram[(window_base/2)+ _2tileblock  +   (vscroll_tile*window_size)];
			pri    = (tiledat&0x8000)>>15;


			if (pri)
			{
				tileno = (tiledat&0x07ff);flipx  = (tiledat&0x0800)>>11;flipy  = (tiledat&0x1000)>>12;col    = (tiledat&0x6000)>>13;
				tileloc = srcgfx + tileno*64 + (flipy?7-vscroll_cols:vscroll_cols)*8;

				x=0;
				while (x<8)
				{
					int dat;
					dat = tileloc[flipx?7-x:x];
					if (dat)
					{
						*renderto = dat|(col<<4);
					}
					else if (shadowmode)
					{
						*renderto &=0x3f;
					}
					x++;
					renderto++;
				}
			}
			else
			{
				renderto+= 8;
			}

//			x=0;
//			while (x<8)
//			{
//				*renderto = mame_rand()&0x3f;
//				x++;renderto++;
//			}
		}
	}

	genesis_vdp_spritebuffer_scanline(current_vdp, line, 1);


}


VIDEO_START(genesis)
{
	genesis_vdp_start (&genesis_vdp);
	oldscreenmode = 0xff; // driver specific
	VIDEO_START_CALL(generic_bitmapped);
}

VIDEO_UPDATE(genesis)
{
//	popmessage("rasterline %04x %i", genesis_vdp.genesis_vdp_regs[0x0a], genesis_vdp.genesis_vdp_regs[0x0a]);


/*
	popmessage("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x r%02x %02x %02x %02x %02x %02x",

	genesis_vdp.genesis_vdp_regs[0x00],
	genesis_vdp.genesis_vdp_regs[0x01],
	genesis_vdp.genesis_vdp_regs[0x02],
	genesis_vdp.genesis_vdp_regs[0x03],
	genesis_vdp.genesis_vdp_regs[0x04],
	genesis_vdp.genesis_vdp_regs[0x05],
	genesis_vdp.genesis_vdp_regs[0x06],
	genesis_vdp.genesis_vdp_regs[0x07],
	genesis_vdp.genesis_vdp_regs[0x08],
	genesis_vdp.genesis_vdp_regs[0x09],
	genesis_vdp.genesis_vdp_regs[0x0a],
	genesis_vdp.genesis_vdp_regs[0x0b],
	genesis_vdp.genesis_vdp_regs[0x0c],
	genesis_vdp.genesis_vdp_regs[0x0d],
	genesis_vdp.genesis_vdp_regs[0x0e],
	genesis_vdp.genesis_vdp_regs[0x0f]
	);
*/

//	popmessage("%02x %02x",genesis_vdp.genesis_vdp_regs[0x11], genesis_vdp.genesis_vdp_regs[0x12]);

	int i;
	for(i=0;i<0x40;i++)
	{
		int r,g,b;
		int paldat;

		paldat = genesis_vdp.genesis_vdp_cram[i];

		r = (paldat & 0x000e);
		g = (paldat & 0x00e0) >>4;
		b = (paldat & 0x0e00) >>8;

		palette_set_color_rgb(machine, i,r<<4,g<<4,b<<4);

	}

	VIDEO_UPDATE_CALL(generic_bitmapped);
	return 0;
}


READ16_HANDLER( genesis_68000_z80_read )
{
	offset <<= 1;

	if (!genesis_68k_has_z80_bus)
	{
		logerror("Attempting to read Z80 without bus!\n");
		return 0x00;
	}

	if ((offset >= 0) && (offset <= 0x3fff))
	{
		offset &=0x1fff; // due to mirror
	//	logerror("Read Mask %04x from Z80 offset %04x\n", mem_mask, offset);
		return memory_region(REGION_CPU2)[offset^1] | (memory_region(REGION_CPU2)[offset] <<8);

	}
	else if ((offset>=0x4000) && (offset<=0x5fff))
	{
//		logerror("Unhandled 68k->Z80 YM read\n");
		switch (offset & 3)
		{
			case 0:
				if (ACCESSING_MSB)	 return YM2612_status_port_0_A_r(0) << 8;
				else 				 return YM2612_read_port_0_r(0);
				break;
			case 2:
				if (ACCESSING_MSB)	return YM2612_status_port_0_B_r(0) << 8;
				else 				return 0;
				break;
		}

	}
	else
	{
		logerror("Unhandled 68k->Z80 read\n");
	}

	return 0x00;
}

WRITE16_HANDLER( genesis_68000_z80_write )
{
	offset <<= 1;

	if (!genesis_68k_has_z80_bus)
	{
		/* james pond 2 attempts this?, should it? */
	//	logerror("Attempting to write Z80 without bus!\n");
		return;
	}

	if ((offset >= 0) && (offset <= 0x3fff))
	{
		offset &=0x1fff; // due to mirror

//		logerror("Write Data %04x Mask %04x to Z80 offset %04x\n", data, mem_mask, offset);

		if ((ACCESSING_LSB) && (ACCESSING_MSB))
		{
		//	logerror("word access wrote to z80!\n");
			// ignore LSB
			memory_region(REGION_CPU2)[offset] = (data & 0xff00)>>8;

			return;
		}

		if (ACCESSING_LSB)
			memory_region(REGION_CPU2)[offset^1] = data & 0x00ff;

		if (ACCESSING_MSB)
			memory_region(REGION_CPU2)[offset] = (data & 0xff00)>>8;
	}
	else if ((offset>=0x4000) && (offset<=0x5fff))
	{
		switch (offset & 3)
		{
			case 0:
				if (ACCESSING_MSB)	YM2612_control_port_0_A_w	(0,	(data >> 8) & 0xff);
				else 				YM2612_data_port_0_A_w		(0,	(data >> 0) & 0xff);
				break;
			case 2:
				if (ACCESSING_MSB)	YM2612_control_port_0_B_w	(0,	(data >> 8) & 0xff);
				else 				YM2612_data_port_0_B_w		(0,	(data >> 0) & 0xff);
				break;
		}
	}
	else
	{
		logerror("Unhandled 68k->Z80 write\n");
	}
}

READ16_HANDLER( genesis_68000_z80_busreq_r )
{
//	logerror("genesis_68000_z80_busreq_r\n");
	if ((genesis_68k_has_z80_bus) && (!genesis_z80_is_reset)) return 0x0000;
	else return 0x0100;

}

WRITE16_HANDLER( genesis_68000_z80_busreq_w )
{
//	logerror("genesis_68000_z80_busreq_w %04x",data);
	// write 0100 requests z80 bus (z80 paused)
	if (data == 0x0000)
	{
		cpunum_set_input_line(machine, 1, INPUT_LINE_HALT,  CLEAR_LINE);
		genesis_68k_has_z80_bus = 0;
//			logerror("-- z80 running %04x\n",data);

	}

	if (data == 0x0100)
	{
		cpunum_set_input_line(machine, 1, INPUT_LINE_HALT,  ASSERT_LINE);
		genesis_68k_has_z80_bus = 1;
//			logerror("-- z80 stopped %04x\n",data);


	}

}

WRITE16_HANDLER ( genesis_68000_z80_reset_w )
{
	//logerror("genesis_68000_z80_reset_w %04x\n",data);
	// write 0 set reset line? (pause)
	// write 1 clear reset line? (run)

	if (data == 0x0000)
	{
		cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, ASSERT_LINE);
		genesis_z80_is_reset = 1;
	}

	if (data == 0x0100)
	{
		cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, CLEAR_LINE);
		genesis_z80_is_reset = 0;
	}
}

static UINT8 genesis_io_ram[0x20];

void genesis_init_io (void)
{

	genesis_io_ram[0x00] = (genesis_region & 0xc0)| (0x00 & 0x3f); // region / pal / segacd etc. important!
	genesis_io_ram[0x01] = 0x7f;
	genesis_io_ram[0x02] = 0x7f;
	genesis_io_ram[0x03] = 0x7f;
	genesis_io_ram[0x04] = 0x00;
	genesis_io_ram[0x05] = 0x00;
	genesis_io_ram[0x06] = 0x00;
	genesis_io_ram[0x07] = 0xff;
	genesis_io_ram[0x08] = 0x00;
	genesis_io_ram[0x09] = 0x00;
	genesis_io_ram[0x0a] = 0xff;
	genesis_io_ram[0x0b] = 0x00;
	genesis_io_ram[0x0c] = 0x00;
	genesis_io_ram[0x0d] = 0xfb;
	genesis_io_ram[0x0e] = 0x00;
	genesis_io_ram[0x0f] = 0x00;


  /*
  	$A10001 = $80 (Bits 7,6,5 depend on the domestic/export, PAL/NTSC jumpers and having a Sega CD or not)
    $A10003 = $7F
    $A10005 = $7F
    $A10007 = $7F
    $A10009 = $00
    $A1000B = $00
    $A1000D = $00
    $A1000F = $FF
    $A10011 = $00
    $A10013 = $00
    $A10015 = $FF
    $A10017 = $00
    $A10019 = $00
    $A1001B = $FB
    $A1001D = $00
    $A1001F = $00
*/

}

READ16_HANDLER ( genesis_68000_io_r )
{
	int paddata,p;
	int inlines, outlines;

//logerror("I/O read .. offset %02x data %02x\n",offset,genesis_io_ram[offset]);

	switch (offset)
	{
		case 0x00: // version register
			return genesis_io_ram[offset];
		case 0x01:
//			logerror("I/O Data A read \n");

/*
                When TH=0          When TH=1
    D6 : (TH)   0                  1
    D5 : (TR)   Start button       Button C
    D4 : (TL)   Button A           Button B
    D3 : (D3)   0                  D-pad Right
    D2 : (D2)   0                  D-pad Left
    D1 : (D1)   D-pad Down         D-pad Down
    D0 : (D0)   D-pad Up           D-pad Up

*/

			/* process pad input for std 3 button pad */


			p = readinputport(0);
			if (genesis_io_ram[offset]&0x40)
			{
				paddata = ((p&0x0f)>>0) | ((p&0xc0)>>2) | 0x40;
			}
			else
			{
				paddata = ((p&0x03)>>0) | ((p&0x30)>>0) | 0x00;
			}

			inlines = (genesis_io_ram[0x04]^0xff)&0x7f;
			outlines = (genesis_io_ram[0x04] | 0x80);

//			logerror ("ioram %02x inlines %02x paddata %02x outlines %02x othdata %02x\n",genesis_io_ram[0x04], inlines, paddata, outlines, genesis_io_ram[0x01]);



			p = (paddata & inlines) | (genesis_io_ram[0x01] & outlines);

			return p | p << 8;


			return genesis_io_ram[offset];
		case 0x02:
//			logerror("I/O Data B read \n");

			p = readinputport(1);
			if (genesis_io_ram[offset]&0x40)
			{
				paddata = ((p&0x0f)>>0) | ((p&0xc0)>>2) | 0x40;
			}
			else
			{
				paddata = ((p&0x03)>>0) | ((p&0x30)>>0) | 0x00;
			}

			inlines = (genesis_io_ram[0x05]^0xff)&0x7f;
			outlines = (genesis_io_ram[0x05] | 0x80);


			p = (paddata & inlines) | (genesis_io_ram[0x02] & outlines);

			return p | p <<8;

		case 0x03:
//			logerror("I/O Data C read \n");
			return genesis_io_ram[offset];

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
		default:
			logerror("Unhandled I/O read \n");
			return genesis_io_ram[offset];

	}

	return genesis_io_ram[offset];

/*
   D7 : Console is 1= Export (USA, Europe, etc.) 0= Domestic (Japan)
    D6 : Video type is 1= PAL, 0= NTSC
    D5 : Sega CD unit is 1= not present, 0= connected.
    D4 : Unused (always returns zero)
    D3 : Bit 3 of version number
    D2 : Bit 2 of version number
    D1 : Bit 1 of version number
    D0 : Bit 0 of version number
    */


//	return 0x30;
//	return mame_rand(machine);
//	return 0xff;
}

/*

 The I/O chip is mapped to $A10000-$A1001F in the 68000/VDP/Z80 banked address space.
 It is normally read and written in byte units at odd addresses. The chip gets /LWR only,
 so writing to even addresses has no effect. Reading even addresses returns the same data
 ou'd get from an odd address. If a word-sized write is done, the LSB is sent to the I/O chip
 and the MSB is ignored.

 */

/*

$A10001  	Version
$A10003 	Port A data
$A10005 	Port B data
$A10007 	Port C data
$A10009 	Port A control
$A1000B 	Port B control
$A1000D 	Port C control
$A1000F 	Port A TxData
$A10011 	Port A RxData
$A10013 	Port A serial control
$A10015 	Port B TxData
$A10017 	Port B RxData
$A10019 	Port B serial control
$A1001B 	Port C TxData
$A1001D 	Port C RxData
$A1001F 	Port C serial contro

*/

WRITE16_HANDLER ( genesis_68000_io_w )
{

//	logerror("I/O write offset %02x data %04x\n",offset,data);

	switch (offset)
	{
		case 0x00:  // Version (read only?)
			logerror("attempted write to version register?!\n");
			break;
		case 0x01: // Port A data
//			logerror("write to data port A with control register A %02x step1 %02x step2 %02x\n", genesis_io_ram[0x04], (genesis_io_ram[0x01] & !((genesis_io_ram[0x04]&0x7f)|0x80)),  (data & ((genesis_io_ram[0x04]&0x7f)|0x80))   );
			genesis_io_ram[0x01] = (genesis_io_ram[0x01] & ((genesis_io_ram[0x04]^0xff)|0x80)) | (data & ((genesis_io_ram[0x04]&0x7f)|0x80));
			break;
		case 0x02: // Port B data
//			logerror("write to data port B with control register B %02x\n", genesis_io_ram[0x05]);
			genesis_io_ram[0x02] = (genesis_io_ram[0x02] & ((genesis_io_ram[0x05]^0xff)|0x80)) | (data & ((genesis_io_ram[0x05]&0x7f)|0x80));
			break;
		case 0x03: // Port C data
//			logerror("write to data port C with control register C %02x\n", genesis_io_ram[0x06]);
			genesis_io_ram[0x03] = (genesis_io_ram[0x03] & ((genesis_io_ram[0x06]^0xff)|0x80)) | (data & ((genesis_io_ram[0x06]&0x7f)|0x80));
			break;
		case 0x04: // Port A control
			genesis_io_ram[offset]=data;
			break;
		case 0x05: // Port B control
			genesis_io_ram[offset]=data;
			break;
		case 0x06: // Port C control
			genesis_io_ram[offset]=data;
			break;

		/* unhandled */
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
		default:
			genesis_io_ram[offset]=data;
			logerror("unhandled IO write (offset %02x data %02x)\n",offset,data);
			break;
	}
}

UINT16 genesis_vdp_data_read ( genvdp *current_vdp )
{
//	logerror("Read VDP Data Port\n");
	/* clear command pending flag */
	UINT16 retdata;

	retdata = 0;
//	return 0;

	current_vdp -> genesis_vdp_command_pending = 0;

	switch ((current_vdp -> genesis_vdp_cmdmode)&0xf)
	{
		case 0x00:// VRAM Read
			retdata = current_vdp -> genesis_vdp_vram[((current_vdp -> genesis_vdp_cmdaddr)&0xffff)>>1];
			break;
		case 0x04: // VSRAM Read
			retdata = current_vdp -> genesis_vdp_vsram[((current_vdp -> genesis_vdp_cmdaddr)&0x7f)>>1];
			break;
		case 0x08: // CRAM Read
			retdata = current_vdp -> genesis_vdp_cram[((current_vdp -> genesis_vdp_cmdaddr)&0x7f)>>1];
			break;

		default:
			logerror("illegal VDP data port read\n");
	}
	current_vdp -> genesis_vdp_cmdaddr += current_vdp -> genesis_vdp_regs[0x0f];

	return retdata;
}

UINT16 genesis_vdp_control_read ( genvdp *current_vdp )
{
	int retvalue;
	//logerror("Read VDP Control Port (Status Register)\n");
	/* clear command pending flag */
	current_vdp -> genesis_vdp_command_pending = 0;

/*
 d15 - Always 0
 d14 - Always 0
 d13 - Always 1
 d12 - Always 1
 d11 - Always 0
 d10 - Always 1
 d9  - FIFO Empty
 d8  - FIFO Full
 d7  - Vertical interrupt pending
 d6  - Sprite overflow on current scan line
 d5  - Sprite collision
 d4  - Odd frame
 d3  - Vertical blanking
 d2  - Horizontal blanking
 d1  - DMA in progress
 d0  - PAL mode flag
 */
	retvalue = 0x3400; // fixed bits

	if (current_vdp->sline>=224) retvalue |= 0x0008;
	if (current_vdp->sline>=224) retvalue |= 0x0080;

	if (video_screen_get_hpos(0) > 0xc0) retvalue |= 0x0004; // ??
	if (!genesis_is_ntsc) retvalue |= 0x0001;




	return retvalue;

//	return mame_rand(machine);
}

UINT16 genesis_vdp_hvcounter_read ( genvdp *current_vdp )
{
	int vcnt = current_vdp->sline;

//	vcnt -=1;

//	if (vcnt > 0xe0) logerror("HV read %04x\n",vcnt);

	if (vcnt>0xea) vcnt -=5;



//	logerror("Read from HV Counters\n");
	return (((vcnt))&0xff)<<8|(video_screen_get_hpos(0)&0xff);
}

UINT16 genesis_vdp_read ( genvdp *current_vdp, offs_t offset, UINT16 mem_mask )
{
	UINT16 retdata;

	retdata = 0x00;

//	logerror("Genesis VDP Read %06x, %04x\n", offset, mem_mask);



	switch (offset)
	{
		case 0x00: // Data Port
		case 0x02: // Data Port (mirror)
			retdata = genesis_vdp_data_read(&genesis_vdp);
			break;

		case 0x04: // Control Port (Status Register)
		case 0x06: // Control Port (Status Register) (mirror)
			retdata = genesis_vdp_control_read(&genesis_vdp);
			break;

		case 0x08: // HV Counters
		case 0x0a: // HV Counters (mirror)
		case 0x0c: // HV Counters (mirror)
		case 0x0e: // HV Counters (mirror)
			retdata = genesis_vdp_hvcounter_read(&genesis_vdp);
			break;

		case 0x10: // SN76489 PSG
		case 0x12: // SN76489 PSG (mirror)
		case 0x14: // SN76489 PSG (mirror)
		case 0x16: // SN76489 PSG (mirror)
			logerror("Read from SN76489 PSG (machine lockup!)\n");
			retdata = 0x00;
			break;

		case 0x18: // nothing
		case 0x1a: // nothing
		case 0x1c: // nothing
		case 0x1e: // nothing
			logerror("Read from unused VDP addresses\n"); break;
			retdata = 0x00;
			break;

	}
	return retdata;
}

void genesis_vdp_data_write ( genvdp *current_vdp, UINT16 data )
{
//	logerror("Genesis VDP DATA Write  DATA: %04x CMDMODE: %02x OFFSET: %06x INC %02x\n", data, current_vdp -> genesis_vdp_cmdmode,  current_vdp -> genesis_vdp_cmdaddr,  current_vdp -> genesis_vdp_regs[0x0f] );
	/* clear command pending flag */
	current_vdp -> genesis_vdp_command_pending = 0;

	/* handle DMA fill .. note i don't think some of this belongs here .. we should call set up dma again?
	   wrong anyway .. see cgfm docs */
	if (current_vdp -> genesis_vdp_fill_mode == 1)
	{
		int i;

		current_vdp -> genesis_vdp_fill_mode = 0;

		current_vdp -> dma_transfer_count =
		  (current_vdp -> genesis_vdp_regs[0x14] << 8)  |
		  (current_vdp -> genesis_vdp_regs[0x13] << 0);

		logerror("fill addr %04x data %04x count %04x\n", current_vdp -> genesis_vdp_cmdaddr, data, current_vdp -> dma_transfer_count);

		for (i = 0; i < current_vdp -> dma_transfer_count;i++)
		{
			current_vdp -> genesis_vdp_vram[((current_vdp -> genesis_vdp_cmdaddr)&0xffff)>>1]=data;
			current_vdp -> genesis_vdp_vram_is_dirty [(((current_vdp -> genesis_vdp_cmdaddr)&0xffff)>>1)>>4]=1;
			current_vdp -> genesis_vdp_cmdaddr += current_vdp -> genesis_vdp_regs[0x0f];
		}


	return;


	}

	switch ((current_vdp -> genesis_vdp_cmdmode)&0xf)
	{
		case 0x1: // VRAM Write
			if (current_vdp -> genesis_vdp_cmdaddr & 1) data = ((data & 0xff00) >> 8)| ((data & 0x00ff) << 8);
			current_vdp -> genesis_vdp_vram[((current_vdp -> genesis_vdp_cmdaddr)&0xffff)>>1]=data;
			current_vdp -> genesis_vdp_vram_is_dirty [(((current_vdp -> genesis_vdp_cmdaddr)&0xffff)>>1)>>4]=1;
			break;
		case 0x3: // CRAM Write
			current_vdp -> genesis_vdp_cram[((current_vdp -> genesis_vdp_cmdaddr)&0x7f)>>1]=data;
			break;
		case 0x5: // VSRAM Write
			current_vdp -> genesis_vdp_vsram[((current_vdp -> genesis_vdp_cmdaddr)&0x7f)>>1]=data;
			break;
		default:
			break;
	}

	current_vdp -> genesis_vdp_cmdaddr += current_vdp -> genesis_vdp_regs[0x0f];

}


void genesis_vdp_set_register ( genvdp *current_vdp, UINT16 data )
{
	int reg, val;

	reg = (data & 0x1f00) >> 8;
	val = (data & 0x00ff);

	current_vdp -> genesis_vdp_regs[reg]=val;

//	logerror("Genesis VDP Register %02x set to %02x\n", reg,  current_vdp -> genesis_vdp_regs[reg]);

	/* clear address (maybe?) + mode flags */
	current_vdp -> genesis_vdp_cmdmode = 0;
	current_vdp -> genesis_vdp_cmdaddr = 0;



}

/*
   1st write
    CD1 CD0 A13 A12 A11 A10 A09 A08     (D31-D24)
    A07 A06 A05 A04 A03 A02 A01 A00     (D23-D16)

   2nd write
   	 ?   ?   ?   ?   ?   ?   ?   ?      (D15-D8)
    CD5 CD4 CD3 CD2  ?   ?  A15 A14     (D7-D0)

*/

void what_mode ( genvdp *current_vdp )
{
/*
 000000b : VRAM read
 000001b : VRAM write
 000011b : CRAM write
 000100b : VSRAM read
 000101b : VSRAM write
 001000b : CRAM read

 100001b : 68k -> VRAM DMA
 100011b : 68k -> CRAM DMA
 100101b : 68k -> VSRAM DMA

 #21: S08 S07 S06 S05 S04 S03 S02 S01
 #22: S16 S15 S14 S13 S12 S11 S10 S09
 #23:  0  S23 S22 S21 S20 S19 S18 S17


 100001b : VRAM Fill?!

 #19: L07 L06 L05 L04 L03 L02 L01 L00
 #20: L15 L14 L13 L12 L11 L10 L08 L08
 The address bits in registers 21, 22, 23 are ignored:
 #21:  ?   ?   ?   ?   ?   ?   ?   ?
 #22:  ?   ?   ?   ?   ?   ?   ?   ?
 #23:  1   0   ?   ?   ?   ?   ?   ?


 110000b : VRAM Copy

 #19: L07 L06 L05 L04 L03 L02 L01 L00
 #20: L15 L14 L13 L12 L11 L10 L08 L08
 The address bits in register 23 are ignored.
 Registers 21, 22 specify the source address in VRAM:
 #21: S07 S06 S05 S04 S03 S02 S01 S00
 #22: S15 S14 S13 S12 S11 S10 S09 S08
 #23:  1   1   ?   ?   ?   ?   ?   ?

*/
}

void genesis_vdp_do_dma ( genvdp *current_vdp )
{
	UINT16  readdata;
	UINT32  sourceaddr;

	int count;


	if (!current_vdp -> dma_transfer_type)
		return;

	count = current_vdp -> dma_transfer_count;

	do {
		sourceaddr = current_vdp -> dma_transfer_start;

		readdata = 0x88;

		if ((sourceaddr >=0) &&  (sourceaddr <=0x3fffff))
		{
			// read from rom
			//logerror("DMA FROM ROM!\n");
			readdata = ((UINT16*)memory_region(REGION_CPU1))[sourceaddr>>1];
		}

		if ((sourceaddr >=0xe00000) &&  (sourceaddr <=0xffffff))
		{
			sourceaddr &= 0xffff;
			readdata = genesis_mainram[sourceaddr>>1];
			// read from ram
		}

		switch ( current_vdp -> dma_transfer_type )
		{
			case 0x01:
				if (current_vdp -> genesis_vdp_cmdaddr & 1) readdata = ((readdata & 0xff00) >> 8)| ((readdata & 0x00ff) << 8);
				current_vdp -> genesis_vdp_vram[(current_vdp -> genesis_vdp_cmdaddr&0xffff) >>1] = readdata;
				current_vdp -> genesis_vdp_vram_is_dirty[((current_vdp -> genesis_vdp_cmdaddr&0xffff) >>1) >>4] = 1;
				break;

			case 0x03:
				if (current_vdp -> genesis_vdp_cmdaddr > 0x7f)
				{
					current_vdp -> dma_transfer_type = 0;
					return; // abandon transfer
				}
				if (current_vdp -> genesis_vdp_cmdaddr & 1) readdata = ((readdata & 0xff00) >> 8)| ((readdata & 0x00ff) << 8);
				current_vdp -> genesis_vdp_cram[(current_vdp -> genesis_vdp_cmdaddr&0x7f) >>1] = readdata;
				break;

			case 0x05:
				if (current_vdp -> genesis_vdp_cmdaddr > 0x7f)
				{
					current_vdp -> dma_transfer_type = 0;
					return; // abandon transfer
				}
				if (current_vdp -> genesis_vdp_cmdaddr & 1) readdata = ((readdata & 0xff00) >> 8)| ((readdata & 0x00ff) << 8);
				current_vdp -> genesis_vdp_vsram[(current_vdp -> genesis_vdp_cmdaddr&0x7f) >>1] = readdata;
				break;
		}


		current_vdp -> dma_transfer_count--;
		current_vdp -> dma_transfer_start+=2;
		current_vdp -> genesis_vdp_cmdaddr += current_vdp -> genesis_vdp_regs[0x0f];

		/* wrap transfer address */
		if (current_vdp -> dma_transfer_start >= 0xffffff) current_vdp -> dma_transfer_start = 0xe00000;

	} while (--count);



}

/*

 110000b : VRAM Copy

 #19: L07 L06 L05 L04 L03 L02 L01 L00
 #20: L15 L14 L13 L12 L11 L10 L08 L08
 The address bits in register 23 are ignored.
 Registers 21, 22 specify the source address in VRAM:
 #21: S07 S06 S05 S04 S03 S02 S01 S00
 #22: S15 S14 S13 S12 S11 S10 S09 S08
 #23:  1   1   ?   ?   ?   ?   ?   ?

 */

void genesis_68k_set_vram_copy ( genvdp *current_vdp )
{
	int count;
	int readdata;
	int sourceaddr;

	current_vdp -> dma_transfer_start =
	  (current_vdp -> genesis_vdp_regs[0x16] << 8)  |
	  (current_vdp -> genesis_vdp_regs[0x15] << 0);

	current_vdp -> dma_transfer_count =
	  (current_vdp -> genesis_vdp_regs[0x14] << 8)  |
	  (current_vdp -> genesis_vdp_regs[0x13] << 0);


	count = current_vdp -> dma_transfer_count;

	do {
		sourceaddr = current_vdp -> dma_transfer_start;

		readdata = 0x88;

		readdata = current_vdp -> genesis_vdp_vram[(sourceaddr&0xffff) >>1];

		current_vdp -> genesis_vdp_vram[(current_vdp -> genesis_vdp_cmdaddr&0xffff) >>1] = readdata;
		current_vdp -> genesis_vdp_vram_is_dirty[((current_vdp -> genesis_vdp_cmdaddr&0xffff) >>1) >>4] = 1;

		current_vdp -> genesis_vdp_cmdaddr += current_vdp -> genesis_vdp_regs[0x0f];
		current_vdp -> dma_transfer_start++;

	} while (--count);


}

void genesis_68k_xram_dma_set ( genvdp *current_vdp )
{

	current_vdp -> dma_transfer_start =
	  (current_vdp -> genesis_vdp_regs[0x17] << 16) |
	  (current_vdp -> genesis_vdp_regs[0x16] << 8)  |
	  (current_vdp -> genesis_vdp_regs[0x15] << 0);

	current_vdp -> dma_transfer_count =
	  (current_vdp -> genesis_vdp_regs[0x14] << 8)  |
	  (current_vdp -> genesis_vdp_regs[0x13] << 0);

	if (current_vdp ->dma_transfer_count ==0)
	{
		logerror("zero length dma!\n");
		current_vdp ->dma_transfer_count = 0xffff;
	}

	current_vdp -> dma_transfer_start<<=1;

	switch ((current_vdp -> genesis_vdp_cmdmode)&0xf)
	{
		case 0x01:
			current_vdp -> dma_transfer_type = 1;
	//		logerror("DMA set 68k -> VRAM source %06x count %06x to %06x\n", (current_vdp -> dma_transfer_start), current_vdp -> dma_transfer_count, current_vdp -> genesis_vdp_cmdaddr );
			break;

		case 0x03:
			current_vdp -> dma_transfer_type = 3;
	//		logerror("DMA set 68k -> CRAM source %06x count %06x to %06x\n", (current_vdp -> dma_transfer_start), current_vdp -> dma_transfer_count, current_vdp -> genesis_vdp_cmdaddr );
			break;

		case 0x05:
			current_vdp -> dma_transfer_type = 5;
	//		logerror("DMA set 68k -> VSRAM source %06x count %06x to %06x\n", (current_vdp -> dma_transfer_start), current_vdp -> dma_transfer_count, current_vdp -> genesis_vdp_cmdaddr );
			break;

		default:
			current_vdp -> dma_transfer_type = 0;
			logerror("DMA set 68k -> INVALID source %06x count %06x to %06x\n", (current_vdp -> dma_transfer_start), current_vdp -> dma_transfer_count, current_vdp -> genesis_vdp_cmdaddr );
			break;
	}

	genesis_vdp_do_dma ( current_vdp );

}


void genesis_vdp_check_dma ( genvdp *current_vdp )
{
	/* check if DMA is actually enabled */
	/* is this correct behavior? or should we check when it actually attempts the transfer? */
	if (!((current_vdp -> genesis_vdp_regs[0x01]) & 0x10))
		return;

	switch ((current_vdp -> genesis_vdp_regs[0x17]) & 0xc0)
	{
		case 0x00:
		case 0x40:
			genesis_68k_xram_dma_set( current_vdp );
			// 68k -> VRAM/CRAM/VSRAM transfer
			break;

		case 0x80:
			logerror("Vram fill!\n");
			current_vdp -> genesis_vdp_fill_mode = 1; // do we need to set .. or can we check these bits later? ..
			// vram fill
			break;

		case 0xc0:
			logerror("Vram copy!\n");
			genesis_68k_set_vram_copy( current_vdp );
			// vram copy
			break;
	}
}

void genesis_vdp_set_cmdmode_cmdaddr ( genvdp *current_vdp, UINT16 data )
{
	int modepart, addrpart;

	if (current_vdp -> genesis_vdp_command_pending) // second part
	{
		modepart = (data & 0x00f0) >> 2; // CD5 CD4 CD3 CD2
		addrpart = (data & 0x0003) << 14;// A15 A14

		current_vdp -> genesis_vdp_cmdmode = (current_vdp -> genesis_vdp_cmdmode &   0x03) | modepart;
		current_vdp -> genesis_vdp_cmdaddr = (current_vdp -> genesis_vdp_cmdaddr & 0x3fff) | addrpart;

//		logerror("Second Part of VDP Command / Address set %02x %08x\n", current_vdp -> genesis_vdp_cmdmode,  current_vdp -> genesis_vdp_cmdaddr);
//		logerror("Registers 21: %02x 22: %02x 23: %02x\n",current_vdp -> genesis_vdp_regs[21], current_vdp -> genesis_vdp_regs[22], current_vdp -> genesis_vdp_regs[23]);
		//what_mode ( current_vdp );

		/* if bit 0x20 of the mode is set we have some kind of DMA transfer request */
		if ((current_vdp -> genesis_vdp_cmdmode)&0x20) genesis_vdp_check_dma ( current_vdp );
	}
	else // first part
	{
		modepart = (data & 0xc000) >> 14; // CD1 CD0
		addrpart = (data & 0x3fff) >> 0; // A13 - A0;

		current_vdp -> genesis_vdp_cmdmode = (current_vdp -> genesis_vdp_cmdmode &   0x3c) | modepart;
		current_vdp -> genesis_vdp_cmdaddr = (current_vdp -> genesis_vdp_cmdaddr & 0xc000) | addrpart;

//		logerror("First Part of VDP Command / Address set %02x %08x\n", current_vdp -> genesis_vdp_cmdmode,  current_vdp -> genesis_vdp_cmdaddr);
	}
}

/* Genesis Control Port

 16-bit register settings writes or 32-bit control writes

*/




void genesis_vdp_control_write ( genvdp *current_vdp, UINT16 data )
{
	if (current_vdp -> genesis_vdp_command_pending) // second part of a command word
	{
//			logerror("Genesis VDP command word second part %04x\n",data);
			genesis_vdp_set_cmdmode_cmdaddr(current_vdp,data);
			current_vdp -> genesis_vdp_command_pending = 0;
	}
	else // first part of command word or register setting
	{
		/* if 10xxxxxx xxxxxxxx this is a register setting command */
		if ((data & 0xc000) == 0x8000)
		{
//			logerror("Genesis VDP set register %04x\n",data);
			genesis_vdp_set_register(current_vdp,data&0x1fff);
			// set register
		}
		else
		{
//			logerror("Genesis VDP command word first part %04x\n",data);
			genesis_vdp_set_cmdmode_cmdaddr(current_vdp,data);
			current_vdp -> genesis_vdp_command_pending = 1;
		}
	}
}

void genesis_vdp_write ( genvdp *current_vdp, offs_t offset, UINT16 data, UINT16 mem_mask )
{
//	if ((offset!=0x00) && (offset !=0x02)) logerror("Genesis VDP Write %06x, %04x, %04x\n", offset, data, mem_mask);

	switch (offset)
	{
		case 0x00: // Data Port
		case 0x02: // Data Port (mirror)
			/* for 8-bit writes data gets duplicated in each half */
			if (!ACCESSING_LSB)	data = (data & 0xff00) | ((data >>8 )& 0x00ff);
			if (!ACCESSING_MSB)	data = (data & 0x00ff) | ((data <<8 )& 0xff00);
			genesis_vdp_data_write(current_vdp,data);
			break;

		case 0x04: // Control Port
		case 0x06: // Control Port (mirror)
			/* for 8-bit writes data gets duplicated in each half */
			if (!ACCESSING_LSB)	data = (data & 0xff00) | ((data >>8 )& 0x00ff);
			if (!ACCESSING_MSB)	data = (data & 0x00ff) | ((data <<8 )& 0xff00);
			genesis_vdp_control_write(current_vdp,data);
			break;

		case 0x08: // HV Counters
		case 0x0a: // HV Counters (mirror)
		case 0x0c: // HV Counters (mirror)
		case 0x0e: // HV Counters (mirror)
			logerror("Attempting to write to HV Counters (machine lockup!)\n");
			break;

		case 0x10: // SN76489 PSG
						SN76496_0_w(0, (data) & 0xff); break;
		case 0x12: // SN76489 PSG (mirror)
						SN76496_0_w(0, (data) & 0xff); break;
		case 0x14: // SN76489 PSG (mirror)
						SN76496_1_w(0, (data) & 0xff); break;
		case 0x16: // SN76489 PSG (mirror)
						SN76496_1_w(0, (data) & 0xff); break;

		//	if (ACCESSING_MSB) SN76496_0_w(0, (data >>8) & 0xff);

	//		break;

		case 0x18: // nothing
		case 0x1a: // nothing
			logerror("Write to VDP offset 0x18 / 0x1a, no effect!\n"); break;
			break;

		case 0x1c: // corrupt VDP state
		case 0x1e: // corrupt VDP state (mirror)
			logerror("Write to VDP offset 0x1c / 0x1e, see http://cgfm2.emuviews.com/txt/newreg.txt \n"); break;
			break;
	}

}
/* Information from Charles MacDonald

 5. The VDP is mirrored at certain locations from C00000h to DFFFFFh. In
    order to explain what addresses are valid, here is a layout of each
    address bit:

    MSB                       LSB
    110n n000 nnnn nnnn 000m mmmm

    '1' - This bit must be 1.
    '0' - This bit must be 0.
    'n' - This bit can have any value.
    'm' - VDP addresses (00-1Fh)

    For example, you could read the status port at D8FF04h or C0A508h,
    but not D00084h or CF00008h. Accessing invalid addresses will
    lock up the machine.

*/

READ16_HANDLER( genesis_68000_vdp_r )
{
	int retdata;

	offset <<= 1; // convert word offset to byte offset;
	if (offset & 0x2700e0) // throw out invalid offsets (see cgfm docs)
	{
		logerror ("invalid vdp read offset!\n");
		return 0x00;
	}
	offset &= 0x1f; // only low 5 bits of offset matter

	retdata = genesis_vdp_read( &genesis_vdp, offset, mem_mask );

	return retdata;
}

WRITE16_HANDLER( genesis_68000_vdp_w )
{
	offset <<= 1; // convert word offset to byte offset;

	if (offset & 0x2700e0) // throw out invalid offsets (see cgfm docs)
	{
		logerror ("invalid vdp write offset!\n");
		return;
	}
	offset &= 0x1f; // only low 5 bits of offset matter

	genesis_vdp_write( &genesis_vdp, offset, data, mem_mask );
}


/* Main Genesis Z80 */

 READ8_HANDLER( genesis_z80_vdp_r )
{
	return 0;
}

WRITE8_HANDLER( genesis_z80_vdp_w )
{
	int mem_mask;
	int dat;

//	logerror("VDP via z80 write address offset %02x data %02x\n",offset,data);


	if (offset > 0x1f )
	{
	//	logerror("illegal vdp via z80 write address\n");
	}

	if (offset & 1)
	{
		mem_mask = 0xff00;
		dat = data&0x00ff;
	}
	else
	{
		mem_mask = 0x00ff;
		dat = (data <<8)&0xff00;
	}

//	if (offset > 0x10) logerror("GEN SN %02x %02x\n",offset,data);

	genesis_vdp_write( &genesis_vdp, offset&0x1e, dat, mem_mask );
}

WRITE8_HANDLER( genesis_z80_bank_sel_w )
{
//int genesis_z80_bank_addr;
//int genesis_z80_bank_partial;


	genesis_z80_bank_partial |= ((data &0x01) << genesis_z80_bank_step);
//	logerror("bank bit %02x set %02x %02x partial %08x\n", genesis_z80_bank_step , data, data &1, genesis_z80_bank_partial);

	genesis_z80_bank_step++;

	if (genesis_z80_bank_step == 9)
	{
		genesis_z80_bank_addr = genesis_z80_bank_partial << 15;
		genesis_z80_bank_partial = 0;
		genesis_z80_bank_step = 0;
//		logerror("z80 68k bank set to %08x\n", genesis_z80_bank_addr);

	}

}
#ifdef LSB_FIRST
	#define BYTE_XOR(a) ((a) ^ 1)
#else
	#define BYTE_XOR(a) (a)
#endif

 READ8_HANDLER ( genesis_banked_68k_r )
{
	UINT32 addr;
//	UINT16 dat;

//	logerror("z80 PC %04x accessing  68k space %08x offset %04x\n", activecpu_get_pc(), genesis_z80_bank_addr, offset);

	addr = genesis_z80_bank_addr | offset;

	if ((addr>=0) && (addr<=0x3fffff))
	{
//		logerror("from rom!\n");
		return memory_region(REGION_CPU1)[BYTE_XOR(addr)];


	}
	else
	{
	//	logerror("unhandled!\n");
	}

	return 0x00;
}

WRITE8_HANDLER ( z80_ym2612_w )
{
	offset &=0x03;
//	logerror("z80 PC %04x accessing z80_ym2612_w offset %02x data %08x\n",  activecpu_get_pc(), offset, data);

	switch (offset)
	{
		case 0:YM2612_control_port_0_A_w(0,data); break;
		case 1:YM2612_data_port_0_A_w(0,data); break;
		case 2:YM2612_control_port_0_B_w(0,data); break;
		case 3:YM2612_data_port_0_B_w(0,data); break;
	}

}


/*

Display

0-223 - display (counter reloaded on 0)
224 - vblank triggered?
225-261 - blanking period (counter reloaded each line)

 The V counter counts up from 00h to EAh, then it jumps back to E5h and
 continues counting up to FFh. This allows it to cover the entire 262 line
 display.

*/

void genesis_init_frame(void)
{
	int i;
	if ((genesis_vdp.genesis_vdp_regs[0x0c]&0x81) != oldscreenmode)
	{
		oldscreenmode = genesis_vdp.genesis_vdp_regs[0x0c]&0x81;
		switch (genesis_vdp.genesis_vdp_regs[0x0c]&0x81)
		{
			case 0x00: // 32 cell
				video_screen_set_visarea(0, 0, 32*8-1, 0, 224-1);
				break;
			case 0x01: // 40 cell corrupted
			case 0x80: // illegal!
			case 0x81: // 40 cell
				video_screen_set_visarea(0, 0, 40*8-1, 0, 224-1);
			break;
		}
	}

	for(i=0;i<0x800;i++)
	{
		if (genesis_vdp.genesis_vdp_vram_is_dirty[i])
		{
			decodechar(Machine->gfx[genesis_vdp.genesis_vdp_index], i,(UINT8 *)genesis_vdp.genesis_vdp_vram);
			genesis_vdp.genesis_vdp_vram_is_dirty[i] = 0;
		}
	}
}

/* this (and the hv counter stuff) appear to be wrong .. various glitches .. rasters not working right in many games */
INTERRUPT_GEN( genesis_interrupt )
{
//	logerror("interrupt %d\n",cpu_getiloops());
	int scan, irqlevel = 0;
	scan = genesis_vdp.sline = 261 - cpu_getiloops();

	genesis_vdp.irq4_counter--;

	if (scan == 0) genesis_init_frame(); // decode gfx tiles etc.

	if (scan==0) genesis_vdp.irq4_counter = genesis_vdp.genesis_vdp_regs[0x0a];

//if ((scan>=225) && (scan>=261)) genesis_vdp.irq4_counter = genesis_vdp.genesis_vdp_regs[0x0a];

//	if (scan>=0xea) genesis_vdp.irq4_counter +=5;



	if (genesis_vdp.irq4_counter==-1)
	{
		if (genesis_vdp.genesis_vdp_regs[0x00] & 0x10) irqlevel = 4;
		genesis_vdp.irq4_counter = genesis_vdp.genesis_vdp_regs[0x0a];

	}

	if (scan == 0xe0)
	{
	//	if (!irqlevel)
			irqlevel = 6;

		cpunum_set_input_line(machine, 1,0, HOLD_LINE); // z80 interrupt, always?
	}


	if (( scan>=0 ) && ( genesis_vdp.sline <224))
	{
		genesis_vdp_render_scanline(&genesis_vdp,genesis_vdp.sline);
		genesis_vdp_draw_scanline(&genesis_vdp,genesis_vdp.sline);
	}

	cpunum_set_input_line(machine, 0,irqlevel, HOLD_LINE);
}

//static void ym3438_interrupt(int state)
//{
	/* nothing? */
//}

MACHINE_RESET ( genesis )
{
//	logerror("MACHINE_RESET ( genesis )\n");
	/* prevent the z80 from running (code must be uploaded by the 68k first) */
	cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, ASSERT_LINE);
	genesis_z80_is_reset = 1;

	cpunum_set_input_line(machine, 1, INPUT_LINE_HALT,  ASSERT_LINE);
	genesis_68k_has_z80_bus = 1;

	memset(memory_region(REGION_CPU2), 0xcf, 0x2000);

	genesis_z80_bank_step = 0;
	genesis_z80_bank_addr = 0;
	genesis_z80_bank_partial = 0;

	memset(genesis_vdp.genesis_vdp_vram, 0x00, 0x10000);
	memset(genesis_vdp.genesis_vdp_vram_is_dirty, 0x00, 0x800);
	memset(genesis_vdp.genesis_vdp_cram, 0x00, 0x100);
	memset(genesis_vdp.genesis_vdp_vsram, 0x00, 0x100);
	memset(genesis_vdp.genesis_vdp_regs, 0x00, 0x20);

	genesis_init_io();
}


void genesis_common_init( void )
{
	/* standard genesis init */

//	genesis_z80ram = auto_malloc (0x2000);
	genesis_mainram = auto_malloc (0x10000);

	memset( genesis_mainram, 0x00, 0x10000);

	/* Another brutal hack to work around 0.101u1 memory changes */
	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MRA8_BANK10);
	memory_install_write8_handler(1, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_BANK10);
	memory_set_bankptr(10, memory_region(REGION_CPU2));

//	memory_set_bankptr(1,memory_region(REGION_USER1)); /* cartridge at BANK1 */
	memory_set_bankptr(2,genesis_mainram); /* BANK2 = mainram */
//	memory_set_bankptr(3,memory_region(REGION_CPU2));  /* BANK3 = mainram */

	/* prevent the z80 from running (code must be uploaded by the 68k first) */
	cpunum_set_input_line(machine, 1, INPUT_LINE_HALT,  ASSERT_LINE);
	genesis_68k_has_z80_bus = 0;

	cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, ASSERT_LINE);
	genesis_z80_is_reset = 1;

//	memset(genesis_z80ram, 0x00, 0x2000);

	/* copy the cart from user region to CPU region ..
	  i load it in the user region to make handling weird games like ssf2
	  easier
	  */
	memcpy(memory_region(REGION_CPU1),memory_region(REGION_USER1),0x400000);

}

DRIVER_INIT ( genesis )
{
	genesis_common_init();
}
