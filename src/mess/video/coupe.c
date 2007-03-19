/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate the video hardware of the coupe.

 At present these are not done using the mame driver standard, they are
 simply plot pixelled.. I will fix this in a future version.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/coupe.h"

unsigned char *sam_screen;

void drawMode4_line(mame_bitmap *bitmap,int y)
{
	int x;
	unsigned char tmp=0;

	for (x=0;x<256;)
	{
		tmp=*(sam_screen + (x/2) + (y*128));
#ifdef MONO
		if (tmp>>4)
		{
			plot_pixel(bitmap, x*2, y, Machine->pens[127]);
			plot_pixel(bitmap, x*2+1, y, Machine->pens[127]);
		}
		else
		{
			plot_pixel(bitmap, x*2, y, Machine->pens[0]);
			plot_pixel(bitmap, x*2+1, y, Machine->pens[0]);
		}
		x++;
		if (tmp&0x0F)
		{
			plot_pixel(bitmap, x*2, y, Machine->pens[127]);
			plot_pixel(bitmap, x*2+1, y, Machine->pens[127]);
		}
		else
		{
			plot_pixel(bitmap, x*2, y, Machine->pens[0]);
			plot_pixel(bitmap, x*2+1, y, Machine->pens[0]);
		}
		x++;
#else
		plot_pixel(bitmap, x*2, y, Machine->pens[CLUT[tmp>>4]]);
		plot_pixel(bitmap, x*2+1, y, Machine->pens[CLUT[tmp>>4]]);
		x++;
		plot_pixel(bitmap, x*2, y, Machine->pens[CLUT[tmp&0x0F]]);
		plot_pixel(bitmap, x*2+1, y, Machine->pens[CLUT[tmp&0x0F]]);				
		x++;
#endif
	}
}

void drawMode3_line(mame_bitmap *bitmap,int y)
{
	int x;
	unsigned char tmp=0;

	for (x=0;x<512;)
	{
		tmp=*(sam_screen + (x/4) + (y*128));
#ifdef MONO
		if (tmp>>6)
			plot_pixel(bitmap,x,y,Machine->pens[127]);
		else
			plot_pixel(bitmap,x,y,Machine->pens[0]);
		x++;
		if ((tmp>>4)&0x03)
			plot_pixel(bitmap,x,y,Machine->pens[127]);
		else
			plot_pixel(bitmap,x,y,Machine->pens[0]);
		x++;
		if ((tmp>>2)&0x03)
			plot_pixel(bitmap,x,y,Machine->pens[127]);
		else
			plot_pixel(bitmap,x,y,Machine->pens[0]);
		x++;
		if (tmp&0x03)
			plot_pixel(bitmap,x,y,Machine->pens[127]);
		else
			plot_pixel(bitmap,x,y,Machine->pens[0]);
		x++;
#else
		plot_pixel(bitmap,x,y,Machine->pens[CLUT[tmp>>6]]);
		x++;
		plot_pixel(bitmap,x,y,Machine->pens[CLUT[(tmp>>4)&0x03]]);
		x++;
		plot_pixel(bitmap,x,y,Machine->pens[CLUT[(tmp>>2)&0x03]]);
		x++;
		plot_pixel(bitmap,x,y,Machine->pens[CLUT[tmp&0x03]]);
		x++;
#endif
	}
}

void drawMode2_line(mame_bitmap *bitmap,int y)
{
	int x,b,scrx;
	unsigned char tmp=0;
	unsigned short ink,pap;
	unsigned char *attr;

	attr=sam_screen + 32*192 + y*32;

	scrx=0;
	for (x=0;x<256/8;x++)
	{
		tmp=*(sam_screen + x + (y*32));
#ifdef MONO
		ink=127;
		pap=0;
#else
		ink=CLUT[(*attr) & 0x07];
		pap=CLUT[((*attr)>>3) & 0x07];
#endif
		attr++;

		for (b=0x80;b!=0;b>>=1)
		{
			if (tmp&b)
			{
				plot_pixel(bitmap,scrx++,y,Machine->pens[ink]);
				plot_pixel(bitmap,scrx++,y,Machine->pens[ink]);
			}
			else
			{
				plot_pixel(bitmap,scrx++,y,Machine->pens[pap]);
				plot_pixel(bitmap,scrx++,y,Machine->pens[pap]);
			}
		}
	}
}

void drawMode1_line(mame_bitmap *bitmap,int y)
{
	int x,b,scrx,scry;
	unsigned char tmp=0;
	unsigned short ink,pap;
	unsigned char *attr;

	attr=sam_screen + 32*192 + (y/8)*32;

	scrx=0;
	scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);
	for (x=0;x<256/8;x++)
	{
		tmp=*(sam_screen + x + (y*32));
#ifdef MONO
		ink=127;
		pap=0;
#else
		ink=CLUT[(*attr) & 0x07];
		pap=CLUT[((*attr)>>3) & 0x07];
#endif
		attr++;
		for (b=0x80;b!=0;b>>=1)
		{
			if (tmp&b)
			{
				plot_pixel(bitmap,scrx++,scry,Machine->pens[ink]);
				plot_pixel(bitmap,scrx++,scry,Machine->pens[ink]);
			}
			else
			{
				plot_pixel(bitmap,scrx++,scry,Machine->pens[pap]);
				plot_pixel(bitmap,scrx++,scry,Machine->pens[pap]);
			}
		}
	}
}




