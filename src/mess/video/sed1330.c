/**********************************************************************

    Seiko-Epson SED1330 LCD Timing Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- CSRDIR is +1 or +AP, but no command is written to select?
	- garbage on screen
	- FX > 8

*/

#include "driver.h"
#include "sed1330.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define SED1330_INSTRUCTION_SYSTEM_SET		0x40
#define SED1330_INSTRUCTION_SLEEP_IN		0x53
#define SED1330_INSTRUCTION_DISP_ON			0x59
#define SED1330_INSTRUCTION_DISP_OFF		0x58
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

#define SED1330_MX_OR			0x00
#define SED1330_MX_XOR			0x01
#define SED1330_MX_AND			0x10
#define SED1330_MX_PRIORITY_OR	0x11

#define SED1330_FC_OFF			0x00
#define SED1330_FC_NO_FLASH		0x01
#define SED1330_FC_FLASH_32		0x10
#define SED1330_FC_FLASH_16		0x11

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

	int d;						/* display enabled */

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
	UINT16 ap;					/* horizontal address pitch of the virtual screen */

	UINT16 sad1;				/* scroll start address */
	UINT16 sad2;				/* scroll start address */
	UINT16 sad3;				/* scroll start address */
	UINT16 sad4;				/* scroll start address */
	int sl1;					/* scroll block screen lines */
	int sl2;					/* scroll block screen lines */
	int hdotscr;				/* horizontal dot scroll */

	UINT16 csr;					/* cursor address register */
	int cd;						/* cursor direction */
	int crx;					/* horizontal cursor size */
	int cry;					/* vertical cursor size/location */
	int cm;						/* cursor shape (0=underscore, 1=block) */
	int fc;						/* cursor flash control */
	int fp;						/* cursor flash control */

	UINT16 sag;					/* character generator RAM start address */

	int mx;						/* screen layer composition method */
	int dm;						/* display mode for screen blocks 1, 3 */
	int ov;						/* graphics mode layer composition */

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

	return !sed1330->bf << 6;
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
logerror("SED1330 '%s' read data\n");
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
				logerror("SED1330 '%s' D6 Correction: %s\n", device->tag, BIT(data, 1) ? "enabled" : "disabled");
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
			if (LOG) logerror("SED1330 '%s' Visible Characters Per Line: %u\n", device->tag, sed1330->cr);
			break;

		case 4:
			sed1330->tcr = data + 1;
			if (LOG) logerror("SED1330 '%s' Total Characters Per Line: %u\n", device->tag, sed1330->tcr);
			break;

		case 5:
			sed1330->lf = data + 1;
			if (LOG) logerror("SED1330 '%s' Frame Height: %u\n", device->tag, sed1330->lf);
			break;

		case 6:
			sed1330->ap = (sed1330->ap & 0xff00) | data;
			break;

		case 7:
			sed1330->ap = (data << 8) | (sed1330->ap & 0xff);
			if (LOG) logerror("SED1330 '%s' Virtual Screen Width: %u\n", device->tag, sed1330->ap);
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;
/*
	case SED1330_INSTRUCTION_SLEEP_IN:
		break;
*/
	case SED1330_INSTRUCTION_DISP_ON:
	case SED1330_INSTRUCTION_DISP_OFF:
		sed1330->d = BIT(data, 0);
		sed1330->fc = data & 0x03;
		sed1330->fp = data >> 2;
		if (LOG)
		{
			logerror("SED1330 '%s' Display: %s\n", device->tag, BIT(data, 0) ? "enabled" : "disabled");
		}
		break;

	case SED1330_INSTRUCTION_SCROLL:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->sad1 = (sed1330->sad1 & 0xff00) | data;
			break;

		case 1:
			sed1330->sad1 = (data << 8) | (sed1330->sad1 & 0xff);
			if (LOG) logerror("SED1330 '%s' Display Page 1 Start Address: %04x\n", device->tag, sed1330->sad1);
			break;

		case 2:
			sed1330->sl1 = data + 1;
			if (LOG) logerror("SED1330 '%s' Display Page 1 Screen Lines: %u\n", device->tag, sed1330->sl1);
			break;

		case 3:
			sed1330->sad2 = (sed1330->sad2 & 0xff00) | data;
			break;

		case 4:
			sed1330->sad2 = (data << 8) | (sed1330->sad2 & 0xff);
			if (LOG) logerror("SED1330 '%s' Display Page 2 Start Address: %04x\n", device->tag, sed1330->sad2);
			break;

		case 5:
			sed1330->sl2 = data + 1;
			if (LOG) logerror("SED1330 '%s' Display Page 2 Screen Lines: %u\n", device->tag, sed1330->sl2);
			break;

		case 6:
			sed1330->sad3 = (sed1330->sad3 & 0xff00) | data;
			break;

		case 7:
			sed1330->sad3 = (data << 8) | (sed1330->sad3 & 0xff);
			if (LOG) logerror("SED1330 '%s' Display Page 3 Start Address: %04x\n", device->tag, sed1330->sad3);
			break;

		case 8:
			sed1330->sad4 = (sed1330->sad4 & 0xff00) | data;
			break;

		case 9:
			sed1330->sad4 = (data << 8) | (sed1330->sad4 & 0xff);
			if (LOG) logerror("SED1330 '%s' Display Page 4 Start Address: %04x\n", device->tag, sed1330->sad4);
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;

	case SED1330_INSTRUCTION_CSRFORM:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->crx = (data & 0x0f) + 1;
			if (LOG) logerror("SED1330 '%s' Horizontal Cursor Size: %u\n", device->tag, sed1330->crx);
			break;

		case 1:
			sed1330->cry = (data & 0x0f) + 1;
			sed1330->cm = BIT(data, 7);
			if (LOG) 
			{
				logerror("SED1330 '%s' Vertical Cursor Location: %u\n", device->tag, sed1330->cry);
				logerror("SED1330 '%s' Cursor Shape: %s\n", device->tag, BIT(data, 7) ? "Block" : "Underscore");
			}
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;

	case SED1330_INSTRUCTION_CGRAM_ADR:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->sag = (sed1330->sag & 0xff00) | data;
			break;

		case 1:
			sed1330->sag = (data << 8) | (sed1330->sag & 0xff);
			if (LOG) logerror("SED1330 '%s' Character Generator RAM Start Address: %04x\n", device->tag, sed1330->sag);
			break;

		default:
			logerror("SED1330 '%s' Invalid parameter byte %02x\n", device->tag, data);
		}
		break;

	case SED1330_INSTRUCTION_CSRDIR_RIGHT:
	case SED1330_INSTRUCTION_CSRDIR_LEFT:
	case SED1330_INSTRUCTION_CSRDIR_UP:
	case SED1330_INSTRUCTION_CSRDIR_DOWN:
		sed1330->cd = data & 0x03;
		if (LOG)
		{
			switch (sed1330->cd)
			{
			case SED1330_CSRDIR_RIGHT:	logerror("SED1330 '%s' Cursor Direction: Right\n", device->tag);	break;
			case SED1330_CSRDIR_LEFT:	logerror("SED1330 '%s' Cursor Direction: Left\n", device->tag);		break;
			case SED1330_CSRDIR_UP:		logerror("SED1330 '%s' Cursor Direction: Up\n", device->tag);		break;
			case SED1330_CSRDIR_DOWN:	logerror("SED1330 '%s' Cursor Direction: Down\n", device->tag);		break;
			}
		}
		break;

	case SED1330_INSTRUCTION_HDOT_SCR:
		sed1330->hdotscr = data & 0x07;
		if (LOG) logerror("SED1330 '%s' Horizontal Dot Scroll: %u\n", device->tag, sed1330->hdotscr);
		break;

	case SED1330_INSTRUCTION_OVLAY:
		sed1330->mx = data & 0x03;
		sed1330->dm = (data >> 2) & 0x03;
		sed1330->ov = BIT(data, 4);

		if (LOG)
		{
			switch (sed1330->mx)
			{
			case SED1330_MX_OR:
				logerror("SED1330 '%s' Layered Screen Composition Method: OR\n", device->tag);
				break;
			
			case SED1330_MX_XOR:
				logerror("SED1330 '%s' Layered Screen Composition Method: Exclusive-OR\n", device->tag);
				break;
			
			case SED1330_MX_AND:
				logerror("SED1330 '%s' Layered Screen Composition Method: AND\n", device->tag);
				break;

			case SED1330_MX_PRIORITY_OR:
				logerror("SED1330 '%s' Layered Screen Composition Method: Priority-OR\n", device->tag);
				break;
			}

			logerror("SED1330 '%s' Display Page 1 Mode: %s\n", device->tag, BIT(data, 2) ? "Graphics" : "Text");
			logerror("SED1330 '%s' Display Page 3 Mode: %s\n", device->tag, BIT(data, 3) ? "Graphics" : "Text");
			logerror("SED1330 '%s' Display Composition Layers: %u\n", device->tag, BIT(data, 4) ? 3 : 2);
		}
		break;

	case SED1330_INSTRUCTION_CSRW:
		switch (sed1330->pbc)
		{
		case 0:
			sed1330->csr = (sed1330->csr & 0xff00) | data;
			if (LOG) logerror("SED1330 '%s' Cursor Address Low %02x %04x %u\n", device->tag, data, sed1330->csr, sed1330->csr/80);
			break;

		case 1:		
			sed1330->csr = (data << 8) | (sed1330->csr & 0xff);
			if (LOG) logerror("SED1330 '%s' Cursor Address High %02x %04x %u\n", device->tag, data, sed1330->csr, sed1330->csr/80);
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
		if (LOG) if (data) logerror("SED1330 '%s' Memory Write %02x to %04x\n", device->tag, data, sed1330->csr);

		devcb_call_write8(&sed1330->out_vd_func, sed1330->csr, data);
/*
		switch (sed1330->cd)
		{
		case SED1330_CSRDIR_RIGHT:
			sed1330->csr++;
			break;

		case SED1330_CSRDIR_LEFT:
			sed1330->csr--;
			break;

		case SED1330_CSRDIR_UP:
			sed1330->csr -= sed1330->ap;
			break;

		case SED1330_CSRDIR_DOWN:
			sed1330->csr += sed1330->ap;
			break;
		}
*/
		sed1330->csr += sed1330->ap; // HACK
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

/*-------------------------------------------------
    draw_graphics_scanline - draw one scanline
-------------------------------------------------*/

static void draw_graphics_scanline(sed1330_t *sed1330, bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 va)
{
	int sx, x;

	for (sx = 0; sx < sed1330->cr; sx++)
	{
		UINT8 data = devcb_call_read8(&sed1330->in_vd_func, va++);

		for (x = 0; x < sed1330->fx; x++)
		{
			*BITMAP_ADDR16(bitmap, y, sed1330->hdotscr + (sx * sed1330->fx) + x) = BIT(data, 7);
			data <<= 1;
		}
	}
}

/*-------------------------------------------------
    update_graphics - draw graphics mode screen
-------------------------------------------------*/

static void update_graphics(sed1330_t *sed1330, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/*-------------------------------------------------
    update_text - draw text mode screen
-------------------------------------------------*/

static void update_text(sed1330_t *sed1330, bitmap_t *bitmap, const rectangle *cliprect)
{
	int y;
	
	for (y = 0; y < sed1330->sl1; y++)
	{
		UINT16 sad2 = sed1330->sad2 + (y * sed1330->ap);
		UINT16 sad4 = sed1330->sad4 + (y * sed1330->ap);

		/* draw graphics display page 2 scanline */
		draw_graphics_scanline(sed1330, bitmap, cliprect, y, sad2);

		/* draw graphics display page 4 scanline */
		draw_graphics_scanline(sed1330, bitmap, cliprect, y + sed1330->sl1, sad4);
	}
}

/*-------------------------------------------------
    sed1330_update - update screen
-------------------------------------------------*/

void sed1330_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	sed1330_t *sed1330 = get_safe_token(device);

	if (sed1330->d)
	{
		if (sed1330->dm)
		{
			update_graphics(sed1330, bitmap, cliprect);
		}
		else
		{
			update_text(sed1330, bitmap, cliprect);
		}
	}
	else
	{
		//fillbitmap(bitmap, 0);
	}
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
		case DEVINFO_STR_NAME:							strcpy(info->s, "Seiko-Epson SED1330");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Seiko-Epson SED1330");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}
