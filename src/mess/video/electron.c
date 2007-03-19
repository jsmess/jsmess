/******************************************************************************
	Acorn Electron driver

	MESS Driver By:

	Wilbert Pol

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/electron.h"

/*
  From the ElectrEm site:

Timing is somewhat of a thorny issue on the Electron. It is almost certain the Electron could have been a much faster machine if BBC Micro OS level compatibility had not been not a design requirement.

When accessing the ROM regions, the CPU always runs at 2Mhz. When accessing the FC (1 Mhz bus) or FD (JIM) pages, the CPU always runs at 1Mhz.

The timing for RAM accesses varies depending on the graphics mode, and how many bytes are required to be read by the video circuits per scanline. When accessing RAM in modes 4-6, the CPU is simply moved to a 1Mhz clock. This occurs for any RAM access at any point during the frame.

In modes 0-3, if the CPU tries to access RAM at any time during which the video circuits are fetching bytes, it is halted by means of receiving a stopped clock until the video circuits next stop fetching bytes.

Each scanline is drawn in exactly 64us, and of that the video circuits fetch bytes for 40us. In modes 0, 1 and 2, 256 scanlines have pixels on, whereas in mode 3 only 250 scanlines are affected as mode 3 is a 'spaced' mode.

As opposed to one clock generator which changes pace, the 1Mhz and 2Mhz clocks are always available, so the ULA acts to simply change which clock is piped to the CPU. This means in half of all cases, a further 2Mhz cycle is lost waiting for the 2Mhz and 1Mhz clocks to synchronise during a 2Mhz to 1Mhz step.

The video circuits run from a constant 2Mhz clock, and generate 312 scanlines a frame, one scanline every 128 cycles. This actually gives means the Electron is running at 50.08 frames a second.

Creating a scanline numbering scheme where the first scanline with pixels is scanline 0, in all modes the end of display interrupt is generated at the end of scanline 255, and the RTC interrupt is generated upon the end of scanline 99.

From investigating some code for vertical split modes printed in Electron User volume 7, issue 7 it seems that the exact timing of the end of display interrupt is somewhere between 24 and 40 cycles after the end of pixels. This may coincide with HSYNC. I have no similarly accurate timing for the real time clock interrupt at this time.

Mode changes are 'immediate', so any change in RAM access timing occurs exactly after the write cycle of the changing instruction. Similarly palette changes take effect immediately. VSYNC is not signalled in any way.

*/

static int map4[256];
static int map16[256];

void electron_video_init( void ) {
	int i;
	for( i = 0; i < 256; i++ ) {
		map4[i] = ( ( i & 0x10 ) >> 3 ) | ( i & 0x01 );
		map16[i] = ( ( i & 0x40 ) >> 3 ) | ( ( i & 0x10 ) >> 2 ) | ( ( i & 0x04 ) >> 1 ) | ( i & 0x01 );
	}
}

INLINE UINT8 read_vram( UINT16 addr ) {
	return ula.vram[ addr % ula.screen_size ];
}

void electron_drawline( void ) {
	int i;
	int x = 0;
	int pal[16];
	rectangle r = Machine->screen[0].visarea;
	r.min_y = r.max_y = ula.scanline;

	/* set up palette */
	switch( ula.screen_mode ) {
	case 0: case 3: case 4: case 6: case 7: /* 2 colour mode */
		pal[0] = ula.current_pal[0];
		pal[1] = ula.current_pal[8];
		break;
	case 1: case 5: /* 4 colour mode */
		pal[0] = ula.current_pal[0];
		pal[1] = ula.current_pal[2];
		pal[2] = ula.current_pal[8];
		pal[3] = ula.current_pal[10];
		break;
	case 2:	/* 16 colour mode */
		for( i = 0; i < 16; i++ ) {
			pal[i] = ula.current_pal[i];
		}
		break;
	}
	/* draw line */
	switch( ula.screen_mode ) {
	case 0:
		for( i = 0; i < 80; i++ ) {
			UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)& 1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)& 1] ] );
		}
		ula.screen_addr += 1;
		if ( ( ula.scanline & 0x07 ) == 7 ) {
			ula.screen_addr += ( 0x280 - 8 );
		}
		break;
	case 1:
		x = 0;
		for( i = 0; i < 80; i++ ) {
			UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
		}
		ula.screen_addr += 1;
		if ( ( ula.scanline & 0x07 ) == 7 ) {
			ula.screen_addr += ( 0x280 - 8 );
		}
		break;
	case 2:
		for( i = 0; i < 80; i++ ) {
			UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map16[pattern>>0]] ] );
		}
		ula.screen_addr += 1;
		if ( ( ula.scanline & 0x07 ) == 7 ) {
			ula.screen_addr += ( 0x280 - 8 );
		}
		break;
	case 3:
		if ( ( ula.scanline > 249 ) || ( ula.scanline % 10 >= 8 ) ) {
			fillbitmap( tmpbitmap, Machine->pens[7], &r );
		} else {
			for( i = 0; i < 80; i++ ) {
				UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)&1] ] );
			}
			ula.screen_addr += 1;
			if ( ( ula.scanline & 0x07 ) == 7 ) {
				ula.screen_addr += ( 0x280 - 8 );
			}
		}
		break;
	case 4:
	case 7:
		for( i = 0; i < 40; i++ ) {
			UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)&1] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)&1] ] );
		}
		ula.screen_addr += 1;
		if ( ( ula.scanline & 0x07 ) == 7 ) {
			ula.screen_addr += ( 0x140 - 8 );
		}
		break;
	case 5:
		for( i = 0; i < 40; i++ ) {
			UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>3]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>2]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>1]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
			plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[map4[pattern>>0]] ] );
		}
		ula.screen_addr += 1;
		if ( ( ula.scanline & 0x07 ) == 7 ) {
			ula.screen_addr += ( 0x140 - 8 );
		}
		break;
	case 6:
		if ( ( ula.scanline > 249 ) || ( ula.scanline % 10 >= 8 ) ) {
			fillbitmap( tmpbitmap, Machine->pens[7], &r );
		} else {
			for( i = 0; i < 40; i++ ) {
				UINT8 pattern = read_vram( ula.screen_addr + i * 8 );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>7)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>6)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>5)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>4)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>3)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>2)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>1)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)&1] ] );
				plot_pixel( tmpbitmap, x++, ula.scanline, Machine->pens[ pal[(pattern>>0)&1] ] );
			}
			ula.screen_addr += 1;
			if ( ( ula.scanline % 10 ) == 7 ) {
				ula.screen_addr += ( 0x140 - 8 );
			}
		}
		break;
	}
}

INTERRUPT_GEN( electron_scanline_interrupt ) {
	if ( ula.scanline < 256 ) {
		electron_drawline();
	}
	ula.scanline = ( ula.scanline + 1 ) % 312;
	if ( ula.scanline == 100 ) {
		electron_interrupt_handler( INT_SET, INT_RTC );
	}
	if ( ula.scanline == 256 ) {
		electron_interrupt_handler( INT_SET, INT_DISPLAY_END );
	}
	if ( ula.scanline == 0 ) {
		ula.screen_addr = ula.screen_start - ula.screen_base;
	}
}

