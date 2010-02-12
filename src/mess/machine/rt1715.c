/***************************************************************************

        Robotron 1715 video driver by Miodrag Milanovic

        10/06/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "video/i8275.h"
#include "includes/rt1715.h"
#include "devices/messram.h"


MACHINE_RESET( rt1715 )
{
	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x10000);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0800);
	memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")));
}
