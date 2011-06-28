/***************************************************************************

    PNG comparison (based on regrep.c)

****************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "osdcore.h"
#include "png.h"


/***************************************************************************
    CONSTANTS & DEFINES
***************************************************************************/

#define BITMAP_SPACE			4

/***************************************************************************
    PROTOTYPES
***************************************************************************/

static int generate_png_diff(const astring *imgfile1, const astring *imgfile2, const astring *outfilename);

/***************************************************************************
    MAIN
***************************************************************************/

/*-------------------------------------------------
    main - main entry point
-------------------------------------------------*/

int main(int argc, char *argv[])
{
	astring *imgfilename1 = NULL, *imgfilename2 = NULL, *outfilename = NULL;

    /* first argument is the directory */
    if (argc < 4)
    {
    	fprintf(stderr, "Usage:\npngcmp <image1> <image2> <outfile>\n");
    	return 1;
    }
    imgfilename1 = astring_dupc(argv[1]);
    imgfilename2 = astring_dupc(argv[2]);
	outfilename = astring_dupc(argv[3]);
	
	int png_res = generate_png_diff(imgfilename1, imgfilename2, outfilename);

	astring_free(outfilename);
	astring_free(imgfilename2);
	astring_free(imgfilename1);
    return png_res;
}

/*-------------------------------------------------
    generate_png_diff - create a new PNG file
    that shows multiple differing PNGs side by
    side with a third set of differences
-------------------------------------------------*/

static int generate_png_diff(const astring *imgfile1, const astring *imgfile2, const astring *outfilename)
{
	bitmap_t *bitmap1 = NULL;
	bitmap_t *bitmap2 = NULL;
	bitmap_t *finalbitmap = NULL;
	int width, height, maxwidth;
	core_file *file = NULL;
	file_error filerr;
	png_error pngerr;
	int error = -1;
	int bitmaps_differ;
	int x, y;

	/* open the source image */
	filerr = core_fopen(astring_c(imgfile1), OPEN_FLAG_READ, &file);
	if (filerr != FILERR_NONE)
		goto error;

	/* load the source image */
	pngerr = png_read_bitmap(file, &bitmap1);
	core_fclose(file);
	if (pngerr != PNGERR_NONE)
		goto error;

	/* open the source image */
	filerr = core_fopen(astring_c(imgfile2), OPEN_FLAG_READ, &file);
	if (filerr != FILERR_NONE)
		goto error;

	/* load the source image */
	pngerr = png_read_bitmap(file, &bitmap2);
	core_fclose(file);
	if (pngerr != PNGERR_NONE)
		goto error;

	/* if the sizes are different, we differ; otherwise start off assuming we are the same */
	bitmaps_differ = (bitmap2->width != bitmap1->width || bitmap2->height != bitmap1->height);

	/* compare scanline by scanline */
	for (y = 0; y < bitmap2->height && !bitmaps_differ; y++)
	{
		UINT32 *base = BITMAP_ADDR32(bitmap1, y, 0);
		UINT32 *curr = BITMAP_ADDR32(bitmap2, y, 0);

		/* scan the scanline */
		for (x = 0; x < bitmap2->width; x++)
			if (*base++ != *curr++)
				break;
		bitmaps_differ = (x != bitmap2->width);
	}

	if (bitmaps_differ)
	{
		/* determine the size of the final bitmap */
		height = width = 0;
		{
			/* determine the maximal width */
			maxwidth = MAX(bitmap1->width, bitmap2->width);
			width = bitmap1->width + BITMAP_SPACE + maxwidth + BITMAP_SPACE + maxwidth;

			/* add to the height */
			height += MAX(bitmap1->height, bitmap2->height);
		}

		/* allocate the final bitmap */
		finalbitmap = bitmap_alloc(width, height, BITMAP_FORMAT_ARGB32);
		if (finalbitmap == NULL)
			goto error;
	
		/* now copy and compare each set of bitmaps */
		int curheight = MAX(bitmap1->height, bitmap2->height);
		/* iterate over rows in these bitmaps */
		for (y = 0; y < curheight; y++)
		{
			UINT32 *src1 = (y < bitmap1->height) ? BITMAP_ADDR32(bitmap1, y, 0) : NULL;
			UINT32 *src2 = (y < bitmap2->height) ? BITMAP_ADDR32(bitmap2, y, 0) : NULL;
			UINT32 *dst1 = BITMAP_ADDR32(finalbitmap, y, 0);
			UINT32 *dst2 = BITMAP_ADDR32(finalbitmap, y, bitmap1->width + BITMAP_SPACE);
			UINT32 *dstdiff = BITMAP_ADDR32(finalbitmap, y, bitmap1->width + BITMAP_SPACE + maxwidth + BITMAP_SPACE);

			/* now iterate over columns */
			for (x = 0; x < maxwidth; x++)
			{
				int pix1 = -1, pix2 = -2;

				if (src1 != NULL && x < bitmap1->width)
					pix1 = dst1[x] = src1[x];
				if (src2 != NULL && x < bitmap2->width)
					pix2 = dst2[x] = src2[x];
				dstdiff[x] = (pix1 != pix2) ? 0xffffffff : 0xff000000;
			}
		}

		/* write the final PNG */
		filerr = core_fopen(astring_c(outfilename), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &file);
		if (filerr != FILERR_NONE)
			goto error;
		pngerr = png_write_bitmap(file, NULL, finalbitmap, 0, NULL);
		core_fclose(file);
		if (pngerr != PNGERR_NONE)
			goto error;
	}

	/* if we get here, we are error free */
	error = bitmaps_differ;

error:
	if (finalbitmap != NULL)
		bitmap_free(finalbitmap);
	if (bitmap1 != NULL)
		bitmap_free(bitmap1);
	if (bitmap2 != NULL)
		bitmap_free(bitmap2);
	if (error == -1)
		osd_rmfile(astring_c(outfilename));
	return error;
}
