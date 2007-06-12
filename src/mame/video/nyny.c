/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

extern UINT8 *nyny_videoram ;
extern UINT8 *nyny_colourram ;

static mame_bitmap *tmpbitmap1;
static mame_bitmap *tmpbitmap2;


/* used by nyny and spiders */
PALETTE_INIT( nyny )
{
	int i;

	for (i = 0;i < machine->drv->total_colors;i++)
	{
		palette_set_color_rgb(machine,i,pal1bit(i >> 0),pal1bit(i >> 1),pal1bit(i >> 2));
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( nyny )
{
	tmpbitmap1 = auto_bitmap_alloc(machine->screen[0].width,machine->screen[0].height,machine->screen[0].format);
	tmpbitmap2 = auto_bitmap_alloc(machine->screen[0].width,machine->screen[0].height,machine->screen[0].format);

	nyny_videoram = auto_malloc(0x4000);
	nyny_colourram = auto_malloc(0x4000);
}

/***************************************************************************
  Stop the video hardware emulation.
***************************************************************************/

WRITE8_HANDLER( nyny_flipscreen_w )
{
	flip_screen_set(data);
}

READ8_HANDLER( nyny_videoram0_r )
{
	return( nyny_videoram[offset] ) ;
}

READ8_HANDLER( nyny_videoram1_r )
{
	return( nyny_videoram[offset+0x2000] ) ;
}

READ8_HANDLER( nyny_colourram0_r )
{
	return( nyny_colourram[offset] ) ;
}

READ8_HANDLER( nyny_colourram1_r )
{
	return( nyny_colourram[offset+0x2000] ) ;
}

WRITE8_HANDLER( nyny_colourram0_w )
{
	int x,y,z,d,v,c;
	nyny_colourram[offset] = data;
	v = nyny_videoram[offset] ;

	x = offset & 0x1f ;
	y = offset >> 5 ;

	d = data & 7 ;
	for ( z=0; z<8; z++ )
	{
		c = v & 1 ;
		*BITMAP_ADDR16(tmpbitmap1, y, x*8+z) = Machine->pens[c*d];
		v >>= 1 ;
	}
}

WRITE8_HANDLER( nyny_videoram0_w )
{
	int x,y,z,c,d;
	nyny_videoram[offset] = data;
	d = nyny_colourram[offset] & 7 ;

	x = offset & 0x1f ;
	y = offset >> 5 ;

	for ( z=0; z<8; z++ )
	{
		c = data & 1 ;
		*BITMAP_ADDR16(tmpbitmap1, y, x*8+z) = Machine->pens[c*d];
		data >>= 1 ;
	}
}

WRITE8_HANDLER( nyny_colourram1_w )
{
	int x,y,z,d,v,c;
	nyny_colourram[offset+0x2000] = data;
	v = nyny_videoram[offset+0x2000] ;

	x = offset & 0x1f ;
	y = offset >> 5 ;

	d = data & 7 ;
	for ( z=0; z<8; z++ )
	{
		c = v & 1 ;
		*BITMAP_ADDR16(tmpbitmap2, y, x*8+z) = Machine->pens[c*d];
		v >>= 1 ;
	}

}

WRITE8_HANDLER( nyny_videoram1_w )
{
	int x,y,z,c,d;
	nyny_videoram[offset+0x2000] = data;
	d = nyny_colourram[offset+0x2000] & 7 ;

	x = offset & 0x1f ;
	y = offset >> 5 ;

	for ( z=0; z<8; z++ )
	{
		c = data & 1 ;
		*BITMAP_ADDR16(tmpbitmap2, y, x*8+z) = Machine->pens[c*d];
		data >>= 1 ;
	}
}

VIDEO_UPDATE( nyny )
{
	copybitmap(bitmap,tmpbitmap2,flip_screen,flip_screen,0,0,&machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap1,flip_screen,flip_screen,0,0,&machine->screen[0].visarea,TRANSPARENCY_COLOR,0);
	return 0;
}
