/**********************************************************************

    Intel 82720 Graphics Display Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - implement FIFO as ring buffer
    - drawing modes
        - character
        - mixed
    - commands
        - WDAT
        - FIGS
        - FIGD
        - GCHRD
        - RDAT
        - DMAR
        - DMAW
    - read-modify-write cycle
        - read data
        - modify data
        - write data

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
	COMMAND_DMAW
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

typedef void (*upd7220_command) (running_device *device);

typedef struct _upd7220_t upd7220_t;
struct _upd7220_t
{
	devcb_resolved_write_line	out_drq_func;
	devcb_resolved_write_line	out_hsync_func;
	devcb_resolved_write_line	out_vsync_func;
	devcb_resolved_write_line	out_blank_func;
	upd7220_display_pixels_func display_func;

	running_device *screen;	/* screen */

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

	/* timers */
	emu_timer *vsync_timer;			/* vertical sync timer */
	emu_timer *hsync_timer;			/* horizontal sync timer */
	emu_timer *blank_timer;			/* CRT blanking timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7220_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == UPD7220));
	return (upd7220_t *)device->token;
}

INLINE const upd7220_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type == UPD7220));
	return (const upd7220_interface *) device->baseconfig().static_config;
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

	attotime duration = video_screen_get_time_until_pos(upd7220->screen, next_y, 0);

	timer_adjust_oneshot(upd7220->vsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( vsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vsync_tick )
{
	running_device *device = (running_device *)ptr;
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
	int y = video_screen_get_vpos(upd7220->screen);

	int next_x = state ? upd7220->hs : 0;
	int next_y = state ? y : ((y + 1) % upd7220->al);

	attotime duration = video_screen_get_time_until_pos(upd7220->screen, next_y, next_x);

	timer_adjust_oneshot(upd7220->hsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( hsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( hsync_tick )
{
	running_device *device = (running_device *)ptr;
	upd7220_t *upd7220 = get_safe_token(device);

	devcb_call_write_line(&upd7220->out_hsync_func, param);

	update_hsync_timer(upd7220, param);
}

/*-------------------------------------------------
    update_blank_timer -
-------------------------------------------------*/

static void update_blank_timer(upd7220_t *upd7220, int state)
{
	int y = video_screen_get_vpos(upd7220->screen);

	int next_x = state ? (upd7220->hs + upd7220->hbp) : (upd7220->hs + upd7220->hbp + (upd7220->aw << 3));
	int next_y = state ? ((y + 1) % (upd7220->vs + upd7220->vbp + upd7220->al + upd7220->vfp - 1)) : y;

	attotime duration = video_screen_get_time_until_pos(upd7220->screen, next_y, next_x);

	timer_adjust_oneshot(upd7220->hsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( blank_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( blank_tick )
{
	running_device *device = (running_device *)ptr;
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

static void recompute_parameters(running_device *device)
{
	upd7220_t *upd7220 = get_safe_token(device);

	int horiz_pix_total = (upd7220->hs + upd7220->hbp + upd7220->aw + upd7220->hfp) * 16;
	int vert_pix_total = upd7220->vs + upd7220->vbp + upd7220->al + upd7220->vfp;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(device->clock * 8) * horiz_pix_total * vert_pix_total;

	rectangle visarea;

	visarea.min_x = (upd7220->hs + upd7220->hbp) * 16;
	visarea.min_y = upd7220->vs + upd7220->vbp;
	visarea.max_x = horiz_pix_total - (upd7220->hfp * 16) - 1;
	visarea.max_y = vert_pix_total - upd7220->vfp - 1;

	if (LOG)
	{
		logerror("uPD7220 '%s' Screen: %u x %u @ %f Hz\n", device->tag.cstr(), horiz_pix_total, vert_pix_total, 1 / ATTOSECONDS_TO_DOUBLE(refresh));
		logerror("uPD7220 '%s' Visible Area: (%u, %u) - (%u, %u)\n", device->tag.cstr(), visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	}

	if (upd7220->m)
	{
		video_screen_configure(upd7220->screen, horiz_pix_total, vert_pix_total, &visarea, refresh);

		update_hsync_timer(upd7220, 0);
		update_vsync_timer(upd7220, 0);
	}
	else
	{
		timer_enable(upd7220->hsync_timer, 0);
		timer_enable(upd7220->vsync_timer, 0);
	}

	update_blank_timer(upd7220, 0);
}

/*-------------------------------------------------
    advance_ead - advance EAD pointer
-------------------------------------------------*/
#ifdef UNUSED_FUNCTION
#define EAD			upd7220->ead
#define DAD			upd7220->dad
#define P			upd7220->pitch
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
#endif
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
	case UPD7220_COMMAND_CURD:	command = COMMAND_CURD;  break;
	case UPD7220_COMMAND_LPRD:	command = COMMAND_LPRD;	 break;
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

static void process_fifo(running_device *device)
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
		logerror("uPD7220 '%s' Invalid Command Byte %02x\n", device->tag.cstr(), upd7220->cr);
		break;

	case COMMAND_RESET: /* reset */
		switch (upd7220->param_ptr)
		{
		case 0:
			if (LOG) logerror("uPD7220 '%s' RESET\n", device->tag.cstr());

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
				logerror("uPD7220 '%s' Mode: %02x\n", device->tag.cstr(), upd7220->mode);
				logerror("uPD7220 '%s' AW: %u\n", device->tag.cstr(), upd7220->aw);
				logerror("uPD7220 '%s' HS: %u\n", device->tag.cstr(), upd7220->hs);
				logerror("uPD7220 '%s' VS: %u\n", device->tag.cstr(), upd7220->vs);
				logerror("uPD7220 '%s' HFP: %u\n", device->tag.cstr(), upd7220->hfp);
				logerror("uPD7220 '%s' HBP: %u\n", device->tag.cstr(), upd7220->hbp);
				logerror("uPD7220 '%s' VFP: %u\n", device->tag.cstr(), upd7220->vfp);
				logerror("uPD7220 '%s' AL: %u\n", device->tag.cstr(), upd7220->al);
				logerror("uPD7220 '%s' VBP: %u\n", device->tag.cstr(), upd7220->vbp);
				logerror("uPD7220 '%s' PITCH: %u\n", device->tag.cstr(), upd7220->pitch);
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
				logerror("uPD7220 '%s' Mode: %02x\n", device->tag.cstr(), upd7220->mode);
				logerror("uPD7220 '%s' AW: %u\n", device->tag.cstr(), upd7220->aw);
				logerror("uPD7220 '%s' HS: %u\n", device->tag.cstr(), upd7220->hs);
				logerror("uPD7220 '%s' VS: %u\n", device->tag.cstr(), upd7220->vs);
				logerror("uPD7220 '%s' HFP: %u\n", device->tag.cstr(), upd7220->hfp);
				logerror("uPD7220 '%s' HBP: %u\n", device->tag.cstr(), upd7220->hbp);
				logerror("uPD7220 '%s' VFP: %u\n", device->tag.cstr(), upd7220->vfp);
				logerror("uPD7220 '%s' AL: %u\n", device->tag.cstr(), upd7220->al);
				logerror("uPD7220 '%s' VBP: %u\n", device->tag.cstr(), upd7220->vbp);
				logerror("uPD7220 '%s' PITCH: %u\n", device->tag.cstr(), upd7220->pitch);
			}

			recompute_parameters(device);
		}
		break;

	case COMMAND_VSYNC: /* vertical sync mode */
		upd7220->m = upd7220->cr & 0x01;

		if (LOG) logerror("uPD7220 '%s' M: %u\n", device->tag.cstr(), upd7220->m);

		recompute_parameters(device);
		break;

	case COMMAND_CCHAR: /* cursor & character characteristics */
		if (upd7220->param_ptr == 4)
		{
			upd7220->lr = (upd7220->pr[1] & 0x1f) + 1;
			upd7220->dc = BIT(upd7220->pr[1], 7);
			upd7220->ctop = upd7220->pr[2] & 0x1f;
			upd7220->sc = BIT(upd7220->pr[2], 5);
			upd7220->br = ((upd7220->pr[3] & 0x07) << 2) | (upd7220->pr[2] >> 6);
			upd7220->cbot = upd7220->pr[3] >> 3;

			if (LOG)
			{
				logerror("uPD7220 '%s' LR: %u\n", device->tag.cstr(), upd7220->lr);
				logerror("uPD7220 '%s' DC: %u\n", device->tag.cstr(), upd7220->dc);
				logerror("uPD7220 '%s' CTOP: %u\n", device->tag.cstr(), upd7220->ctop);
				logerror("uPD7220 '%s' SC: %u\n", device->tag.cstr(), upd7220->sc);
				logerror("uPD7220 '%s' BR: %u\n", device->tag.cstr(), upd7220->br);
				logerror("uPD7220 '%s' CBOT: %u\n", device->tag.cstr(), upd7220->cbot);
			}
		}
		break;

	case COMMAND_START: /* start display & end idle mode */
		upd7220->de = 1;

		if (LOG) logerror("uPD7220 '%s' DE: 1\n", device->tag.cstr());
		break;

	case COMMAND_BCTRL: /* display blanking control */
		upd7220->de = upd7220->cr & 0x01;

		if (LOG) logerror("uPD7220 '%s' DE: %u\n", device->tag.cstr(), upd7220->de);
		break;

	case COMMAND_ZOOM: /* zoom factors specify */
		if (flag == FIFO_PARAMETER)
		{
			upd7220->gchr = upd7220->pr[1] & 0x0f;
			upd7220->disp = upd7220->pr[1] >> 4;

			if (LOG) logerror("uPD7220 '%s' GCHR: %01x\n", device->tag.cstr(), upd7220->gchr);
			if (LOG) logerror("uPD7220 '%s' DISP: %01x\n", device->tag.cstr(), upd7220->disp);
		}
		break;

	case COMMAND_CURS: /* cursor position specify */
		if (upd7220->param_ptr == 4)
		{
			upd7220->ead = ((upd7220->pr[3] & 0x03) << 16) | (upd7220->pr[2] << 8) | upd7220->pr[1];
			upd7220->dad = upd7220->pr[3] >> 4;

			if (LOG) logerror("uPD7220 '%s' EAD: %06x\n", device->tag.cstr(), upd7220->ead);
			if (LOG) logerror("uPD7220 '%s' DAD: %01x\n", device->tag.cstr(), upd7220->ead);
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
				if (LOG) logerror("uPD7220 '%s' RA%u: %02x\n", device->tag.cstr(), upd7220->ra_addr, data);

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

			if (LOG) logerror("uPD7220 '%s' PITCH: %u\n", device->tag.cstr(), upd7220->pitch);
		}
		break;

	case COMMAND_WDAT: /* write data into display memory */
		logerror("uPD7220 '%s' Unimplemented command WDAT\n", device->tag.cstr());

		if (flag == FIFO_PARAMETER)
		{
			upd7220->param_ptr = 0;
		}
		break;

	case COMMAND_MASK: /* mask register load */
		if (upd7220->param_ptr == 3)
		{
			upd7220->mask = (upd7220->pr[2] << 8) | upd7220->pr[1];

			if (LOG) logerror("uPD7220 '%s' MASK: %04x\n", device->tag.cstr(), upd7220->mask);
		}
		break;

	case COMMAND_FIGS: /* figure drawing parameters specify */
		logerror("uPD7220 '%s' Unimplemented command FIGS\n", device->tag.cstr());
		break;

	case COMMAND_FIGD: /* figure draw start */
		logerror("uPD7220 '%s' Unimplemented command FIGD\n", device->tag.cstr());
		break;

	case COMMAND_GCHRD: /* graphics character draw and area filling start */
		logerror("uPD7220 '%s' Unimplemented command GCHRD\n", device->tag.cstr());
		break;

	case COMMAND_RDAT: /* read data from display memory */
		logerror("uPD7220 '%s' Unimplemented command RDAT\n", device->tag.cstr());
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
		logerror("uPD7220 '%s' Unimplemented command DMAR\n", device->tag.cstr());
		break;

	case COMMAND_DMAW: /* DMA write request */
		logerror("uPD7220 '%s' Unimplemented command DMAW\n", device->tag.cstr());
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

	if (LOG) logerror("uPD7220 '%s' External Synchronization: %u\n", device->tag.cstr(), state);

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

/*-------------------------------------------------
    draw_text_line - draw text scanline
-------------------------------------------------*/

static void draw_text_line(running_device *device, bitmap_t *bitmap, UINT32 addr, int y, int wd)
{
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

static void update_text(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
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
			draw_text_line(device, bitmap, addr, y, wd);
		}

		sy = y + 1;
	}
}

/*-------------------------------------------------
    draw_graphics_line - draw graphics scanline
-------------------------------------------------*/

static void draw_graphics_line(running_device *device, bitmap_t *bitmap, UINT32 addr, int y, int wd)
{
	upd7220_t *upd7220 = get_safe_token(device);
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int sx;

	for (sx = 0; sx < upd7220->aw; sx++)
	{
		UINT16 data = memory_raw_read_word(space, addr & 0x3ffff);
		upd7220->display_func(device, bitmap, y, sx << 4, addr, data);
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

static void update_graphics(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	upd7220_t *upd7220 = get_safe_token(device);

	UINT32 addr, sad;
	UINT16 len;
	int im, wd, area;
	int y, sy = 0;

	for (area = 0; area < 2; area++)
	{
		get_graphics_partition(upd7220, area, &sad, &len, &im, &wd);

		for (y = sy; y < sy + len; y++)
		{
			addr = sad + (y * upd7220->pitch);

			if (im)
				draw_graphics_line(device, bitmap, addr, y, wd);
			else
				draw_text_line(device, bitmap, addr, y, wd);
		}

		sy = y + 1;
	}
}

/*-------------------------------------------------
    upd7220_update - update screen
-------------------------------------------------*/

void upd7220_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	upd7220_t *upd7220 = get_safe_token(device);

	if (upd7220->de)
	{
		switch (upd7220->mode & UPD7220_MODE_DISPLAY_MASK)
		{
		case UPD7220_MODE_DISPLAY_MIXED:
		case UPD7220_MODE_DISPLAY_GRAPHICS:
			update_graphics(device, bitmap, cliprect);
			break;

		case UPD7220_MODE_DISPLAY_CHARACTER:
			update_text(device, bitmap, cliprect);
			break;

		case UPD7220_MODE_DISPLAY_INVALID:
			logerror("uPD7220 '%s' Invalid Display Mode!\n", device->tag.cstr());
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

static ADDRESS_MAP_START( upd7220, 0, 16 )
	AM_RANGE(0x00000, 0x3ffff) AM_RAM
ADDRESS_MAP_END

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

	/* get the screen device */
	upd7220->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(upd7220->screen != NULL);

	/* create the timers */
	upd7220->vsync_timer = timer_alloc(device->machine, vsync_tick, (void *)device);
	upd7220->hsync_timer = timer_alloc(device->machine, hsync_tick, (void *)device);
	upd7220->blank_timer = timer_alloc(device->machine, blank_tick, (void *)device);

	/* set initial values */
	upd7220->fifo_ptr = -1;
	upd7220->sr = UPD7220_SR_FIFO_EMPTY;

	for (i = 0; i < 16; i++)
	{
		upd7220->fifo[i] = 0;
		upd7220->fifo_flag[i] = FIFO_EMPTY;
	}

	/* register for state saving */
	state_save_register_device_item_array(device, 0, upd7220->ra);
	state_save_register_device_item(device, 0, upd7220->sr);
	state_save_register_device_item(device, 0, upd7220->mode);
	state_save_register_device_item(device, 0, upd7220->de);
	state_save_register_device_item(device, 0, upd7220->aw);
	state_save_register_device_item(device, 0, upd7220->al);
	state_save_register_device_item(device, 0, upd7220->vs);
	state_save_register_device_item(device, 0, upd7220->vfp);
	state_save_register_device_item(device, 0, upd7220->vbp);
	state_save_register_device_item(device, 0, upd7220->hs);
	state_save_register_device_item(device, 0, upd7220->hfp);
	state_save_register_device_item(device, 0, upd7220->hbp);
	state_save_register_device_item(device, 0, upd7220->m);
	state_save_register_device_item(device, 0, upd7220->dc);
	state_save_register_device_item(device, 0, upd7220->sc);
	state_save_register_device_item(device, 0, upd7220->br);
	state_save_register_device_item(device, 0, upd7220->lr);
	state_save_register_device_item(device, 0, upd7220->ctop);
	state_save_register_device_item(device, 0, upd7220->cbot);
	state_save_register_device_item(device, 0, upd7220->ead);
	state_save_register_device_item(device, 0, upd7220->dad);
	state_save_register_device_item(device, 0, upd7220->lad);
	state_save_register_device_item(device, 0, upd7220->disp);
	state_save_register_device_item(device, 0, upd7220->gchr);
	state_save_register_device_item(device, 0, upd7220->mask);
	state_save_register_device_item(device, 0, upd7220->pitch);
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
		case DEVINFO_INT_DATABUS_WIDTH_0:				info->i = 16;										break;
		case DEVINFO_INT_ADDRBUS_WIDTH_0:				info->i = 18;										break;
		case DEVINFO_INT_ADDRBUS_SHIFT_0:				info->i = -1;										break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(upd7220);				break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_DEFAULT_MEMORY_MAP_0:			info->default_map16 = ADDRESS_MAP_NAME(upd7220);	break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd7220);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd7220);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}
