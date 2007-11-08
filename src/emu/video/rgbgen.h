/***************************************************************************

    rgbgen.h

    General RGB utilities.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __RGBGEN__
#define __RGBGEN__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* intermediate RGB values are stored in a struct */
typedef struct { INT16 dummy, r, g, b; } rgbint;

/* intermediate RGB values are stored in a struct */
typedef struct { INT16 a, r, g, b; } rgbaint;



/***************************************************************************
    BASIC CONVERSIONS
***************************************************************************/

/*-------------------------------------------------
    rgb_comp_to_rgbint - converts a trio of RGB
    components to an rgbint type
-------------------------------------------------*/

INLINE void rgb_comp_to_rgbint(rgbint *rgb, INT16 r, INT16 g, INT16 b)
{
	rgb->r = r;
	rgb->g = g;
	rgb->b = b;
}


/*-------------------------------------------------
    rgba_comp_to_rgbint - converts a quad of RGB
    components to an rgbint type
-------------------------------------------------*/

INLINE void rgba_comp_to_rgbaint(rgbaint *rgb, INT16 a, INT16 r, INT16 g, INT16 b)
{
	rgb->a = a;
	rgb->r = r;
	rgb->g = g;
	rgb->b = b;
}


/*-------------------------------------------------
    rgb_to_rgbint - converts a packed trio of RGB
    components to an rgbint type
-------------------------------------------------*/

INLINE void rgb_to_rgbint(rgbint *rgb, rgb_t color)
{
	rgb->r = RGB_RED(color);
	rgb->g = RGB_GREEN(color);
	rgb->b = RGB_BLUE(color);
}


/*-------------------------------------------------
    rgba_to_rgbaint - converts a packed quad of RGB
    components to an rgbint type
-------------------------------------------------*/

INLINE void rgba_to_rgbaint(rgbaint *rgb, rgb_t color)
{
	rgb->a = RGB_ALPHA(color);
	rgb->r = RGB_RED(color);
	rgb->g = RGB_GREEN(color);
	rgb->b = RGB_BLUE(color);
}


/*-------------------------------------------------
    rgbint_to_rgb - converts an rgbint back to
    a packed trio of RGB values
-------------------------------------------------*/

INLINE rgb_t rgbint_to_rgb(const rgbint *color)
{
	return MAKE_RGB(color->r, color->g, color->b);
}


/*-------------------------------------------------
    rgbaint_to_rgba - converts an rgbint back to
    a packed quad of RGB values
-------------------------------------------------*/

INLINE rgb_t rgbaint_to_rgba(const rgbaint *color)
{
	return MAKE_ARGB(color->a, color->r, color->g, color->b);
}


/*-------------------------------------------------
    rgbint_to_rgb_clamp - converts an rgbint back
    to a packed trio of RGB values, clamping them
    to bytes first
-------------------------------------------------*/

INLINE rgb_t rgbint_to_rgb_clamp(const rgbint *color)
{
	UINT8 r = (color->r < 0) ? 0 : (color->r > 255) ? 255 : color->r;
	UINT8 g = (color->g < 0) ? 0 : (color->g > 255) ? 255 : color->g;
	UINT8 b = (color->b < 0) ? 0 : (color->b > 255) ? 255 : color->b;
	return MAKE_RGB(r, g, b);
}


/*-------------------------------------------------
    rgbaint_to_rgba_clamp - converts an rgbint back
    to a packed quad of RGB values, clamping them
    to bytes first
-------------------------------------------------*/

INLINE rgb_t rgbaint_to_rgba_clamp(const rgbaint *color)
{
	UINT8 a = (color->a < 0) ? 0 : (color->a > 255) ? 255 : color->a;
	UINT8 r = (color->r < 0) ? 0 : (color->r > 255) ? 255 : color->r;
	UINT8 g = (color->g < 0) ? 0 : (color->g > 255) ? 255 : color->g;
	UINT8 b = (color->b < 0) ? 0 : (color->b > 255) ? 255 : color->b;
	return MAKE_ARGB(a, r, g, b);
}



/***************************************************************************
    CORE MATH
***************************************************************************/

/*-------------------------------------------------
    rgbint_add - add two rgbint values
-------------------------------------------------*/

INLINE void rgbint_add(rgbint *color1, const rgbint *color2)
{
	color1->r += color2->r;
	color1->g += color2->g;
	color1->b += color2->b;
}


/*-------------------------------------------------
    rgbaint_add - add two rgbaint values
-------------------------------------------------*/

INLINE void rgbaint_add(rgbaint *color1, const rgbaint *color2)
{
	color1->a += color2->a;
	color1->r += color2->r;
	color1->g += color2->g;
	color1->b += color2->b;
}


/*-------------------------------------------------
    rgbint_sub - subtract two rgbint values
-------------------------------------------------*/

INLINE void rgbint_sub(rgbint *color1, const rgbint *color2)
{
	color1->r -= color2->r;
	color1->g -= color2->g;
	color1->b -= color2->b;
}


/*-------------------------------------------------
    rgbaint_sub - subtract two rgbaint values
-------------------------------------------------*/

INLINE void rgbaint_sub(rgbaint *color1, const rgbaint *color2)
{
	color1->a -= color2->a;
	color1->r -= color2->r;
	color1->g -= color2->g;
	color1->b -= color2->b;
}


/*-------------------------------------------------
    rgbint_subr - reverse subtract two rgbint
    values
-------------------------------------------------*/

INLINE void rgbint_subr(rgbint *color1, const rgbint *color2)
{
	color1->r = color2->r - color1->r;
	color1->g = color2->g - color1->g;
	color1->b = color2->b - color1->b;
}


/*-------------------------------------------------
    rgbaint_subr - reverse subtract two rgbaint
    values
-------------------------------------------------*/

INLINE void rgbaint_subr(rgbaint *color1, const rgbaint *color2)
{
	color1->a = color2->a - color1->a;
	color1->r = color2->r - color1->r;
	color1->g = color2->g - color1->g;
	color1->b = color2->b - color1->b;
}



/***************************************************************************
    HIGHER LEVEL OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    rgbint_blend - blend two colors by the given
    scale factor
-------------------------------------------------*/

INLINE void rgbint_blend(rgbint *color1, const rgbint *color2, UINT8 color1scale)
{
	int scale1 = (int)color1scale + 1;
	int scale2 = 256 - scale1;

	color1->r = (color1->r * scale1 + color2->r * scale2) >> 8;
	color1->g = (color1->g * scale1 + color2->g * scale2) >> 8;
	color1->b = (color1->b * scale1 + color2->b * scale2) >> 8;
}


/*-------------------------------------------------
    rgbaint_blend - blend two colors by the given
    scale factor
-------------------------------------------------*/

INLINE void rgbaint_blend(rgbaint *color1, const rgbaint *color2, UINT8 color1scale)
{
	int scale1 = (int)color1scale + 1;
	int scale2 = 256 - scale1;

	color1->a = (color1->a * scale1 + color2->a * scale2) >> 8;
	color1->r = (color1->r * scale1 + color2->r * scale2) >> 8;
	color1->g = (color1->g * scale1 + color2->g * scale2) >> 8;
	color1->b = (color1->b * scale1 + color2->b * scale2) >> 8;
}


/*-------------------------------------------------
    rgbint_scale_and_clamp - scale the given
    color by an 8.8 scale factor and clamp to
    byte values
-------------------------------------------------*/

INLINE void rgbint_scale_and_clamp(rgbint *color, INT16 colorscale)
{
	color->r = (color->r * colorscale) >> 8;
	if ((UINT16)color->r > 255) { color->r = (color->r < 0) ? 0 : 255; }
	color->g = (color->g * colorscale) >> 8;
	if ((UINT16)color->g > 255) { color->g = (color->g < 0) ? 0 : 255; }
	color->b = (color->b * colorscale) >> 8;
	if ((UINT16)color->b > 255) { color->b = (color->b < 0) ? 0 : 255; }
}


/*-------------------------------------------------
    rgbaint_scale_and_clamp - scale the given
    color by an 8.8 scale factor and clamp to
    byte values
-------------------------------------------------*/

INLINE void rgbaint_scale_and_clamp(rgbaint *color, INT16 colorscale)
{
	color->a = (color->a * colorscale) >> 8;
	if ((UINT16)color->a > 255) { color->a = (color->a < 0) ? 0 : 255; }
	color->r = (color->r * colorscale) >> 8;
	if ((UINT16)color->r > 255) { color->r = (color->r < 0) ? 0 : 255; }
	color->g = (color->g * colorscale) >> 8;
	if ((UINT16)color->g > 255) { color->g = (color->g < 0) ? 0 : 255; }
	color->b = (color->b * colorscale) >> 8;
	if ((UINT16)color->b > 255) { color->b = (color->b < 0) ? 0 : 255; }
}


/*-------------------------------------------------
    rgb_bilinear_filter - bilinear filter between
    four pixel values; this code is derived from
    code provided by Michael Herf
-------------------------------------------------*/

INLINE rgb_t rgb_bilinear_filter(rgb_t rgb00, rgb_t rgb01, rgb_t rgb10, rgb_t rgb11, UINT8 u, UINT8 v)
{
	UINT32 agd1 = (rgb01 & 0xff00ff00) - (rgb00 & 0xff00ff00);
	UINT32 agd2 = (rgb11 & 0xff00ff00) - (rgb10 & 0xff00ff00);
	UINT32 rbd1 = (rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff);
	UINT32 rbd2 = (rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff);
	agd1 >>= 8;
	agd2 >>= 8;

	rbd1 *= u;  rgb01 = (rgb00 & 0x00ff00ff);
	agd1 *= u;  rbd1 >>= 8;
	rbd2 *= u;  rgb11 = (rgb10 & 0x00ff00ff);
	agd2 *= u;  rbd2 >>= 8;

	rgb01 += rbd1;
	rgb00 += agd1;

	rgb11 += rbd2;
	rgb10 += agd2;

	rgb01 &= 0x00ff00ff;
	rgb00 &= 0xff00ff00;
	rgb10 &= 0xff00ff00;
	rgb11 &= 0x00ff00ff;

	// ---- rgb11 is now rb for the bottom
	//      rgb10 is     ag for the bottom
	//      rgb01 is     rb for the top
	//      rgb00 is     ag for the top

	rgb10 -= rgb00;  // agd
	rgb11 -= rgb01;  // rbd
	rgb10 >>= 8;

	rgb11 *= v;
	rgb10 *= v;
	rgb11 >>= 8;

	rgb00 += rgb10;
	rgb01 += rgb11;

	rgb00 &= 0xff00ff00;
	rgb01 &= 0x00ff00ff;

	return rgb00 | rgb01;
}


/*-------------------------------------------------
    rgba_bilinear_filter - bilinear filter between
    four pixel values; this code is derived from
    code provided by Michael Herf
-------------------------------------------------*/

INLINE rgb_t rgba_bilinear_filter(rgb_t rgb00, rgb_t rgb01, rgb_t rgb10, rgb_t rgb11, UINT8 u, UINT8 v)
{
	UINT32 agd1 = (rgb01 & 0xff00ff00) - (rgb00 & 0xff00ff00);
	UINT32 agd2 = (rgb11 & 0xff00ff00) - (rgb10 & 0xff00ff00);
	UINT32 rbd1 = (rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff);
	UINT32 rbd2 = (rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff);
	agd1 >>= 8;
	agd2 >>= 8;

	rbd1 *= u;  rgb01 = (rgb00 & 0x00ff00ff);
	agd1 *= u;  rbd1 >>= 8;
	rbd2 *= u;  rgb11 = (rgb10 & 0x00ff00ff);
	agd2 *= u;  rbd2 >>= 8;

	rgb01 += rbd1;
	rgb00 += agd1;

	rgb11 += rbd2;
	rgb10 += agd2;

	rgb01 &= 0x00ff00ff;
	rgb00 &= 0xff00ff00;
	rgb10 &= 0xff00ff00;
	rgb11 &= 0x00ff00ff;

	// ---- rgb11 is now rb for the bottom
	//      rgb10 is     ag for the bottom
	//      rgb01 is     rb for the top
	//      rgb00 is     ag for the top

	rgb10 -= rgb00;  // agd
	rgb11 -= rgb01;  // rbd
	rgb10 >>= 8;

	rgb11 *= v;
	rgb10 *= v;
	rgb11 >>= 8;

	rgb00 += rgb10;
	rgb01 += rgb11;

	rgb00 &= 0xff00ff00;
	rgb01 &= 0x00ff00ff;

	return rgb00 | rgb01;
}


/*-------------------------------------------------
    rgbint_bilinear_filter - bilinear filter between
    four pixel values; this code is derived from
    code provided by Michael Herf
-------------------------------------------------*/

INLINE void rgbint_bilinear_filter(rgbint *color, rgb_t rgb00, rgb_t rgb01, rgb_t rgb10, rgb_t rgb11, UINT8 u, UINT8 v)
{
	UINT32 agd1 = (rgb01 & 0xff00ff00) - (rgb00 & 0xff00ff00);
	UINT32 agd2 = (rgb11 & 0xff00ff00) - (rgb10 & 0xff00ff00);
	UINT32 rbd1 = (rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff);
	UINT32 rbd2 = (rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff);
	agd1 >>= 8;
	agd2 >>= 8;

	rbd1 *= u;  rgb01 = (rgb00 & 0x00ff00ff);
	agd1 *= u;  rbd1 >>= 8;
	rbd2 *= u;  rgb11 = (rgb10 & 0x00ff00ff);
	agd2 *= u;  rbd2 >>= 8;

	rgb01 += rbd1;
	rgb00 += agd1;

	rgb11 += rbd2;
	rgb10 += agd2;

	rgb01 &= 0x00ff00ff;
	rgb00 &= 0xff00ff00;
	rgb10 &= 0xff00ff00;
	rgb11 &= 0x00ff00ff;

	// ---- rgb11 is now rb for the bottom
	//      rgb10 is     ag for the bottom
	//      rgb01 is     rb for the top
	//      rgb00 is     ag for the top

	rgb10 -= rgb00;  // agd
	rgb11 -= rgb01;  // rbd
	rgb10 >>= 8;

	rgb11 *= v;
	rgb10 *= v;
	rgb11 >>= 8;

	rgb00 += rgb10;
	rgb01 += rgb11;

	color->r = (rgb01 >> 16) & 0xff;
	color->g = (rgb00 >> 8) & 0xff;
	color->b = (rgb01 >> 0) & 0xff;
}


/*-------------------------------------------------
    rgbaint_bilinear_filter - bilinear filter between
    four pixel values; this code is derived from
    code provided by Michael Herf
-------------------------------------------------*/

INLINE void rgbaint_bilinear_filter(rgbaint *color, rgb_t rgb00, rgb_t rgb01, rgb_t rgb10, rgb_t rgb11, UINT8 u, UINT8 v)
{
	UINT32 agd1 = (rgb01 & 0xff00ff00) - (rgb00 & 0xff00ff00);
	UINT32 agd2 = (rgb11 & 0xff00ff00) - (rgb10 & 0xff00ff00);
	UINT32 rbd1 = (rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff);
	UINT32 rbd2 = (rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff);
	agd1 >>= 8;
	agd2 >>= 8;

	rbd1 *= u;  rgb01 = (rgb00 & 0x00ff00ff);
	agd1 *= u;  rbd1 >>= 8;
	rbd2 *= u;  rgb11 = (rgb10 & 0x00ff00ff);
	agd2 *= u;  rbd2 >>= 8;

	rgb01 += rbd1;
	rgb00 += agd1;

	rgb11 += rbd2;
	rgb10 += agd2;

	rgb01 &= 0x00ff00ff;
	rgb00 &= 0xff00ff00;
	rgb10 &= 0xff00ff00;
	rgb11 &= 0x00ff00ff;

	// ---- rgb11 is now rb for the bottom
	//      rgb10 is     ag for the bottom
	//      rgb01 is     rb for the top
	//      rgb00 is     ag for the top

	rgb10 -= rgb00;  // agd
	rgb11 -= rgb01;  // rbd
	rgb10 >>= 8;

	rgb11 *= v;
	rgb10 *= v;
	rgb11 >>= 8;

	rgb00 += rgb10;
	rgb01 += rgb11;

	color->a = (rgb00 >> 24) & 0xff;
	color->r = (rgb01 >> 16) & 0xff;
	color->g = (rgb00 >> 8) & 0xff;
	color->b = (rgb01 >> 0) & 0xff;
}


#endif /* __RGBUTIL__ */
