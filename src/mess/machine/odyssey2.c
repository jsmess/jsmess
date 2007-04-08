/***************************************************************************

  odyssey2.c

  Machine file to handle emulation of the Odyssey 2.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/odyssey2.h"
#include "image.h"
#include "devices/cartslot.h"
#include "zlib.h"
#include "osdmess.h"

static UINT8 *ram;
static UINT8 p1, p2;

DRIVER_INIT( odyssey2 )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	ram        = auto_malloc(256);

	for (i = 0; i < 256; i++)
    {
		gfx[i] = i;     /* TODO: Why i and not 0? */
		ram[i] = 0;
    }
}

MACHINE_RESET( odyssey2 )
{
    /* jump to "last" bank, will work for all sizes due to being mirrored */
    memory_set_bankptr(1, memory_region(REGION_USER1) + 0x800*3);

    p1 = 0xFF;
    p2 = 0xFF;
    return;
}

/****** External RAM ******************************/

READ8_HANDLER( odyssey2_bus_r )
{
    if ((p1 & (P1_VDC_COPY_MODE_ENABLE | P1_VDC_ENABLE)) == 0)
		return odyssey2_video_r(offset); /* seems to have higher priority than ram??? */

    else if (!(p1 & P1_EXT_RAM_ENABLE))
		return ram[offset];

    return 0;
}

WRITE8_HANDLER( odyssey2_bus_w )
{
    if ((p1 & (P1_EXT_RAM_ENABLE | P1_VDC_COPY_MODE_ENABLE)) == 0x00)
		ram[offset] = data;

    else if (!(p1 & P1_VDC_ENABLE))
		odyssey2_video_w(offset, data);
}

/***** 8048 Ports ************************/

READ8_HANDLER( odyssey2_getp1 )
{
    UINT8 data = p1;

    logerror("%.9f p1 read %.2x\n", timer_get_time(), data);
    return data;
}

WRITE8_HANDLER( odyssey2_putp1 )
{
    p1 = data;

    /* ROM is mirrored so this will be OK for all sizes 2k/4k/8k */
    memory_set_bankptr(1, memory_region(REGION_USER1) + (0x800 * (p1 & 0x03)));

    logerror("%.6f p1 written %.2x\n", timer_get_time(), data);
}

READ8_HANDLER( odyssey2_getp2 )
{
    UINT8 h = 0xFF;
    int i, j;

    if (!(p1 & P1_KEYBOARD_SCAN_ENABLE))
	{
		if ((p2 & P2_KEYBOARD_SELECT_MASK) <= 5)  /* read keyboard */
			h &= readinputport(p2 & P2_KEYBOARD_SELECT_MASK);

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
    
    logerror("%.6f p2 read %.2x\n", timer_get_time(), p2);
    return p2;
}

WRITE8_HANDLER( odyssey2_putp2 )
{
    p2 = data;

    logerror("%.6f p2 written %.2x\n", timer_get_time(), data);
}

READ8_HANDLER( odyssey2_getbus )
{
    UINT8 data = 0xff;

    if ((p2 & P2_KEYBOARD_SELECT_MASK) != 0)
		data &= readinputport(6);       /* read joystick 1 */

    if ((p2 & P2_KEYBOARD_SELECT_MASK) == 0)
		data &= readinputport(7);       /* read joystick 2 */

    logerror("%.6f bus read %.2x\n", timer_get_time(), data);
    return data;
}

WRITE8_HANDLER( odyssey2_putbus )
{
    logerror("%.6f bus written %.2x\n", timer_get_time(), data);
}

///////////////////////////////////

int odyssey2_cart_verify(const UINT8 *cartdata, size_t size)
{
    if (   (size == 2048)
        || (size == 4096)
        || (size == 8192))
    {
		return IMAGE_VERIFY_PASS;
    }

    return IMAGE_VERIFY_FAIL;
}
