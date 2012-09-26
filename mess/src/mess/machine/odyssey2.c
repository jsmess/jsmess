/***************************************************************************

  odyssey2.c

  Machine file to handle emulation of the Odyssey 2.

***************************************************************************/

#include "emu.h"
#include "includes/odyssey2.h"
#include "imagedev/cartslot.h"
#include "sound/sp0256.h"



static void odyssey2_switch_banks(running_machine &machine)
{
	odyssey2_state *state = machine.driver_data<odyssey2_state>();
	switch ( state->m_cart_size ) {
	case 12288:
		/* 12KB cart support (for instance, KTAA as released) */
		memory_set_bankptr( machine, "bank1", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0xC00 );
		memory_set_bankptr( machine, "bank2", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0xC00 + 0x800 );
		break;
	case 16384:
		/* 16KB cart support (for instance, full sized version KTAA) */
		memory_set_bankptr( machine, "bank1", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0x1000 + 0x400 );
		memory_set_bankptr( machine, "bank2", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0x1000 + 0xC00 );
		break;
	default:
		memory_set_bankptr(machine, "bank1", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0x800);
		memory_set_bankptr(machine, "bank2", machine.region("user1")->base() + (state->m_p1 & 0x03) * 0x800 );
		break;
	}
}

void odyssey2_the_voice_lrq_callback(device_t *device, int state) {
	odyssey2_state *drvstate = device->machine().driver_data<odyssey2_state>();
	drvstate->m_the_voice_lrq_state = state;
}

READ8_HANDLER( odyssey2_t0_r ) {
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
	return ( state->m_the_voice_lrq_state == ASSERT_LINE ) ? 0 : 1;
}

DRIVER_INIT( odyssey2 )
{
	odyssey2_state *state = machine.driver_data<odyssey2_state>();
	int i;
	UINT8 *gfx = machine.region("gfx1")->base();
	state->m_ram        = auto_alloc_array(machine, UINT8, 256);

	for (i = 0; i < 256; i++)
    {
		gfx[i] = i;     /* TODO: Why i and not 0? */
		state->m_ram[i] = 0;
    }
}

MACHINE_RESET( odyssey2 )
{
	odyssey2_state *state = machine.driver_data<odyssey2_state>();
	/* jump to "last" bank, will work for all sizes due to being mirrored */
	state->m_p1 = 0xFF;
	state->m_p2 = 0xFF;
	odyssey2_switch_banks(machine);
}

/****** External RAM ******************************/

READ8_HANDLER( odyssey2_bus_r )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
    if ((state->m_p1 & (P1_VDC_COPY_MODE_ENABLE | P1_VDC_ENABLE)) == 0)
		return odyssey2_video_r(space, offset); /* seems to have higher priority than ram??? */

    else if (!(state->m_p1 & P1_EXT_RAM_ENABLE))
		return state->m_ram[offset];

    return 0;
}

WRITE8_HANDLER( odyssey2_bus_w )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
    if ((state->m_p1 & (P1_EXT_RAM_ENABLE | P1_VDC_COPY_MODE_ENABLE)) == 0x00) {
		state->m_ram[offset] = data;
		if ( offset & 0x80 ) {
			if ( data & 0x20 ) {
				logerror("voice write %02X, data = %02X (p1 = %02X)\n", offset, data, state->m_p1 );
				sp0256_ALD_w( space->machine().device("sp0256_speech"), 0, offset & 0x7F );
			} else {
				/* TODO: Reset sp0256 in this case */
			}
		}
	}

    else if (!(state->m_p1 & P1_VDC_ENABLE))
		odyssey2_video_w(space, offset, data);
}

READ8_HANDLER( g7400_bus_r )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
	if ((state->m_p1 & (P1_VDC_COPY_MODE_ENABLE | P1_VDC_ENABLE)) == 0) {
		return odyssey2_video_r(space, offset); /* seems to have higher priority than ram??? */
	}
	else if (!(state->m_p1 & P1_EXT_RAM_ENABLE)) {
		return state->m_ram[offset];
	} else {
//      return ef9341_r( offset & 0x02, offset & 0x01 );
	}

	return 0;
}

WRITE8_HANDLER( g7400_bus_w )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
	if ((state->m_p1 & (P1_EXT_RAM_ENABLE | P1_VDC_COPY_MODE_ENABLE)) == 0x00) {
		state->m_ram[offset] = data;
	}
	else if (!(state->m_p1 & P1_VDC_ENABLE)) {
		odyssey2_video_w(space, offset, data);
	} else {
//      ef9341_w( offset & 0x02, offset & 0x01, data );
	}
}

/***** 8048 Ports ************************/

READ8_HANDLER( odyssey2_getp1 )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
	UINT8 data = state->m_p1;

	logerror("%.9f p1 read %.2x\n", space->machine().time().as_double(), data);
	return data;
}

WRITE8_HANDLER( odyssey2_putp1 )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
	state->m_p1 = data;

	odyssey2_switch_banks(space->machine());

	odyssey2_lum_w ( space, 0, state->m_p1 >> 7 );

	logerror("%.6f p1 written %.2x\n", space->machine().time().as_double(), data);
}

READ8_HANDLER( odyssey2_getp2 )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
    UINT8 h = 0xFF;
    int i, j;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5" };

    if (!(state->m_p1 & P1_KEYBOARD_SCAN_ENABLE))
	{
		if ((state->m_p2 & P2_KEYBOARD_SELECT_MASK) <= 5)  /* read keyboard */
		{
			h &= input_port_read(space->machine(), keynames[state->m_p2 & P2_KEYBOARD_SELECT_MASK]);
		}

		for (i= 0x80, j = 0; i > 0; i >>= 1, j++)
		{
			if (!(h & i))
			{
				state->m_p2 &= ~0x10;                   /* set key was pressed indicator */
				state->m_p2 = (state->m_p2 & ~0xE0) | (j << 5);  /* column that was pressed */

				break;
			}
		}

        if (h == 0xFF)  /* active low inputs, so no keypresses */
            state->m_p2 = state->m_p2 | 0xF0;
    }

    else
        state->m_p2 = state->m_p2 | 0xF0;

    logerror("%.6f p2 read %.2x\n", space->machine().time().as_double(), state->m_p2);
    return state->m_p2;
}

WRITE8_HANDLER( odyssey2_putp2 )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
    state->m_p2 = data;

    logerror("%.6f p2 written %.2x\n", space->machine().time().as_double(), data);
}

READ8_HANDLER( odyssey2_getbus )
{
	odyssey2_state *state = space->machine().driver_data<odyssey2_state>();
    UINT8 data = 0xff;

    if ((state->m_p2 & P2_KEYBOARD_SELECT_MASK) == 1)
		data &= input_port_read(space->machine(), "JOY0");       /* read joystick 1 */

    if ((state->m_p2 & P2_KEYBOARD_SELECT_MASK) == 0)
		data &= input_port_read(space->machine(), "JOY1");       /* read joystick 2 */

    logerror("%.6f bus read %.2x\n", space->machine().time().as_double(), data);
    return data;
}

WRITE8_HANDLER( odyssey2_putbus )
{
    logerror("%.6f bus written %.2x\n", space->machine().time().as_double(), data);
}

///////////////////////////////////

#ifdef UNUSED_FUNCTION
int odyssey2_cart_verify(const UINT8 *cartdata, size_t size)
{
	odyssey2_state *state = machine.driver_data<odyssey2_state>();
	state->m_cart_size = size;
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
#endif
