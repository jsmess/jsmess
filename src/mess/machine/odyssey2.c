/***************************************************************************

  odyssey2.c

  Machine file to handle emulation of the Odyssey 2.

***************************************************************************/

#include "emu.h"
#include "includes/odyssey2.h"
#include "devices/cartslot.h"
#include "sound/sp0256.h"


static int the_voice_lrq_state;
static UINT8 *ram;
static UINT8 p1, p2;
static size_t	cart_size;

static void odyssey2_switch_banks(running_machine *machine)
{
	switch ( cart_size ) {
	case 12288:
		/* 12KB cart support (for instance, KTAA as released) */
		memory_set_bankptr( machine, "bank1", memory_region(machine, "user1") + (p1 & 0x03) * 0xC00 );
		memory_set_bankptr( machine, "bank2", memory_region(machine, "user1") + (p1 & 0x03) * 0xC00 + 0x800 );
		break;
	case 16384:
		/* 16KB cart support (for instance, full sized version KTAA) */
		memory_set_bankptr( machine, "bank1", memory_region(machine, "user1") + (p1 & 0x03) * 0x1000 + 0x400 );
		memory_set_bankptr( machine, "bank2", memory_region(machine, "user1") + (p1 & 0x03) * 0x1000 + 0xC00 );
		break;
	default:
		memory_set_bankptr(machine, "bank1", memory_region(machine, "user1") + (p1 & 0x03) * 0x800);
		memory_set_bankptr(machine, "bank2", memory_region(machine, "user1") + (p1 & 0x03) * 0x800 );
		break;
	}
}

void odyssey2_the_voice_lrq_callback(running_device *device, int state) {
	the_voice_lrq_state = state;
}

READ8_HANDLER( odyssey2_t0_r ) {
	return ( the_voice_lrq_state == ASSERT_LINE ) ? 0 : 1;
}

DRIVER_INIT( odyssey2 )
{
	int i;
	UINT8 *gfx = memory_region(machine, "gfx1");
	ram        = auto_alloc_array(machine, UINT8, 256);

	for (i = 0; i < 256; i++)
    {
		gfx[i] = i;     /* TODO: Why i and not 0? */
		ram[i] = 0;
    }
}

MACHINE_RESET( odyssey2 )
{
	/* jump to "last" bank, will work for all sizes due to being mirrored */
	p1 = 0xFF;
	p2 = 0xFF;
	odyssey2_switch_banks(machine);
}

/****** External RAM ******************************/

READ8_HANDLER( odyssey2_bus_r )
{
    if ((p1 & (P1_VDC_COPY_MODE_ENABLE | P1_VDC_ENABLE)) == 0)
		return odyssey2_video_r(space, offset); /* seems to have higher priority than ram??? */

    else if (!(p1 & P1_EXT_RAM_ENABLE))
		return ram[offset];

    return 0;
}

WRITE8_HANDLER( odyssey2_bus_w )
{
    if ((p1 & (P1_EXT_RAM_ENABLE | P1_VDC_COPY_MODE_ENABLE)) == 0x00) {
		ram[offset] = data;
		if ( offset & 0x80 ) {
			if ( data & 0x20 ) {
				logerror("voice write %02X, data = %02X (p1 = %02X)\n", offset, data, p1 );
				sp0256_ALD_w( space->machine->device("sp0256_speech"), 0, offset & 0x7F );
			} else {
				/* TODO: Reset sp0256 in this case */
			}
		}
	}

    else if (!(p1 & P1_VDC_ENABLE))
		odyssey2_video_w(space, offset, data);
}

READ8_HANDLER( g7400_bus_r )
{
	if ((p1 & (P1_VDC_COPY_MODE_ENABLE | P1_VDC_ENABLE)) == 0) {
		return odyssey2_video_r(space, offset); /* seems to have higher priority than ram??? */
	}
	else if (!(p1 & P1_EXT_RAM_ENABLE)) {
		return ram[offset];
	} else {
//      return ef9341_r( offset & 0x02, offset & 0x01 );
	}

	return 0;
}

WRITE8_HANDLER( g7400_bus_w )
{
	if ((p1 & (P1_EXT_RAM_ENABLE | P1_VDC_COPY_MODE_ENABLE)) == 0x00) {
		ram[offset] = data;
	}
	else if (!(p1 & P1_VDC_ENABLE)) {
		odyssey2_video_w(space, offset, data);
	} else {
//      ef9341_w( offset & 0x02, offset & 0x01, data );
	}
}

/***** 8048 Ports ************************/

READ8_HANDLER( odyssey2_getp1 )
{
	UINT8 data = p1;

	logerror("%.9f p1 read %.2x\n", attotime_to_double(timer_get_time(space->machine)), data);
	return data;
}

WRITE8_HANDLER( odyssey2_putp1 )
{
	p1 = data;

	odyssey2_switch_banks(space->machine);

	odyssey2_lum_w ( space, 0, p1 >> 7 );

	logerror("%.6f p1 written %.2x\n", attotime_to_double(timer_get_time(space->machine)), data);
}

READ8_HANDLER( odyssey2_getp2 )
{
    UINT8 h = 0xFF;
    int i, j;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5" };

    if (!(p1 & P1_KEYBOARD_SCAN_ENABLE))
	{
		if ((p2 & P2_KEYBOARD_SELECT_MASK) <= 5)  /* read keyboard */
		{
			h &= input_port_read(space->machine, keynames[p2 & P2_KEYBOARD_SELECT_MASK]);
		}

		for (i= 0x80, j = 0; i > 0; i >>= 1, j++)
		{
			if (!(h & i))
			{
				p2 &= ~0x10;                   /* set key was pressed indicator */
				p2 = (p2 & ~0xE0) | (j << 5);  /* column that was pressed */

				break;
			}
		}

        if (h == 0xFF)  /* active low inputs, so no keypresses */
            p2 = p2 | 0xF0;
    }

    else
        p2 = p2 | 0xF0;

    logerror("%.6f p2 read %.2x\n", attotime_to_double(timer_get_time(space->machine)), p2);
    return p2;
}

WRITE8_HANDLER( odyssey2_putp2 )
{
    p2 = data;

    logerror("%.6f p2 written %.2x\n", attotime_to_double(timer_get_time(space->machine)), data);
}

READ8_HANDLER( odyssey2_getbus )
{
    UINT8 data = 0xff;

    if ((p2 & P2_KEYBOARD_SELECT_MASK) == 1)
		data &= input_port_read(space->machine, "JOY0");       /* read joystick 1 */

    if ((p2 & P2_KEYBOARD_SELECT_MASK) == 0)
		data &= input_port_read(space->machine, "JOY1");       /* read joystick 2 */

    logerror("%.6f bus read %.2x\n", attotime_to_double(timer_get_time(space->machine)), data);
    return data;
}

WRITE8_HANDLER( odyssey2_putbus )
{
    logerror("%.6f bus written %.2x\n", attotime_to_double(timer_get_time(space->machine)), data);
}

///////////////////////////////////

int odyssey2_cart_verify(const UINT8 *cartdata, size_t size)
{
	cart_size = size;
    if (   (size == 2048)
        || (size == 4096)
        || (size == 8192)
		|| (size == 12288)
		|| (size == 16384))
    {
		return IMAGE_VERIFY_PASS;
    }

    return IMAGE_VERIFY_FAIL;
}
