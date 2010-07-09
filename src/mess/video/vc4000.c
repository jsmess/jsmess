#include <assert.h>
#include "emu.h"

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

#define STICKCENTRE	(105)
#define STICKLOW	(20)
#define STICKHIGH	(225)

#define VC4000_END_LINE (269)

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
	UINT8 scolor;
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

	bitmap_t *bitmap;
} vc4000_video;

static UINT8 sprite_collision[0x20];
static UINT8 background_collision[0x20];
static UINT8 joy1_x,joy1_y,joy2_x,joy2_y;

VIDEO_START(vc4000)
{
	screen_device *screen = screen_first(*machine);
	int width = screen->width();
	int height = screen->height();
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

	joy1_x = STICKCENTRE;
	joy1_y = STICKCENTRE;
	joy2_x = STICKCENTRE;
	joy2_y = STICKCENTRE;

	memset(&vc4000_video, 0, sizeof(vc4000_video));
	for (i=0; i<3; i++)
	{
		vc4000_video.sprites[i].data = &vc4000_video.reg.d.sprites[i];
		vc4000_video.sprites[i].mask = 1 << i;
	}
	vc4000_video.sprites[3].data = &vc4000_video.reg.d.sprite4;
	vc4000_video.sprites[3].mask = 1 << 3;

	vc4000_video.bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
}

INLINE UINT8 vc4000_joystick_return_to_centre(UINT8 joy)
{
	UINT8 data;

	if (joy > (STICKCENTRE+5))
		data=joy-5;
	else
	if (joy < (STICKCENTRE-5))
		data=joy+5;
	else
		data=105;

	return data;
}

READ8_HANDLER(vc4000_video_r)
{
	UINT8 data=0;
	if (offset > 0xcf) offset &= 0xcf;	// c0-cf is mirrored at d0-df, e0-ef, f0-ff
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
		if (!activecpu_get_reg(S2650_FO)) data=input_port_read(machine, "JOY1_X");
		else data=input_port_read(machine, "JOY1_Y");
		break;
	case 0xcd:
		if (!activecpu_get_reg(S2650_FO)) data=input_port_read(machine, "JOY2_X");
		else data=input_port_read(machine, "JOY2_Y");
		break;
#else

	case 0xcc:		/* left joystick */
		if (input_port_read(space->machine, "CONFIG")&1)
		{		/* paddle */
			if (!cpu_get_reg(space->machine->device("maincpu"), S2650_FO))
			{
				data = input_port_read(space->machine, "JOYS") & 0x03;
				switch (data)
				{
				case 0x01:
					joy1_x-=5;
					if (joy1_x < STICKLOW) joy1_x=STICKLOW;
					break;
				case 0x02:
					joy1_x+=5;
					if (joy1_x > STICKHIGH) joy1_x=STICKHIGH;
					break;
				case 0x00:
					joy1_x = vc4000_joystick_return_to_centre(joy1_x);
				}
				data=joy1_x;
			}
			else
			{
				data = input_port_read(space->machine, "JOYS") & 0x0c;
				switch (data)
				{
				case 0x08:
					joy1_y-=5;
					if (joy1_y < STICKLOW) joy1_y=STICKLOW;
					break;
				case 0x04:
					joy1_y+=5;
					if (joy1_y > STICKHIGH) joy1_y=STICKHIGH;
					break;
		//      case 0x00:
		//          joy1_y = vc4000_joystick_return_to_centre(joy1_y);
				}
				data=joy1_y;
			}
		}
		else
		{		/* buttons */
			if (!cpu_get_reg(space->machine->device("maincpu"), S2650_FO))
			{
				data = input_port_read(space->machine, "JOYS") & 0x03;
				switch (data)
				{
				case 0x01:
					joy1_x=STICKLOW;
					break;
				case 0x02:
					joy1_x=STICKHIGH;
					break;
				default:			/* autocentre */
					joy1_x=STICKCENTRE;
				}
				data=joy1_x;
			}
			else
			{
				data = input_port_read(space->machine, "JOYS") & 0x0c;
				switch (data)
				{
				case 0x08:
					joy1_y=STICKLOW;
					break;
				case 0x04:
					joy1_y=STICKHIGH;
					break;
				default:
					joy1_y=STICKCENTRE;
				}
				data=joy1_y;
			}
		}
		break;

	case 0xcd:		/* right joystick */
		if (input_port_read(space->machine, "CONFIG")&1)
		{
			if (!cpu_get_reg(space->machine->device("maincpu"), S2650_FO))
			{
				data = input_port_read(space->machine, "JOYS") & 0x30;
				switch (data)
				{
				case 0x10:
					joy2_x-=5;
					if (joy2_x < STICKLOW) joy2_x=STICKLOW;
					break;
				case 0x20:
					joy2_x+=5;
					if (joy2_x > STICKHIGH) joy2_x=STICKHIGH;
				case 0x00:
					joy2_x = vc4000_joystick_return_to_centre(joy2_x);
					break;
				}
				data=joy2_x;
			}
			else
			{
				data = input_port_read(space->machine, "JOYS") & 0xc0;
				switch (data)
				{
				case 0x80:
					joy2_y-=5;
					if (joy2_y < STICKLOW) joy2_y=STICKLOW;
					break;
				case 0x40:
					joy2_y+=5;
					if (joy2_y > STICKHIGH) joy2_y=STICKHIGH;
					break;
			//  case 0x00:
			//      joy2_y = vc4000_joystick_return_to_centre(joy2_y);
				}
				data=joy2_y;
			}
		}
		else
		{
			if (!cpu_get_reg(space->machine->device("maincpu"), S2650_FO))
			{
				data = input_port_read(space->machine, "JOYS") & 0x30;
				switch (data)
				{
				case 0x10:
					joy2_x=STICKLOW;
					break;
				case 0x20:
					joy2_x=STICKHIGH;
					break;
				default:			/* autocentre */
					joy2_x=STICKCENTRE;
				}
				data=joy2_x;
			}
			else
			{
				data = input_port_read(space->machine, "JOYS") & 0xc0;
				switch (data)
				{
				case 0x80:
					joy2_y=STICKLOW;
					break;
				case 0x40:
					joy2_y=STICKHIGH;
					break;
				default:
					joy2_y=STICKCENTRE;
				}
				data=joy2_y;
			}
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
//  vc4000_video.reg.data[offset]=data;
	if (offset > 0xcf) offset &= 0xcf;	// c0-cf is mirrored at d0-df, e0-ef, f0-ff

	switch (offset) {

	case 0xc0:						// Sprite size
		vc4000_video.sprites[0].size=1<<(data&3);
		vc4000_video.sprites[1].size=1<<((data>>2)&3);
		vc4000_video.sprites[2].size=1<<((data>>4)&3);
		vc4000_video.sprites[3].size=1<<((data>>6)&3);
		break;

	case 0xc1:						// Sprite 1+2 color
		vc4000_video.sprites[0].scolor=((~data>>3)&7);
		vc4000_video.sprites[1].scolor=(~data&7);
		break;

	case 0xc2:						// Sprite 2+3 color
		vc4000_video.sprites[2].scolor=((~data>>3)&7);
		vc4000_video.sprites[3].scolor=(~data&7);
		break;

	case 0xc3:						// Score control
		vc4000_video.reg.d.score_control = data;
		break;

	case 0xc6:						// Background color
		vc4000_video.reg.d.background = data;
		break;

	case 0xc7:						// Soundregister
		vc4000_video.reg.data[offset] = data;
		vc4000_soundport_w(space->machine->device("custom"), 0, data);
		break;

	case 0xc8:						// Digits 1 and 2
		vc4000_video.reg.d.bcd[0] = data;
		break;

	case 0xc9:						// Digits 3 and 4
		vc4000_video.reg.d.bcd[1] = data;
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
	return vc4000_video.line >= VC4000_END_LINE ? 0x80 : 0;
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

static UINT8 objects[256];

static void vc4000_draw_digit(bitmap_t *bitmap, int x, int y, int d, int line)
{
	static const int digit_to_segment[0x10]={
	0x0fff, 0x007c, 0x17df, 0x15ff, 0x1c7d, 0x1df7, 0x1ff7, 0x007f, 0x1fff, 0x1dff
	};

	int i=line,j;

	for (j=0; j<sizeof(led[0]); j++)
	{
		if (digit_to_segment[d]&(1<<(led[i][j]-'a')) )
			*BITMAP_ADDR16(bitmap, y+i, x+j) = ((vc4000_video.reg.d.background>>4)&7)^7;
	}
}

INLINE void vc4000_collision_plot(UINT8 *collision, UINT8 data, UINT8 color, int scale)
{
	int i,j,m;

	for (j=0,m=0x80; j<8; j++, m>>=1)
	{
		if (data&m)
			for (i=0; i<scale; i++, collision++) *collision|=color;
		else
			collision+=scale;
	}
}


static void vc4000_sprite_update(bitmap_t *bitmap, UINT8 *collision, SPRITE *This)
{
	int i,j,m;

	if (vc4000_video.line==0)
	{
		This->y=This->data->y1;
		This->state=0;
		This->delay=0;
		This->finished=FALSE;
	}

	This->finished_now=FALSE;

	if (vc4000_video.line>VC4000_END_LINE) return;

	switch (This->state)
	{
	case 0:
		if (vc4000_video.line != This->y + 2) break;
		This->state++;

	case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:case 9:case 10:

		vc4000_collision_plot(collision+This->data->x1, This->data->bitmap[This->state-1],This->mask,This->size);

		for (j=0,m=0x80; j<8; j++, m>>=1)
		{
			if (This->data->bitmap[This->state-1]&m)
			{
				for (i=0; i<This->size; i++)
				{
					objects[This->data->x1 + i + j*This->size] |= This->scolor;
					objects[This->data->x1 + i + j*This->size] &= 7;
				}
			}
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
			This->y=0;
		else
			This->y++;

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

		vc4000_collision_plot(collision+This->data->x2,This->data->bitmap[This->state-13],This->mask,This->size);
		for (j=0,m=0x80; j<8; j++, m>>=1)
		{
			if (This->data->bitmap[This->state-13]&m)
			{
				for (i=0; i<This->size; i++)
				{
					objects[This->data->x2 + i + j*This->size] |= This->scolor;
					objects[This->data->x2 + i + j*This->size] &= 7;
				}
			}
		}
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
}

INLINE void vc4000_draw_grid(running_machine *machine, UINT8 *collision)
{
	screen_device *screen = screen_first(*machine);
	int width = screen->width();
	int height = screen->height();
	int i, j, m, x, line=vc4000_video.line-20;
	int w, k;

	if (vc4000_video.line>=height) return;

	plot_box(vc4000_video.bitmap, 0, vc4000_video.line, width, 1, (vc4000_video.reg.d.background)&7);

	if (line<0 || line>=200) return;
	if (~vc4000_video.reg.d.background & 8) return;

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
	for (x=30, j=0, m=0x80; j<16; j++, x+=8, m>>=1)
	{
		if (vc4000_video.reg.d.grid[i][j>>3]&m)
		{
			int l;
			for (l=0; l<w; l++) collision[x+l]|=0x10;
			plot_box(vc4000_video.bitmap, x, vc4000_video.line, w, 1, (vc4000_video.reg.d.background>>4)&7);
		}
		if (j==7) m=0x100;
	}
}

INTERRUPT_GEN( vc4000_video_line )
{
	int x,y,i;
	UINT8 collision[400]={0}; // better alloca or gcc feature of non constant long automatic arrays
	static UINT8 irq_pause=0;
	const rectangle &visarea = device->machine->primary_screen->visible_area();
	assert(ARRAY_LENGTH(collision) >= device->machine->primary_screen->width());

	vc4000_video.line++;
	if (irq_pause) irq_pause++;
	if (vc4000_video.line>311) vc4000_video.line=0;

	if (vc4000_video.line==0)
	{
		vc4000_video.background_collision=0;
		vc4000_video.sprite_collision=0;
		vc4000_video.reg.d.sprite_collision=0;
//      logerror("begin of frame\n");
	}

	if (irq_pause>10)
	{
		cputag_set_input_line(device->machine, "maincpu", 0, CLEAR_LINE);
		irq_pause = 0;
	}

	if (vc4000_video.line <= VC4000_END_LINE)
	{
		vc4000_draw_grid(device->machine, collision);

		/* init object colours */
		for (i=visarea.min_x; i<visarea.max_x; i++) objects[i]=8;

		/* calculate object colours and OR overlapping object colours */
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[0]);
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[1]);
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[2]);
		vc4000_sprite_update(vc4000_video.bitmap, collision, &vc4000_video.sprites[3]);

		for (i=visarea.min_x; i<visarea.max_x; i++)
		{
			vc4000_video.sprite_collision|=sprite_collision[collision[i]];
			vc4000_video.background_collision|=background_collision[collision[i]];
			/* display final object colours */
			if (objects[i] < 8)
					*BITMAP_ADDR16(vc4000_video.bitmap, vc4000_video.line, i) = objects[i];
		}

		y = vc4000_video.reg.d.score_control&1?200:20;

		if ((vc4000_video.line>=y)&&(vc4000_video.line<y+20))
		{
			x = 60;
			vc4000_draw_digit(vc4000_video.bitmap, x, y, vc4000_video.reg.d.bcd[0]>>4, vc4000_video.line-y);
			vc4000_draw_digit(vc4000_video.bitmap, x+16, y, vc4000_video.reg.d.bcd[0]&0xf, vc4000_video.line-y);
			if (vc4000_video.reg.d.score_control&2)	x -= 16;
			vc4000_draw_digit(vc4000_video.bitmap, x+48, y, vc4000_video.reg.d.bcd[1]>>4, vc4000_video.line-y);
			vc4000_draw_digit(vc4000_video.bitmap, x+64, y, vc4000_video.reg.d.bcd[1]&0xf, vc4000_video.line-y);
		}
	}
	if (vc4000_video.line==VC4000_END_LINE) vc4000_video.reg.d.sprite_collision |=0x40;

	if (((vc4000_video.line == VC4000_END_LINE) |
		(vc4000_video.sprites[3].finished_now) |
		(vc4000_video.sprites[2].finished_now) |
		(vc4000_video.sprites[1].finished_now) |
		(vc4000_video.sprites[0].finished_now)) && (!irq_pause))
		{
			cputag_set_input_line_and_vector(device->machine, "maincpu", 0, ASSERT_LINE, 3);
			irq_pause=1;
		}
}

VIDEO_UPDATE( vc4000 )
{
	copybitmap(bitmap, vc4000_video.bitmap, 0, 0, 0, 0, cliprect);
	return 0;
}
