#include "driver.h"
#include "abcbus.h"
#include "devices/basicdsk.h"

typedef struct _abcbus_daisy_state abcbus_daisy_state;
struct _abcbus_daisy_state
{
	abcbus_daisy_state *	next;			/* next device */
	const device_config *	device;			/* associated device */
	abcbus_card_select		card_select;	/* card select callback */
};

static abcbus_daisy_state *daisy_state;

void abcbus_init(running_machine *machine, const char *cputag, const abcbus_daisy_chain *daisy)
{
	astring *tempstring = astring_alloc();
	abcbus_daisy_state *head = NULL;
	abcbus_daisy_state **tailptr = &head;

	/* create a linked list of devices */
	for ( ; daisy->devtype != NULL; daisy++)
	{
		*tailptr = auto_malloc(sizeof(**tailptr));
		(*tailptr)->next = NULL;
		(*tailptr)->device = devtag_get_device(machine, daisy->devtype, device_inherit_tag(tempstring, cputag, daisy->devname));
		if ((*tailptr)->device == NULL)
			fatalerror("Unable to locate device '%s'", daisy->devname);
		(*tailptr)->card_select = (abcbus_card_select)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CARD_SELECT);
		tailptr = &(*tailptr)->next;
	}
	
	astring_free(tempstring);

	daisy_state = head;
}

WRITE8_HANDLER( abcbus_channel_w )
{
	abcbus_daisy_state *daisy = daisy_state;

	/* loop over all devices and call their card select function */
	for ( ; daisy != NULL; daisy = daisy_state->next)
		(*daisy->card_select)(daisy->device, data);
}

READ8_HANDLER( abcbus_reset_r )
{
	abcbus_daisy_state *daisy = daisy_state;

	/* loop over all devices and call their reset function */
	for ( ; daisy != NULL; daisy = daisy->next)
		device_reset(daisy->device);

	return 0xff;
}

DEVICE_IMAGE_LOAD( abc_floppy )
{
	int size, tracks, heads, sectors;

	if (image_has_been_created(image))
		return INIT_FAIL;

	size = image_length (image);
	switch (size)
	{
	case 80*1024: /* Scandia Metric FD2 */
		tracks = 40;
		heads = 1;
		sectors = 8;
		break;
	case 160*1024: /* ABC 830 */
		tracks = 40;
		heads = 1;
		sectors = 16;
		break;
	case 640*1024: /* ABC 832/834 */
		tracks = 80;
		heads = 2;
		sectors = 16;
		break;
	case 1001*1024: /* ABC 838 */
		tracks = 77;
		heads = 2;
		sectors = 26;
		break;
	default:
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, tracks, heads, sectors, 256, 0, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
