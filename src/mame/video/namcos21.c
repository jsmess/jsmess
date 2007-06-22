/***************************************************************************
Namco System 21 Video Hardware

- sprite hardware is identical to Namco System NB1
- there are no tilemaps
- 3d graphics are managed by DSP processors

TODO: use fixed point arithmetic while rendering quads

TODO: incorporate proper per-pixel transparency, instead of overloading the zbuffer

TODO: add depth cueing support (palette bank selection using most significant bits of zcode), if this
      is determined to be a feature of real hardware.
*/

#include "driver.h"
#include "namcos2.h"
#include "namcoic.h"
#include "namcos21.h"

#define FRAMEBUFFER_SIZE_IN_BYTES (sizeof(UINT16)*NAMCOS21_POLY_FRAME_WIDTH*NAMCOS21_POLY_FRAME_HEIGHT)

/* work (hidden) framebuffer */
static UINT16 *mpPolyFrameBufferPens;
static INT16 *mpPolyFrameBufferZ;

/* visible framebuffer */
static UINT16 *mpPolyFrameBufferPens2;
static INT16 *mpPolyFrameBufferZ2;

static void
AllocatePolyFrameBuffer( void )
{
	mpPolyFrameBufferZ     = auto_malloc( FRAMEBUFFER_SIZE_IN_BYTES );
	mpPolyFrameBufferPens  = auto_malloc( FRAMEBUFFER_SIZE_IN_BYTES );
	mpPolyFrameBufferZ2    = auto_malloc( FRAMEBUFFER_SIZE_IN_BYTES );
	mpPolyFrameBufferPens2 = auto_malloc( FRAMEBUFFER_SIZE_IN_BYTES );

		namcos21_ClearPolyFrameBuffer();
		namcos21_ClearPolyFrameBuffer();
} /* AllocatePolyFrameBuffer */

void
namcos21_ClearPolyFrameBuffer( void )
{
	int i;
	INT16 *temp1;
	UINT16 *temp2;

	/* swap work and visible framebuffers */
	temp1 = mpPolyFrameBufferZ;
	mpPolyFrameBufferZ = mpPolyFrameBufferZ2;
	mpPolyFrameBufferZ2 = temp1;

	temp2 = mpPolyFrameBufferPens;
	mpPolyFrameBufferPens = mpPolyFrameBufferPens2;
	mpPolyFrameBufferPens2 = temp2;

	/* wipe work zbuffer */
	for( i=0; i<NAMCOS21_POLY_FRAME_WIDTH*NAMCOS21_POLY_FRAME_HEIGHT; i++ )
	{
		mpPolyFrameBufferZ[i] = 0x7fff;
	}
} /* namcos21_ClearPolyFrameBuffer */

static void
CopyVisiblePolyFrameBuffer( mame_bitmap *bitmap, const rectangle *clip )
{ /* blit the visible framebuffer */
	int sy;
	for( sy=clip->min_y; sy<=clip->max_y; sy++ )
	{
		UINT16 *dest = BITMAP_ADDR16(bitmap, sy, 0);
		const INT16 *pZ   = mpPolyFrameBufferZ2 + NAMCOS21_POLY_FRAME_WIDTH*sy;
		const UINT16 *pPen = mpPolyFrameBufferPens2 + NAMCOS21_POLY_FRAME_WIDTH*sy;
		int sx;
		for( sx=clip->min_x; sx<=clip->max_x; sx++ )
		{
			if( pZ[sx]!=0x7fff )
			{ /* for now, assume pixels with max zcode are transparent */
				dest[sx] = pPen[sx];
			}
		}
	}
} /* CopyVisiblePolyFrameBuffer */

static int objcode2tile( int code )
{ /* callback for sprite drawing code in namcoic.c */
	return code;
} /* objcode2tile */

VIDEO_START( namcos21 )
{
	AllocatePolyFrameBuffer();
	namco_obj_init(
		0,		/* gfx bank */
		0xf,	/* reverse palette mapping */
		objcode2tile );
} /* VIDEO_START( namcos21 ) */

static void
update_palette( void )
{
	int i;
	INT16 data1,data2;
	int r,g,b;

	/*
    Palette:
        0x0000..0x1fff  sprite palettes (0x10 sets of 0x100 colors)

        0x2000..0x3fff  polygon palette bank0 (0x10 sets of 0x200 colors)
            (in starblade, some palette animation effects are performed here)

        0x4000..0x5fff  polygon palette bank1 (0x10 sets of 0x200 colors)

        0x6000..0x7fff  polygon palette bank2 (0x10 sets of 0x200 colors)

        The polygon-dedicated color sets within a bank typically increase in
        intensity from very dark to full intensity.

        Probably the selected palette is determined by most significant bits of z-code.
        This is not yet hooked up.
    */
	for( i=0; i<NAMCOS21_NUM_COLORS; i++ )
	{
		data1 = paletteram16[0x00000/2+i];
		data2 = paletteram16[0x10000/2+i];

		r = data1>>8;
		g = data1&0xff;
		b = data2&0xff;

		palette_set_color( Machine,i, MAKE_RGB(r,g,b) );
	}
} /* update_palette */

VIDEO_UPDATE( namcos21_default )
{
	int pivot = 3;
	int pri;

	update_palette();

	/* paint background */
	fillbitmap( bitmap, get_black_pen(machine), cliprect );

	/* draw low priority 2d sprites */
	for( pri=0; pri<pivot; pri++ )
	{
		namco_obj_draw(machine, bitmap, cliprect, pri );
	}

	CopyVisiblePolyFrameBuffer( bitmap, cliprect );

	/* draw high priority 2d sprites */
	for( pri=pivot; pri<8; pri++ )
	{
		namco_obj_draw(machine, bitmap, cliprect, pri );
	}
	return 0;
}

VIDEO_UPDATE( namcos21_winrun )
{
	/* NOT WORKING!
     *
     * videoram16 points to a framebuffer used by Winning Run '91 hardware, probably a linear
     * pixel map used as a predecessor to the newer hardware's sprite system.
     */
	int sx,sy;
	for( sy=0; sy<512; sy++ )
	{
		UINT16 *pDest = BITMAP_ADDR16(bitmap, sy, 0);
		for( sx=0; sx<512; sx++ )
		{
			int color = 0;
			int offs = sy*0x400 + (sx/8)*0x10;
			if( videoram16[offs/2]&(1<<(sx&7)) )
			{
				color = 1;
			}
			pDest[sx] = color;
		}
	}
	namcos21_dspram16[0x100/2] = 0x0001;
	return 0;
}

/*********************************************************************************************/

typedef struct
{
	double x,y;
	double z;
} vertex;

typedef struct
{
	double x;
	double z;
} edge;

#define SWAP(A,B) { const void *temp = A; A = B; B = temp; }

static void
renderscanline_flat( const edge *e1, const edge *e2, int sy, unsigned color )
{
	if( e1->x > e2->x )
	{
		SWAP(e1,e2);
	}

	{
		UINT16 *pDest = mpPolyFrameBufferPens + sy*NAMCOS21_POLY_FRAME_WIDTH;
		INT16 *pZBuf = mpPolyFrameBufferZ     + sy*NAMCOS21_POLY_FRAME_WIDTH;

		int x0 = (int)e1->x;
		int x1 = (int)e2->x;
		int w = x1-x0;
		if( w )
		{
			double z = e1->z;
			double dz = (e2->z - e1->z)/w;
			int x, crop;
			crop = - x0;
			if( crop>0 )
			{
				z += crop*dz;
				x0 = 0;
			}
			if( x1>NAMCOS21_POLY_FRAME_WIDTH-1 )
			{
				x1 = NAMCOS21_POLY_FRAME_WIDTH-1;
			}

			for( x=x0; x<x1; x++ )
			{
				INT16 zz = (INT16)z;
				if( zz<pZBuf[x] )
				{
					pDest[x] = color;
					pZBuf[x] = zz;
				}
				z += dz;
			}
		}
	}
} /* renderscanline_flat */

static void
rendertri(
		const vertex *v0,
		const vertex *v1,
		const vertex *v2,
		unsigned color )
{
	int dy,ystart,yend,crop;

	/* first, sort so that v0->y <= v1->y <= v2->y */
	for(;;)
	{
		if( v0->y > v1->y )
		{
			SWAP(v0,v1);
		}
		else if( v1->y > v2->y )
		{
			SWAP(v1,v2);
		}
		else
		{
			break;
		}
	}

	ystart = v0->y;
	yend   = v2->y;
	dy = yend-ystart;
	if( dy )
	{
		int y;
		edge e1; /* short edge (top and bottom) */
		edge e2; /* long (common) edge */

		double dx2dy = (v2->x - v0->x)/dy;
		double dz2dy = (v2->z - v0->z)/dy;

		double dx1dy;
		double dz1dy;

		e2.x = v0->x;
		e2.z = v0->z;
		crop = - ystart;
		if( crop>0 )
		{
			e2.x += dx2dy*crop;
			e2.z += dz2dy*crop;
		}

		ystart = v0->y;
		yend = v1->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v0->x;
			e1.z = v0->z;

			dx1dy = (v1->x - v0->x)/dy;
			dz1dy = (v1->z - v0->z)/dy;

			crop = - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.z += dz1dy*crop;
				ystart = 0;
			}
			if( yend>NAMCOS21_POLY_FRAME_HEIGHT-1 ) yend = NAMCOS21_POLY_FRAME_HEIGHT-1;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline_flat( &e1, &e2, y, color );

				e2.x += dx2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.z += dz1dy;
			}
		}

		ystart = v1->y;
		yend = v2->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v1->x;
			e1.z = v1->z;

			dx1dy = (v2->x - v1->x)/dy;
			dz1dy = (v2->z - v1->z)/dy;

			crop = 0 - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.z += dz1dy*crop;
				ystart = 0;
			}
			if( yend>NAMCOS21_POLY_FRAME_HEIGHT-1 ) yend = NAMCOS21_POLY_FRAME_HEIGHT-1;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline_flat( &e1, &e2, y, color );

				e2.x += dx2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.z += dz1dy;
			}
		}
	}
} /* rendertri */

void
namcos21_DrawQuad( int sx[4], int sy[4], int zcode[4], int color )
{
	vertex a,b,c,d;

	color &= 0x7fff;

//      0x0000..0x1fff  sprite palettes (0x10 sets of 0x100 colors)
//      0x2000..0x3fff  polygon palette bank0 (0x10 sets of 0x200 colors)
//      0x4000..0x5fff  polygon palette bank1 (0x10 sets of 0x200 colors)
//      0x6000..0x7fff  polygon palette bank2 (0x10 sets of 0x200 colors)

	/* map color code to hardware pen */
	switch( color>>8 )
	{
	case 0:
		color = 0x4000 - 0x400 + (color&0xff) + 0x100;
		break;

	case 2:
		color = 0x4000 - 0x400 + (color&0xff) + 0x000;
		break;

	default:
		/* unused? */
		mame_printf_debug ( "unk col=0x%04x\n", color );
		break;
	}

	a.x = sx[0];
	a.y = sy[0];
	a.z = zcode[0];

	b.x = sx[1];
	b.y = sy[1];
	b.z = zcode[1];

	c.x = sx[2];
	c.y = sy[2];
	c.z = zcode[2];

	d.x = sx[3];
	d.y = sy[3];
	d.z = zcode[3];

	rendertri( &a, &b, &c, color );
	rendertri( &c, &d, &a, color );
} /* namcos21_DrawQuad */
