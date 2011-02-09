/***************************************************************************

    IBM-PC printer interface

***************************************************************************/

#include "emu.h"
#include "pc_lpt.h"
#include "machine/ctronics.h"


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( pc_lpt_ack_w );


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pc_lpt_state pc_lpt_state;
struct _pc_lpt_state
{
	device_t *centronics;

	devcb_resolved_write_line out_irq_func;

	int ack;

	/* control latch */
	int strobe;
	int autofd;
	int init;
	int select;
	int irq_enabled;
};


/***************************************************************************
    CENTRONICS INTERFACE
***************************************************************************/

static const centronics_interface pc_centronics_config =
{
	TRUE,
	DEVCB_LINE(pc_lpt_ack_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_CONFIG_FRAGMENT( pc_lpt )
	MCFG_CENTRONICS_ADD("centronics", pc_centronics_config)
MACHINE_CONFIG_END


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE pc_lpt_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == PC_LPT);

	return (pc_lpt_state *)downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( pc_lpt )
{
	pc_lpt_state *lpt = get_safe_token(device);
	const pc_lpt_interface *intf = (const pc_lpt_interface *)device->baseconfig().static_config();
	/* validate some basic stuff */
	assert(device->baseconfig().static_config() != NULL);

	/* get centronics device */
	lpt->centronics = device->subdevice("centronics");
	assert(lpt->centronics != NULL);

	/* resolve callbacks */
	devcb_resolve_write_line(&lpt->out_irq_func, &intf->out_irq_func, device);

	/* register for state saving */
	device->save_item(NAME(lpt->ack));
	device->save_item(NAME(lpt->strobe));
	device->save_item(NAME(lpt->autofd));
	device->save_item(NAME(lpt->init));
	device->save_item(NAME(lpt->select));
	device->save_item(NAME(lpt->irq_enabled));
}

static DEVICE_RESET( pc_lpt )
{
	pc_lpt_state *lpt = get_safe_token(device);

	lpt->strobe = TRUE;
	lpt->autofd = TRUE;
	lpt->init = FALSE;
	lpt->select = TRUE;
	lpt->irq_enabled = FALSE;
}

DEVICE_GET_INFO( pc_lpt )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(pc_lpt_state);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_CONFIG_NAME(pc_lpt);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(pc_lpt);		break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(pc_lpt);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "PC-LPT");				break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Parallel port");		break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");					break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);				break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");	break;
	}
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( pc_lpt_ack_w )
{
	pc_lpt_state *lpt = get_safe_token(device->owner());

	if (lpt->irq_enabled && lpt->ack == TRUE && state == FALSE)
	{
		/* pulse irq when going from high to low */
		devcb_call_write_line(&lpt->out_irq_func, TRUE);
		devcb_call_write_line(&lpt->out_irq_func, FALSE);
	}

	lpt->ack = state;
}


READ8_DEVICE_HANDLER( pc_lpt_data_r )
{
	pc_lpt_state *lpt = get_safe_token(device);
	return centronics_data_r(lpt->centronics, 0);
}


WRITE8_DEVICE_HANDLER( pc_lpt_data_w )
{
	pc_lpt_state *lpt = get_safe_token(device);
	centronics_data_w(lpt->centronics, 0, data);
}


READ8_DEVICE_HANDLER( pc_lpt_status_r )
{
	pc_lpt_state *lpt = get_safe_token(device);
	UINT8 result = 0;

	result |= centronics_fault_r(lpt->centronics) << 3;
	result |= centronics_vcc_r(lpt->centronics) << 4; /* SELECT is connected to VCC */
	result |= !centronics_pe_r(lpt->centronics) << 5;
	result |= centronics_ack_r(lpt->centronics) << 6;
	result |= !centronics_busy_r(lpt->centronics) << 7;

	return result;
}


READ8_DEVICE_HANDLER( pc_lpt_control_r )
{
	pc_lpt_state *lpt = get_safe_token(device);
	UINT8 result = 0;

	/* return latch state */
	result |= lpt->strobe << 0;
	result |= lpt->autofd << 1;
	result |= lpt->init << 2;
	result |= lpt->select << 3;
	result |= lpt->irq_enabled << 4;

	return result;
}


WRITE8_DEVICE_HANDLER( pc_lpt_control_w )
{
	pc_lpt_state *lpt = get_safe_token(device);

	logerror("pc_lpt_control_w: 0x%02x\n", data);

	/* save to latch */
	lpt->strobe = BIT(data, 0);
	lpt->autofd = BIT(data, 1);
	lpt->init = BIT(data, 2);
	lpt->select = BIT(data, 3);
	lpt->irq_enabled = BIT(data, 4);

	/* output to centronics */
	centronics_strobe_w(lpt->centronics, lpt->strobe);
	centronics_autofeed_w(lpt->centronics, lpt->autofd);
	centronics_init_w(lpt->centronics, lpt->init);
}


READ8_DEVICE_HANDLER( pc_lpt_r )
{
	switch (offset)
	{
	case 0: return pc_lpt_data_r(device, 0);
	case 1: return pc_lpt_status_r(device, 0);
	case 2: return pc_lpt_control_r(device, 0);
	}

	/* if we reach this its an error */
	logerror("PC-LPT %s: Read from invalid offset %x\n", device->tag(), offset);

	return 0xff;
}


WRITE8_DEVICE_HANDLER( pc_lpt_w )
{
	switch (offset)
	{
	case 0: pc_lpt_data_w(device, 0, data); break;
	case 1: break;
	case 2:	pc_lpt_control_w(device, 0, data); break;
	}
}

DEFINE_LEGACY_DEVICE(PC_LPT, pc_lpt);
