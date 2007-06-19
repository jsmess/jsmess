#include "driver.h"
#include "abcbus.h"

static int abcbus_strobe;

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

WRITE8_HANDLER( abcbus_select_w )
{
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
