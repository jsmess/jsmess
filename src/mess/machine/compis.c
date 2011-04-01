/******************************************************************************

    compis.c
    machine driver

    Per Ola Ingvarsson
    Tomas Karlsson

 ******************************************************************************/

/*-------------------------------------------------------------------------*/
/* Include files                                                           */
/*-------------------------------------------------------------------------*/

#include "emu.h"
#include "video/i82720.h"
#include "machine/i8255a.h"
#include "machine/mm58274c.h"
#include "machine/pic8259.h"
#include "includes/compis.h"
#include "machine/pit8253.h"
#include "machine/upd765.h"
#include "machine/msm8251.h"
#include "machine/ctronics.h"

/*-------------------------------------------------------------------------*/
/* Defines, constants, and global variables                                */
/*-------------------------------------------------------------------------*/

/* CPU 80186 */
#define LATCH_INTS 1
#define LOG_PORTS 1
#define LOG_INTERRUPTS 1
#define LOG_TIMER 1
#define LOG_OPTIMIZATION 1
#define LOG_DMA 1
#define CPU_RESUME_TRIGGER	7123

/* Keyboard */
static const UINT8 compis_keyb_codes[6][16] = {
{0x39, 0x32, 0x29, 0x20, 0x17, 0x0e, 0x05, 0x56, 0x4d, 0x44, 0x08, 0x57, 0x59, 0x4e, 0x43, 0x3a},
{0x31, 0x28, 0x1f, 0x16, 0x0d, 0x04, 0x55, 0x4c, 0x4f, 0x58, 0x00, 0x07, 0xff, 0x42, 0x3b, 0x30},
{0x27, 0x1e, 0x15, 0x0c, 0x03, 0x54, 0x06, 0x50, 0x01, 0xfe, 0x38, 0x2f, 0x26, 0x1d, 0x14, 0x0b},
{0x02, 0x53, 0x4a, 0x48, 0x51, 0xfe, 0xfd, 0xfc, 0x40, 0xfc, 0xfd, 0x0f, 0x18, 0x21, 0x22, 0x34},
{0x10, 0x11, 0x1a, 0x23, 0x1b, 0x2d, 0x04, 0x0a, 0x13, 0x1c, 0x2a, 0x3c, 0x33, 0x19, 0x46, 0x2b},
{0x2c, 0x3e, 0x35, 0x12, 0x3f, 0x24, 0x25, 0x37, 0x2e, 0x45, 0x3d, 0x47, 0x36, 0x00, 0x00, 0x00}
};
enum COMPIS_KEYB_SHIFT
{
	KEY_CAPS_LOCK = 0xff,
	KEY_SHIFT = 0xfe,
	KEY_SUPER_SHIFT = 0xfd,
	KEY_CTRL = 0xfc
};

enum COMPIS_USART_STATES
{
	COMPIS_USART_STATUS_TX_READY = 0x01,
	COMPIS_USART_STATUS_RX_READY = 0x02,
	COMPIS_USART_STATUS_TX_EMPTY = 0x04
};

/* Compis interrupt handling */
enum COMPIS_INTERRUPTS
{
	COMPIS_NMI_KEYB		= 0x00,	/* Default not used */
	COMPIS_INT_8251_TXRDY	= 0x01,	/* Default not used */
	COMPIS_INT_8274		= 0x03
};

enum COMPIS_INTERRUPT_REQUESTS
{
	COMPIS_IRQ_SBX0_INT1	 = 0x00,
	COMPIS_IRQ_SBX0_INT0	 = 0x01, /* Default not used */
	COMPIS_IRQ_8251_RXRDY	 = 0x02,
	COMPIS_IRQ_80150_SYSTICK = 0x03, /* Default not used */
	COMPIS_IRQ_ACK_J7	 = 0x04,
	COMPIS_IRQ_SBX1_INT1	 = 0x05, /* Default not used */
	COMPIS_IRQ_SBX1_INT0	 = 0x06, /* Default not used */
	COMPIS_IRQ_80150_DELAY	 = 0x07
};

/* Main emulation */


/*-------------------------------------------------------------------------*/
/* Name: compis_irq_set                                                    */
/* Desc: IRQ - Issue an interrupt request                                  */
/*-------------------------------------------------------------------------*/
#ifdef UNUSED_FUNCTION
void compis_irq_set(UINT8 irq)
{
	cputag_set_input_line_vector(machine, "maincpu", 0, irq);
	cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
}
#endif


/*-------------------------------------------------------------------------*/
/*  Keyboard                                                               */
/*-------------------------------------------------------------------------*/
static void compis_keyb_update(running_machine &machine)
{
	compis_state *state = machine.driver_data<compis_state>();
	UINT8 key_code;
	UINT8 key_status;
	UINT8 irow;
	UINT8 icol;
	UINT16 data;
	UINT16 ibit;
	static const char *const rownames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5" };

	key_code = 0;
	key_status = 0x80;

	for (irow = 0; irow < 6; irow++)
	{
		data = input_port_read(machine, rownames[irow]);
		if (data != 0)
		{
			ibit = 1;
			for (icol = 0; icol < 16; icol++)
			{
				if (data & ibit)
				{
					switch(compis_keyb_codes[irow][icol])
					{
						case KEY_SHIFT:
							key_status |= 0x01;
							break;
						case KEY_CAPS_LOCK:
							key_status |= 0x02;
							break;
						case KEY_CTRL:
							key_status |= 0x04;
							break;
						case KEY_SUPER_SHIFT:
							key_status |= 0x08;
							break;
						default:
							key_code = compis_keyb_codes[irow][icol];
					}
				}
				ibit <<= 1;
			}
		}
	}
	if (key_code != 0)
	{
		state->m_compis.keyboard.key_code = key_code;
		state->m_compis.keyboard.key_status = key_status;
		state->m_compis.usart.status |= COMPIS_USART_STATUS_RX_READY;
		state->m_compis.usart.bytes_sent = 0;
//      compis_osp_pic_irq(COMPIS_IRQ_8251_RXRDY);
	}
}

static void compis_keyb_init(compis_state *state)
{
	state->m_compis.keyboard.key_code = 0;
	state->m_compis.keyboard.key_status = 0x80;
	state->m_compis.usart.status = 0;
	state->m_compis.usart.bytes_sent = 0;
}

/*-------------------------------------------------------------------------*/
/*  FDC iSBX-218A                                                          */
/*-------------------------------------------------------------------------*/
static void compis_fdc_reset(running_machine &machine)
{
	device_t *fdc = machine.device("upd765");

	upd765_reset(fdc, 0);

	/* set FDC at reset */
	upd765_reset_w(fdc, 1);
}

static void compis_fdc_tc(running_machine &machine, int state)
{
	device_t *fdc = machine.device("upd765");
	/* Terminal count if iSBX-218A has DMA enabled */
	if (input_port_read(machine, "DSW1"))
	{
		upd765_tc_w(fdc, state);
	}
}

static WRITE_LINE_DEVICE_HANDLER( compis_fdc_int )
{
	compis_state *drvstate = device->machine().driver_data<compis_state>();
	/* No interrupt requests if iSBX-218A has DMA enabled */
	if (!input_port_read(device->machine(), "DSW1") && state)
	{
		if (drvstate->m_devices.pic8259_master)
		{
			pic8259_ir0_w(drvstate->m_devices.pic8259_master, 1);
			pic8259_ir0_w(drvstate->m_devices.pic8259_master, 0);
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( compis_fdc_dma_drq )
{
	/* DMA requst if iSBX-218A has DMA enabled */
	if (input_port_read(device->machine(), "DSW1") && state)
	{
		//compis_dma_drq(state, read);
	}
}

const upd765_interface compis_fdc_interface =
{
	DEVCB_LINE(compis_fdc_int),
	DEVCB_LINE(compis_fdc_dma_drq),
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

READ16_HANDLER (compis_fdc_dack_r)
{
	device_t *fdc = space->machine().device("upd765");
	UINT16 data;
	data = 0xffff;
	/* DMA acknowledge if iSBX-218A has DMA enabled */
	if (input_port_read(space->machine(), "DSW1"))
	{
		data = upd765_dack_r(fdc, 0);
	}

	return data;
}

WRITE8_HANDLER (compis_fdc_w)
{
	device_t *fdc = space->machine().device("upd765");
	switch(offset)
	{
		case 2:
			upd765_data_w(fdc, 0, data);
			break;
		default:
			printf("FDC Unknown Port Write %04X = %04X\n", offset, data);
			break;
	}
}

READ8_HANDLER (compis_fdc_r)
{
	device_t *fdc = space->machine().device("upd765");
	UINT16 data;
	data = 0xffff;
	switch(offset)
	{
		case 0:
			data = upd765_status_r(fdc, 0);
			break;
		case 2:
			data = upd765_data_r(fdc, 0);
			break;
		default:
			printf("FDC Unknown Port Read %04X\n", offset);
			break;
	}

	return data;
}



/*-------------------------------------------------------------------------*/
/* Bit 0: J5-4                                                             */
/* Bit 1: J5-5                                                     */
/* Bit 2: J6-3 Cassette read                                               */
/* Bit 3: J2-6 DSR / S8-4 Test                                             */
/* Bit 4: J4-6 DSR / S8-3 Test                                             */
/* Bit 5: J7-11 Centronics BUSY                                            */
/* Bit 6: J7-13 Centronics SELECT                              */
/* Bit 7: Tmr0                                                     */
/*-------------------------------------------------------------------------*/
static READ8_DEVICE_HANDLER( compis_ppi_port_b_r )
{
	UINT8 data;

	/* DIP switch - Test mode */
	data = input_port_read(device->machine(), "DSW0");

	/* Centronics busy */
	data |= centronics_busy_r(device) << 5;
	data |= centronics_vcc_r(device) << 6;

	return data;
}

/*-------------------------------------------------------------------------*/
/* Bit 0: J5-1                                                         */
/* Bit 1: J5-2                                                         */
/* Bit 2: Select: 1=time measure, DSR from J2/J4 pin 6. 0=read cassette    */
/* Bit 3: Datex: Tristate datex output (low)                   */
/* Bit 4: V2-5 Floppy motor on/off                                     */
/* Bit 5: J7-1 Centronics STROBE                                       */
/* Bit 6: V2-4 Floppy Soft reset                               */
/* Bit 7: V2-3 Floppy Terminal count                                   */
/*-------------------------------------------------------------------------*/
static WRITE8_DEVICE_HANDLER( compis_ppi_port_c_w )
{
	/* Centronics Strobe */
	centronics_strobe_w(device, BIT(data, 5));

	/* FDC Reset */
	if (BIT(data, 6))
		compis_fdc_reset(device->machine());

	/* FDC Terminal count */
	compis_fdc_tc(device->machine(), BIT(data, 7));
}

I8255A_INTERFACE( compis_ppi_interface )
{
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", compis_ppi_port_b_r),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", compis_ppi_port_c_w)
};


/*-------------------------------------------------------------------------*/
/*  PIT 8253                                                               */
/*-------------------------------------------------------------------------*/

const struct pit8253_config compis_pit8253_config =
{
	{
		/* Timer0 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL },
		/* Timer1 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL },
		/* Timer2 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL }
	}
};

const struct pit8253_config compis_pit8254_config =
{
	{
		/* Timer0 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL },
		/* Timer1 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL },
		/* Timer2 */
		{4770000/4, DEVCB_NULL, DEVCB_NULL }
	}
};

/*-------------------------------------------------------------------------*/
/*  OSP PIT 8254                                                           */
/*-------------------------------------------------------------------------*/

READ16_DEVICE_HANDLER ( compis_osp_pit_r )
{
	return pit8253_r(device, offset);
}

WRITE16_DEVICE_HANDLER ( compis_osp_pit_w )
{
	pit8253_w(device, offset, data);
}


/*-------------------------------------------------------------------------*/
/*  USART 8251                                                             */
/*-------------------------------------------------------------------------*/
static WRITE_LINE_DEVICE_HANDLER( compis_usart_rxready )
{
#if 0
	if (state)
		compis_pic_irq(COMPIS_IRQ_8251_RXRDY);
#endif
}

const msm8251_interface compis_usart_interface=
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(compis_usart_rxready),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

READ16_HANDLER ( compis_usart_r )
{
	device_t *uart = space->machine().device("uart");
	return msm8251_data_r(uart, offset);
}

WRITE16_HANDLER ( compis_usart_w )
{
	device_t *uart = space->machine().device("uart");
	switch (offset)
	{
		case 0x00:
			msm8251_data_w(uart, 0, data);
			break;
		case 0x01:
			msm8251_control_w(uart, 0, data);
			break;
		default:
			logerror("USART Unknown Port Write %04X = %04X\n", offset, data);
			break;
	}
}

/*************************************
 *
 *  80186 interrupt controller
 *
 *************************************/
static IRQ_CALLBACK(int_callback)
{
	compis_state *state = device->machine().driver_data<compis_state>();
	if (LOG_INTERRUPTS)
		logerror("(%f) **** Acknowledged interrupt vector %02X\n", device->machine().time().as_double(), state->m_i186.intr.poll_status & 0x1f);

	/* clear the interrupt */
	device_set_input_line(device, 0, CLEAR_LINE);
	state->m_i186.intr.pending = 0;

	/* clear the request and set the in-service bit */
#if LATCH_INTS
	state->m_i186.intr.request &= ~state->m_i186.intr.ack_mask;
#else
	state->m_i186.intr.request &= ~(state->m_i186.intr.ack_mask & 0x0f);
#endif
	state->m_i186.intr.in_service |= state->m_i186.intr.ack_mask;
	if (state->m_i186.intr.ack_mask == 0x0001)
	{
		switch (state->m_i186.intr.poll_status & 0x1f)
		{
			case 0x08:	state->m_i186.intr.status &= ~0x01;	break;
			case 0x12:	state->m_i186.intr.status &= ~0x02;	break;
			case 0x13:	state->m_i186.intr.status &= ~0x04;	break;
		}
	}
	state->m_i186.intr.ack_mask = 0;

	/* a request no longer pending */
	state->m_i186.intr.poll_status &= ~0x8000;

	/* return the vector */
	return state->m_i186.intr.poll_status & 0x1f;
}


static void update_interrupt_state(running_machine &machine)
{
	compis_state *state = machine.driver_data<compis_state>();
	int i, j, new_vector = 0;

	if (LOG_INTERRUPTS) logerror("update_interrupt_status: req=%02X stat=%02X serv=%02X\n", state->m_i186.intr.request, state->m_i186.intr.status, state->m_i186.intr.in_service);

	/* loop over priorities */
	for (i = 0; i <= state->m_i186.intr.priority_mask; i++)
	{
		/* note: by checking 4 bits, we also verify that the mask is off */
		if ((state->m_i186.intr.timer & 15) == i)
		{
			/* if we're already servicing something at this level, don't generate anything new */
			if (state->m_i186.intr.in_service & 0x01)
				return;

			/* if there's something pending, generate an interrupt */
			if (state->m_i186.intr.status & 0x07)
			{
				if (state->m_i186.intr.status & 1)
					new_vector = 0x08;
				else if (state->m_i186.intr.status & 2)
					new_vector = 0x12;
				else if (state->m_i186.intr.status & 4)
					new_vector = 0x13;
				else
					popmessage("Invalid timer interrupt!");

				/* set the clear mask and generate the int */
				state->m_i186.intr.ack_mask = 0x0001;
				goto generate_int;
			}
		}

		/* check DMA interrupts */
		for (j = 0; j < 2; j++)
			if ((state->m_i186.intr.dma[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (state->m_i186.intr.in_service & (0x04 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (state->m_i186.intr.request & (0x04 << j))
				{
					new_vector = 0x0a + j;

					/* set the clear mask and generate the int */
					state->m_i186.intr.ack_mask = 0x0004 << j;
					goto generate_int;
				}
			}

		/* check external interrupts */
		for (j = 0; j < 4; j++)
			if ((state->m_i186.intr.ext[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (state->m_i186.intr.in_service & (0x10 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (state->m_i186.intr.request & (0x10 << j))
				{
					/* otherwise, generate an interrupt for this request */
					new_vector = 0x0c + j;

					/* set the clear mask and generate the int */
					state->m_i186.intr.ack_mask = 0x0010 << j;
					goto generate_int;
				}
			}
	}
	return;

generate_int:
	/* generate the appropriate interrupt */
	state->m_i186.intr.poll_status = 0x8000 | new_vector;
	if (!state->m_i186.intr.pending)
		cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
	state->m_i186.intr.pending = 1;
	machine.scheduler().trigger(CPU_RESUME_TRIGGER);
	if (LOG_OPTIMIZATION) logerror("  - trigger due to interrupt pending\n");
	if (LOG_INTERRUPTS) logerror("(%f) **** Requesting interrupt vector %02X\n", machine.time().as_double(), new_vector);
}


static void handle_eoi(running_machine &machine,int data)
{
	compis_state *state = machine.driver_data<compis_state>();
	int i, j;

	/* specific case */
	if (!(data & 0x8000))
	{
		/* turn off the appropriate in-service bit */
		switch (data & 0x1f)
		{
			case 0x08:	state->m_i186.intr.in_service &= ~0x01;	break;
			case 0x12:	state->m_i186.intr.in_service &= ~0x01;	break;
			case 0x13:	state->m_i186.intr.in_service &= ~0x01;	break;
			case 0x0a:	state->m_i186.intr.in_service &= ~0x04;	break;
			case 0x0b:	state->m_i186.intr.in_service &= ~0x08;	break;
			case 0x0c:	state->m_i186.intr.in_service &= ~0x10;	break;
			case 0x0d:	state->m_i186.intr.in_service &= ~0x20;	break;
			case 0x0e:	state->m_i186.intr.in_service &= ~0x40;	break;
			case 0x0f:	state->m_i186.intr.in_service &= ~0x80;	break;
			default:	logerror("%05X:ERROR - 80186 EOI with unknown vector %02X\n", cpu_get_pc(machine.device("maincpu")), data & 0x1f);
		}
		if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for vector %02X\n", machine.time().as_double(), data & 0x1f);
	}

	/* non-specific case */
	else
	{
		/* loop over priorities */
		for (i = 0; i <= 7; i++)
		{
			/* check for in-service timers */
			if ((state->m_i186.intr.timer & 7) == i && (state->m_i186.intr.in_service & 0x01))
			{
				state->m_i186.intr.in_service &= ~0x01;
				if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for timer\n", machine.time().as_double());
				return;
			}

			/* check for in-service DMA interrupts */
			for (j = 0; j < 2; j++)
				if ((state->m_i186.intr.dma[j] & 7) == i && (state->m_i186.intr.in_service & (0x04 << j)))
				{
					state->m_i186.intr.in_service &= ~(0x04 << j);
					if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for DMA%d\n", machine.time().as_double(), j);
					return;
				}

			/* check external interrupts */
			for (j = 0; j < 4; j++)
				if ((state->m_i186.intr.ext[j] & 7) == i && (state->m_i186.intr.in_service & (0x10 << j)))
				{
					state->m_i186.intr.in_service &= ~(0x10 << j);
					if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for INT%d\n", machine.time().as_double(), j);
					return;
				}
		}
	}
}


/*************************************
 *
 *  80186 internal timers
 *
 *************************************/

static TIMER_CALLBACK(internal_timer_int)
{
	compis_state *state = machine.driver_data<compis_state>();
	int which = param;
	struct timer_state *t = &state->m_i186.timer[which];

	if (LOG_TIMER) logerror("Hit interrupt callback for timer %d\n", which);

	/* set the max count bit */
	t->control |= 0x0020;

	/* request an interrupt */
	if (t->control & 0x2000)
	{
		state->m_i186.intr.status |= 0x01 << which;
		update_interrupt_state(machine);
		if (LOG_TIMER) logerror("  Generating timer interrupt\n");
	}

	/* if we're continuous, reset */
	if (t->control & 0x0001)
	{
		int count = t->maxA ? t->maxA : 0x10000;
		t->int_timer->adjust(attotime::from_hz(2000000) * count, which);
		if (LOG_TIMER) logerror("  Repriming interrupt\n");
	}
	else
		t->int_timer->adjust(attotime::never, which);
}


static void internal_timer_sync(running_machine &machine, int which)
{
	compis_state *state = machine.driver_data<compis_state>();
	struct timer_state *t = &state->m_i186.timer[which];

	/* if we have a timing timer running, adjust the count */
	if (t->time_timer_active)
	{
		attotime current_time = t->time_timer->elapsed();
		int net_clocks = ((current_time - t->last_time) * 2000000).seconds;
		t->last_time = current_time;

		/* set the max count bit if we passed the max */
		if ((int)t->count + net_clocks >= t->maxA)
			t->control |= 0x0020;

		/* set the new count */
		if (t->maxA != 0)
			t->count = (t->count + net_clocks) % t->maxA;
		else
			t->count = t->count + net_clocks;
	}
}


static void internal_timer_update(running_machine &machine,
                                  int which,
                                  int new_count,
                                  int new_maxA,
                                  int new_maxB,
                                  int new_control)
{
	compis_state *state = machine.driver_data<compis_state>();
	struct timer_state *t = &state->m_i186.timer[which];
	int update_int_timer = 0;

	/* if we have a new count and we're on, update things */
	if (new_count != -1)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(machine, which);
			update_int_timer = 1;
		}
		t->count = new_count;
	}

	/* if we have a new max and we're on, update things */
	if (new_maxA != -1 && new_maxA != t->maxA)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(machine, which);
			update_int_timer = 1;
		}
		t->maxA = new_maxA;
		if (new_maxA == 0)
		{
        		new_maxA = 0x10000;
		}
	}

	/* if we have a new max and we're on, update things */
	if (new_maxB != -1 && new_maxB != t->maxB)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(machine, which);
			update_int_timer = 1;
		}

		t->maxB = new_maxB;

		if (new_maxB == 0)
		{
        		new_maxB = 0x10000;
		}
	}


	/* handle control changes */
	if (new_control != -1)
	{
		int diff;

		/* merge back in the bits we don't modify */
		new_control = (new_control & ~0x1fc0) | (t->control & 0x1fc0);

		/* handle the /INH bit */
		if (!(new_control & 0x4000))
			new_control = (new_control & ~0x8000) | (t->control & 0x8000);
		new_control &= ~0x4000;

		/* check for control bits we don't handle */
		diff = new_control ^ t->control;
		if (diff & 0x001c)
		  logerror("%05X:ERROR! -unsupported timer mode %04X\n",
			   cpu_get_pc(machine.device("maincpu")), new_control);

		/* if we have real changes, update things */
		if (diff != 0)
		{
			/* if we're going off, make sure our timers are gone */
			if ((diff & 0x8000) && !(new_control & 0x8000))
			{
				/* compute the final count */
				internal_timer_sync(machine, which);

				/* nuke the timer and force the interrupt timer to be recomputed */
				t->time_timer->adjust(attotime::never, which);
				t->time_timer_active = 0;
				update_int_timer = 1;
			}

			/* if we're going on, start the timers running */
			else if ((diff & 0x8000) && (new_control & 0x8000))
			{
				/* start the timing */
				t->time_timer->adjust(attotime::never, which);
				t->time_timer_active = 1;
				update_int_timer = 1;
			}

			/* if something about the interrupt timer changed, force an update */
			if (!(diff & 0x8000) && (diff & 0x2000))
			{
				internal_timer_sync(machine, which);
				update_int_timer = 1;
			}
		}

		/* set the new control register */
		t->control = new_control;
	}

	/* update the interrupt timer */
	if (update_int_timer)
	{
	    	if ((t->control & 0x8000) && (t->control & 0x2000))
	    	{
	        	int diff = t->maxA - t->count;
	        	if (diff <= 0)
	        		diff += 0x10000;
	        	t->int_timer->adjust(attotime::from_hz(2000000) * diff, which);
	        	if (LOG_TIMER) logerror("Set interrupt timer for %d\n", which);
	    	}
	    	else
	    	{
	        	t->int_timer->adjust(attotime::never, which);
	    	}
	}
}



/*************************************
 *
 *  80186 internal DMA
 *
 *************************************/

static TIMER_CALLBACK(dma_timer_callback)
{
	compis_state *state = machine.driver_data<compis_state>();
	int which = param;
	struct dma_state *d = &state->m_i186.dma[which];

	/* force an update and see if we're really done */
	//stream_update(dma_stream, 0);

	/* complete the status update */
	d->control &= ~0x0002;
	d->source += d->count;
	d->count = 0;

	/* check for interrupt generation */
	if (d->control & 0x0100)
	{
		if (LOG_DMA) logerror("DMA%d timer callback - requesting interrupt: count = %04X, source = %04X\n", which, d->count, d->source);
		state->m_i186.intr.request |= 0x04 << which;
		update_interrupt_state(machine);
	}
}


static void update_dma_control(running_machine &machine, int which, int new_control)
{
	compis_state *state = machine.driver_data<compis_state>();
	struct dma_state *d = &state->m_i186.dma[which];
	int diff;

	/* handle the CHG bit */
	if (!(new_control & 0x0004))
	  new_control = (new_control & ~0x0002) | (d->control & 0x0002);
	new_control &= ~0x0004;

	/* check for control bits we don't handle */
	diff = new_control ^ d->control;
	if (diff & 0x6811)
	  logerror("%05X:ERROR! - unsupported DMA mode %04X\n",
		   cpu_get_pc(machine.device("maincpu")), new_control);

	/* if we're going live, set a timer */
	if ((diff & 0x0002) && (new_control & 0x0002))
	{
		/* make sure the parameters meet our expectations */
		if ((new_control & 0xfe00) != 0x1600)
		{
			logerror("Unexpected DMA control %02X\n", new_control);
		}
		else if (/*!is_redline &&*/ ((d->dest & 1) || (d->dest & 0x3f) > 0x0b))
		{
			logerror("Unexpected DMA destination %02X\n", d->dest);
		}
		else if (/*is_redline && */ (d->dest & 0xf000) != 0x4000 && (d->dest & 0xf000) != 0x5000)
		{
			logerror("Unexpected DMA destination %02X\n", d->dest);
		}

		/* otherwise, set a timer */
		else
		{
//          int count = d->count;

			/* adjust for redline racer */
        	// int dacnum = (d->dest & 0x3f) / 2;

			if (LOG_DMA) logerror("Initiated DMA %d - count = %04X, source = %04X, dest = %04X\n", which, d->count, d->source, d->dest);

			d->finished = 0;
/*          d->finish_timer->adjust(
         attotime::from_hz(dac[dacnum].frequency) * (double)count, which);*/
		}
	}

	/* set the new control register */
	d->control = new_control;
}



/*************************************
 *
 *  80186 internal I/O reads
 *
 *************************************/


READ16_HANDLER( compis_i186_internal_port_r )
{
	compis_state *state = space->machine().driver_data<compis_state>();
	int temp, which;

	switch (offset)
	{
		case 0x11:
			logerror("%05X:ERROR - read from 80186 EOI\n", cpu_get_pc(&space->device()));
			break;

		case 0x12:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt poll\n", cpu_get_pc(&space->device()));
			if (state->m_i186.intr.poll_status & 0x8000)
				int_callback(space->machine().device("maincpu"), 0);
			return state->m_i186.intr.poll_status;

		case 0x13:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt poll status\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.poll_status;

		case 0x14:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt mask\n", cpu_get_pc(&space->device()));
			temp  = (state->m_i186.intr.timer  >> 3) & 0x01;
			temp |= (state->m_i186.intr.dma[0] >> 1) & 0x04;
			temp |= (state->m_i186.intr.dma[1] >> 0) & 0x08;
			temp |= (state->m_i186.intr.ext[0] << 1) & 0x10;
			temp |= (state->m_i186.intr.ext[1] << 2) & 0x20;
			temp |= (state->m_i186.intr.ext[2] << 3) & 0x40;
			temp |= (state->m_i186.intr.ext[3] << 4) & 0x80;
			return temp;

		case 0x15:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt priority mask\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.priority_mask;

		case 0x16:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt in-service\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.in_service;

		case 0x17:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt request\n", cpu_get_pc(&space->device()));
			temp = state->m_i186.intr.request & ~0x0001;
			if (state->m_i186.intr.status & 0x0007)
				temp |= 1;
			return temp;

		case 0x18:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt status\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.status;

		case 0x19:
			if (LOG_PORTS) logerror("%05X:read 80186 timer interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.timer;

		case 0x1a:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA 0 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.dma[0];

		case 0x1b:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA 1 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.dma[1];

		case 0x1c:
			if (LOG_PORTS) logerror("%05X:read 80186 INT 0 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.ext[0];

		case 0x1d:
			if (LOG_PORTS) logerror("%05X:read 80186 INT 1 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.ext[1];

		case 0x1e:
			if (LOG_PORTS) logerror("%05X:read 80186 INT 2 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.ext[2];

		case 0x1f:
			if (LOG_PORTS) logerror("%05X:read 80186 INT 3 interrupt control\n", cpu_get_pc(&space->device()));
			return state->m_i186.intr.ext[3];

		case 0x28:
		case 0x2c:
		case 0x30:
			if (LOG_PORTS) logerror("%05X:read 80186 Timer %d count\n", cpu_get_pc(&space->device()), (offset - 0x28) / 4);
			which = (offset - 0x28) / 4;
			if (!(offset & 1))
				internal_timer_sync(space->machine(), which);
			return state->m_i186.timer[which].count;

		case 0x29:
		case 0x2d:
		case 0x31:
			if (LOG_PORTS) logerror("%05X:read 80186 Timer %d max A\n", cpu_get_pc(&space->device()), (offset - 0x29) / 4);
			which = (offset - 0x29) / 4;
			return state->m_i186.timer[which].maxA;

		case 0x2a:
		case 0x2e:
			logerror("%05X:read 80186 Timer %d max B\n", cpu_get_pc(&space->device()), (offset - 0x2a) / 4);
			which = (offset - 0x2a) / 4;
			return state->m_i186.timer[which].maxB;

		case 0x2b:
		case 0x2f:
		case 0x33:
			if (LOG_PORTS) logerror("%05X:read 80186 Timer %d control\n", cpu_get_pc(&space->device()), (offset - 0x2b) / 4);
			which = (offset - 0x2b) / 4;
			return state->m_i186.timer[which].control;

		case 0x50:
			if (LOG_PORTS) logerror("%05X:read 80186 upper chip select\n", cpu_get_pc(&space->device()));
			return state->m_i186.mem.upper;

		case 0x51:
			if (LOG_PORTS) logerror("%05X:read 80186 lower chip select\n", cpu_get_pc(&space->device()));
			return state->m_i186.mem.lower;

		case 0x52:
			if (LOG_PORTS) logerror("%05X:read 80186 peripheral chip select\n", cpu_get_pc(&space->device()));
			return state->m_i186.mem.peripheral;

		case 0x53:
			if (LOG_PORTS) logerror("%05X:read 80186 middle chip select\n", cpu_get_pc(&space->device()));
			return state->m_i186.mem.middle;

		case 0x54:
			if (LOG_PORTS) logerror("%05X:read 80186 middle P chip select\n", cpu_get_pc(&space->device()));
			return state->m_i186.mem.middle_size;

		case 0x60:
		case 0x68:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d lower source address\n", cpu_get_pc(&space->device()), (offset - 0x60) / 8);
			which = (offset - 0x60) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].source;

		case 0x61:
		case 0x69:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d upper source address\n", cpu_get_pc(&space->device()), (offset - 0x61) / 8);
			which = (offset - 0x61) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].source >> 16;

		case 0x62:
		case 0x6a:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d lower dest address\n", cpu_get_pc(&space->device()), (offset - 0x62) / 8);
			which = (offset - 0x62) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].dest;

		case 0x63:
		case 0x6b:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d upper dest address\n", cpu_get_pc(&space->device()), (offset - 0x63) / 8);
			which = (offset - 0x63) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].dest >> 16;

		case 0x64:
		case 0x6c:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d transfer count\n", cpu_get_pc(&space->device()), (offset - 0x64) / 8);
			which = (offset - 0x64) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].count;

		case 0x65:
		case 0x6d:
			if (LOG_PORTS) logerror("%05X:read 80186 DMA%d control\n", cpu_get_pc(&space->device()), (offset - 0x65) / 8);
			which = (offset - 0x65) / 8;
//          stream_update(dma_stream, 0);
			return state->m_i186.dma[which].control;

		default:
			logerror("%05X:read 80186 port %02X\n", cpu_get_pc(&space->device()), offset);
			break;
	}
	return 0x00;
}

/*************************************
 *
 *  80186 internal I/O writes
 *
 *************************************/

WRITE16_HANDLER( compis_i186_internal_port_w )
{
	compis_state *state = space->machine().driver_data<compis_state>();
	int temp, which, data16 = data;

	switch (offset)
	{
		case 0x11:
			if (LOG_PORTS) logerror("%05X:80186 EOI = %04X\n", cpu_get_pc(&space->device()), data16);
			handle_eoi(space->machine(),0x8000);
			update_interrupt_state(space->machine());
			break;

		case 0x12:
			logerror("%05X:ERROR - write to 80186 interrupt poll = %04X\n", cpu_get_pc(&space->device()), data16);
			break;

		case 0x13:
			logerror("%05X:ERROR - write to 80186 interrupt poll status = %04X\n", cpu_get_pc(&space->device()), data16);
			break;

		case 0x14:
			if (LOG_PORTS) logerror("%05X:80186 interrupt mask = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.timer  = (state->m_i186.intr.timer  & ~0x08) | ((data16 << 3) & 0x08);
			state->m_i186.intr.dma[0] = (state->m_i186.intr.dma[0] & ~0x08) | ((data16 << 1) & 0x08);
			state->m_i186.intr.dma[1] = (state->m_i186.intr.dma[1] & ~0x08) | ((data16 << 0) & 0x08);
			state->m_i186.intr.ext[0] = (state->m_i186.intr.ext[0] & ~0x08) | ((data16 >> 1) & 0x08);
			state->m_i186.intr.ext[1] = (state->m_i186.intr.ext[1] & ~0x08) | ((data16 >> 2) & 0x08);
			state->m_i186.intr.ext[2] = (state->m_i186.intr.ext[2] & ~0x08) | ((data16 >> 3) & 0x08);
			state->m_i186.intr.ext[3] = (state->m_i186.intr.ext[3] & ~0x08) | ((data16 >> 4) & 0x08);
			update_interrupt_state(space->machine());
			break;

		case 0x15:
			if (LOG_PORTS) logerror("%05X:80186 interrupt priority mask = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.priority_mask = data16 & 0x0007;
			update_interrupt_state(space->machine());
			break;

		case 0x16:
			if (LOG_PORTS) logerror("%05X:80186 interrupt in-service = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.in_service = data16 & 0x00ff;
			update_interrupt_state(space->machine());
			break;

		case 0x17:
			if (LOG_PORTS) logerror("%05X:80186 interrupt request = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.request = (state->m_i186.intr.request & ~0x00c0) | (data16 & 0x00c0);
			update_interrupt_state(space->machine());
			break;

		case 0x18:
			if (LOG_PORTS) logerror("%05X:WARNING - wrote to 80186 interrupt status = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.status = (state->m_i186.intr.status & ~0x8007) | (data16 & 0x8007);
			update_interrupt_state(space->machine());
			break;

		case 0x19:
			if (LOG_PORTS) logerror("%05X:80186 timer interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.timer = data16 & 0x000f;
			break;

		case 0x1a:
			if (LOG_PORTS) logerror("%05X:80186 DMA 0 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.dma[0] = data16 & 0x000f;
			break;

		case 0x1b:
			if (LOG_PORTS) logerror("%05X:80186 DMA 1 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.dma[1] = data16 & 0x000f;
			break;

		case 0x1c:
			if (LOG_PORTS) logerror("%05X:80186 INT 0 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.ext[0] = data16 & 0x007f;
			break;

		case 0x1d:
			if (LOG_PORTS) logerror("%05X:80186 INT 1 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.ext[1] = data16 & 0x007f;
			break;

		case 0x1e:
			if (LOG_PORTS) logerror("%05X:80186 INT 2 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.ext[2] = data16 & 0x001f;
			break;

		case 0x1f:
			if (LOG_PORTS) logerror("%05X:80186 INT 3 interrupt control = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.intr.ext[3] = data16 & 0x001f;
			break;

		case 0x28:
		case 0x2c:
		case 0x30:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d count = %04X\n", cpu_get_pc(&space->device()), (offset - 0x28) / 4, data16);
			which = (offset - 0x28) / 4;
			internal_timer_update(space->machine(),which, data16, -1, -1, -1);
			break;

		case 0x29:
		case 0x2d:
		case 0x31:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d max A = %04X\n", cpu_get_pc(&space->device()), (offset - 0x29) / 4, data16);
			which = (offset - 0x29) / 4;
			internal_timer_update(space->machine(),which, -1, data16, -1, -1);
			break;

		case 0x2a:
		case 0x2e:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d max B = %04X\n", cpu_get_pc(&space->device()), (offset - 0x2a) / 4, data16);
			which = (offset - 0x2a) / 4;
			internal_timer_update(space->machine(),which, -1, -1, data16, -1);
			break;

		case 0x2b:
		case 0x2f:
		case 0x33:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d control = %04X\n", cpu_get_pc(&space->device()), (offset - 0x2b) / 4, data16);
			which = (offset - 0x2b) / 4;
			internal_timer_update(space->machine(),which, -1, -1, -1, data16);
			break;

		case 0x50:
			if (LOG_PORTS) logerror("%05X:80186 upper chip select = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.mem.upper = data16 | 0xc038;
			break;

		case 0x51:
			if (LOG_PORTS) logerror("%05X:80186 lower chip select = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.mem.lower = (data16 & 0x3fff) | 0x0038; //printf("%X\n",state->m_i186.mem.lower);
			break;

		case 0x52:
			if (LOG_PORTS) logerror("%05X:80186 peripheral chip select = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.mem.peripheral = data16 | 0x0038;
			break;

		case 0x53:
			if (LOG_PORTS) logerror("%05X:80186 middle chip select = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.mem.middle = data16 | 0x01f8;
			break;

		case 0x54:
			if (LOG_PORTS) logerror("%05X:80186 middle P chip select = %04X\n", cpu_get_pc(&space->device()), data16);
			state->m_i186.mem.middle_size = data16 | 0x8038;

			temp = (state->m_i186.mem.peripheral & 0xffc0) << 4;
			if (state->m_i186.mem.middle_size & 0x0040)
			{
//              install_mem_read_handler(2, temp, temp + 0x2ff, peripheral_r);
//              install_mem_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}
			else
			{
				temp &= 0xffff;
//              install_port_read_handler(2, temp, temp + 0x2ff, peripheral_r);
//              install_port_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}

			/* we need to do this at a time when the I86 context is swapped in */
			/* this register is generally set once at startup and never again, so it's a good */
			/* time to set it up */
			device_set_irq_callback(&space->device(), int_callback);
			break;

		case 0x60:
		case 0x68:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d lower source address = %04X\n", cpu_get_pc(&space->device()), (offset - 0x60) / 8, data16);
			which = (offset - 0x60) / 8;
//          stream_update(dma_stream, 0);
			state->m_i186.dma[which].source = (state->m_i186.dma[which].source & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0x61:
		case 0x69:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d upper source address = %04X\n", cpu_get_pc(&space->device()), (offset - 0x61) / 8, data16);
			which = (offset - 0x61) / 8;
//          stream_update(dma_stream, 0);
			state->m_i186.dma[which].source = (state->m_i186.dma[which].source & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0x62:
		case 0x6a:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d lower dest address = %04X\n", cpu_get_pc(&space->device()), (offset - 0x62) / 8, data16);
			which = (offset - 0x62) / 8;
//          stream_update(dma_stream, 0);
			state->m_i186.dma[which].dest = (state->m_i186.dma[which].dest & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0x63:
		case 0x6b:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d upper dest address = %04X\n", cpu_get_pc(&space->device()), (offset - 0x63) / 8, data16);
			which = (offset - 0x63) / 8;
//          stream_update(dma_stream, 0);
			state->m_i186.dma[which].dest = (state->m_i186.dma[which].dest & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0x64:
		case 0x6c:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d transfer count = %04X\n", cpu_get_pc(&space->device()), (offset - 0x64) / 8, data16);
			which = (offset - 0x64) / 8;
//          stream_update(dma_stream, 0);
			state->m_i186.dma[which].count = data16;
			break;

		case 0x65:
		case 0x6d:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d control = %04X\n", cpu_get_pc(&space->device()), (offset - 0x65) / 8, data16);
			which = (offset - 0x65) / 8;
//          stream_update(dma_stream, 0);
			update_dma_control(space->machine(), which, data16);
			break;

		case 0x7f:
			if (LOG_PORTS) logerror("%05X:80186 relocation register = %04X\n", cpu_get_pc(&space->device()), data16);

			/* we assume here there that this doesn't happen too often */
			/* plus, we can't really remove the old memory range, so we also assume that it's */
			/* okay to leave us mapped where we were */
			temp = (data16 & 0x0fff) << 8;
			if (data16 & 0x1000)
			{
				space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_legacy_read_handler(temp, temp + 0xff, FUNC(compis_i186_internal_port_r));
				space->machine().device("maincpu")->memory().space(AS_PROGRAM)->install_legacy_write_handler(temp, temp + 0xff, FUNC(compis_i186_internal_port_w));
			}
			else
			{
				temp &= 0xffff;
				space->machine().device("maincpu")->memory().space(AS_IO)->install_legacy_read_handler(temp, temp + 0xff, FUNC(compis_i186_internal_port_r));
				space->machine().device("maincpu")->memory().space(AS_IO)->install_legacy_write_handler(temp, temp + 0xff, FUNC(compis_i186_internal_port_w));
			}
/*          popmessage("Sound CPU reset");*/
			break;

		default:
			logerror("%05X:80186 port %02X = %04X\n", cpu_get_pc(&space->device()), offset, data16);
			break;
	}
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: CPU - Initialize the 80186 CPU                                    */
/*-------------------------------------------------------------------------*/
static void compis_cpu_init(running_machine &machine)
{
	compis_state *state = machine.driver_data<compis_state>();
	/* create timers here so they stick around */
	state->m_i186.timer[0].int_timer = machine.scheduler().timer_alloc(FUNC(internal_timer_int));
	state->m_i186.timer[1].int_timer = machine.scheduler().timer_alloc(FUNC(internal_timer_int));
	state->m_i186.timer[2].int_timer = machine.scheduler().timer_alloc(FUNC(internal_timer_int));
	state->m_i186.timer[0].time_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_i186.timer[1].time_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_i186.timer[2].time_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_i186.dma[0].finish_timer = machine.scheduler().timer_alloc(FUNC(dma_timer_callback));
	state->m_i186.dma[1].finish_timer = machine.scheduler().timer_alloc(FUNC(dma_timer_callback));
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Driver - Init                                                     */
/*-------------------------------------------------------------------------*/

/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( compis_pic8259_master_set_int_line )
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( compis_pic8259_slave_set_int_line )
{
	compis_state *drvstate = device->machine().driver_data<compis_state>();
	if (drvstate->m_devices.pic8259_master)
		pic8259_ir2_w(drvstate->m_devices.pic8259_master, state);
}

const struct pic8259_interface compis_pic8259_master_config =
{
	DEVCB_LINE(compis_pic8259_master_set_int_line)
};

const struct pic8259_interface compis_pic8259_slave_config =
{
	DEVCB_LINE(compis_pic8259_slave_set_int_line)
};


static IRQ_CALLBACK( compis_irq_callback )
{
	compis_state *state = device->machine().driver_data<compis_state>();
	return pic8259_acknowledge(state->m_devices.pic8259_master);
}

#if 0
static const compis_gdc_interface i82720_interface =
{
	GDC_MODE_HRG,
	0x8000
};
#endif


DRIVER_INIT( compis )
{
	compis_state *state = machine.driver_data<compis_state>();
//	compis_init( &i82720_interface );

	device_set_irq_callback(machine.device("maincpu"), compis_irq_callback);
	memset (&state->m_compis, 0, sizeof (state->m_compis) );
}

MACHINE_START( compis )
{
	/* CPU */
	compis_cpu_init(machine);
}
/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Machine - Init                                                    */
/*-------------------------------------------------------------------------*/
MACHINE_RESET( compis )
{
	compis_state *state = machine.driver_data<compis_state>();
	/* FDC */
	compis_fdc_reset(machine);

	/* Keyboard */
	compis_keyb_init(state);

	/* OSP PIC 8259 */
	device_set_irq_callback(machine.device("maincpu"), compis_irq_callback);

	state->m_devices.pic8259_master = machine.device("pic8259_master");
	state->m_devices.pic8259_slave = machine.device("pic8259_slave");
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Interrupt - Vertical Blanking Interrupt                           */
/*-------------------------------------------------------------------------*/
INTERRUPT_GEN( compis_vblank_int )
{
//  compis_gdc_vblank_int();
	compis_keyb_update(device->machine());
}
