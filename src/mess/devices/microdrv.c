/*********************************************************************

    microdrv.c

    MESS interface to the Sinclair Microdrive image abstraction code

*********************************************************************/

#include "emu.h"
#include "microdrv.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG 1

#define MDV_SECTOR_COUNT			255
#define MDV_SECTOR_LENGTH			686
#define MDV_IMAGE_LENGTH			(MDV_SECTOR_COUNT * MDV_SECTOR_LENGTH)

#define MDV_PREAMBLE_LENGTH			12
#define MDV_GAP_LENGTH				120

#define MDV_OFFSET_HEADER_PREAMBLE	0
#define MDV_OFFSET_HEADER			MDV_OFFSET_HEADER_PREAMBLE + MDV_PREAMBLE_LENGTH
#define MDV_OFFSET_DATA_PREAMBLE	28
#define MDV_OFFSET_DATA				MDV_OFFSET_DATA_PREAMBLE + MDV_PREAMBLE_LENGTH
#define MDV_OFFSET_GAP				566

#define MDV_BITRATE					120000 // invalid, from ZX microdrive

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _microdrive_t microdrive_t;
struct _microdrive_t
{
	devcb_resolved_write_line out_comms_out_func;

	int clk;
	int comms_in;
	int comms_out;
	int erase;
	int read_write;

	UINT8 *left;
	UINT8 *right;

	int bit_offset;
	int byte_offset;

	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE HELPERS
***************************************************************************/

INLINE microdrive_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MICRODRIVE);
	return (microdrive_t *) downcast<legacy_device_base *>(device)->token();
}

/***************************************************************************
    LIVE DEVICE
***************************************************************************/

static TIMER_CALLBACK( bit_timer_tick )
{
	device_t *device = (device_t *) ptr;
	microdrive_t *mdv = get_safe_token(device);

	mdv->bit_offset++;

	if (mdv->bit_offset == 8)
	{
		mdv->bit_offset = 0;
		mdv->byte_offset++;

		if (mdv->byte_offset == MDV_IMAGE_LENGTH)
		{
			mdv->byte_offset = 0;
		}
	}
}

WRITE_LINE_DEVICE_HANDLER( microdrive_clk_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (LOG) logerror("Microdrive '%s' CLK: %u\n", device->tag(), state);

	if (!mdv->clk && state)
	{
		mdv->comms_out = mdv->comms_in;

		if (LOG) logerror("Microdrive '%s' COMMS OUT: %u\n", device->tag(), mdv->comms_out);

		devcb_call_write_line(&mdv->out_comms_out_func, mdv->comms_out);

		mdv->bit_timer->enable(mdv->comms_out);
	}

	mdv->clk = state;
}

WRITE_LINE_DEVICE_HANDLER( microdrive_comms_in_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (LOG) logerror("Microdrive '%s' COMMS IN: %u\n", device->tag(), state);

	mdv->comms_in = state;
}

WRITE_LINE_DEVICE_HANDLER( microdrive_erase_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (LOG) logerror("Microdrive '%s' ERASE: %u\n", device->tag(), state);

	mdv->erase = state;
}

WRITE_LINE_DEVICE_HANDLER( microdrive_read_write_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (LOG) logerror("Microdrive '%s' READ/WRITE: %u\n", device->tag(), state);

	mdv->read_write = state;
}

WRITE_LINE_DEVICE_HANDLER( microdrive_data1_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (mdv->comms_out && !mdv->read_write)
	{
		// TODO
	}
}

WRITE_LINE_DEVICE_HANDLER( microdrive_data2_w )
{
	microdrive_t *mdv = get_safe_token(device);

	if (mdv->comms_out && !mdv->read_write)
	{
		// TODO
	}
}

READ_LINE_DEVICE_HANDLER( microdrive_data1_r )
{
	microdrive_t *mdv = get_safe_token(device);

	int data = 0;

	if (mdv->comms_out && mdv->read_write)
	{
		data = BIT(mdv->left[mdv->byte_offset], 7 - mdv->bit_offset);
	}

	return data;
}

READ_LINE_DEVICE_HANDLER( microdrive_data2_r )
{
	microdrive_t *mdv = get_safe_token(device);

	int data = 0;

	if (mdv->comms_out && mdv->read_write)
	{
		data = BIT(mdv->right[mdv->byte_offset], 7 - mdv->bit_offset);
	}

	return data;
}

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

static DEVICE_START( microdrive )
{
	microdrive_t *mdv = get_safe_token(device);
	const microdrive_config *config = (const microdrive_config*) device->baseconfig().static_config();

	// resolve callbacks
	devcb_resolve_write_line(&mdv->out_comms_out_func, &config->out_comms_out_func, device);

	// allocate track buffers
	mdv->left = auto_alloc_array(device->machine(), UINT8, MDV_IMAGE_LENGTH / 2);
	mdv->right = auto_alloc_array(device->machine(), UINT8, MDV_IMAGE_LENGTH / 2);

	// allocate timers
	mdv->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_timer_tick), (void *) device);
	mdv->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(MDV_BITRATE));
	mdv->bit_timer->enable(0);
}

static DEVICE_IMAGE_LOAD( microdrive )
{
	device_t *device = &image.device();
	microdrive_t *mdv = get_safe_token(device);

	if (image.length() != MDV_IMAGE_LENGTH)
		return IMAGE_INIT_FAIL;

	for (int i = 0; i < MDV_IMAGE_LENGTH / 2; i++)
	{
		image.fread(mdv->left, 1);
		image.fread(mdv->right, 1);
	}

	mdv->bit_offset = 0;
	mdv->byte_offset = 0;

	return IMAGE_INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( microdrive )
{
	device_t *device = &image.device();
	microdrive_t *mdv = get_safe_token(device);

	memset(mdv->left, 0, MDV_IMAGE_LENGTH / 2);
	memset(mdv->right, 0, MDV_IMAGE_LENGTH / 2);
}

DEVICE_GET_INFO( microdrive )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(microdrive_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_CASSETTE; break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(microdrive); break;
		case DEVINFO_FCT_IMAGE_LOAD:					info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(microdrive); break;
		case DEVINFO_FCT_IMAGE_UNLOAD:					info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(microdrive); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Microdrive"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Microdrive"); break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "mdv"); break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(MICRODRIVE, microdrive);
