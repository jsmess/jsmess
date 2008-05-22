/***************************************************************************

  galaxy.c

  Functions to emulate the video hardware of the Galaksija.
  
  20/05/2008 - Real video implementation by Miodrag Milanovic
  01/03/2008 - Update by Miodrag Milanovic to make Galaksija video work with new SVN code  
***************************************************************************/

#include "driver.h"
#include "includes/galaxy.h"
#include "cpu/z80/z80.h"

const gfx_layout galaxy_charlayout =
{
	8, 13,				/* 8x8 characters */
	128,				/* 128 characters */
	1,				/* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{7, 6, 5, 4, 3, 2, 1, 0},
	{0*128*8, 1*128*8,  2*128*8,  3*128*8,
	 4*128*8, 5*128*8,  6*128*8,  7*128*8,
	 8*128*8, 9*128*8, 10*8*128, 11*128*8, 12*128*8},
	8 				/* each character takes 1 consecutive byte */
};

const unsigned char galaxy_palette[2*3] =
{
	0xff, 0xff, 0xff,		/* White */
	0x00, 0x00, 0x00		/* Black */
};

PALETTE_INIT( galaxy )
{
	int i;

	for ( i = 0; i < sizeof(galaxy_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, galaxy_palette[i*3], galaxy_palette[i*3+1], galaxy_palette[i*3+2]);
	}
}

UINT32 gal_cnt = 0;
static UINT8 code = 0;
static UINT8 first = 0;

emu_timer *gal_video_timer = NULL;

TIMER_CALLBACK( gal_video )
{	
	int y,x;
	if (galaxy_interrupts_enabled==TRUE) {
		UINT8 *gfx = memory_region(REGION_GFX1);	
		UINT8 dat = (gal_latch_value & 0x3c) >> 2;
		if ((gal_cnt > 96 * 2) && (gal_cnt < 96 * 210)) { // display on screen just first 208 lines			
			if (((gal_cnt) % 2)==0) { //fetch code
	  			UINT16 addr = (cpunum_get_reg(0, Z80_I) << 8) |  cpunum_get_reg(0, Z80_R) | ((gal_latch_value & 0x80) ^ 0x80);  		  		
	  			if (first==0 && (cpunum_get_reg(0, Z80_R) & 0x1f) ==0) {
		  			// Due to a fact that on real processor latch value is set at
		  			// the end of last cycle we need to skip dusplay of double 	  			
		  			// first char in each row	  			
		  			code = 0xff;
		  			first = 1;
				} else {
					code = program_read_byte(addr) & 0xbf;
					code += (code & 0x80) >> 1;	
					code = gfx[(code & 0x7f) +(dat << 7 )];		
					first = 0;				
				}
				y = gal_cnt / 96 - 2;
				x = (gal_cnt % 96)*4;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 0) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 1) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 2) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 3) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 4) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 5) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 6) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 7) & 1; 
			}			
		}	
		gal_cnt++;		
	}		
}

VIDEO_START ( galaxy ) 
{
	return VIDEO_START_CALL ( generic_bitmapped );
}

VIDEO_UPDATE( galaxy )
{
	timer_adjust_periodic(gal_video_timer, attotime_zero, 0, attotime_never);
	if (galaxy_interrupts_enabled == FALSE) {
		rectangle black_area = {0,384-1,0,212-2};
		fillbitmap(tmpbitmap, 1, &black_area);				
	}
	galaxy_interrupts_enabled = FALSE;		
	return VIDEO_UPDATE_CALL ( generic_bitmapped );
}

