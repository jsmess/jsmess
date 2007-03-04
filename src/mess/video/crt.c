/*
	vidhrdw/crt.c

	CRT video emulation for TX-0 and PDP-1.


Theory of operation:

	What makes such CRT devices so odd is that there is no video processor, no
	scan logic, no refresh logic.  The beam position and intensity is
	controlled by the program completely: in order to draw an object, the
	program must direct the beam to each point of the object, and in order to
	refresh it, the program must redraw the object periodically.

	Since the refresh rates are highly variable (completely controlled by the
	program), I need to simulate CRT remanence: the intensity of each pixel on
	display decreases regularly.  In order to keep this efficient, I keep a
	list of non-black pixels, and only process these pixels on each refresh.
	In order to improve efficiency further, I keep a distinct list for each
	line of the display: I have found that it improves drawing speed slightly
	(probably because it improves the cache hit rate).


	Raphael Nabet 2002-2004
	Based on earlier work by Chris Salomon
*/

#include <math.h>

#include "driver.h"

#include "video/crt.h"


typedef struct
{
	int intensity;		/* current intensity of the pixel */
							/* a node is not in the list when (intensity == -1) */
	int next;			/* index of next pixel in list */
} point;

static point *list;		/* array of (crt_window_width*crt_window_height) point */
static int *list_head;	/* head of the list of lit pixels (index in the array) */
							/* keep a separate list for each display line (makes the video code slightly faster) */

static int decay_counter;	/* incremented each frame (tells for how many frames the CRT has decayed between two screen refresh) */

/* CRT window */
static int window_offset_x, window_offset_y, window_width, window_height;

enum
{
	intensity_pixel_not_in_list = -1	/* special value that tells that the node is not in list */
};
static int num_intensity_levels;
#define intensity_new_pixel num_intensity_levels


/*
	video_start_crt

	video init
*/
int video_start_crt(int num_levels, int offset_x, int offset_y, int width, int height)
{
	int i;

	num_intensity_levels = num_levels;
	window_offset_x = offset_x;
	window_offset_y = offset_y;
	window_width = width;
	window_height = height;

	/* alloc the arrays */
	list = auto_malloc(window_width * window_height * sizeof(point));

	list_head = auto_malloc(window_height * sizeof(int));

	/* fill with black and set up list as empty */
	for (i=0; i<(window_width * window_height); i++)
	{
		list[i].intensity = intensity_pixel_not_in_list;
	}

	for (i=0; i<window_height; i++)
		list_head[i] = -1;

	decay_counter = 0;

	return 0;
}


/*
	crt_plot

	schedule a pixel to be plotted
*/
void crt_plot(int x, int y)
{
	point *node;
	int list_index;

	/* compute pixel coordinates */
	if (x<0) x=0;
	if (y<0) y=0;
	if ((x>(window_width-1)) || ((y>window_height-1)))
		return;
	y = (window_height-1) - y;

	/* find entry in list */
	list_index = x + y*window_width;

	node = & list[list_index];

	if (node->intensity == intensity_pixel_not_in_list)
	{	/* insert node in list if it is not in it */
		node->next = list_head[y];
		list_head[y] = list_index;
	}
	/* set intensity */
	node->intensity = intensity_new_pixel;
}


/*
	video_eof_crt

	keep track of time
*/
VIDEO_EOF( crt )
{
	decay_counter++;
}


/*
	video_update_crt

	update the bitmap
*/
void video_update_crt(mame_bitmap *bitmap)
{
	int i, p_i;
	int y;

	//if (decay_counter)
	{
		/* some time has elapsed: let's update the screen */
		for (y=0; y<window_height; y++)
		{
			UINT16 *line = BITMAP_ADDR16(bitmap, y+window_offset_y, 0);

			p_i = -1;

			for (i=list_head[y]; (i != -1); i=list[i].next)
			{
				point *node = & list[i];
				int x = (i % window_width) + window_offset_x;

				if (node->intensity == intensity_new_pixel)
					/* new pixel: set to max intensity */
					node->intensity = num_intensity_levels-1;
				else
				{
					/* otherwise, apply intensity decay */
					node->intensity -= decay_counter;
					if (node->intensity < 0)
						node->intensity = 0;
				}

				/* draw pixel on screen */
				//plot_pixel(bitmap, x, y+crt_window_offset_y, Machine->pens[node->intensity]);
				line[x] = Machine->pens[node->intensity];

				if (node->intensity != 0)
					p_i = i;	/* current node will be next iteration's previous node */
				else
				{	/* delete current node */
					node->intensity = intensity_pixel_not_in_list;
					if (p_i != -1)
						list[p_i].next = node->next;
					else
						list_head[y] = node->next;
				}
			}
		}

		decay_counter = 0;
	}
}
