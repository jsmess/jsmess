#include <assert.h>
#include "driver.h"

#include "includes/vc4000.h"
#include "cpu/s2650/s2650.h"

/*
  emulation of signetics 2636 video/audio device

  seams to me like a special microcontroller
  mask programmed

  note about mame s2636 emulation:
  less encapsuled
  missing grid, sound generation, retriggered sprites, paddle reading, score display
  not "rasterline" based
 */

static UINT8 sprite_collision[0x20];
static UINT8 background_collision[0x20];

typedef struct {
    UINT8 bitmap[10],x1,x2,y1,y2, res1, res2;
} SPRITE_HELPER;

typedef struct {
    const SPRITE_HELPER *data;
    int mask;
    int state;
    int delay;
    int size;
    int y;
    int finished;
    int finished_now;
} SPRITE;

static struct
{
    SPRITE sprites[4];
    int line;
    UINT8 sprite_collision;
    UINT8 background_collision;
    union
	{
		UINT8 data[0x100];
		struct
		{
			SPRITE_HELPER sprites[3];
			UINT8 res[0x10];
			SPRITE_HELPER sprite4;
			UINT8 res2[0x30];
			UINT8 grid[20][2];
			UINT8 grid_control[5];
			UINT8 res3[0x13];
			UINT8 sprite_sizes;
			UINT8 sprite_colors[2];
			UINT8 score_control;
			UINT8 res4[2];
			UINT8 background;
			UINT8 sound;
			UINT8 bcd[2];
			UINT8 background_collision;
			UINT8 sprite_collision;
		} d;
    } reg;

	mame_bitmap *bitmap;
} vc4000_video =
{
    { 
		{ &vc4000_video.reg.d.sprites[0],1 },
		{ &vc4000_video.reg.d.sprites[1],2 }, 
		{ &vc4000_video.reg.d.sprites[2],4 },
		{ &vc4000_video.reg.d.sprite4,8 }
    }    
};

VIDEO_START(vc4000)
{
    int i;
    
	for (i=0;i<0x20; i++)
	{
		sprite_collision[i]=0;
		if ((i&3)==3) sprite_collision[i]|=0x20;
		if ((i&5)==5) sprite_collision[i]|=0x10;
		if ((i&9)==9) sprite_collision[i]|=8;
		if ((i&6)==6) sprite_collision[i]|=4;
		if ((i&0xa)==0xa) sprite_collision[i]|=2;
		if ((i&0xc)==0xc) sprite_collision[i]|=1;
		background_collision[i]=0;
		if ((i&0x11)==0x11) background_collision[i]|=0x80;
		if ((i&0x12)==0x12) background_collision[i]|=0x40;
		if ((i&0x14)==0x14) background_collision[i]|=0x20;
		if ((i&0x18)==0x18) background_collision[i]|=0x10;
    }

	vc4000_video.bitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
    return 0;
}

READ8_HANDLER(vc4000_video_r)
{
	UINT8 data=0;
	switch (offset) {

	case 0xca:		// Background-sprite collision (bit 7-4) and sprite finished (bit 3-0)
		data |= vc4000_video.background_collision;
		vc4000_video.background_collision=0;
		if (vc4000_video.sprites[3].finished)
		{
			data |= 1;
			vc4000_video.sprites[3].finished = FALSE;
		}
		
		if (vc4000_video.sprites[2].finished)
		{
			data |= 2;
			vc4000_video.sprites[2].finished = FALSE;
		}
		if (vc4000_video.sprites[1].finished)
		{
			data |= 4;
			vc4000_video.sprites[1].finished = FALSE;
		}
		if (vc4000_video.sprites[0].finished)
		{
			data |= 8;
			vc4000_video.sprites[0].finished = FALSE;
		}
		break;

	case 0xcb:		//    VRST-Flag (bit 6) and intersprite collision (bit5-0)
		data = vc4000_video.sprite_collision | (vc4000_video.reg.d.sprite_collision & 0xc0);
		vc4000_video.sprite_collision = 0;
		vc4000_video.reg.d.sprite_collision &= 0xbf;
		break;

#ifndef ANALOG_HACK
	case 0xcc:
		if (!activecpu_get_reg(S2650_FO)) data=input_port_7_r(0)<<3;
		else data=input_port_8_r(0)<<3;
		break;
	case 0xcd:
		if (!activecpu_get_reg(S2650_FO)) data=input_port_9_r(0)<<3;
		else data=input_port_10_r(0)<<3;
		break;
#else

	case 0xcc:
		data = 0x66;
		// between 20 and 225
		if (!activecpu_get_reg(S2650_FO))
		{
			if (input_port_7_r(0)&0x1) data=20; 
			if (input_port_7_r(0)&0x2) data=225;
		}
		else
		{
			if (input_port_7_r(0)&0x4) data=225;
			if (input_port_7_r(0)&0x8) data=20;
		}
		break;

	case 0xcd:
		data = 0x66;
		if (!activecpu_get_reg(S2650_FO))
		{
			if (input_port_7_r(0)&0x10) data=20;
			if (input_port_7_r(0)&0x20) data=225;
		}
		else
		{
			if (input_port_7_r(0)&0x40) data=225;
			if (input_port_7_r(0)&0x80) data=20;
		}
		break;
#endif

	default:
		data = vc4000_video.reg.data[offset];
		break;
	}
	return data;
}

WRITE8_HANDLER(vc4000_video_w)
{
    switch (offset) {

    case 0xc0:						// Sprite size
	vc4000_video.sprites[0].size=1<<(data&3);
	vc4000_video.sprites[1].size=1<<((data>>2)&3);
	vc4000_video.sprites[2].size=1<<((data>>4)&3);
	vc4000_video.sprites[3].size=1<<((data>>6)&3);
	break;

    case 0xc7:						// Soundregister
	vc4000_video.reg.data[offset]=data;
	vc4000_soundport_w(0, data);
	break;

    case 0xca:			// Background-sprite collision (bit 7-4) and sprite finished (bit 3-0)
	vc4000_video.reg.data[offset]=data;
	vc4000_video.background_collision=data;
	break;

    case 0xcb:					// VRST-Flag (bit 6) and intersprite collision (bit5-0)
	vc4000_video.reg.data[offset]=data;
	vc4000_video.sprite_collision=data;
	break;
    default:
	vc4000_video.reg.data[offset]=data;
    }
}


READ8_HANDLER(vc4000_vsync_r)
{
    return vc4000_video.line >= 269 ? 0x80 : 0;
}

static const char led[20][12+1] =
{
    "aaaabbbbcccc",
    "aaaabbbbcccc",
    "aaaabbbbcccc",
    "aaaabbbbcccc",
    "llll    dddd",
    "llll    dddd",
    "llll    dddd",
    "llll    dddd",
    "kkkkmmmmeeee",
    "kkkkmmmmeeee",
    "kkkkmmmmeeee",
    "kkkkmmmmeeee",
    "jjjj    ffff",
    "jjjj    ffff",
    "jjjj    ffff",
    "jjjj    ffff",
    "iiiihhhhgggg",
    "iiiihhhhgggg",
    "iiiihhhhgggg",
    "iiiihhhhgggg"
};

static void vc4000_draw_digit(mame_bitmap *bitmap, int x, int y, int d, int line)
{
    static const int digit_to_segment[0x10]={ 
	0x0fff, 0x007c, 0x17df, 0x15ff, 0x1c7d, 0x1df7, 0x1ff7, 0x007f, 0x1fff, 0x1dff
    };
    int i,j;
    i=line;
    for (j=0; j<sizeof(led[0]); j++) {
	if (digit_to_segment[d]&(1<<(led[i][j]-'a')) ) {
	    plot_pixel(bitmap, x+j, y+i, Machine->pens[((vc4000_video.reg.d.background>>4)&7)^7]);
	}
    }
}

INLINE void vc4000_collision_plot(UINT8 *collision, UINT8 data, UINT8 color, int scale)
{
    int j,m;
    for (j=0,m=0x80; j<8; j++, m>>=1) {
	if (data&m) {
	    int i;
	    for (i=0; i<scale; i++, collision++) *collision|=color;
	} else
	    collision+=scale;
    }
}


static void vc4000_sprite_update(mame_bitmap *bitmap, UINT8 *collision, SPRITE *This)
{

    if (vc4000_video.line==0)
    {
	  This->y=This->data->y1;
	  This->state=0;
	  This->delay=0;
	  This->finished=FALSE;
    }
   This->finished_now=FALSE;

    if (vc4000_video.line>269) return;

    switch (This->state) {
	case 0:
		if (vc4000_video.line != This->y) break;
		This->state++;
	case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:case 9:case 10:

		if (vc4000_video.line<bitmap->height && This->data->x1<bitmap->width)
		{
		  vc4000_collision_plot(collision+This->data->x1, This->data->bitmap[This->state-1],
					This->mask,This->size);
		  drawgfxzoom(bitmap, Machine->gfx[0], This->data->bitmap[This->state-1],0,
			      0,0,This->data->x1,vc4000_video.line,
			      0, TRANSPARENCY_PEN,0, This->size<<16, This->size<<16);
		}
		This->delay++;
		if (This->delay>=This->size)
		{
		    This->delay=0;
		    This->state++;
		}
		if (This->state>10)
		{
		   This->finished=TRUE;
		   This->finished_now=TRUE;
		}
		break;
	case 11:

		This->y=This->data->y2;

		if (This->y==255)
		{
		    This->y=0;
		}
		else
		{
		This->y++;
		}

		if (This->y>252) break;
		
		This->delay=0;
		This->state++;

	case 12:
		if (This->y!=0)
		{
		    This->y--;
		    break;
		}
		This->state++;

	case 13: case 14: case 15: case 16: case 17: case 18: case 19:case 20:case 21:case 22:
//		if (vc4000_video.line<bitmap->height && This->data->x2<bitmap->width)
		if (vc4000_video.line < 269)
		{
		  vc4000_collision_plot(collision+This->data->x2,This->data->bitmap[This->state-13],
					This->mask,This->size);
		  drawgfxzoom(bitmap, Machine->gfx[0], This->data->bitmap[This->state-13],0,
			      0,0,This->data->x2,vc4000_video.line,
			      0, TRANSPARENCY_PEN,0, This->size<<16, This->size<<16);
		This->delay++;
		if (This->delay<This->size) break;
		This->delay=0;
		This->state++;
		if (This->state<23) break;
		This->finished=TRUE;
		This->finished_now=TRUE;
		This->state=11;
		break;
		}
		break;
    }
}

INLINE void vc4000_draw_grid(UINT8 *collision)
{
	int i, j, m, x, line=vc4000_video.line-20;
	int w, k;

	if (vc4000_video.line>=Machine->screen[0].height) return;

	plot_box(vc4000_video.bitmap, 0, vc4000_video.line, Machine->screen[0].width, 1, Machine->pens[(vc4000_video.reg.d.background)&7]);

	if (line<0 || line>=200) return;
	if (!vc4000_video.reg.d.background&8) return;

	i=(line/20)*2;
	if (line%20>=2) i++;

	k=vc4000_video.reg.d.grid_control[i>>2];
	switch (k>>6) {
		default:
		case 0:case 2: w=1;break;
		case 1: w=2;break;
		case 3: w=4;break;
	}
	switch (i&3) {
	case 0: 
		if (k&1) w=8;break;
	case 1:
		if ((line%40)<=10) {
			if (k&2) w=8;
		} else {
			if (k&4) w=8;
		}
		break;
	case 2: if (k&8) w=8;break;
	case 3:
		if ((line%40)<=30) {
			if (k&0x10) w=8;
		} else {
			if (k&0x20) w=8;
		}
		break;
	}
	for (x=30, j=0, m=0x80; j<16; j++, x+=8, m>>=1) {
		if (vc4000_video.reg.d.grid[i][j>>3]&m) {
			int l;
			for (l=0; l<w; l++) {
			collision[x+l]|=0x10;
			}
			plot_box(vc4000_video.bitmap, x, vc4000_video.line, w, 1, Machine->pens[(vc4000_video.reg.d.background>>4)&7]);
		}
		if (j==7) m=0x100;
	}
}

INTERRUPT_GEN( vc4000_video_line )
{
	int x,y,i;
	UINT8 collision[400]={0}; // better alloca or gcc feature of non constant long automatic arrays
	assert(sizeof(collision)>=Machine->screen[0].width);

	vc4000_video.line++;
	vc4000_video.line%=312;

	if (vc4000_video.line==0)
	{
		vc4000_video.background_collision=0;
		vc4000_video.sprite_collision=0;
		vc4000_video.reg.d.sprite_collision=0;
		logerror("begin of frame\n");
	}

	if (vc4000_video.line <= 270)
	{
		vc4000_draw_grid(collision);

		Machine->gfx[0]->colortable[1]=Machine->pens[8|((vc4000_video.reg.d.sprite_colors[0]>>3)&7)];
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[0]);
		Machine->gfx[0]->colortable[1]=Machine->pens[8|(vc4000_video.reg.d.sprite_colors[0]&7)];
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[1]);
		Machine->gfx[0]->colortable[1]=Machine->pens[8|((vc4000_video.reg.d.sprite_colors[1]>>3)&7)];
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[2]);
		Machine->gfx[0]->colortable[1]=Machine->pens[8|(vc4000_video.reg.d.sprite_colors[1]&7)];
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[3]);

		for (i=0; i<256; i++)
		{
			vc4000_video.sprite_collision|=sprite_collision[collision[i]];
			vc4000_video.background_collision|=background_collision[collision[i]];
		}

		y = vc4000_video.reg.d.score_control&1?180:0;
		y += 20;
		if ((vc4000_video.line>=y)&&(vc4000_video.line<y+20))
		{
			x = 58;
			vc4000_draw_digit(vc4000_video.bitmap, x, y, vc4000_video.reg.d.bcd[0]>>4, vc4000_video.line-y);
			vc4000_draw_digit(vc4000_video.bitmap, x+16, y, vc4000_video.reg.d.bcd[0]&0xf, vc4000_video.line-y);
			x = 106;
			if (vc4000_video.reg.d.score_control&2)
				x += 16;
			vc4000_draw_digit(vc4000_video.bitmap, x, y, vc4000_video.reg.d.bcd[1]>>4, vc4000_video.line-y);
			vc4000_draw_digit(vc4000_video.bitmap, x+16, y, vc4000_video.reg.d.bcd[1]&0xf, vc4000_video.line-y);
		}
	}
	if (vc4000_video.line==269) vc4000_video.reg.d.sprite_collision |=0x40;

	if ((vc4000_video.line == 269) |
		(vc4000_video.sprites[3].finished_now) |
		(vc4000_video.sprites[2].finished_now) |
		(vc4000_video.sprites[1].finished_now) |
		(vc4000_video.sprites[0].finished_now))
	{ 
		cpunum_set_input_line_vector(0, 0, 3);
		cpunum_set_input_line(0, 0, PULSE_LINE);
	}
}

VIDEO_UPDATE( vc4000 )
{
	copybitmap(bitmap, vc4000_video.bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	return 0;
}
