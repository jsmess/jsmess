#include "driver.h"
#include "upd7220.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define UPD7220_COMMAND_RESET	0x00
#define UPD7220_COMMAND_SYNC	0x0e // & 0xfe
#define UPD7220_COMMAND_VSYNC	0x6e // & 0xfe
#define UPD7220_COMMAND_CCHAR	0x4b
#define UPD7220_COMMAND_START	0x6b
#define UPD7220_COMMAND_BCTRL	0x0c // & 0xfe
#define UPD7220_COMMAND_ZOOM	0x46
#define UPD7220_COMMAND_CURS	0x49
#define UPD7220_COMMAND_PRAM	0x70 // & 0xf0
#define UPD7220_COMMAND_PITCH	0x47
#define UPD7220_COMMAND_WDAT	0x20 // & 0xe4
#define UPD7220_COMMAND_MASK	0x4a
#define UPD7220_COMMAND_FIGS	0x4c
#define UPD7220_COMMAND_FIGD	0x6c
#define UPD7220_COMMAND_GCHRD	0x68
#define UPD7220_COMMAND_RDAT	0xa0 // & 0xe4
#define UPD7220_COMMAND_CURD	0xe0
#define UPD7220_COMMAND_LPRD	0xc0
#define UPD7220_COMMAND_DMAR	0xa4 // & 0xe4
#define UPD7220_COMMAND_DMAW	0x24 // & 0xe4

#define UPD7220_SR_DATA_READY			0x01
#define UPD7220_SR_FIFO_FULL			0x02
#define UPD7220_SR_FIFO_EMPTY			0x04
#define UPD7220_SR_DRAWING_IN_PROGRESS	0x08
#define UPD7220_SR_DMA_EXECUTE			0x10
#define UPD7220_SR_VSYNC_ACTIVE			0x20
#define UPD7220_SR_HBLANK_ACTIVE		0x40
#define UPD7220_SR_LIGHT_PEN_DETECT		0x80

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7220_t upd7220_t;
struct _upd7220_t
{
	devcb_resolved_read8		in_data_func;
	devcb_resolved_write8		out_data_func;
	devcb_resolved_write_line	out_drq_func;
	devcb_resolved_write_line	out_hsync_func;
	devcb_resolved_write_line	out_vsync_func;
	devcb_resolved_write_line	out_blank_func;

	int clock;

	UINT8 ra[16];					/* parameter RAM */

	UINT8 sr;						/* status register */
	UINT8 mode;						/* mode of operation */

	int de;							/* display enabled */
	int aw;							/* active display words per line - 2 (must be even number with bit 0 = 0) */
	int al;							/* active display lines per video field */
	int vs;							/* vertical sync width - 1 */
	int vfp;						/* vertical front porch width - 1 */
	int vbp;						/* vertical back porch width - 1 */
	int hs;							/* horizontal sync width - 1 */
	int hfp;						/* horizontal front porch width - 1 */
	int hbp;						/* horizontal back porch width - 1 */
	int m;							/* 0 = accept external vertical sync (slave mode) / 1 = generate & output vertical sync (master mode) */

	int dc;							/* display cursor */
	int sc;							/* 0 = blinking cursor / 1 = steady cursor */
	int br;							/* blink rate */
	int lr;							/* lines per character row - 1 */
	int ctop;						/* cursor top line number in the row */
	int cbot;						/* cursor bottom line number in the row (CBOT < LR) */
	UINT32 ead;						/* execute word address */
	UINT16 dad;						/* dot address within the word */
	UINT32 lad;						/* light pen address */

	int disp;						/* display zoom factor */
	int gchr;						/* zoom factor for graphics character writing and area filling */
	UINT16 mask;					/* mask register */
	UINT8 pitch;					/* number of word addresses in display memory in the horizontal direction */

	/* timers */
	emu_timer *vsync_timer;			/* vertical sync timer */
	emu_timer *hsync_timer;			/* horizontal sync timer */
	emu_timer *blank_timer;			/* CRT blanking timer */

	const device_config *screen;	/* screen */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7220_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (upd7220_t *)device->token;
}

INLINE const upd7220_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == UPD7220));
	return (const upd7220_interface *) device->static_config;
}

INLINE void queue(upd7220_t *upd7220, UINT8 data, int command)
{
}

INLINE UINT16 dequeue(upd7220_t *upd7220)
{
	return 0;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    recompute_parameters -
-------------------------------------------------*/
#ifdef UNUSED_FUNCTION
static void recompute_parameters(upd7220_t *upd7220)
{
	rectangle visarea;

	int horiz_pix_total;
	int vert_pix_total;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(upd7220->clock) * (upd7220->aw + 2) * upd7220->al;

	visarea.min_x = upd7220->hs + upd7220->hbp + 1;
	visarea.min_y = upd7220->vs + upd7220->vbp + 1;
	visarea.max_x = 0;
	visarea.max_y = upd7220->al + 2 - upd7220->vfp;

	video_screen_configure(upd7220->screen, horiz_pix_total, vert_pix_total, &visarea, refresh);
}
#endif
/*-------------------------------------------------
    update_vsync_timer -
-------------------------------------------------*/

static void update_vsync_timer(upd7220_t *upd7220, int state)
{
	int next_y = state ? (upd7220->vs + 1) : 0;

	attotime duration = video_screen_get_time_until_pos(upd7220->screen, next_y, 0);

	timer_adjust_oneshot(upd7220->vsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( vsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vsync_tick )
{
	const device_config *device = ptr;
	upd7220_t *upd7220 = get_safe_token(device);

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
	const device_config *device = ptr;
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

	int next_x = state ? (upd7220->hs + upd7220->hbp + 1) : 0;
	int next_y = state ? y : ((y + 1) % upd7220->al);

	attotime duration = video_screen_get_time_until_pos(upd7220->screen, next_y, next_x);

	timer_adjust_oneshot(upd7220->hsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( blank_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( blank_tick )
{
	const device_config *device = ptr;
	upd7220_t *upd7220 = get_safe_token(device);

	devcb_call_write_line(&upd7220->out_blank_func, param);

	update_blank_timer(upd7220, param);
}

/*-------------------------------------------------
    process_command - process command
-------------------------------------------------*/

#ifdef UNUSED_CODE
static void process_command(upd7220_t *upd7220, UINT8 data)
{
	switch (data)
	{
	case UPD7220_COMMAND_RESET:
		break;
	case UPD7220_COMMAND_CCHAR:
		break;
	case UPD7220_COMMAND_START:
		break;
	case UPD7220_COMMAND_ZOOM:
		break;
	case UPD7220_COMMAND_CURS:
		break;
	case UPD7220_COMMAND_PITCH:
		break;
	case UPD7220_COMMAND_MASK:
		break;
	case UPD7220_COMMAND_FIGS:
		break;
	case UPD7220_COMMAND_FIGD:
		break;
	case UPD7220_COMMAND_CURD:
		break;
	case UPD7220_COMMAND_LPRD:
		break;
	default:
		switch (data & 0xfe)
		{
		case UPD7220_COMMAND_SYNC:
			break;
		case UPD7220_COMMAND_VSYNC:
			break;
		case UPD7220_COMMAND_BCTRL:
			break;
		default:
			switch (data & 0xf0)
			{
			case UPD7220_COMMAND_PRAM:
			default:
				switch (data & 0xe4)
				{
				case UPD7220_COMMAND_WDAT:
					break;
				case UPD7220_COMMAND_RDAT:
					break;
				case UPD7220_COMMAND_DMAR:
					break;
				case UPD7220_COMMAND_DMAW:
					break;
				default:
					logerror("Invalid command %02x\n", data);
				}
			}
		}
	}
}
#endif

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
		data = dequeue(upd7220);
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
		queue(upd7220, data, 1);
	}
	else
	{
		/* parameter into FIFO */
		queue(upd7220, data, 0);
	}
}

/*-------------------------------------------------
    upd7220_dack_w - DMA acknowledge w/data
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd7220_dack_w )
{
}

/*-------------------------------------------------
    upd7220_lpen_w - light pen strobe
-------------------------------------------------*/

void upd7220_lpen_w(const device_config *device, int state)
{
}

/*-------------------------------------------------
    ROM( upd7220 )
-------------------------------------------------*/

ROM_START( upd7220 )
	ROM_REGION( 0x100, "upd7220", ROMREGION_LOADBYNAME )
	ROM_LOAD( "upd7220.bin", 0x000, 0x100, NO_DUMP ) /* internal 128x14 control ROM */
ROM_END

/*-------------------------------------------------
    DEVICE_START( upd7220 )
-------------------------------------------------*/

static DEVICE_START( upd7220 )
{
	upd7220_t *upd7220 = get_safe_token(device);
	const upd7220_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&upd7220->in_data_func, &intf->in_data_func, device);
	devcb_resolve_write8(&upd7220->out_data_func, &intf->out_data_func, device);
	devcb_resolve_write_line(&upd7220->out_drq_func, &intf->out_drq_func, device);
	devcb_resolve_write_line(&upd7220->out_hsync_func, &intf->out_hsync_func, device);
	devcb_resolve_write_line(&upd7220->out_vsync_func, &intf->out_vsync_func, device);

	/* get the screen device */
	upd7220->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(upd7220->screen != NULL);

	/* create the timers */
	upd7220->vsync_timer = timer_alloc(device->machine, vsync_tick, (void *)device);
	upd7220->hsync_timer = timer_alloc(device->machine, hsync_tick, (void *)device);
	upd7220->blank_timer = timer_alloc(device->machine, blank_tick, (void *)device);

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
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(upd7220);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
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
