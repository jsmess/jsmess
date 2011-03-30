/**********************************************************************

    Intel 82720 Graphics Display Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - implement FIFO as ring buffer
    - commands
        - DMAR
        - DMAW
    - incomplete / unimplemented FIGD / GCHRD draw modes
        - Arc
        - FIGD character
		- slanted character
    	- GCHRD character (needs rewrite)
    - read-modify-write cycle
        - read data
        - modify data
        - write data
	- QX-10 diagnostic test has positioning bugs with the bitmap display test;
	- QX-10 diagnostic test misses the zooming factor (external pin);
	- compis2 SAD address for bitmap is 0x20000 for whatever reason (presumably missing banking);
	- A5105 has a FIFO bug with the RDAT, should be a lot larger when it scrolls up;

    - honor visible area
    - wide mode (32-bit access)
    - light pen

*/

#include "emu.h"
#include "upd7220.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

// todo typedef
enum
{
	COMMAND_INVALID = -1,
	COMMAND_RESET,
	COMMAND_SYNC,
	COMMAND_VSYNC,
	COMMAND_CCHAR,
	COMMAND_START,
	COMMAND_BCTRL,
	COMMAND_ZOOM,
	COMMAND_CURS,
	COMMAND_PRAM,
	COMMAND_PITCH,
	COMMAND_WDAT,
	COMMAND_MASK,
	COMMAND_FIGS,
	COMMAND_FIGD,
	COMMAND_GCHRD,
	COMMAND_RDAT,
	COMMAND_CURD,
	COMMAND_LPRD,
	COMMAND_DMAR,
	COMMAND_DMAW,
	COMMAND_5A
};

enum
{
	FIFO_READ = 0,
	FIFO_WRITE
};

enum
{
	FIFO_EMPTY = -1,
	FIFO_PARAMETER,
	FIFO_COMMAND
};

#define UPD7220_COMMAND_RESET			0x00
#define UPD7220_COMMAND_SYNC			0x0e // & 0xfe
#define UPD7220_COMMAND_VSYNC			0x6e // & 0xfe
#define UPD7220_COMMAND_CCHAR			0x4b
#define UPD7220_COMMAND_START			0x6b
#define UPD7220_COMMAND_BCTRL			0x0c // & 0xfe
#define UPD7220_COMMAND_ZOOM			0x46
#define UPD7220_COMMAND_CURS			0x49
#define UPD7220_COMMAND_PRAM			0x70 // & 0xf0
#define UPD7220_COMMAND_PITCH			0x47
#define UPD7220_COMMAND_WDAT			0x20 // & 0xe4
#define UPD7220_COMMAND_MASK			0x4a
#define UPD7220_COMMAND_FIGS			0x4c
#define UPD7220_COMMAND_FIGD			0x6c
#define UPD7220_COMMAND_GCHRD			0x68
#define UPD7220_COMMAND_RDAT			0xa0 // & 0xe4
#define UPD7220_COMMAND_CURD			0xe0
#define UPD7220_COMMAND_LPRD			0xc0
#define UPD7220_COMMAND_DMAR			0xa4 // & 0xe4
#define UPD7220_COMMAND_DMAW			0x24 // & 0xe4
#define UPD7220_COMMAND_5A				0x5a

#define UPD7220_SR_DATA_READY			0x01
#define UPD7220_SR_FIFO_FULL			0x02
#define UPD7220_SR_FIFO_EMPTY			0x04
#define UPD7220_SR_DRAWING_IN_PROGRESS	0x08
#define UPD7220_SR_DMA_EXECUTE			0x10
#define UPD7220_SR_VSYNC_ACTIVE			0x20
#define UPD7220_SR_HBLANK_ACTIVE		0x40
#define UPD7220_SR_LIGHT_PEN_DETECT		0x80

#define UPD7220_MODE_S					0x01
#define UPD7220_MODE_REFRESH_RAM		0x04
#define UPD7220_MODE_I					0x08
#define UPD7220_MODE_DRAW_ON_RETRACE	0x10
#define UPD7220_MODE_DISPLAY_MASK		0x22
#define UPD7220_MODE_DISPLAY_MIXED		0x00
#define UPD7220_MODE_DISPLAY_GRAPHICS	0x02
#define UPD7220_MODE_DISPLAY_CHARACTER	0x20
#define UPD7220_MODE_DISPLAY_INVALID	0x22

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*upd7220_command) (device_t *device);

typedef struct _upd7220_t upd7220_t;
struct _upd7220_t
{
	devcb_resolved_write_line	out_drq_func;
	devcb_resolved_write_line	out_hsync_func;
	devcb_resolved_write_line	out_vsync_func;
	devcb_resolved_write_line	out_blank_func;
	upd7220_display_pixels_func display_func;
	upd7220_draw_text_line	    draw_text_func;

	screen_device *screen;	/* screen */

	int clock;						/* device clock */

	UINT16 mask;					/* mask register */
	UINT8 pitch;					/* number of word addresses in display memory in the horizontal direction */
	UINT32 ead;						/* execute word address */
	UINT16 dad;						/* dot address within the word */
	UINT32 lad;						/* light pen address */

	UINT8 ra[16];					/* parameter RAM */
	int ra_addr;					/* parameter RAM address */

	UINT8 sr;						/* status register */
	UINT8 cr;						/* command register */
	UINT8 pr[17];					/* parameter byte register */
	int param_ptr;					/* parameter pointer */

	UINT8 fifo[16];					/* FIFO data queue */
	int fifo_flag[16];				/* FIFO flag queue */
	int fifo_ptr;					/* FIFO pointer */
	int fifo_dir;					/* FIFO direction */

	UINT8 mode;						/* mode of operation */
	UINT8 draw_mode;				/* mode of drawing */

	int de;							/* display enabled */
	int m;							/* 0 = accept external vertical sync (slave mode) / 1 = generate & output vertical sync (master mode) */
	int aw;							/* active display words per line - 2 (must be even number with bit 0 = 0) */
	int al;							/* active display lines per video field */
	int vs;							/* vertical sync width - 1 */
	int vfp;						/* vertical front porch width - 1 */
	int vbp;						/* vertical back porch width - 1 */
	int hs;							/* horizontal sync width - 1 */
	int hfp;						/* horizontal front porch width - 1 */
	int hbp;						/* horizontal back porch width - 1 */

	int dc;							/* display cursor */
	int sc;							/* 0 = blinking cursor / 1 = steady cursor */
	int br;							/* blink rate */
	int ctop;						/* cursor top line number in the row */
	int cbot;						/* cursor bottom line number in the row (CBOT < LR) */
	int lr;							/* lines per character row - 1 */

	int disp;						/* display zoom factor */
	int gchr;						/* zoom factor for graphics character writing and area filling */

	struct {
		UINT8 dir;					/* figs param 0: drawing direction */
		UINT8 figure_type;			/* figs param 1: figure type */
		UINT16 dc;					/* figs param 2: */
		UINT16 d;					/* figs param 3: */
		UINT16 d1;					/* figs param 4: */
		UINT16 d2;					/* figs param 5: */
		UINT16 dm;					/* figs param 6: */
	}figs;

	/* timers */
	emu_timer *vsync_timer;			/* vertical sync timer */
	emu_timer *hsync_timer;			/* horizontal sync timer */
	emu_timer *blank_timer;			/* CRT blanking timer */

	UINT8 vram[0x40000];
	UINT32 vram_bank;

	UINT8 bitmap_mod;
};

static const int x_dir[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
static const int y_dir[8] = { 1, 1, 0,-1,-1,-1, 0, 1};
static const int x_dir_dot[8] = { 1, 1, 0,-1,-1,-1, 0, 1};
static const int y_dir_dot[8] = { 0,-1,-1,-1, 0, 1, 1, 1};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7220_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == UPD7220));
	return (upd7220_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const upd7220_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == UPD7220));
	return (const upd7220_interface *) device->baseconfig().static_config();
}

INLINE void fifo_clear(upd7220_t *upd7220)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		upd7220->fifo[i] = 0;
		upd7220->fifo_flag[i] = FIFO_EMPTY;
	}

	upd7220->fifo_ptr = -1;

	upd7220->sr &= ~UPD7220_SR_DATA_READY;
	upd7220->sr |= UPD7220_SR_FIFO_EMPTY;
	upd7220->sr &= ~UPD7220_SR_FIFO_FULL;
}

INLINE int fifo_param_count(upd7220_t *upd7220)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if (upd7220->fifo_flag[i] != FIFO_PARAMETER) break;
	}

	return i;
}

INLINE void fifo_set_direction(upd7220_t *upd7220, int dir)
{
	if (upd7220->fifo_dir != dir)
	{
		fifo_clear(upd7220);
	}

	upd7220->fifo_dir = dir;
}

INLINE void queue(upd7220_t *upd7220, UINT8 data, int flag)
{
	if (upd7220->fifo_ptr < 15)
	{
		upd7220->fifo_ptr++;

		upd7220->fifo[upd7220->fifo_ptr] = data;
		upd7220->fifo_flag[upd7220->fifo_ptr] = flag;

		if (upd7220->fifo_ptr == 16)
		{
			upd7220->sr |= UPD7220_SR_FIFO_FULL;
		}

		upd7220->sr &= ~UPD7220_SR_FIFO_EMPTY;
	}
	else
	{
		// TODO what happen? somebody set us up the bomb
		printf("FIFO?\n");
	}
}

INLINE void dequeue(upd7220_t *upd7220, UINT8 *data, int *flag)
{
	int i;

	*data = upd7220->fifo[0];
	*flag = upd7220->fifo_flag[0];

	if (upd7220->fifo_ptr > -1)
	{
		for (i = 0; i < 15; i++)
		{
			upd7220->fifo[i] = upd7220->fifo[i + 1];
			upd7220->fifo_flag[i] = upd7220->fifo_flag[i + 1];
		}

		upd7220->fifo[15] = 0;
		upd7220->fifo_flag[15] = 0;

		upd7220->fifo_ptr--;

		if (upd7220->fifo_ptr == -1)
		{
			upd7220->sr &= ~UPD7220_SR_DATA_READY;
			upd7220->sr |= UPD7220_SR_FIFO_EMPTY;
		}
	}
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    update_vsync_timer -
-------------------------------------------------*/

static void update_vsync_timer(upd7220_t *upd7220, int state)
{
	int next_y = state ? upd7220->vs : 0;

	attotime duration = upd7220->screen->time_until_pos(next_y, 0);

	upd7220->vsync_timer->adjust(duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( vsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vsync_tick )
{
	device_t *device = (device_t *)ptr;
	upd7220_t *upd7220 = get_safe_token(device);

	if (param)
	{
		upd7220->sr |= UPD7220_SR_VSYNC_ACTIVE;
	}
	else
	{
		upd7220->sr &= ~UPD7220_SR_VSYNC_ACTIVE;
	}

	devcb_call_write_line(&upd7220->out_vsync_func, param);

	update_vsync_timer(upd7220, param);
}

/*-------------------------------------------------
    update_hsync_timer -
-------------------------------------------------*/

static void update_hsync_timer(upd7220_t *upd7220, int state)
{
	int y = upd7220->screen->vpos();

	int next_x = state ? upd7220->hs : 0;
	int next_y = state ? y : ((y + 1) % upd7220->al);

	attotime duration = upd7220->screen->time_until_pos(next_y, next_x);

	upd7220->hsync_timer->adjust(duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( hsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( hsync_tick )
{
	device_t *device = (device_t *)ptr;
	upd7220_t *upd7220 = get_safe_token(device);

	if (param)
	{
		upd7220->sr |= UPD7220_SR_HBLANK_ACTIVE;
	}
	else
	{
		upd7220->sr &= ~UPD7220_SR_HBLANK_ACTIVE;
	}

	devcb_call_write_line(&upd7220->out_hsync_func, param);

	update_hsync_timer(upd7220, param);
}

/*-------------------------------------------------
    update_blank_timer -
-------------------------------------------------*/

static void update_blank_timer(upd7220_t *upd7220, int state)
{
	int y = upd7220->screen->vpos();

	int next_x = state ? (upd7220->hs + upd7220->hbp) : (upd7220->hs + upd7220->hbp + (upd7220->aw << 3));
	int next_y = state ? ((y + 1) % (upd7220->vs + upd7220->vbp + upd7220->al + upd7220->vfp - 1)) : y;

	attotime duration = upd7220->screen->time_until_pos(next_y, next_x);

	upd7220->hsync_timer->adjust(duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( blank_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( blank_tick )
{
	device_t *device = (device_t *)ptr;
	upd7220_t *upd7220 = get_safe_token(device);

	if (param)
	{
		upd7220->sr |= UPD7220_SR_HBLANK_ACTIVE;
	}
	else
	{
		upd7220->sr &= ~UPD7220_SR_HBLANK_ACTIVE;
	}

	devcb_call_write_line(&upd7220->out_blank_func, param);

	update_blank_timer(upd7220, param);
}

/*-------------------------------------------------
    recompute_parameters -
-------------------------------------------------*/

static void recompute_parameters(device_t *device)
{
	upd7220_t *upd7220 = get_safe_token(device);

	int horiz_pix_total = (upd7220->hs + upd7220->hbp + upd7220->aw + upd7220->hfp) * 8;
	int vert_pix_total = upd7220->vs + upd7220->vbp + upd7220->al + upd7220->vfp;

	//printf("%d %d %d %d\n",upd7220->hs,upd7220->hbp,upd7220->aw,upd7220->hfp);
	//printf("%d %d\n",upd7220->aw * 8,upd7220->pitch * 8);

	if(horiz_pix_total == 0 || vert_pix_total == 0) //bail out if screen params aren't valid
		return;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(60); //HZ_TO_ATTOSECONDS(device->clock() * 8) * horiz_pix_total * vert_pix_total;

	rectangle visarea;

	visarea.min_x = 0; //(upd7220->hs + upd7220->hbp) * 8;
	visarea.min_y = 0; //upd7220->vs + upd7220->vbp;
	visarea.max_x = upd7220->aw * 8 - 1;//horiz_pix_total - (upd7220->hfp * 8) - 1;
	visarea.max_y = upd7220->al - 1;//vert_pix_total - upd7220->vfp - 1;


	if (LOG)
	{
		logerror("uPD7220 '%s' Screen: %u x %u @ %f Hz\n", device->tag(), horiz_pix_total, vert_pix_total, 1 / ATTOSECONDS_TO_DOUBLE(refresh));
		logerror("uPD7220 '%s' Visible Area: (%u, %u) - (%u, %u)\n", device->tag(), visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	}

	if (upd7220->m)
	{
		upd7220->screen->configure(horiz_pix_total, vert_pix_total, visarea, refresh);

		update_hsync_timer(upd7220, 0);
		update_vsync_timer(upd7220, 0);
	}
	else
	{
		upd7220->hsync_timer->enable(0);
		upd7220->vsync_timer->enable(0);
	}

	update_blank_timer(upd7220, 0);
}

/*-----------------------------------------------------
    reset_figs_param - reset figs after each vram rmw
-----------------------------------------------------*/

static void reset_figs_param(upd7220_t *upd7220)
{
	upd7220->figs.dc = 0x0000;
	upd7220->figs.d = 0x0008;
	upd7220->figs.d1 = 0x0008;
	upd7220->figs.d2 = 0x0000;
	upd7220->figs.dm = 0x0000;
}

/*-------------------------------------------------
    advance_ead - advance EAD pointer
-------------------------------------------------*/

#define EAD			upd7220->ead
#define DAD			upd7220->dad
#define P			x_dir[upd7220->figs.dir] + (y_dir[upd7220->figs.dir] * upd7220->pitch)
#define MSB(value)	(BIT(value, 15))
#define LSB(value)	(BIT(value, 0))
#define LR(value)	((value << 1) | MSB(value))
#define RR(value)	((LSB(value) << 15) | (value >> 1))

static void advance_ead(upd7220_t *upd7220)
{
	switch (upd7220->draw_mode & 0x07)
	{
	case 0:
		EAD += P;
		break;

	case 1:
		EAD += P;
		if (MSB(DAD)) EAD++;
		DAD = LR(DAD);
		break;

	case 2:
		if (MSB(DAD)) EAD++;
		DAD = LR(DAD);
		break;

	case 3:
		EAD -= P;
		if (MSB(DAD)) EAD++;
		DAD = LR(DAD);
		break;

	case 4:
		EAD -= P;
		break;

	case 5:
		EAD -= P;
		if (LSB(DAD)) EAD--;
		DAD = RR(DAD);
		break;

	case 6:
		if (LSB(DAD)) EAD--;
		DAD = RR(DAD);
		break;

	case 7:
		EAD += P;
		if (LSB(DAD)) EAD--;
		DAD = RR(DAD);
		break;
	}

	EAD &= 0x3ffff;
}

/*-------------------------------------------------
    read_vram - read in the device memory space
-------------------------------------------------*/

static void read_vram(upd7220_t *upd7220,UINT8 type, UINT8 mod)
{
	int i;

	if(type == 1)
	{
		logerror("uPD7220 invalid type 1 RDAT parameter\n");
		return;
	}

	if(mod)
		logerror("uPD7220 RDAT used with mod = %02x?\n",mod);

	for(i=0;i<upd7220->figs.dc;i++)
	{
		switch(type)
		{
			case 0:
				queue(upd7220, upd7220->vram[upd7220->ead*2], 0);
				queue(upd7220, upd7220->vram[upd7220->ead*2+1], 0);
				break;
			case 2:
				queue(upd7220, upd7220->vram[upd7220->ead*2], 0);
				break;
			case 3:
				queue(upd7220, upd7220->vram[upd7220->ead*2+1], 0);
				break;
		}

		advance_ead(upd7220);
	}
}

/*-------------------------------------------------
    write_vram - write in the device memory space
-------------------------------------------------*/

static void write_vram(upd7220_t *upd7220,UINT8 type, UINT8 mod)
{
	int i;
	UINT16 result;
	if(type == 1)
	{
		printf("uPD7220 invalid type 1 WDAT parameter\n");
		return;
	}

	result = 0;

	switch(type)
	{
		case 0:
			result = (upd7220->pr[1] & 0xff);
			result |= (upd7220->pr[2] << 8);
			result &= upd7220->mask;
			break;
		case 2:
			result = (upd7220->pr[1] & 0xff);
			result &= (upd7220->mask & 0xff);
			break;
		case 3:
			result = (upd7220->pr[1] << 8);
			result &= (upd7220->mask & 0xff00);
			break;
	}

	//if(result)
	{
		//printf("%04x %02x %02x %04x %02x %02x\n",upd7220->vram[upd7220->ead],upd7220->pr[1],upd7220->pr[2],upd7220->mask,type,mod);
		//printf("%04x %02x %02x\n",upd7220->ead,upd7220->figs.dir,upd7220->pitch);
		//printf("%04x %04x %02x %04x\n",upd7220->ead,result,mod,upd7220->figs.dc);
	}

	for(i=0;i<upd7220->figs.dc + 1;i++)
	{
		switch(mod & 3)
		{
			case 0x00: //replace
				if(type == 0 || type == 2)
					upd7220->vram[upd7220->ead*2+0] = result & 0xff;
				if(type == 0 || type == 3)
					upd7220->vram[upd7220->ead*2+1] = result >> 8;
				break;
			case 0x01: //complement
				if(type == 0 || type == 2)
					upd7220->vram[upd7220->ead*2+0] ^= result & 0xff;
				if(type == 0 || type == 3)
					upd7220->vram[upd7220->ead*2+1] ^= result >> 8;
				break;
			case 0x02: //reset to zero
				if(type == 0 || type == 2)
					upd7220->vram[upd7220->ead*2+0] &= ~result;
				if(type == 0 || type == 3)
					upd7220->vram[upd7220->ead*2+1] &= ~(result >> 8);
				break;
			case 0x03: //set to one
				if(type == 0 || type == 2)
					upd7220->vram[upd7220->ead*2+0] |= result;
				if(type == 0 || type == 3)
					upd7220->vram[upd7220->ead*2+1] |= (result >> 8);
				break;
		}

		advance_ead(upd7220);
	}
}

static UINT16 check_pattern(upd7220_t *upd7220, UINT16 pattern)
{
	UINT16 res;

	res = 0;

	switch(upd7220->bitmap_mod & 3)
	{
		case 0: res = pattern; break; //replace
		case 1: res = pattern; break; //complement
		case 2: res = 0; break; //reset to zero
		case 3: res |= 0xffff; break; //set to one
	}

	return res;
}

static void draw_pixel(upd7220_t *upd7220,int x,int y,UINT16 tile_data)
{
	UINT32 addr = (y * upd7220->pitch * 2 + (x >> 3)) & 0x3ffff;
	int dad;

	dad = x & 0x7;

	if((upd7220->bitmap_mod & 3) == 1)
	{
		upd7220->vram[addr + upd7220->vram_bank] ^= ((tile_data) & (0x80 >> (dad)));
	}
	else
	{
		upd7220->vram[addr + upd7220->vram_bank] &= ~(0x80 >> (dad));
		upd7220->vram[addr + upd7220->vram_bank] |= ((tile_data) & (0x80 >> (dad)));
	}
}

static void draw_line(upd7220_t *upd7220,int x,int y)
{
	int line_size,i;
	const int line_x_dir[8] = { 0, 1, 1, 0, 0,-1,-1, 0};
	const int line_y_dir[8] = { 1, 0, 0,-1,-1, 0, 0, 1};
	const int line_x_step[8] = { 1, 0, 0, 1,-1, 0, 0,-1 };
	const int line_y_step[8] = { 0, 1,-1, 0, 0,-1, 1, 0 };
	UINT16 line_pattern;
	int line_step = 0;
	UINT8 dot;

	line_size = upd7220->figs.dc + 1;
	line_pattern = check_pattern(upd7220,(upd7220->ra[8]) | (upd7220->ra[9]<<8));

	for(i = 0;i<line_size;i++)
	{
		line_step = (upd7220->figs.d1 * i);
		line_step/= (upd7220->figs.dc + 1);
		line_step >>= 1;
		dot = ((line_pattern >> (i & 0xf)) & 1) << 7;
		draw_pixel(upd7220,x + (line_step*line_x_step[upd7220->figs.dir]),y + (line_step*line_y_step[upd7220->figs.dir]),dot >> ((x + line_step*line_x_step[upd7220->figs.dir]) & 0x7));
		x += line_x_dir[upd7220->figs.dir];
		y += line_y_dir[upd7220->figs.dir];
	}

	/* TODO: check me*/
	x += (line_step*line_x_step[upd7220->figs.dir]);
	y += (line_step*line_y_step[upd7220->figs.dir]);

	upd7220->ead = (x >> 4) + (y * upd7220->pitch);
	upd7220->dad = x & 0x0f;
}

static void draw_rectangle(upd7220_t *upd7220,int x,int y)
{
	int i;
	const int rect_x_dir[8] = { 0, 1, 0,-1, 1, 1,-1,-1 };
	const int rect_y_dir[8] = { 1, 0,-1, 0, 1,-1,-1, 1 };
	UINT8 rect_type,rect_dir;
	UINT16 line_pattern;
	UINT8 dot;

	printf("uPD7220 rectangle check: %d %d %02x %08x\n",x,y,upd7220->figs.dir,upd7220->ead);

	line_pattern = check_pattern(upd7220,(upd7220->ra[8]) | (upd7220->ra[9]<<8));
	rect_type = (upd7220->figs.dir & 1) << 2;
	rect_dir = rect_type | (((upd7220->figs.dir >> 1) + 0) & 3);

	for(i = 0;i < upd7220->figs.d;i++)
	{
		dot = ((line_pattern >> ((i+upd7220->dad) & 0xf)) & 1) << 7;
		draw_pixel(upd7220,x,y,dot >> (x & 0x7));
		x+=rect_x_dir[rect_dir];
		y+=rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((upd7220->figs.dir >> 1) + 1) & 3);

	for(i = 0;i < upd7220->figs.d2;i++)
	{
		dot = ((line_pattern >> ((i+upd7220->dad) & 0xf)) & 1) << 7;
		draw_pixel(upd7220,x,y,dot >> (x & 0x7));
		x+=rect_x_dir[rect_dir];
		y+=rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((upd7220->figs.dir >> 1) + 2) & 3);

	for(i = 0;i < upd7220->figs.d;i++)
	{
		dot = ((line_pattern >> ((i+upd7220->dad) & 0xf)) & 1) << 7;
		draw_pixel(upd7220,x,y,dot >> (x & 0x7));
		x+=rect_x_dir[rect_dir];
		y+=rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((upd7220->figs.dir >> 1) + 3) & 3);

	for(i = 0;i < upd7220->figs.d2;i++)
	{
		dot = ((line_pattern >> ((i+upd7220->dad) & 0xf)) & 1) << 7;
		draw_pixel(upd7220,x,y,dot >> (x & 0x7));
		x+=rect_x_dir[rect_dir];
		y+=rect_y_dir[rect_dir];
	}

	upd7220->ead = (x >> 4) + (y * upd7220->pitch);
	upd7220->dad = x & 0x0f;

}

/*-------------------------------------------------
    draw_char - put per-pixel VRAM data (charset)
-------------------------------------------------*/

static void draw_char(upd7220_t *upd7220,int x,int y)
{
	int xi,yi;
	int xsize,ysize;
	UINT8 tile_data;

	/* snippet for character checking */
	#if 0
	for(yi=0;yi<8;yi++)
	{
		for(xi=0;xi<8;xi++)
		{
			printf("%d",(upd7220->ra[(yi & 7) | 8] >> xi) & 1);
		}
		printf("\n");
	}
	#endif

	xsize = upd7220->figs.d & 0x3ff;
	/* Guess: D has presumably upper bits for ysize, QX-10 relies on this (TODO: check this on any real HW) */
	ysize = ((upd7220->figs.d & 0x400) + upd7220->figs.dc) + 1;

	/* TODO: internal direction, zooming, size stuff bigger than 8, rewrite using draw_pixel function */
	for(yi=0;yi<ysize;yi++)
	{
		switch(upd7220->figs.dir & 7)
		{
			case 0: tile_data = BITSWAP8(upd7220->ra[((yi) & 7) | 8],0,1,2,3,4,5,6,7); break; // TODO
			case 2:	tile_data = BITSWAP8(upd7220->ra[((yi) & 7) | 8],0,1,2,3,4,5,6,7); break;
			case 6:	tile_data = BITSWAP8(upd7220->ra[((ysize-1-yi) & 7) | 8],7,6,5,4,3,2,1,0); break;
			default: tile_data = BITSWAP8(upd7220->ra[((yi) & 7) | 8],7,6,5,4,3,2,1,0);
					 printf("%d %d %d\n",upd7220->figs.dir,xsize,ysize);
					 break;
		}

		for(xi=0;xi<xsize;xi++)
		{
			UINT32 addr = ((y+yi) * upd7220->pitch * 2) + ((x+xi) >> 3);

			upd7220->vram[(addr + upd7220->vram_bank) & 0x3ffff] &= ~(1 << (xi & 7));
			upd7220->vram[(addr + upd7220->vram_bank) & 0x3ffff] |= ((tile_data) & (1 << (xi & 7)));
		}
	}

	upd7220->ead = ((x+8*x_dir_dot[upd7220->figs.dir]) >> 4) + ((y+8*y_dir_dot[upd7220->figs.dir]) * upd7220->pitch);
	upd7220->dad = ((x+8*x_dir_dot[upd7220->figs.dir]) & 0xf);
}

/*-------------------------------------------------
    translate_command - translate command byte
-------------------------------------------------*/

static int translate_command(UINT8 data)
{
	int command = COMMAND_INVALID;

	switch (data)
	{
	case UPD7220_COMMAND_RESET:	command = COMMAND_RESET; break;
	case UPD7220_COMMAND_CCHAR:	command = COMMAND_CCHAR; break;
	case UPD7220_COMMAND_START:	command = COMMAND_START; break;
	case UPD7220_COMMAND_ZOOM:	command = COMMAND_ZOOM;  break;
	case UPD7220_COMMAND_CURS:	command = COMMAND_CURS;  break;
	case UPD7220_COMMAND_PITCH:	command = COMMAND_PITCH; break;
	case UPD7220_COMMAND_MASK:	command = COMMAND_MASK;	 break;
	case UPD7220_COMMAND_FIGS:	command = COMMAND_FIGS;	 break;
	case UPD7220_COMMAND_FIGD:	command = COMMAND_FIGD;  break;
	case UPD7220_COMMAND_GCHRD:	command = COMMAND_GCHRD; break;
	case UPD7220_COMMAND_CURD:	command = COMMAND_CURD;  break;
	case UPD7220_COMMAND_LPRD:	command = COMMAND_LPRD;	 break;
	case UPD7220_COMMAND_5A:	command = COMMAND_5A;    break;
	default:
		switch (data & 0xfe)
		{
		case UPD7220_COMMAND_SYNC:  command = COMMAND_SYNC;  break;
		case UPD7220_COMMAND_VSYNC: command = COMMAND_VSYNC; break;
		case UPD7220_COMMAND_BCTRL:	command = COMMAND_BCTRL; break;
		default:
			switch (data & 0xf0)
			{
			case UPD7220_COMMAND_PRAM: command = COMMAND_PRAM; break;
			default:
				switch (data & 0xe4)
				{
				case UPD7220_COMMAND_WDAT: command = COMMAND_WDAT; break;
				case UPD7220_COMMAND_RDAT: command = COMMAND_RDAT; break;
				case UPD7220_COMMAND_DMAR: command = COMMAND_DMAR; break;
				case UPD7220_COMMAND_DMAW: command = COMMAND_DMAW; break;
				}
			}
		}
	}

	return command;
}

/*-------------------------------------------------
    process_fifo - process a single byte in FIFO
-------------------------------------------------*/

static void process_fifo(device_t *device)
{
	upd7220_t *upd7220 = get_safe_token(device);
	UINT8 data;
	int flag;

	dequeue(upd7220, &data, &flag);

	if (flag == FIFO_COMMAND)
	{
		upd7220->cr = data;
		upd7220->param_ptr = 1;
	}
	else
	{
		upd7220->pr[upd7220->param_ptr] = data;
		upd7220->param_ptr++;
	}

	switch (translate_command(upd7220->cr))
	{
	case COMMAND_INVALID:
		printf("uPD7220 '%s' Invalid Command Byte %02x\n", device->tag(), upd7220->cr);
		break;

	case COMMAND_5A:
		if (upd7220->param_ptr == 4)
			printf("uPD7220 '%s' Undocumented Command 0x5A Executed %02x %02x %02x\n", device->tag(),upd7220->pr[1],upd7220->pr[2],upd7220->pr[3] );
		break;

	case COMMAND_RESET: /* reset */
		switch (upd7220->param_ptr)
		{
		case 0:
			if (LOG) logerror("uPD7220 '%s' RESET\n", device->tag());

			upd7220->de = 0;
			upd7220->ra[0] = upd7220->ra[1] = upd7220->ra[2] = 0;
			upd7220->ra[3] = 0x19;
			upd7220->ead = 0;
			upd7220->dad = 0;
			upd7220->mask = 0;
			break;

		case 9:
			upd7220->mode = upd7220->pr[1];
			upd7220->aw = upd7220->pr[2] + 2;
			upd7220->hs = (upd7220->pr[3] & 0x1f) + 1;
			upd7220->vs = ((upd7220->pr[4] & 0x03) << 3) | (upd7220->pr[3] >> 5);
			upd7220->hfp = (upd7220->pr[4] >> 2) + 1;
			upd7220->hbp = (upd7220->pr[5] & 0x3f) + 1;
			upd7220->vfp = upd7220->pr[6] & 0x3f;
			upd7220->al = ((upd7220->pr[8] & 0x03) << 8) | upd7220->pr[7];
			upd7220->vbp = upd7220->pr[8] >> 2;

			upd7220->pitch = upd7220->aw;

			if (LOG)
			{
				logerror("uPD7220 '%s' Mode: %02x\n", device->tag(), upd7220->mode);
				logerror("uPD7220 '%s' AW: %u\n", device->tag(), upd7220->aw);
				logerror("uPD7220 '%s' HS: %u\n", device->tag(), upd7220->hs);
				logerror("uPD7220 '%s' VS: %u\n", device->tag(), upd7220->vs);
				logerror("uPD7220 '%s' HFP: %u\n", device->tag(), upd7220->hfp);
				logerror("uPD7220 '%s' HBP: %u\n", device->tag(), upd7220->hbp);
				logerror("uPD7220 '%s' VFP: %u\n", device->tag(), upd7220->vfp);
				logerror("uPD7220 '%s' AL: %u\n", device->tag(), upd7220->al);
				logerror("uPD7220 '%s' VBP: %u\n", device->tag(), upd7220->vbp);
				logerror("uPD7220 '%s' PITCH: %u\n", device->tag(), upd7220->pitch);
			}

			recompute_parameters(device);
			break;
		}
		break;

	case COMMAND_SYNC: /* sync format specify */
		if (upd7220->param_ptr == 9)
		{
			upd7220->mode = upd7220->pr[1];
			upd7220->aw = upd7220->pr[2] + 2;
			upd7220->hs = (upd7220->pr[3] & 0x1f) + 1;
			upd7220->vs = ((upd7220->pr[4] & 0x03) << 3) | (upd7220->pr[3] >> 5);
			upd7220->hfp = (upd7220->pr[4] >> 2) + 1;
			upd7220->hbp = (upd7220->pr[5] & 0x3f) + 1;
			upd7220->vfp = upd7220->pr[6] & 0x3f;
			upd7220->al = ((upd7220->pr[8] & 0x03) << 8) | upd7220->pr[7];
			upd7220->vbp = upd7220->pr[8] >> 2;

			upd7220->pitch = upd7220->aw;

			if (LOG)
			{
				logerror("uPD7220 '%s' Mode: %02x\n", device->tag(), upd7220->mode);
				logerror("uPD7220 '%s' AW: %u\n", device->tag(), upd7220->aw);
				logerror("uPD7220 '%s' HS: %u\n", device->tag(), upd7220->hs);
				logerror("uPD7220 '%s' VS: %u\n", device->tag(), upd7220->vs);
				logerror("uPD7220 '%s' HFP: %u\n", device->tag(), upd7220->hfp);
				logerror("uPD7220 '%s' HBP: %u\n", device->tag(), upd7220->hbp);
				logerror("uPD7220 '%s' VFP: %u\n", device->tag(), upd7220->vfp);
				logerror("uPD7220 '%s' AL: %u\n", device->tag(), upd7220->al);
				logerror("uPD7220 '%s' VBP: %u\n", device->tag(), upd7220->vbp);
				logerror("uPD7220 '%s' PITCH: %u\n", device->tag(), upd7220->pitch);
			}

			recompute_parameters(device);
		}
		break;

	case COMMAND_VSYNC: /* vertical sync mode */
		upd7220->m = upd7220->cr & 0x01;

		if (LOG) logerror("uPD7220 '%s' M: %u\n", device->tag(), upd7220->m);

		recompute_parameters(device);
		break;

	case COMMAND_CCHAR: /* cursor & character characteristics */
		if(upd7220->param_ptr == 2)
		{
			upd7220->lr = (upd7220->pr[1] & 0x1f) + 1;
			upd7220->dc = BIT(upd7220->pr[1], 7);
		}

		if(upd7220->param_ptr == 3)
		{
			upd7220->ctop = upd7220->pr[2] & 0x1f;
			upd7220->sc = BIT(upd7220->pr[2], 5);
		}

		if(upd7220->param_ptr == 4)
		{
			upd7220->br = ((upd7220->pr[3] & 0x07) << 2) | (upd7220->pr[2] >> 6);
			upd7220->cbot = upd7220->pr[3] >> 3;
		}

		if (0)
		{
			logerror("uPD7220 '%s' LR: %u\n", device->tag(), upd7220->lr);
			logerror("uPD7220 '%s' DC: %u\n", device->tag(), upd7220->dc);
			logerror("uPD7220 '%s' CTOP: %u\n", device->tag(), upd7220->ctop);
			logerror("uPD7220 '%s' SC: %u\n", device->tag(), upd7220->sc);
			logerror("uPD7220 '%s' BR: %u\n", device->tag(), upd7220->br);
			logerror("uPD7220 '%s' CBOT: %u\n", device->tag(), upd7220->cbot);
		}
		break;

	case COMMAND_START: /* start display & end idle mode */
		upd7220->de = 1;

		if (LOG) logerror("uPD7220 '%s' DE: 1\n", device->tag());
		break;

	case COMMAND_BCTRL: /* display blanking control */
		upd7220->de = upd7220->cr & 0x01;

		if (LOG) logerror("uPD7220 '%s' DE: %u\n", device->tag(), upd7220->de);
		break;

	case COMMAND_ZOOM: /* zoom factors specify */
		if (flag == FIFO_PARAMETER)
		{
			upd7220->gchr = upd7220->pr[1] & 0x0f;
			upd7220->disp = upd7220->pr[1] >> 4;

			if (LOG) logerror("uPD7220 '%s' GCHR: %01x\n", device->tag(), upd7220->gchr);
			if (LOG) logerror("uPD7220 '%s' DISP: %01x\n", device->tag(), upd7220->disp);
		}
		break;

	case COMMAND_CURS: /* cursor position specify */
		if (upd7220->param_ptr >= 3)
		{
			UINT8 upper_addr;

			upper_addr = (upd7220->param_ptr == 3) ? 0 : (upd7220->pr[3] & 0x03);

			upd7220->ead = (upper_addr << 16) | (upd7220->pr[2] << 8) | upd7220->pr[1];

			if (LOG) logerror("uPD7220 '%s' EAD: %06x\n", device->tag(), upd7220->ead);

			if(upd7220->param_ptr == 4)
			{
				upd7220->dad = upd7220->pr[3] >> 4;
				if (LOG) logerror("uPD7220 '%s' DAD: %01x\n", device->tag(), upd7220->dad);
			}
		}
		break;

	case COMMAND_PRAM: /* parameter RAM load */
		if (flag == FIFO_COMMAND)
		{
			upd7220->ra_addr = upd7220->cr & 0x0f;
		}
		else
		{
			if (upd7220->ra_addr < 16)
			{
				if (LOG) logerror("uPD7220 '%s' RA%u: %02x\n", device->tag(), upd7220->ra_addr, data);

				upd7220->ra[upd7220->ra_addr] = data;
				upd7220->ra_addr++;
			}

			upd7220->param_ptr = 0;
		}
		break;

	case COMMAND_PITCH: /* pitch specification */
		if (flag == FIFO_PARAMETER)
		{
			upd7220->pitch = data;

			if (LOG) logerror("uPD7220 '%s' PITCH: %u\n", device->tag(), upd7220->pitch);
		}
		break;

	case COMMAND_WDAT: /* write data into display memory */
		upd7220->bitmap_mod = upd7220->cr & 3;

		if (upd7220->param_ptr == 3 || (upd7220->param_ptr == 2 && upd7220->cr & 0x10))
		{
			//printf("%02x = %02x %02x (%c) %04x\n",upd7220->cr,upd7220->pr[2],upd7220->pr[1],upd7220->pr[1],EAD);
			fifo_set_direction(upd7220, FIFO_WRITE);

			write_vram(upd7220,(upd7220->cr & 0x18) >> 3,upd7220->cr & 3);
			reset_figs_param(upd7220);
			upd7220->param_ptr = 1;
		}
		break;

	case COMMAND_MASK: /* mask register load */
		if (upd7220->param_ptr == 3)
		{
			upd7220->mask = (upd7220->pr[2] << 8) | upd7220->pr[1];

			if (LOG) logerror("uPD7220 '%s' MASK: %04x\n", device->tag(), upd7220->mask);
		}
		break;

	case COMMAND_FIGS: /* figure drawing parameters specify */
		if (upd7220->param_ptr == 2)
		{
			upd7220->figs.dir = upd7220->pr[1] & 0x7;
			upd7220->figs.figure_type = (upd7220->pr[1] & 0xf8) >> 3;

			//if(upd7220->figs.dir != 2)
			//	printf("DIR %02x\n",upd7220->pr[1]);
		}

		if (upd7220->param_ptr == 4)
			upd7220->figs.dc = (upd7220->pr[2]) | ((upd7220->pr[3] & 0x3f) << 8);

		if (upd7220->param_ptr == 6)
			upd7220->figs.d = (upd7220->pr[4]) | ((upd7220->pr[5] & 0x3f) << 8);

		if (upd7220->param_ptr == 8)
			upd7220->figs.d2 = (upd7220->pr[6]) | ((upd7220->pr[7] & 0x3f) << 8);

		if (upd7220->param_ptr == 10)
			upd7220->figs.d1 = (upd7220->pr[8]) | ((upd7220->pr[9] & 0x3f) << 8);

		if (upd7220->param_ptr == 12)
			upd7220->figs.dm = (upd7220->pr[10]) | ((upd7220->pr[11] & 0x3f) << 8);

		break;

	case COMMAND_FIGD: /* figure draw start */
		if(upd7220->figs.figure_type == 0)
		{
			UINT16 line_pattern = check_pattern(upd7220,(upd7220->ra[8]) | (upd7220->ra[9]<<8));
			UINT8 dot = ((line_pattern >> (0 & 0xf)) & 1) << 7;

			draw_pixel(upd7220,((upd7220->ead % upd7220->pitch) << 4) | (upd7220->dad & 0xf),(upd7220->ead / upd7220->pitch),dot);
		}
		else if(upd7220->figs.figure_type == 1)
			draw_line(upd7220,((upd7220->ead % upd7220->pitch) << 4) | (upd7220->dad & 0xf),(upd7220->ead / upd7220->pitch));
		else if(upd7220->figs.figure_type == 8)
			draw_rectangle(upd7220,((upd7220->ead % upd7220->pitch) << 4) | (upd7220->dad & 0xf),(upd7220->ead / upd7220->pitch));
		else
			printf("uPD7220 '%s' Unimplemented command FIGD %02x\n", device->tag(),upd7220->figs.figure_type);

		reset_figs_param(upd7220);
		upd7220->sr |= UPD7220_SR_DRAWING_IN_PROGRESS;
		break;

	case COMMAND_GCHRD: /* graphics character draw and area filling start */
		if(upd7220->figs.figure_type == 2)
			draw_char(upd7220,((upd7220->ead % upd7220->pitch) << 4) | (upd7220->dad & 0xf),(upd7220->ead / upd7220->pitch));
		else
			printf("uPD7220 '%s' Unimplemented command GCHRD %02x\n", device->tag(),upd7220->figs.figure_type);

		reset_figs_param(upd7220);
		upd7220->sr |= UPD7220_SR_DRAWING_IN_PROGRESS;
		break;

	case COMMAND_RDAT: /* read data from display memory */
		fifo_set_direction(upd7220, FIFO_READ);

		read_vram(upd7220,(upd7220->cr & 0x18) >> 3,upd7220->cr & 3);
		reset_figs_param(upd7220);

		upd7220->sr |= UPD7220_SR_DATA_READY;
		break;

	case COMMAND_CURD: /* cursor address read */
		fifo_set_direction(upd7220, FIFO_READ);

		queue(upd7220, upd7220->ead & 0xff, 0);
		queue(upd7220, (upd7220->ead >> 8) & 0xff, 0);
		queue(upd7220, upd7220->ead >> 16, 0);
		queue(upd7220, upd7220->dad & 0xff, 0);
		queue(upd7220, upd7220->dad >> 8, 0);

		upd7220->sr |= UPD7220_SR_DATA_READY;
		break;

	case COMMAND_LPRD: /* light pen address read */
		fifo_set_direction(upd7220, FIFO_READ);

		queue(upd7220, upd7220->lad & 0xff, 0);
		queue(upd7220, (upd7220->lad >> 8) & 0xff, 0);
		queue(upd7220, upd7220->lad >> 16, 0);

		upd7220->sr |= UPD7220_SR_DATA_READY;
		upd7220->sr &= ~UPD7220_SR_LIGHT_PEN_DETECT;
		break;

	case COMMAND_DMAR: /* DMA read request */
		printf("uPD7220 '%s' Unimplemented command DMAR\n", device->tag());
		break;

	case COMMAND_DMAW: /* DMA write request */
		printf("uPD7220 '%s' Unimplemented command DMAW\n", device->tag());
		break;
	}
}

/*-------------------------------------------------
    upd7220_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd7220_r )
{
	upd7220_t *upd7220 = get_safe_token(device);

	UINT8 data;

	if (offset & 1)
	{
		/* FIFO read */
		int flag;
		fifo_set_direction(upd7220, FIFO_READ);
		dequeue(upd7220, &data, &flag);
	}
	else
	{
		/* status register */
		data = upd7220->sr;

		/* TODO: timing of these */
		upd7220->sr &= ~UPD7220_SR_DRAWING_IN_PROGRESS;
		upd7220->sr &= ~UPD7220_SR_DMA_EXECUTE;
	}

	return data;
}

/*-------------------------------------------------
    upd7220_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd7220_w )
{
	upd7220_t *upd7220 = get_safe_token(device);

	if (offset & 1)
	{
		/* command into FIFO */
		fifo_set_direction(upd7220, FIFO_WRITE);
		queue(upd7220, data, 1);
	}
	else
	{
		/* parameter into FIFO */
//      fifo_set_direction(upd7220, FIFO_WRITE);
		queue(upd7220, data, 0);
	}

	process_fifo(device);
}

WRITE8_DEVICE_HANDLER( upd7220_bank_w )
{
	upd7220_t *upd7220 = get_safe_token(device);

	upd7220->vram_bank = 0x8000 * (data & 7);
}

/*-------------------------------------------------
    upd7220_dack_w - DMA acknowledge read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd7220_dack_r )
{
	return 0;
}

/*-------------------------------------------------
    upd7220_dack_w - DMA acknowledge write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd7220_dack_w )
{
}

/*-------------------------------------------------
    upd7220_ext_sync_w - external synchronization
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd7220_ext_sync_w )
{
	upd7220_t *upd7220 = get_safe_token(device);

	//if (LOG) logerror("uPD7220 '%s' External Synchronization: %u\n", device->tag(), state);

	if (state)
	{
		upd7220->sr |= UPD7220_SR_VSYNC_ACTIVE;
	}
	else
	{
		upd7220->sr &= ~UPD7220_SR_VSYNC_ACTIVE;
	}
}

/*-------------------------------------------------
    upd7220_lpen_w - light pen strobe
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd7220_lpen_w )
{
	/* only if 2 rising edges on the lpen input occur at the same
       point during successive video fields are the pulses accepted */

	/*

        1. compute the address of the location on the CRT
        2. compare with LAD
        3. if not equal move address to LAD
        4. if equal set LPEN DETECT flag to 1

    */
}

INLINE void get_text_partition(upd7220_t *upd7220, int index, UINT32 *sad, UINT16 *len, int *im, int *wd)
{
	*sad = ((upd7220->ra[(index * 4) + 1] & 0x1f) << 8) | upd7220->ra[(index * 4) + 0];
	*len = ((upd7220->ra[(index * 4) + 3] & 0x3f) << 4) | (upd7220->ra[(index * 4) + 2] >> 4);
	*im = BIT(upd7220->ra[(index * 4) + 3], 6);
	*wd = BIT(upd7220->ra[(index * 4) + 3], 7);
}

/*-------------------------------------------------
    update_text - update text mode screen
-------------------------------------------------*/

static void update_text(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	upd7220_t *upd7220 = get_safe_token(device);

	UINT32 addr, sad;
	UINT16 len;
	int im, wd, area;
	int y, sy = 0;

	for (area = 0; area < 4; area++)
	{
		get_text_partition(upd7220, area, &sad, &len, &im, &wd);

		for (y = sy; y < sy + len; y++)
		{
			addr = sad + (y * upd7220->pitch);
			if (upd7220->draw_text_func) upd7220->draw_text_func(device, bitmap, upd7220->vram, addr, y, wd, upd7220->pitch,0,0,upd7220->aw * 8 - 1,upd7220->al - 1, upd7220->lr, upd7220->dc, upd7220->ead);
		}

		sy = y + 1;
	}
}

/*-------------------------------------------------
    draw_graphics_line - draw graphics scanline
-------------------------------------------------*/

static void draw_graphics_line(device_t *device, bitmap_t *bitmap, UINT32 addr, int y, int wd)
{
	upd7220_t *upd7220 = get_safe_token(device);
	address_space *space = device->machine().device("maincpu")->memory().space(AS_PROGRAM);
	int sx;

	for (sx = 0; sx < upd7220->pitch * 2; sx++)
	{
		UINT16 data = space->direct().read_raw_word(addr & 0x3ffff); //TODO: remove me

		if((sx << 3) < upd7220->aw * 16 && y < upd7220->al)
			upd7220->display_func(device, bitmap, y, sx << 3, addr, data, upd7220->vram);

		if (wd) addr += 2; else addr++;
	}
}

INLINE void get_graphics_partition(upd7220_t *upd7220, int index, UINT32 *sad, UINT16 *len, int *im, int *wd)
{
	*sad = ((upd7220->ra[(index * 4) + 2] & 0x03) << 16) | (upd7220->ra[(index * 4) + 1] << 8) | upd7220->ra[(index * 4) + 0];
	*len = ((upd7220->ra[(index * 4) + 3] & 0x3f) << 4) | (upd7220->ra[(index * 4) + 2] >> 4);
	*im = BIT(upd7220->ra[(index * 4) + 3], 6);
	*wd = BIT(upd7220->ra[(index * 4) + 3], 7);
}

/*-------------------------------------------------
    update_graphics - update graphics mode screen
-------------------------------------------------*/

static void update_graphics(device_t *device, bitmap_t *bitmap, const rectangle *cliprect, int force_bitmap)
{
	upd7220_t *upd7220 = get_safe_token(device);

	UINT32 addr, sad;
	UINT16 len;
	int im, wd, area;
	int y, tsy = 0, bsy = 0;

	for (area = 0; area < 2; area++)
	{
		get_graphics_partition(upd7220, area, &sad, &len, &im, &wd);

		for (y = 0; y < len; y++)
		{
			if (im || force_bitmap)
			{
				addr = (sad & 0x3ffff) + (y * upd7220->pitch * 2);

				if(upd7220->display_func)
					draw_graphics_line(device, bitmap, addr, y + bsy, wd);
			}
			else
			{
				/* TODO: text params are more limited compared to graphics */
				addr = (sad & 0x3ffff) + (y * upd7220->pitch);

				if (upd7220->draw_text_func)
					upd7220->draw_text_func(device, bitmap, upd7220->vram, addr, y + tsy, wd, upd7220->pitch,0,0,upd7220->aw * 8 - 1,len + bsy - 1,upd7220->lr, upd7220->dc, upd7220->ead);
			}
		}

		if(upd7220->lr)
			tsy += (y / upd7220->lr);
		bsy += y;
	}
}

/*-------------------------------------------------
    upd7220_update - update screen
-------------------------------------------------*/

void upd7220_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	upd7220_t *upd7220 = get_safe_token(device);

	if (upd7220->de)
	{
		switch (upd7220->mode & UPD7220_MODE_DISPLAY_MASK)
		{
		case UPD7220_MODE_DISPLAY_MIXED:
			update_graphics(device, bitmap, cliprect,0);
			break;

		case UPD7220_MODE_DISPLAY_GRAPHICS:
			update_graphics(device, bitmap, cliprect,1);
			break;

		case UPD7220_MODE_DISPLAY_CHARACTER:
			update_text(device, bitmap, cliprect);
			break;

		case UPD7220_MODE_DISPLAY_INVALID:
			logerror("uPD7220 '%s' Invalid Display Mode!\n", device->tag());
		}
	}
}

/*-------------------------------------------------
    ROM( upd7220 )
-------------------------------------------------*/

ROM_START( upd7220 )
	ROM_REGION( 0x100, "upd7220", ROMREGION_LOADBYNAME )
	ROM_LOAD( "upd7220.bin", 0x000, 0x100, NO_DUMP ) /* internal 128x14 control ROM */
ROM_END

/*-------------------------------------------------
    ADDRESS_MAP( upd7220 )
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd7220_vram_r )
{
	upd7220_t *upd7220 = get_safe_token(device);

	return upd7220->vram[offset];
}

WRITE8_DEVICE_HANDLER( upd7220_vram_w )
{
	upd7220_t *upd7220 = get_safe_token(device);

	upd7220->vram[offset] = data;
}

/*-------------------------------------------------
    DEVICE_START( upd7220 )
-------------------------------------------------*/

static DEVICE_START( upd7220 )
{
	upd7220_t *upd7220 = get_safe_token(device);
	const upd7220_interface *intf = get_interface(device);
	int i;

	/* resolve callbacks */
	devcb_resolve_write_line(&upd7220->out_drq_func, &intf->out_drq_func, device);
	devcb_resolve_write_line(&upd7220->out_hsync_func, &intf->out_hsync_func, device);
	devcb_resolve_write_line(&upd7220->out_vsync_func, &intf->out_vsync_func, device);
	upd7220->display_func = intf->display_func;
	upd7220->draw_text_func = intf->draw_text_func;

	/* get the screen device */
	upd7220->screen = device->machine().device<screen_device>(intf->screen_tag);
	assert(upd7220->screen != NULL);

	/* create the timers */
	upd7220->vsync_timer = device->machine().scheduler().timer_alloc(FUNC(vsync_tick), (void *)device);
	upd7220->hsync_timer = device->machine().scheduler().timer_alloc(FUNC(hsync_tick), (void *)device);
	upd7220->blank_timer = device->machine().scheduler().timer_alloc(FUNC(blank_tick), (void *)device);

	/* set initial values */
	upd7220->fifo_ptr = -1;
	upd7220->sr = UPD7220_SR_FIFO_EMPTY;

	for (i = 0; i < 16; i++)
	{
		upd7220->fifo[i] = 0;
		upd7220->fifo_flag[i] = FIFO_EMPTY;
	}

	/* register for state saving */
	device->save_item(NAME(upd7220->ra));
	device->save_item(NAME(upd7220->sr));
	device->save_item(NAME(upd7220->mode));
	device->save_item(NAME(upd7220->de));
	device->save_item(NAME(upd7220->aw));
	device->save_item(NAME(upd7220->al));
	device->save_item(NAME(upd7220->vs));
	device->save_item(NAME(upd7220->vfp));
	device->save_item(NAME(upd7220->vbp));
	device->save_item(NAME(upd7220->hs));
	device->save_item(NAME(upd7220->hfp));
	device->save_item(NAME(upd7220->hbp));
	device->save_item(NAME(upd7220->m));
	device->save_item(NAME(upd7220->dc));
	device->save_item(NAME(upd7220->sc));
	device->save_item(NAME(upd7220->br));
	device->save_item(NAME(upd7220->lr));
	device->save_item(NAME(upd7220->ctop));
	device->save_item(NAME(upd7220->cbot));
	device->save_item(NAME(upd7220->ead));
	device->save_item(NAME(upd7220->dad));
	device->save_item(NAME(upd7220->lad));
	device->save_item(NAME(upd7220->disp));
	device->save_item(NAME(upd7220->gchr));
	device->save_item(NAME(upd7220->mask));
	device->save_item(NAME(upd7220->pitch));
}

/*-------------------------------------------------
    DEVICE_RESET( upd7220 )
-------------------------------------------------*/

static DEVICE_RESET( upd7220 )
{
	upd7220_t *upd7220 = get_safe_token(device);

	devcb_call_write_line(&upd7220->out_drq_func, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd7220 )
-------------------------------------------------*/

DEVICE_GET_INFO( upd7220 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd7220_t);						break;
		case DEVINFO_INT_DATABUS_WIDTH_0:				info->i = 8;										break;
		case DEVINFO_INT_ADDRBUS_WIDTH_0:				info->i = 18;										break;
		case DEVINFO_INT_ADDRBUS_SHIFT_0:				info->i = -1;										break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(upd7220);				break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_DEFAULT_MEMORY_MAP_0:			info->default_map8 = NULL; 							break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd7220);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd7220);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "upd7220");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}

DEFINE_LEGACY_MEMORY_DEVICE(UPD7220, upd7220);
