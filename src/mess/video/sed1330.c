/**********************************************************************

    SED1330 LCD Timing Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- everything

*/

#include "driver.h"
#include "sed1330.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

#define SED1330_INSTRUCTION_SYSTEM_SET		0x40
#define SED1330_INSTRUCTION_SLEEP_IN		0x53
#define SED1330_INSTRUCTION_DISP_ON			0x58
#define SED1330_INSTRUCTION_DISP_OFF		0x59
#define SED1330_INSTRUCTION_SCROLL			0x44
#define SED1330_INSTRUCTION_CSRFORM			0x5d
#define SED1330_INSTRUCTION_CGRAM_ADR		0x5c
#define SED1330_INSTRUCTION_CSRDIR_RIGHT	0x4c
#define SED1330_INSTRUCTION_CSRDIR_LEFT		0x4d
#define SED1330_INSTRUCTION_CSRDIR_UP		0x4e
#define SED1330_INSTRUCTION_CSRDIR_DOWN		0x4f
#define SED1330_INSTRUCTION_HDOT_SCR		0x5a
#define SED1330_INSTRUCTION_OVLAY			0x5b
#define SED1330_INSTRUCTION_CSRW			0x46
#define SED1330_INSTRUCTION_CSRR			0x47
#define SED1330_INSTRUCTION_MWRITE			0x42
#define SED1330_INSTRUCTION_MREAD			0x43

#define SED1330_CSRDIR_RIGHT	0x00
#define SED1330_CSRDIR_LEFT		0x01
#define SED1330_CSRDIR_UP		0x10
#define SED1330_CSRDIR_DOWN		0x11

static const int SED1330_CYCLES[] = {
	4, 4, 4, 4, 4, -1, -1, -1, 4, 4, 4, 4, 6, 6, 36, 36
};

#define SED1330_MODE_EXTERNAL_CG	0x01	/* not supported */
#define SED1330_MODE_GRAPHIC		0x02	/* text mode not supported */
#define SED1330_MODE_CURSOR			0x04	/* not supported */
#define SED1330_MODE_BLINK			0x08	/* not supported */
#define SED1330_MODE_MASTER			0x10	/* not supported */
#define SED1330_MODE_DISPLAY_ON		0x20

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _sed1330_t sed1330_t;
struct _sed1330_t
{
	devcb_resolved_read8		in_vd_func;
	devcb_resolved_write8		out_vd_func;

	int bf;						/* busy flag */

	UINT8 ir;					/* instruction register */

	int m0;						/*  */
	int m1;						/*  */
	int m2;						/*  */
	int ws;						/*  */
	int iv;						/*  */
	int fx;						/* horizontal character size */
	int fy;						/* vertical character size */
	int wf;						/* AC frame drive waveform period */
	int cr;						/* address range covered by 1 display line */
	int tcr;					/* line length (including horizontal blanking) */
	int lf;						/* frame height in lines */
	UINT16 ap;					/* horizontal address range of the virtual screen */

	int hdotscr;				/* horizontal dot scroll */

	UINT16 csr;					/* cursor address register */
	int csrdir;					/* cursor direction */

	int pbc;					/* parameter byte counter */

	/* devices */
	const device_config *screen;

	/* timers */
	emu_timer *busy_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE sed1330_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (sed1330_t *)device->token;
}

INLINE const sed1330_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == SED1330));
	return (const sed1330_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( busy_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( busy_tick )
{
	const device_config *device = ptr;
	sed1330_t *sed1330 = get_safe_token(device);

	/* clear busy flag */
	sed1330->bf = 0;
}

/*-------------------------------------------------
    set_busy_flag - set busy flag and arm timer
					to clear it later
-------------------------------------------------*/
#ifdef UNUSED_FUNCTION
static void set_busy_flag(sed1330_t *sed1330, int period)
{
	/* set busy flag */
	sed1330->bf = 1;

	/* adjust busy timer */
	timer_adjust_oneshot(sed1330->busy_timer, ATTOTIME_IN_USEC(period), 0);
}
#endif
/*-------------------------------------------------
    sed1330_status_r - status read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( sed1330_status_r )
{
	sed1330_t *sed1330 = get_safe_token(device);

	if (LOG) logerror("SED1330 '%s' Status Read: %s\n", device->tag, sed1330->bf ? "busy" : "ready");

	return sed1330->bf << 6;
}

/*-------------------------------------------------
    sed1330_control_w - control write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( sed1330_command_w )
{
	sed1330_t *sed1330 = get_safe_token(device);

	sed1330->ir = data;
	sed1330->pbc = 0;
}

/*-------------------------------------------------
    sed1330_data_r - data read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( sed1330_data_r )
{
//	sed1330_t *sed1330 = get_safe_token(device);

	return 0;
}

/*-------------------------------------------------
    sed1330_data_w - data write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( sed1330_data_w )
{
	sed1330_t *sed1330 = get_safe_token(device);

	switch (sed1330->ir)
	{
	case SED1330_INSTRUCTION_SYSTEM_SET:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->m0 = BIT(data, 0);
			sed1330->m1 = BIT(data, 1);
			sed1330->m2 = BIT(data, 2);
			sed1330->ws = BIT(data, 3);
			sed1330->iv = BIT(data, 5);

			if (LOG)
			{
				logerror("SED1330 '%s' %s CG ROM\n", device->tag, BIT(data, 0) ? "External" : "Internal");
				logerror("SED1330 '%s' D6 Correction %s\n", device->tag, BIT(data, 1) ? "enabled" : "disabled");
				logerror("SED1330 '%s' Character Height: %u\n", device->tag, BIT(data, 2) ? 16 : 8);
				logerror("SED1330 '%s' %s Panel Drive\n", device->tag, BIT(data, 3) ? "Dual" : "Single");
				logerror("SED1330 '%s' Screen Top-Line Correction: %s\n", device->tag, BIT(data, 5) ? "disabled" : "enabled");
			}
			break;

		case 1:
			sed1330->fx = (data & 0x07) + 1;
			sed1330->wf = BIT(data, 7);

			if (LOG)
			{
				logerror("SED1330 '%s' Horizontal Character Size: %u\n", device->tag, sed1330->fx);
				logerror("SED1330 '%s' %s AC Drive\n", device->tag, BIT(data, 7) ? "2-frame" : "16-line");
			}
			break;

		case 2:
			sed1330->fy = (data & 0x0f) + 1;
			if (LOG) logerror("SED1330 '%s' Vertical Character Size: %u\n", device->tag, sed1330->fy);
			break;

		case 3:
			sed1330->cr = data + 1;
			break;

		case 4:
			sed1330->tcr = data + 1;
			break;

		case 5:
			sed1330->lf = data + 1;
			break;

		case 6:
			sed1330->ap = (sed1330->ap & 0xff00) | data;
			break;

		case 7:
			sed1330->ap = (data << 8) | (sed1330->ap & 0xff);
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;
/*
	case SED1330_INSTRUCTION_SLEEP_IN:
		break;

	case SED1330_INSTRUCTION_DISP_ON:
		break;

	case SED1330_INSTRUCTION_DISP_OFF:
		break;

	case SED1330_INSTRUCTION_SCROLL:
		break;

	case SED1330_INSTRUCTION_CSRFORM:
		break;

	case SED1330_INSTRUCTION_CGRAM_ADR:
		break;
*/
	case SED1330_INSTRUCTION_CSRDIR_RIGHT:
		sed1330->csrdir = data & 0x03;
		if (LOG) logerror("SED1330 '%s' Cursor Direction: Right\n", device->tag);
		break;
/*
	case SED1330_INSTRUCTION_CSRDIR_LEFT:
		sed1330->csrdir = data & 0x03;
		if (LOG) logerror("SED1330 '%s' Cursor Direction: Left\n", device->tag);
		break;

	case SED1330_INSTRUCTION_CSRDIR_UP:
		sed1330->csrdir = data & 0x03;
		if (LOG) logerror("SED1330 '%s' Cursor Direction: Up\n", device->tag);
		break;

	case SED1330_INSTRUCTION_CSRDIR_DOWN:
		sed1330->csrdir = data & 0x03;
		if (LOG) logerror("SED1330 '%s' Cursor Direction: Down\n", device->tag);
		break;
*/
	case SED1330_INSTRUCTION_HDOT_SCR:
		sed1330->hdotscr = data & 0x07;
		if (LOG) logerror("SED1330 '%s' Horizontal Dot Scroll: %u\n", device->tag, sed1330->hdotscr);
		break;
/*
	case SED1330_INSTRUCTION_OVLAY:
		break;
*/
	case SED1330_INSTRUCTION_CSRW:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->csr = (sed1330->csr & 0xff00) | data;
			if (LOG) logerror("SED1330 '%s' Cursor Address %04x\n", device->tag, sed1330->csr);
			break;

		case 1:		
			sed1330->csr = (data << 8) | (sed1330->csr & 0xff);
			if (LOG) logerror("SED1330 '%s' Cursor Address %04x\n", device->tag, sed1330->csr);
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;
/*
	case SED1330_INSTRUCTION_CSRR:
		break;
*/
	case SED1330_INSTRUCTION_MWRITE:
		devcb_call_write8(&sed1330->out_vd_func, sed1330->csr, data);

		switch (sed1330->csrdir)
		{
		case SED1330_CSRDIR_RIGHT:
			sed1330->csr++;
			break;
			
		default:
			logerror("SED1330 '%s' Unsupported cursor direction\n", device->tag);
		}

		break;
/*
	case SED1330_INSTRUCTION_MREAD:
		break;
*/
	default:
		logerror("SED1330 '%s' Unsupported instruction %02x\n", device->tag, sed1330->ir);
	}

	sed1330->pbc++;
}
#ifdef UNUSED_FUNCTION
/*-------------------------------------------------
    draw_scanline - draw one scanline
-------------------------------------------------*/

static void draw_scanline(sed1330_t *sed1330, bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 ra)
{
}

/*-------------------------------------------------
    update_graphics - draw graphics mode screen
-------------------------------------------------*/

static void update_graphics(sed1330_t *sed1330, bitmap_t *bitmap, const rectangle *cliprect)
{
}
#endif
/*-------------------------------------------------
    sed1330_update - update screen
-------------------------------------------------*/

void sed1330_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/*-------------------------------------------------
    DEVICE_START( sed1330 )
-------------------------------------------------*/

static DEVICE_START( sed1330 )
{
	sed1330_t *sed1330 = get_safe_token(device);
	const sed1330_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&sed1330->in_vd_func, &intf->in_vd_func, device);
	devcb_resolve_write8(&sed1330->out_vd_func, &intf->out_vd_func, device);

	/* get the screen device */
	sed1330->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(sed1330->screen != NULL);

	/* create the busy timer */
	sed1330->busy_timer = timer_alloc(device->machine, busy_tick, (void *)device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, sed1330->);
}

/*-------------------------------------------------
    DEVICE_RESET( sed1330 )
-------------------------------------------------*/

static DEVICE_RESET( sed1330 )
{
//	sed1330_t *sed1330 = get_safe_token(device);

}

/*-------------------------------------------------
    DEVICE_GET_INFO( sed1330 )
-------------------------------------------------*/

DEVICE_GET_INFO( sed1330 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(sed1330_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(sed1330);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(sed1330);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, " SED1330");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, " SED1330");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}
