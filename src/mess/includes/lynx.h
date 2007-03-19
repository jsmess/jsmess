#ifndef LYNX_H
#define LYNX_H

#include "driver.h"
#include "sound/custom.h"

extern UINT32 lynx_palette[0x10];

void lynx_draw_lines(int newline);

void lynx_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions);


#define PAD_UP 0x80
#define PAD_DOWN 0x40
#define PAD_LEFT 0x20
#define PAD_RIGHT 0x10

MACHINE_START( lynx );

extern UINT8 *lynx_mem_0000;
extern UINT8 *lynx_mem_fc00;
extern UINT8 *lynx_mem_fd00;
extern UINT8 *lynx_mem_fe00;
extern UINT8 *lynx_mem_fffa;
extern size_t lynx_mem_fe00_size;

extern UINT16 lynx_granularity;
extern int lynx_rotate;

typedef struct {
	UINT8 data[0x100];
} MIKEY;

extern MIKEY mikey;
READ8_HANDLER( lynx_memory_config_r );
WRITE8_HANDLER( lynx_memory_config_w );
WRITE8_HANDLER(mikey_write);
READ8_HANDLER(mikey_read);
WRITE8_HANDLER(suzy_write);
READ8_HANDLER(suzy_read);
void lynx_timer_count_down(int nr);

void lynx_audio_reset(void);
void lynx_audio_write(int offset, UINT8 data);
UINT8 lynx_audio_read(int offset);
void lynx_audio_count_down(int nr);
void *lynx_custom_start(int clock, const struct CustomSound_interface *config);
void *lynx2_custom_start(int clock, const struct CustomSound_interface *config);
void lynx_custom_update (void);

#endif /* LYNX_H */


#ifdef INCLUDE_LYNX_LINE_FUNCTION
	int j, xi, wi, i;
	int b, p, color;

	i=blitter.mem[blitter.bitmap];
	blitter.memory_accesses++;
	for (xi=blitter.x, p=0, b=0, j=1, wi=0; (j<i);) {
	    if (p<bits) {
		b=(b<<8)|blitter.mem[blitter.bitmap+j];
		j++;
		p+=8;
		blitter.memory_accesses++;
	    }
	    for (;(p>=bits);) {
		color=blitter.color[(b>>(p-bits))&mask]; p-=bits;
		for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
		    if ((xi>=0)&&(xi<160)) {
			lynx_plot_pixel(blitter.mode, xi, y, color);
		    }
		}
		wi-=blitter.width;
	    }
	}
#endif

#ifdef INCLUDE_LYNX_LINE_RLE_FUNCTION

	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p=0, j=0, b=0, xi=blitter.x, wi=0; ; ) { // through the rle entries
	    if (p<5+bits) { // under 7 bits no complete entry
		j++;
		if (j>=blitter.mem[blitter.bitmap]) return;
		p+=8;
		b=(b<<8)|blitter.mem[blitter.bitmap+j];
		blitter.memory_accesses++;
	    }
	    t=(b>>(p-1))&1;p--;
	    count=((b>>(p-4))&0xf)+1;p-=4;
	    if (t) { // count of different pixels
		for (;count; count--) {
		    if (p<bits) {
			j++;
			if (j>=blitter.mem[blitter.bitmap]) return;
			p+=8;
			b=(b<<8)|blitter.mem[blitter.bitmap+j];
			blitter.memory_accesses++;
		    }
		    color=blitter.color[(b>>(p-bits))&mask];p-=bits;
		    for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
			if ((xi>=0)&&(xi<160)) {
			    lynx_plot_pixel(blitter.mode, xi, y, color);
			}
		    }
		    wi-=blitter.width;
		}
	    } else { // count of same pixels
		if (count==0) return;
		if (p<bits) {
		    j++;
		    if (j>=blitter.mem[blitter.bitmap]) return;
		    p+=8;
		    b=(b<<8)|blitter.mem[blitter.bitmap+j];
		    blitter.memory_accesses++;
		}
		color=blitter.color[(b>>(p-bits))&mask];p-=bits;
		for (;count; count--) {
		    for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
			if ((xi>=0)&&(xi<160)) {
			    lynx_plot_pixel(blitter.mode, xi, y, color);
			}
		    }
		    wi-=blitter.width;
		}
	    }
	}

#endif
