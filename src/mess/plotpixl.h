/***************************************************************************

    plotpixl.h

    Legacy plot_pixel() function that was removed from MAME (this should be
	removed from MESS as well)

***************************************************************************/

#ifndef __PLOTPIXL_H__
#define __PLOTPIXL_H__

/* plot a pixel in a bitmap of arbitrary depth */
INLINE void plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	switch (bitmap->bpp)
	{
		case 8:		*BITMAP_ADDR8(bitmap, y, x) = (UINT8)color;		break;
		case 16:	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;	break;
		case 32:	*BITMAP_ADDR32(bitmap, y, x) = (UINT32)color;	break;
	}
}


#endif /* __PLOTPIXL_H__ */
