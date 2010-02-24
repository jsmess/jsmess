/**********************************************************************

    SMC CRT9007 CRT Video Processor and Controller VPAC emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "crt9007.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _crt9007_t crt9007_t;
struct _crt9007_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_dmar_func;
	devcb_resolved_write_line	out_hs_func;
	devcb_resolved_write_line	out_vs_func;
	devcb_resolved_read8		in_vd_func;

	UINT8 reg[64];
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE crt9007_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (crt9007_t *)device->token;
}

INLINE const crt9007_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type == CRT9007);
	return (const crt9007_interface *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/
/*-------------------------------------------------
    crt9007_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( crt9007_r )
{
	crt9007_t *crt9007 = get_safe_token(device);

	UINT8 data = 0;

	swítch (offset)
	{
	case 0x15:
		if (LOG) logerror("CRT9007 '%s' Start\n", device->tag.cstr());
		break;

	case 0x16:
		if (LOG) logerror("CRT9007 '%s' Reset\n", device->tag.cstr());
		break;

	case 0x38:
		data = crt9007->reg[0x18];
		break;

	case 0x39:
		data = crt9007->reg[0x19];
		break;

	case 0x3a:
		break;

	case 0x3b:
		break;

	case 0x3c:
		break;

	default:
		logerror("CRT9007 '%s' Read from Invalid Register: %02x!\n", device->tag.cstr(), offset);
	}

	return data;
}

/*-------------------------------------------------
    crt9007_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( crt9007_w )
{
	crt9007_t *crt9007 = get_safe_token(device);

	crt9007->reg[offset] = data;

	switch (offset)
	{
	case 0x00:
		if (LOG) logerror("CRT9007 '%s' Characters per Horizontal Period: %u\n", device->tag.cstr(), data);
		break;
	
	case 0x01:
		if (LOG) logerror("CRT9007 '%s' Characters per Data Row: %u\n", device->tag.cstr(), data + 1);
		break;

	case 0x02:
		if (LOG) logerror("CRT9007 '%s' Horizontal Delay: %u\n", device->tag.cstr(), data);
		break;

	case 0x03:
		if (LOG) logerror("CRT9007 '%s' Horizontal Sync Width: %u\n", device->tag.cstr(), data);
		break;

	case 0x04:
		if (LOG) logerror("CRT9007 '%s' Vertical Sync Width: %u\n", device->tag.cstr(), data);
		break;

	case 0x05:
		if (LOG) logerror("CRT9007 '%s' Vertical Delay: %u\n", device->tag.cstr(), data - 1);
		break;

	case 0x06:
		if (LOG) 
		{
			logerror("CRT9007 '%s' Pin Configuration: %u\n", device->tag.cstr(), data >> 6);
			logerror("CRT9007 '%s' Cursor Skew: %u\n", device->tag.cstr(), (data >> 3) & 0x07);
			logerror("CRT9007 '%s' Blank Skew: %u\n", device->tag.cstr(), data & 0x07);
		}
		break;

	case 0x07:
		if (LOG) logerror("CRT9007 '%s' Visible Data Rows per Frame: %u\n", device->tag.cstr(), data + 1);
		break;

	case 0x08:
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Data Row: %u\n", device->tag.cstr(), (data & 0x1f) + 1);
		break;

	case 0x09:
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Frame: %u\n", device->tag.cstr(), ((crt9007->reg[0x08] << 3) & 0xff00) | data);
		break;

	case 0x0a:
		if (LOG)
		{
			logerror("CRT9007 '%s' DMA Burst Count: %u\n", device->tag.cstr(), (data & 0x0f) + 1);
			logerror("CRT9007 '%s' DMA Burst Delay: %u\n", device->tag.cstr(), (data >> 4) & 0x07);
			logerror("CRT9007 '%s' DMA Disable: %u\n", device->tag.cstr(), BIT(data, 7));
		}
		break;

	case 0x0b:
		if (LOG)
		{
			logerror("CRT9007 '%s' %s Height Cursor\n", device->tag.cstr(), BIT(data, 0) ? "Single" : "Double");
			logerror("CRT9007 '%s' Operation Mode: %u\n", device->tag.cstr(), (data >> 1) & 0x07);
			logerror("CRT9007 '%s' Interlace Mode: %u\n", device->tag.cstr(), (data >> 4) & 0x03);
			logerror("CRT9007 '%s' %s Mechanism\n", device->tag.cstr(), BIT(data, 6) ? "Page Blank" : "Smooth Scroll");
		}
		break;

	case 0x0c:
		break;

	case 0x0d:
		if (LOG)
		{
			logerror("CRT9007 '%s' Table Start Register: %04x\n", device->tag.cstr(), ((data << 8) & 0x3f00) | crt9007->reg[0x0c]);
			logerror("CRT9007 '%s' Address Mode: %u\n", device->tag.cstr(), (data >> 6) & 0x03);
		}
		break;

	case 0x0e:
		break;

	case 0x0f:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxialiary Address Register 1: %04x\n", device->tag.cstr(), ((data << 8) & 0x3f00) | crt9007->reg[0x0e]);
			logerror("CRT9007 '%s' Row Attributes: %u\n", device->tag.cstr(), (data >> 6) & 0x03);
		}
		break;

	case 0x10:
		if (LOG) logerror("CRT9007 '%s' Sequential Break Register 1: %u\n", device->tag.cstr(), data);
		break;

	case 0x11:
		if (LOG) logerror("CRT9007 '%s' Data Row Start Register: %u\n", device->tag.cstr(), data);
		break;

	case 0x12:
		if (LOG) logerror("CRT9007 '%s' Data Row End/Sequential Break Register 2: %u\n", device->tag.cstr(), data);
		break;

	case 0x13:
		break;

	case 0x14:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxiliary Address Register 2: %04x\n", device->tag.cstr(), ((data << 8) & 0x3f00) | crt9007->reg[0x13]);
			logerror("CRT9007 '%s' Row Attributes: %u\n", device->tag.cstr(), (data >> 6) & 0x03);
		}
		break;

	case 0x15:
		if (LOG) logerror("CRT9007 '%s' Start\n", device->tag.cstr());
		break;

	case 0x16:
		if (LOG) logerror("CRT9007 '%s' Reset\n", device->tag.cstr());
		break;

	case 0x17:
		if (LOG)
		{
			logerror("CRT9007 '%s' Smooth Scroll Offset: %u\n", device->tag.cstr(), (data >> 1) & 0x3f);
			logerror("CRT9007 '%s' Smooth Scroll Offset Overflow: %u\n", device->tag.cstr(), BIT(data, 7));
		}
		break;

	case 0x18:
		if (LOG) logerror("CRT9007 '%s' Vertical Cursor Register: %u\n", device->tag.cstr(), data);
		break;

	case 0x19:
		if (LOG) logerror("CRT9007 '%s' Horizontal Cursor Register: %u\n", device->tag.cstr(), data);
		break;

	case 0x1a:
		if (LOG)
		{
			logerror("CRT9007 '%s' Frame Timer: %u\n", device->tag.cstr(), BIT(data, 0));
			logerror("CRT9007 '%s' Light Pen Interrupt: %u\n", device->tag.cstr(), BIT(data, 5));
			logerror("CRT9007 '%s' Vertical Retrace Interrupt: %u\n", device->tag.cstr(), BIT(data, 6));
		}
		break;

	default:
		logerror("CRT9007 '%s' Write to Invalid Register: %02x!\n", device->tag.cstr(), offset);
	}
}

/*-------------------------------------------------
    crt9007_update - update screen
-------------------------------------------------*/

void crt9007_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/*-------------------------------------------------
    DEVICE_START( crt9007 )
-------------------------------------------------*/

static DEVICE_START( crt9007 )
{
	crt9007_t *crt9007 = (crt9007_t *)device->token;
	const crt9007_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&crt9007->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&crt9007->out_dmar_func, &intf->out_dmar_func, device);
	devcb_resolve_write_line(&crt9007->out_hs_func, &intf->out_hs_func, device);
	devcb_resolve_write_line(&crt9007->out_vs_func, &intf->out_vs_func, device);
	devcb_resolve_read8(&crt9007->in_vd_func, &intf->in_vd_func, device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, crt9007->);
}

/*-------------------------------------------------
    DEVICE_RESET( crt9007 )
-------------------------------------------------*/

static DEVICE_RESET( crt9007 )
{
//	crt9007_t *crt9007 = (crt9007_t *)device->token;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( crt9007 )
-------------------------------------------------*/

DEVICE_GET_INFO( crt9007 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(crt9007_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(crt9007);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(crt9007);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
