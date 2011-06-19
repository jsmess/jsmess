/***************************************************************************

        MC-80.xx by Miodrag Milanovic

        15/05/2009 Initial implementation
        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/mc80.h"

/*****************************************************************************/
/*                            Implementation for MC80.2x                     */
/*****************************************************************************/

static IRQ_CALLBACK(mc8020_irq_callback)
{
	return 0x00;
}

MACHINE_RESET(mc8020)
{
	device_set_irq_callback(machine.device("maincpu"), mc8020_irq_callback);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
	z80ctc_trg0_w(device, state);
	z80ctc_trg1_w(device, state);

}

Z80CTC_INTERFACE( mc8020_ctc_intf )
{
	0,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_LINE(ctc_z0_w),
	DEVCB_LINE(ctc_z1_w),
	DEVCB_LINE(ctc_z2_w)
};


static READ8_DEVICE_HANDLER (mc80_port_b_r)
{
	return 0;
}

static READ8_DEVICE_HANDLER (mc80_port_a_r)
{
	return 0;
}

static WRITE8_DEVICE_HANDLER (mc80_port_a_w)
{
}

static WRITE8_DEVICE_HANDLER (mc80_port_b_w)
{
}

Z80PIO_INTERFACE( mc8020_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(mc80_port_a_r),
	DEVCB_HANDLER(mc80_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(mc80_port_b_r),
	DEVCB_HANDLER(mc80_port_b_w),
	DEVCB_NULL
};

/*****************************************************************************/
/*                            Implementation for MC80.3x                     */
/*****************************************************************************/

WRITE8_HANDLER( mc8030_zve_write_protect_w )
{
}

WRITE8_HANDLER( mc8030_vis_w )
{
	mc80_state *state = space->machine().driver_data<mc80_state>();
	// reg C
	// 7 6 5 4 -- module
	//         3 - 0 left half, 1 right half
	//           2 1 0
	//           =====
	//           0 0 0 - dark
	//           0 0 1 - light
	//           0 1 0 - in reg pixel
	//           0 1 1 - negate in reg pixel
	//           1 0 x - operation code in B reg
	// reg B
	//
	UINT16 addr = ((offset & 0xff00) >> 2) | ((offset & 0x08) << 2) | (data >> 3);
	static const UINT8 val[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
	int c = offset & 1;
	state->m_mc8030_video_mem[addr] = state->m_mc8030_video_mem[addr] | (val[data & 7]*c);

}

WRITE8_HANDLER( mc8030_eprom_prog_w )
{
}

static IRQ_CALLBACK(mc8030_irq_callback)
{
	return 0x20;
}

MACHINE_RESET(mc8030)
{
	device_set_irq_callback(machine.device("maincpu"), mc8030_irq_callback);
}

static READ8_DEVICE_HANDLER (zve_port_a_r)
{
	return 0xff;
}

static READ8_DEVICE_HANDLER (zve_port_b_r)
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER (zve_port_a_w)
{
}

static WRITE8_DEVICE_HANDLER (zve_port_b_w)
{
}

Z80PIO_INTERFACE( mc8030_zve_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(zve_port_a_r),
	DEVCB_HANDLER(zve_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(zve_port_b_r),
	DEVCB_HANDLER(zve_port_b_w),
	DEVCB_NULL
};

static READ8_DEVICE_HANDLER (asp_port_a_r)
{
	return 0xff;
}

static READ8_DEVICE_HANDLER (asp_port_b_r)
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER (asp_port_a_w)
{
}

static WRITE8_DEVICE_HANDLER (asp_port_b_w)
{
}

Z80PIO_INTERFACE( mc8030_asp_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(asp_port_a_r),
	DEVCB_HANDLER(asp_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(asp_port_b_r),
	DEVCB_HANDLER(asp_port_b_w),
	DEVCB_NULL
};

Z80CTC_INTERFACE( mc8030_zve_z80ctc_intf )
{
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

Z80CTC_INTERFACE( mc8030_asp_z80ctc_intf )
{
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

const z80sio_interface mc8030_asp_z80sio_intf =
{
	0,	/* interrupt handler */
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};

