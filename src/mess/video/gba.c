/***************************************************************************

  gba.c

  File to handle emulation of the video hardware of the Game Boy Advance

  By R. Belmont & MooglyGuy

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/gba.h"

#define VERBOSE_LEVEL	(0)
#define DISABLE_ROZ	(0)

static UINT16 mosaic_offset[16][4096];

INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(machine->cpu[0]), buf );
	}
}

static void draw_4bpp_tile(gba_state *gba_state, UINT16 *scanline, UINT32 gba_vrambase, UINT16 tilenum, int scnx, int tiley, int vflip, int hflip, int pal, int mosX)
{
	int pixx, yofs;
	UINT8 *pgba_vram = (UINT8 *)&gba_state->gba_vram[gba_vrambase>>2], pixel;
	UINT16 *pgba_pram = (UINT16 *)gba_state->gba_pram;

	if (vflip)
	{
		yofs = 28-(tiley*4);
	}
	else
	{
		yofs = tiley*4;
	}

	if (hflip)
	{
		for (pixx = 0; pixx < 8; pixx+= 2)
		{
			pixel = pgba_vram[(tilenum*32)+(3-(pixx/2))+yofs];

			if (pixel & 0xf0)
			{
				scanline[scnx+pixx] = pgba_pram[(pixel>>4)+pal]&0x7fff;
			}
			if (pixel & 0x0f)
			{
				scanline[scnx+pixx+1] = pgba_pram[(pixel&0xf)+pal]&0x7fff;
			}
		}
	}
	else
	{
		for (pixx = 0; pixx < 8; pixx+= 2)
		{
			pixel = pgba_vram[(tilenum*32)+(pixx/2)+yofs];

			if (pixel & 0x0f)
			{
				scanline[scnx+pixx] = pgba_pram[(pixel&0xf)+pal]&0x7fff;
			}
			if (pixel & 0xf0)
			{
				scanline[scnx+pixx+1] = pgba_pram[(pixel>>4)+pal]&0x7fff;
			}
		}
	}
}

static void draw_8bpp_tile(gba_state *gba_state, UINT16 *scanline, UINT32 gba_vrambase, UINT16 tilenum, int scnx, int tiley, int vflip, int hflip, int pal, int mosX)
{
	int pixx, yofs;
	UINT8 *pgba_vram = (UINT8 *)&gba_state->gba_vram[gba_vrambase>>2], pixel;
	UINT16 *pgba_pram = (UINT16 *)gba_state->gba_pram;

	if (vflip)
	{
		yofs = 56-(tiley*8);
	}
	else
	{
		yofs = tiley*8;
	}

	if (hflip)
	{
		for (pixx = 0; pixx < 8; pixx++)
		{
			pixel = pgba_vram[(tilenum*32)+(7-pixx)+yofs];

			if (pixel)
			{
				scanline[scnx+pixx] = pgba_pram[pixel+pal]&0x7fff;
			}
		}
	}
	else
	{
		for (pixx = 0; pixx < 8; pixx++)
		{
			pixel = pgba_vram[(tilenum*32)+pixx+yofs];

			if (pixel)
			{
				scanline[scnx+pixx] = pgba_pram[pixel+pal]&0x7fff;
			}
		}
	}
}

static void draw_roz_scanline(gba_state *gba_state, UINT16 *scanline, int ypos, UINT32 enablemask, UINT32 ctrl, INT32 X, INT32 Y, INT32 PA, INT32 PB, INT32 PC, INT32 PD, int priority)
{
	UINT32 base, mapbase, size, ovr;
	INT32 sizes[4] = { 128, 256, 512, 1024 };
	INT32 cx, cy, x, ax, ay;
	UINT8 *mgba_vram = (UINT8 *)gba_state->gba_vram, tile;
	UINT16 *pgba_pram = (UINT16 *)gba_state->gba_pram;
	UINT16 pixel;

	if ((gba_state->DISPCNT & enablemask) && ((ctrl & 3) == priority))
	{
		base = ((ctrl & BGCNT_CHARBASE) >> BGCNT_CHARBASE_SHIFT) * 0x4000;			// VRAM base of tiles
		mapbase = ((ctrl & BGCNT_SCREENBASE) >> BGCNT_SCREENBASE_SHIFT) * 0x800;	// VRAM base of map
		size = (ctrl & BGCNT_SCREENSIZE) >> BGCNT_SCREENSIZE_SHIFT;					// size of map in submaps
		ovr = ctrl & BGCNT_PALETTESET_WRAP;

		// sign extend roz parameters
		if (X & 0x08000000) X |= 0xf0000000;
		if (Y & 0x08000000) Y |= 0xf0000000;
		if (PA & 0x8000) PA |= 0xffff0000;
		if (PB & 0x8000) PB |= 0xffff0000;
		if (PC & 0x8000) PC |= 0xffff0000;
		if (PD & 0x8000) PD |= 0xffff0000;

		cx = X;
		cy = Y;

		cx += ypos * PB;
		cy += ypos * PD;

		if(ctrl & BGCNT_MOSAIC)
		{
			int mosaicy = ((gba_state->MOSAIC & 0xf0) >> 4) + 1;
			int y = ypos % mosaicy;
			cx -= y*PB;
			cy -= y*PD;
		}

		ax = cx >> 8;
		ay = cy >> 8;

		if(ctrl & BGCNT_PALETTESET_WRAP)
		{
			ax %= sizes[size];
			ay %= sizes[size];
			if(ax < 0)
			{
				ax += sizes[size];
			}
			if(ay < 0)
			{
				ay += sizes[size];
			}
		}

		if(ctrl & BGCNT_PALETTE256)
		{
			for(x = 0; x < 240; x++)
			{
				if(!(ax < 0 || ay < 0 || ax >= sizes[size] || ay >= sizes[size]))
				{
					int tilex = ax & 7;
					int tiley = ay & 7;

					tile = mgba_vram[mapbase + (ax / 8) + (ay / 8) * (sizes[size] / 8)];
					pixel = mgba_vram[base + (tile * 64) + (tiley * 8) + tilex];

					// plot it
					if (pixel)
					{
						scanline[x] = pgba_pram[pixel] & 0x7fff;
					}
				}

				cx += PA;
				cy += PC;

				ax = cx >> 8;
				ay = cy >> 8;

				if(ctrl & BGCNT_PALETTESET_WRAP)
				{
					ax %= sizes[size];
					ay %= sizes[size];
					if(ax < 0)
					{
						ax += sizes[size];
					}
					if(ay < 0)
					{
						ay += sizes[size];
					}
				}
			}
		}
		else
		{
			for(x = 0; x < 240; x++)
			{
				if(!(ax < 0 || ay < 0 || ax >= sizes[size] || ay >= sizes[size]))
				{
					int tilex = ax & 7;
					int tiley = ay & 7;

					tile = mgba_vram[mapbase + (ax / 8) + (ay / 8) * (sizes[size] / 8)];
					pixel = mgba_vram[base + (tile * 64) + (tiley * 8) + tilex];

					// plot it
					if (pixel)
					{
						scanline[x] = pgba_pram[pixel] & 0x7fff;
					}
				}

				cx += PA;
				cy += PC;

				ax = cx >> 8;
				ay = cy >> 8;

				if(ctrl & BGCNT_PALETTESET_WRAP)
				{
					ax %= sizes[size];
					ay %= sizes[size];
					if(ax < 0)
					{
						ax += sizes[size];
					}
					if(ay < 0)
					{
						ay += sizes[size];
					}
				}
			}
		}

		if(ctrl & BGCNT_MOSAIC)
		{
			int mosaicx = (gba_state->MOSAIC & 0x0f) + 1;
			if(mosaicx > 1)
			{
				int m = 1;
				for(x = 0; x < 239; x++)
				{
					scanline[x+1] = scanline[x];
					m++;
					if(m == mosaicx)
					{
						m = 1;
						x++;
					}
				}
			}
		}
	}
}

static void draw_bg_scanline(gba_state *gba_state, UINT16 *scanline, int ypos, UINT32 enablemask, UINT32 ctrl, UINT32 hofs, UINT32 vofs, int priority)
{
	UINT32 base, mapbase, size, mode;
	UINT16 *mgba_vram = (UINT16 *)gba_state->gba_vram, tile;
	int mx, my, tx, mosX, mosY;

	if (ctrl & 0x40)	// mosaic
	{
		mosX = gba_state->MOSAIC & 0xf;
		mosY = (gba_state->MOSAIC>>4) & 0xf;

		ypos = mosaic_offset[mosY][ypos];
	}
	else
	{
		mosX = mosY = 0;
	}

	if ((gba_state->DISPCNT & enablemask) && ((ctrl & BGCNT_PRIORITY) == priority))
	{
		base = ((ctrl>>2)&3) * 0x4000;		// VRAM base of tiles
		mapbase = ((ctrl>>8)&0x1f) * 0x800;	// VRAM base of map
		size = ((ctrl>>14) & 3);		// size of map in submaps
		mode = (ctrl & 0x80) ? 1 : 0;	// 1 for 8bpp, 0 for 4bpp

		mx = hofs/8;		// X tile offset
		my = (ypos + vofs) / 8;	// Y tile offset

		switch (size)
		{
			case 0:	// 32x32
				for (tx = 0; tx < 31; tx++)
				{
					tile = mgba_vram[(mapbase>>1) + ((tx + mx) % 32) + ((my % 32) * 32)];

					if (mode)
					{
						draw_8bpp_tile(gba_state, scanline, base, (tile&0x3ff)<<1, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, 0, mosX);
					}
					else
					{
						draw_4bpp_tile(gba_state, scanline, base, tile&0x3ff, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, ((tile&0xf000)>>8), mosX);
					}
				}
				break;

			case 1:	// 64x32
				for (tx = 0; tx < 31; tx++)
				{
					int mapxofs = ((tx + mx) % 64);

					if (mapxofs >= 32)
					{
						mapxofs %= 32;
						mapxofs += (32*32);	// offset to reach the second page of the tilemap
					}

					tile = mgba_vram[(mapbase>>1) + mapxofs + ((my % 32) * 32)];

					if (mode)
					{
						draw_8bpp_tile(gba_state, scanline, base, (tile&0x3ff)<<1, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, 0, mosX);
					}
					else
					{
						draw_4bpp_tile(gba_state, scanline, base, tile&0x3ff, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, ((tile&0xf000)>>8), mosX);
					}
				}
				break;

			case 2:	// 32x64
				for (tx = 0; tx < 31; tx++)
				{
					int mapyofs = (my % 64);

					if (mapyofs >= 32)
					{
						mapyofs %= 32;
						mapyofs += 32;
					}

					tile = mgba_vram[(mapbase>>1) + ((tx + mx) % 32) + (mapyofs * 32)];

					if (mode)
					{
						draw_8bpp_tile(gba_state, scanline, base, (tile&0x3ff)<<1, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, 0, mosX);
					}
					else
					{
						draw_4bpp_tile(gba_state, scanline, base, tile&0x3ff, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, ((tile&0xf000)>>8), mosX);
					}
				}
				break;

			case 3: // 64x64
			#if 0
				if (1)
				{
					int mapyofs = (my % 64);

					if (mapyofs >= 32)
					{
						mapyofs %= 32;
						mapyofs += 64;
					}

//					printf("%d yofs %d (ypos %d vofs %d my %d) tileofs %x\n", video_screen_get_vpos(machine->primary_screen), mapyofs, ypos, vofs, my, (mapbase>>1) + (mapyofs * 32));
				}
			#endif

				for (tx = 0; tx < 31; tx++)
				{
					int mapxofs = ((tx + mx) % 64);
					int mapyofs = (my % 64);

					if (mapyofs >= 32)
					{
						mapyofs %= 32;
						mapyofs += 64;
					}

					if (mapxofs >= 32)
					{
						mapxofs %= 32;
						mapxofs += (32*32);	// offset to reach the second/fourth page of the tilemap
					}

					tile = mgba_vram[(mapbase>>1) + mapxofs + (mapyofs * 32)];

					if (mode)
					{
						draw_8bpp_tile(gba_state, scanline, base, (tile&0x3ff)<<1, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, 0, mosX);
					}
					else
					{
						draw_4bpp_tile(gba_state, scanline, base, tile&0x3ff, (tx*8)-(hofs % 8), (ypos + vofs) % 8, (tile&0x800)?1:0, (tile&0x400)?1:0, ((tile&0xf000)>>8), mosX);
					}
				}
				break;
		}
	}
}

static void draw_mode3_scanline(gba_state *gba_state, UINT16 *scanline, int y)
{
	int x;
	UINT16 *pgba_vram = (UINT16 *)gba_state->gba_vram;

	for (x = 0; x < 240; x++)
	{
		scanline[x] = pgba_vram[(240*y)+x]&0x7fff;
	}
}

static void draw_mode4_scanline(gba_state *gba_state, UINT16 *scanline, int y)
{
	UINT32 base;
	int x;
	UINT8 *pgba_vram = (UINT8 *)gba_state->gba_vram, pixel;
	UINT16 *pgba_pram = (UINT16 *)gba_state->gba_pram;

	if (gba_state->DISPCNT & DISPCNT_FRAMESEL)
	{
		base = 0xa000;
	}
	else
	{
		base = 0;
	}

	for (x = 0; x < 240; x++)
	{
		pixel = pgba_vram[base+(240*y)+x];
		if (pixel != 0)
		{
			scanline[x] = pgba_pram[pixel]&0x7fff;
		}
	}
}

static void draw_mode5_scanline(gba_state *gba_state, UINT16 *scanline, int y)
{
	UINT32 base;
	int x;
	UINT16 *pgba_vram = (UINT16 *)gba_state->gba_vram;

	if (gba_state->DISPCNT & DISPCNT_FRAMESEL)
	{
		base = 0xa000>>1;
	}
	else
	{
		base = 0;
	}

	for (x = 0; x < 160; x++)
	{
		scanline[x] = pgba_vram[base+(160*y)+x];
	}
}

static void draw_gba_oam(gba_state *gba_state, running_machine *machine, UINT16 *scanline, int y, int priority)
{
	INT16 gba_oamindex;
	UINT8 tilex, tiley;
	UINT32 tilebytebase, tileindex, tiledrawindex;
	UINT8 width, height;
	UINT16 *pgba_oam = (UINT16 *)gba_state->gba_oam;

	if( gba_state->DISPCNT & DISPCNT_OBJ_EN )
	{
		for( gba_oamindex = 127; gba_oamindex >= 0; gba_oamindex-- )
		{
			UINT16 attr0, attr1, attr2;
			INT32 cury, lg;
			UINT8 *src;
			UINT16 j;

			attr0 = pgba_oam[(4*gba_oamindex)+0];
			attr1 = pgba_oam[(4*gba_oamindex)+1];
			attr2 = pgba_oam[(4*gba_oamindex)+2];

			if (((attr0 & OBJ_MODE) != OBJ_MODE_WINDOW) && (((attr2 >> 10) & 3) == priority))
			{
				width = 0;
				height = 0;
				switch (attr0 & OBJ_SHAPE )
				{
					case OBJ_SHAPE_SQR:
						switch(attr1 & OBJ_SIZE )
						{
							case OBJ_SIZE_8:
								width = 1;
								height = 1;
								break;
							case OBJ_SIZE_16:
								width = 2;
								height = 2;
								break;
							case OBJ_SIZE_32:
								width = 4;
								height = 4;
								break;
							case OBJ_SIZE_64:
								width = 8;
								height = 8;
								break;
						}
						break;
					case OBJ_SHAPE_HORIZ:
						switch(attr1 & OBJ_SIZE )
						{
							case OBJ_SIZE_8:
								width = 2;
								height = 1;
								break;
							case OBJ_SIZE_16:
								width = 4;
								height = 1;
								break;
							case OBJ_SIZE_32:
								width = 4;
								height = 2;
								break;
							case OBJ_SIZE_64:
								width = 8;
								height = 4;
								break;
						}
						break;
					case OBJ_SHAPE_VERT:
						switch(attr1 & OBJ_SIZE )
						{
							case OBJ_SIZE_8:
								width = 1;
								height = 2;
								break;
							case OBJ_SIZE_16:
								width = 1;
								height = 4;
								break;
							case OBJ_SIZE_32:
								width = 2;
								height = 4;
								break;
							case OBJ_SIZE_64:
								width = 4;
								height = 8;
								break;
						}
						break;
					default:
						width = 0;
						height = 0;
						verboselog(machine, 0, "OAM error: Trying to draw OBJ with OBJ_SHAPE = 3!\n" );
						break;
				}

				tiledrawindex = tileindex = (attr2 & OBJ_TILENUM);
				tilebytebase = 0x10000;	// the index doesn't change in the higher modes, we just ignore sprites that are out of range

 				if (attr0 & OBJ_ROZMODE_ROZ)
				{
					INT16 sx, sy;
					INT32 fx, fy, ax, ay, rx, ry, offs;
					UINT8 blockparam;
					INT16 dx, dy, dmx, dmy;
					UINT8 color;

					width *= 8;
					height *= 8;

					if ((attr0 & OBJ_ROZMODE) == OBJ_ROZMODE_DISABLE)
					{
						continue;
					}

					sx = (attr1 & OBJ_X_COORD);
					sy = (attr0 & OBJ_Y_COORD);

					lg = width;

					if(sx & 0x100)
					{
						sx |= 0xffffff00;
					}

					if(sy >= 160)
					{
						sy |= 0xffffff00;
					}

					fx = width;
					fy = height;

					if((attr0 & OBJ_ROZMODE) == OBJ_ROZMODE_DBLROZ)
					{
						fx *= 2;
						fy *= 2;
						lg *= 2;
					}

					if((y < sy) || (y >= sy + fy) || (sx >= 240) || (sx + fx <= 0))
					{
						continue;
					}

					cury = y - sy;

					blockparam = ((attr1 & OBJ_SCALE_PARAM) >> OBJ_SCALE_PARAM_SHIFT) << 4;

					dx  = (INT16)pgba_oam[blockparam+3];
					dmx = (INT16)pgba_oam[blockparam+7];
					dy  = (INT16)pgba_oam[blockparam+11];
					dmy = (INT16)pgba_oam[blockparam+15];

					rx = (width << 7) - (fx >> 1)*dx - (fy >> 1)*dmx + cury * dmx;
					ry = (height << 7) - (fx >> 1)*dy - (fy >> 1)*dmy + cury * dmy;

					if (sx < 0)
					{
						if(sx + fx <= 0)
						{
							continue;
						}

						lg += sx;
						rx -= sx*dx;
						ry -= sx*dy;
						sx = 0;
					}
					else
					{
						if(sx + fx > 256)
						{
							lg = 256 - sx;
						}
					}

					if(attr0 & OBJ_PALMODE_256)
					{
						UINT16 *pgba_pram = (UINT16*)gba_state->gba_pram;

						src = (UINT8*)&gba_state->gba_vram[tilebytebase >> 2];
						if(!src)
						{
							continue;
						}

						src += tiledrawindex * 32;

						for(j = 0; j < lg; ++j, ++sx)
						{
							ax = (rx >> 8);
							ay = (ry >> 8);

							if(ax >= 0 && ay >= 0 && ax < width && ay < height)
							{
								if((gba_state->DISPCNT & DISPCNT_VRAM_MAP) == DISPCNT_VRAM_MAP_2D)
								{
									offs = (ax & 0x7) + ((ax & 0xfff8) << 3) + ((ay >> 3) << 10) + ((ay & 0x7) << 3);
								}
								else
								{
									offs = (ax & 0x7) + ((ax & 0xfff8) << 3) + (((ay >> 3) * width) << 3) + ((ay & 0x7) << 3);
								}

								color = src[offs];

								if(color)
								{
									scanline[sx] = pgba_pram[256+color] & 0x7fff;
								}
							}

							rx += dx;
							ry += dy;
						}
					}
					else
					{
						UINT16 *pgba_pram = (UINT16*)gba_state->gba_pram;

						src = (UINT8*)&gba_state->gba_vram[tilebytebase >> 2];
						if(!src)
						{
							continue;
						}

						src += tiledrawindex * 32;

						for(j = 0; j < lg; ++j, ++sx)
						{
							ax = (rx >> 8);
							ay = (ry >> 8);

							if(ax >= 0 && ay >= 0 && ax < width && ay < height)
							{
								if((gba_state->DISPCNT & DISPCNT_VRAM_MAP) == DISPCNT_VRAM_MAP_2D)
								{
									offs = ((ax >> 1) & 0x3) + (((ax >> 1) & 0xfffc) << 3) + ((ay >> 3) << 10) + ((ay & 0x7) << 2);
								}
								else
								{
									offs = ((ax >> 1) & 0x3) + (((ax >> 1) & 0xfffc) << 3) + (((ay >> 3) * width) << 2) + ((ay & 0x7) << 2);
								}

								color = src[offs];

								if(ax & 1)
								{
									color >>= 4;
								}
								else
								{
									color &= 0x0f;
								}

								if(color)
								{
									scanline[sx] = pgba_pram[256 + ((attr2 & 0xf000) >> 8) + color] & 0x7fff;
								}
							}

							rx += dx;
							ry += dy;
						}
					}
				}
				else
				{
					INT16 sx, sy;
					int vflip, hflip;

					if ((attr0 & OBJ_ROZMODE) == OBJ_ROZMODE_DISABLE)
					{
						continue;
					}

					sx = (attr1 & OBJ_X_COORD);
					sy = (attr0 & OBJ_Y_COORD);
					vflip = (attr1 & OBJ_VFLIP) ? 1 : 0;
					hflip = (attr1 & OBJ_HFLIP) ? 1 : 0;

					if (sy > 160)
					{
						sy = 0 - (255-sy);
					}

					if (sx > 256)
					{
						sx = 0 - (511-sx);
					}

					// on this scanline?
					if ((y >= sy) && (y < (sy+(height*8))))
					{
						// Y = scanline we're drawing
						// SY = starting Y position of sprite
						if (vflip)
						{
							tiley = (sy+(height*8)-1) - y;
						}
						else
						{
							tiley = y - sy;
						}

						if (( gba_state->DISPCNT & DISPCNT_VRAM_MAP ) == DISPCNT_VRAM_MAP_2D)
						{
							tiledrawindex = (tileindex + ((tiley/8) * 32));
						}
						else
						{
							if ((attr0 & OBJ_PALMODE) == OBJ_PALMODE_16)
							{
								tiledrawindex = (tileindex + ((tiley/8) * width));
							}
							else
							{
								tiledrawindex = (tileindex + ((tiley/8) * (width*2)));
							}
						}

						if (hflip)
						{
							tiledrawindex += (width-1);
							for (tilex = 0; tilex < width; tilex++)
							{
								if ((attr0 & OBJ_PALMODE) == OBJ_PALMODE_16)
								{
									draw_4bpp_tile(gba_state, scanline, tilebytebase, tiledrawindex, sx+(tilex*8), tiley % 8, 0, hflip, 256+((attr2&0xf000)>>8), 0);
								}
								else
								{
									draw_8bpp_tile(gba_state, scanline, tilebytebase, tiledrawindex, sx+(tilex*8), tiley % 8, 0, hflip, 256, 0);
									tiledrawindex++;
								}
								tiledrawindex--;
							}
						}
						else
						{
							for (tilex = 0; tilex < width; tilex++)
							{
								if ((attr0 & OBJ_PALMODE) == OBJ_PALMODE_16)
								{
									draw_4bpp_tile(gba_state, scanline, tilebytebase, tiledrawindex, sx+(tilex*8), tiley % 8, 0, hflip, 256+((attr2&0xf000)>>8), 0);
								}
								else
								{
									draw_8bpp_tile(gba_state, scanline, tilebytebase, tiledrawindex, sx+(tilex*8), tiley % 8, 0, hflip, 256, 0);
									tiledrawindex++;
								}
								tiledrawindex++;
							}
						}
					}
				}
			}
		}
	}
}

void gba_draw_scanline(running_machine *machine, int y)
{
	UINT16 *pgba_pram, *scanline;
	int prio;
	bitmap_t *bitmap = tmpbitmap;
	int i;
	static UINT16 xferscan[240+2048];	// up to 1024 pixels of slop on either side to allow easier clip handling
	gba_state *gba_state = machine->driver_data;

	pgba_pram = (UINT16 *)gba_state->gba_pram;

	scanline = BITMAP_ADDR16(bitmap, y, 0);

	// forced blank
	if (gba_state->DISPCNT & DISPCNT_BLANK)
	{
		// forced blank is white
		for (i = 0; i < 240; i++)
		{
			scanline[i] = 0x7fff;
		}
		return;
	}

	// BG color
	for (i = 0; i < 240; i++)
	{
		xferscan[1024+i] = pgba_pram[0]&0x7fff;
	}


//	printf("mode %d\n", (gba_state->DISPCNT & 7));

//	if (y == 0) printf("DISPCNT %04x\n", gba_state->DISPCNT);

	switch (gba_state->DISPCNT & 7)
	{
		case 0:
			for (prio = 3; prio >= 0; prio--)
			{
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG3_EN, gba_state->BG3CNT, gba_state->BG3HOFS, gba_state->BG3VOFS, prio);
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2HOFS, gba_state->BG2VOFS, prio);
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG1_EN, gba_state->BG1CNT, gba_state->BG1HOFS, gba_state->BG1VOFS, prio);
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG0_EN, gba_state->BG0CNT, gba_state->BG0HOFS, gba_state->BG0VOFS, prio);
				draw_gba_oam(gba_state, machine, &xferscan[1024], y, prio);
			}
			break;

		case 1:
			for (prio = 3; prio >= 0; prio--)
			{
				draw_roz_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2X, gba_state->BG2Y, gba_state->BG2PA, gba_state->BG2PB, gba_state->BG2PC, gba_state->BG2PD, prio);
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG1_EN, gba_state->BG1CNT, gba_state->BG1HOFS, gba_state->BG1VOFS, prio);
				draw_bg_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG0_EN, gba_state->BG0CNT, gba_state->BG0HOFS, gba_state->BG0VOFS, prio);
				draw_gba_oam(gba_state, machine, &xferscan[1024], y, prio);
			}
			break;

		case 2:
			for (prio = 3; prio >= 0; prio--)
			{
				draw_roz_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG3_EN, gba_state->BG3CNT, gba_state->BG3X, gba_state->BG3Y, gba_state->BG3PA, gba_state->BG3PB, gba_state->BG3PC, gba_state->BG3PD, prio);
				draw_roz_scanline(gba_state, &xferscan[1024], y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2X, gba_state->BG2Y, gba_state->BG2PA, gba_state->BG2PB, gba_state->BG2PC, gba_state->BG2PD, prio);
				draw_gba_oam(gba_state, machine, &xferscan[1024], y, prio);
			}
			break;

		case 3:
			draw_mode3_scanline(gba_state, &xferscan[1024], y);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 3);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 2);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 1);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 0);
			break;

		case 4:
			draw_mode4_scanline(gba_state, &xferscan[1024], y);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 3);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 2);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 1);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 0);
			break;

		case 5:
			draw_mode5_scanline(gba_state, &xferscan[1024], y);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 3);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 2);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 1);
			draw_gba_oam(gba_state, machine, &xferscan[1024], y, 0);
			break;
	}


	// copy the working scanline to the real one
	memcpy(scanline, &xferscan[1024], 240*2);

	return;
}

void gba_video_start(running_machine *machine)
{
	int level, x;

	/* generate a table to make mosaic fast */
	for (level = 0; level < 16; level++)
	{
		for (x = 0; x < 4096; x++)
		{
			mosaic_offset[level][x] = (x / (level + 1)) * (level + 1);
		}
	}
}
