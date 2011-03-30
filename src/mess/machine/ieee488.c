/**********************************************************************

    IEEE-488.1 General Purpose Interface Bus emulation
    (aka HP-IB, GPIB, CBM IEEE)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	EOI = 0,
	DAV,
	NRFD,
	NDAC,
	IFC,
	SRQ,
	ATN,
	REN,
	SIGNAL_COUNT
};

static const char *const SIGNAL_NAME[] = { "EOI", "DAV", "NRFD", "NDAC", "IFC", "SRQ", "ATN", "REN" };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ieee488_daisy_state ieee488_daisy_state;
struct _ieee488_daisy_state
{
	ieee488_daisy_state			*next;			/* next device */
	device_t *device;		/* associated device */

	int line[SIGNAL_COUNT];						/* control lines' state */
	UINT8 dio;									/* data lines' state */

	devcb_resolved_write_line	out_line_func[SIGNAL_COUNT];
};

typedef struct _ieee488_t ieee488_t;
struct _ieee488_t
{
	ieee488_daisy_state *daisy_state;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE ieee488_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == IEEE488);
	return (ieee488_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ieee488_daisy_chain *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == IEEE488);
	return (const ieee488_daisy_chain *) device->baseconfig().static_config();
}

INLINE int get_signal(device_t *bus, int line)
{
	ieee488_t *ieee488 = get_safe_token(bus);
	ieee488_daisy_state *daisy = ieee488->daisy_state;
	int state = 1;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		state &= daisy->line[line];
	}

	return state;
}

INLINE int get_data(device_t *bus)
{
	ieee488_t *ieee488 = get_safe_token(bus);
	ieee488_daisy_state *daisy = ieee488->daisy_state;
	UINT8 data = 0xff;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		data &= daisy->dio;
	}

	return data;
}

INLINE void set_signal(device_t *bus, device_t *device, int line, int state)
{
	ieee488_t *ieee488 = get_safe_token(bus);
	ieee488_daisy_state *daisy = ieee488->daisy_state;
	int new_state = 1;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		if (!strcmp(daisy->device->tag(), device->tag()))
		{
			if (daisy->line[line] != state)
			{
				if (LOG) logerror("IEEE-488: '%s' %s %u\n", device->tag(), SIGNAL_NAME[line], state);
				daisy->line[line] = state;
			}
			break;
		}
	}

	new_state = get_signal(bus, line);
	daisy = ieee488->daisy_state;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		devcb_call_write_line(&daisy->out_line_func[line], new_state);
	}

	if (LOG) logerror("IEEE-488: EOI %u DAV %u NRFD %u NDAC %u IFC %u SRQ %u ATN %u REN %u DIO %02x\n",
		get_signal(bus, EOI), get_signal(bus, DAV), get_signal(bus, NRFD), get_signal(bus, NDAC),
		get_signal(bus, IFC), get_signal(bus, SRQ), get_signal(bus, ATN), get_signal(bus, REN), get_data(bus));
}

INLINE void set_data(device_t *bus, device_t *device, UINT8 data)
{
	ieee488_t *ieee488 = get_safe_token(bus);
	ieee488_daisy_state *daisy = ieee488->daisy_state;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		if (!strcmp(daisy->device->tag(), device->tag()))
		{
			if (LOG) logerror("IEEE-488: '%s' DIO %02x\n", device->tag(), data);
			daisy->dio = data;
			break;
		}
	}

	if (LOG) logerror("IEEE-488: EOI %u DAV %u NRFD %u NDAC %u IFC %u SRQ %u ATN %u REN %u DIO %02x\n",
		get_signal(bus, EOI), get_signal(bus, DAV), get_signal(bus, NRFD), get_signal(bus, NDAC),
		get_signal(bus, IFC), get_signal(bus, SRQ), get_signal(bus, ATN), get_signal(bus, REN), get_data(bus));
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

void ieee488_eoi_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, EOI, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_eoi_r )
{
	return get_signal(device, EOI);
}

void ieee488_dav_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, DAV, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_dav_r )
{
	return get_signal(device, DAV);
}

void ieee488_nrfd_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, NRFD, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_nrfd_r )
{
	return get_signal(device, NRFD);
}

void ieee488_ndac_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, NDAC, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_ndac_r )
{
	return get_signal(device, NDAC);
}

void ieee488_ifc_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, IFC, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_ifc_r )
{
	return get_signal(device, IFC);
}

void ieee488_srq_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, SRQ, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_srq_r )
{
	return get_signal(device, SRQ);
}

void ieee488_atn_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, ATN, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_atn_r )
{
	return get_signal(device, ATN);
}

void ieee488_ren_w(device_t *bus, device_t *device, int state)
{
	set_signal(bus, device, REN, state);
}

READ_LINE_DEVICE_HANDLER( ieee488_ren_r )
{
	return get_signal(device, REN);
}

READ8_DEVICE_HANDLER( ieee488_dio_r )
{
	return get_data(device);
}

void ieee488_dio_w(device_t *bus, device_t *device, UINT8 data)
{
	set_data(bus, device, data);
}

/*-------------------------------------------------
    DEVICE_START( ieee488 )
-------------------------------------------------*/

static DEVICE_START( ieee488 )
{
	ieee488_t *ieee488 = get_safe_token(device);
	const ieee488_daisy_chain *daisy = get_interface(device);
	int i;

	ieee488_daisy_state *head = NULL;
	ieee488_daisy_state **tailptr = &head;

	/* create a linked list of devices */
	for ( ; daisy->tag != NULL; daisy++)
	{
		*tailptr = auto_alloc(device->machine(), ieee488_daisy_state);

		/* find device */
		(*tailptr)->next = NULL;
		(*tailptr)->device = device->machine().device(daisy->tag);

		if ((*tailptr)->device == NULL)
		{
			fatalerror("Unable to locate device '%s'", daisy->tag);
		}

		/* set control lines to 'false' state */
		for (i = EOI; i < SIGNAL_COUNT; i++)
		{
			(*tailptr)->line[i] = 1;
		}

		/* set data lines to 'false' state */
		(*tailptr)->dio = 0xff;

		/* resolve callbacks */
		devcb_resolve_write_line(&(*tailptr)->out_line_func[EOI], &daisy->out_eoi_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[DAV], &daisy->out_dav_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[NRFD], &daisy->out_nrfd_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[NDAC], &daisy->out_ndac_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[IFC], &daisy->out_ifc_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[SRQ], &daisy->out_srq_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[ATN], &daisy->out_atn_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_line_func[REN], &daisy->out_ren_func, device);

		tailptr = &(*tailptr)->next;
	}

	ieee488->daisy_state = head;

	/* register for state saving */
//  device->save_item(NAME(ieee488->));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( ieee488 )
-------------------------------------------------*/

DEVICE_GET_INFO( ieee488 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ieee488_t);								break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(ieee488);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							/* Nothing */												break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "IEEE-488.1 Bus");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "IEEE-488");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(IEEE488, ieee488);
