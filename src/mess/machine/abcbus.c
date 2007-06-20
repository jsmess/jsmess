#include "driver.h"
#include "abcbus.h"
#include "devices/basicdsk.h"

static int abcbus_strobe;
static int abcbus_channel;

READ8_HANDLER( abcbus_data_r )
{
	return 0xff;
}

WRITE8_HANDLER( abcbus_data_w )
{
}

READ8_HANDLER( abcbus_status_r )
{
	return 0xff;
}

WRITE8_HANDLER( abcbus_channel_w )
{
	abcbus_channel = data;
}

WRITE8_HANDLER( abcbus_command_w )
{
}

READ8_HANDLER( abcbus_reset_r )
{
	abcbus_strobe = CLEAR_LINE;

	return 0xff;
}

READ8_HANDLER( abcbus_strobe_r )
{
	abcbus_strobe = ASSERT_LINE;

	return 0xff;
}

WRITE8_HANDLER( abcbus_strobe_w )
{
	abcbus_strobe = ASSERT_LINE;
}

DEVICE_LOAD( abc_floppy )
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
