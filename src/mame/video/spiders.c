/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "spiders.h"
#include "video/crtc6845.h"

static UINT8 bitflip[256];
static int *screenbuffer;

#define SCREENBUFFER_SIZE	0x2000

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( spiders )
{
	int loop;

	// Use a temp bitmap so user change (dip switches, etc.) does not effect the bitmap
	tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);

	for(loop=0;loop<256;loop++)
	{
		bitflip[loop]=(loop&0x01)?0x80:0x00;
		bitflip[loop]|=(loop&0x02)?0x40:0x00;
		bitflip[loop]|=(loop&0x04)?0x20:0x00;
		bitflip[loop]|=(loop&0x08)?0x10:0x00;
		bitflip[loop]|=(loop&0x10)?0x08:0x00;
		bitflip[loop]|=(loop&0x20)?0x04:0x00;
		bitflip[loop]|=(loop&0x40)?0x02:0x00;
		bitflip[loop]|=(loop&0x80)?0x01:0x00;
	}
	screenbuffer = auto_malloc(SCREENBUFFER_SIZE*sizeof(int));
	memset(screenbuffer,1,SCREENBUFFER_SIZE*sizeof(int));
	return 0;
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( spiders )
{
	int loop,data0,data1,data2,col;

	size_t crtc6845_mem_size;
	int video_addr,increment;

	crtc6845_mem_size=crtc6845_horiz_disp*crtc6845_vert_disp*8;

	video_addr=crtc6845_start_addr & 0x7f;

	if(spiders_video_flip)
	{
		video_addr+=crtc6845_mem_size-1;
		increment=-1;
	}
	else
	{
		increment=1;
	}

	// Access bank1
	if(crtc6845_page_flip)
		video_addr+=0x2000;

	for(loop=0;loop<crtc6845_mem_size;loop++)
	{
		int i,x,y,combo;

		if(spiders_video_flip)
		{
			data0=bitflip[spiders_ram[0x0000+video_addr]];
			data1=bitflip[spiders_ram[0x4000+video_addr]];
			data2=bitflip[spiders_ram[0x8000+video_addr]];
		}
		else
		{
			data0=spiders_ram[0x0000+video_addr];
			data1=spiders_ram[0x4000+video_addr];
			data2=spiders_ram[0x8000+video_addr];
		}
		combo=data0|(data1<<8)|(data2<<16);

        // Check if we need to update the bitmap or the bitmap already has the right colour
		if(screenbuffer[loop]!=combo)
		{
			y=loop/0x20;

			for(i=0;i<8;i++)
			{
				x=((loop%0x20)<<3)+i;
				col=((data2&0x01)<<2)+((data1&0x01)<<1)+(data0&0x01);

				plot_pixel(tmpbitmap, x, y, Machine->pens[col]);

				data0 >>= 1;
				data1 >>= 1;
				data2 >>= 1;
			}
			screenbuffer[loop]=combo;
		}
		video_addr+=increment;
	}
	/* Now copy the temp bitmap to the screen */
    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	return 0;
}
