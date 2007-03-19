/******************************************************************************

	compis.c
	machine driver

	Per Ola Ingvarsson
	Tomas Karlsson
			
 ******************************************************************************/

/*-------------------------------------------------------------------------*/
/* Include files                                                           */
/*-------------------------------------------------------------------------*/

#include "driver.h"
#include "cpu/i86/i186intf.h"
#include "video/generic.h"
#include "machine/8255ppi.h"
#include "machine/mm58274c.h"
#include "machine/pic8259.h"
#include "includes/compis.h"
#include "machine/pit8253.h"
#include "machine/nec765.h"
#include "includes/msm8251.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"

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

struct mem_state
{
	UINT16	lower;
	UINT16	upper;
	UINT16	middle;
	UINT16	middle_size;
	UINT16	peripheral;
};

struct timer_state
{
	UINT16	control;
	UINT16	maxA;
	UINT16	maxB;
	UINT16	count;
	mame_timer *	int_timer;
	mame_timer *	time_timer;
	UINT8	time_timer_active;
	double	last_time;
};

struct dma_state
{
	UINT32	source;
	UINT32	dest;
	UINT16	count;
	UINT16	control;
	UINT8	finished;
	mame_timer *	finish_timer;
};

struct intr_state
{
	UINT8	pending;
	UINT16	ack_mask;
	UINT16	priority_mask;
	UINT16	in_service;
	UINT16	request;
	UINT16	status;
	UINT16	poll_status;
	UINT16	timer;
	UINT16	dma[2];
	UINT16	ext[4];
};

static struct i186_state
{
	struct timer_state	timer[3];
	struct dma_state	dma[2];
	struct intr_state	intr;
	struct mem_state	mem;
} i186;

/* Keyboard */
const UINT8 compis_keyb_codes[6][16] = {
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
typedef struct
{
	UINT8 nationality;   /* Character set, keyboard layout (Swedish) */
	UINT8 release_time;  /* Autorepeat release time (0.8)	*/
	UINT8 speed;	     /* Transmission speed (14)		*/
	UINT8 roll_over;     /* Key roll-over (MKEY)		*/
	UINT8 click;	     /* Key click (NO)			*/
	UINT8 break_nmi;     /* Keyboard break (NMI)		*/
	UINT8 beep_freq;     /* Beep frequency (low)		*/
	UINT8 beep_dura;     /* Beep duration (short)		*/
	UINT8 password[8];   /* Password			*/
	UINT8 owner[16];     /* Owner				*/
	UINT8 network_id;    /* Network workstation number (1)	*/
	UINT8 boot_order[4]; /* Boot device order (FD HD NW PD)	*/
	UINT8 key_code;
	UINT8 key_status;
} TYP_COMPIS_KEYBOARD;

/* USART 8251 */
typedef struct
{
	UINT8 status;
	UINT8 bytes_sent;
} TYP_COMPIS_USART;

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

/* Printer */
typedef struct
{
	UINT8 data;
	UINT8 strobe;
} TYP_COMPIS_PRINTER;


/* Main emulation */
typedef struct
{
	TYP_COMPIS_PRINTER	printer;	/* Printer */
	TYP_COMPIS_USART	usart;		/* USART 8251 */
	TYP_COMPIS_KEYBOARD	keyboard;	/* Keyboard  */
} TYP_COMPIS;

static TYP_COMPIS compis;


/*-------------------------------------------------------------------------*/
/* Name: compis_irq_set                                                    */
/* Desc: IRQ - Issue an interrupt request                                  */
/*-------------------------------------------------------------------------*/
void compis_irq_set(UINT8 irq)
{
	cpunum_set_input_line_vector(0, 0, irq);
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

/*-------------------------------------------------------------------------*/
/*  OSP PIC 8259                                                           */
/*-------------------------------------------------------------------------*/

void compis_osp_pic_irq(UINT8 irq)
{
	pic8259_set_irq_line(0, irq, 1);
	pic8259_set_irq_line(0, irq, 0);
}

 READ8_HANDLER ( compis_osp_pic_r )
{
	return pic8259_0_r (offset >> 1);
}

WRITE8_HANDLER ( compis_osp_pic_w )
{
	pic8259_0_w (offset >> 1, data);
}

/*-------------------------------------------------------------------------*/
/*  Keyboard                                                               */
/*-------------------------------------------------------------------------*/
void compis_keyb_update(void)
{
	UINT8 key_code;
	UINT8 key_status;
	UINT8 irow;
	UINT8 icol;
	UINT16 data;
	UINT16 ibit;
	
	key_code = 0;
	key_status = 0x80;

	for (irow = 0; irow < 6; irow++)
	{
		data = readinputport(irow);
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
		compis.keyboard.key_code = key_code;
		compis.keyboard.key_status = key_status;
		compis.usart.status |= COMPIS_USART_STATUS_RX_READY;
		compis.usart.bytes_sent = 0;
//		compis_osp_pic_irq(COMPIS_IRQ_8251_RXRDY);
	}
}

void compis_keyb_init(void)
{
	compis.keyboard.key_code = 0;
	compis.keyboard.key_status = 0x80;
	compis.usart.status = 0;
	compis.usart.bytes_sent = 0;
}

/*-------------------------------------------------------------------------*/
/*  FDC iSBX-218A                                                          */
/*-------------------------------------------------------------------------*/
static void compis_fdc_reset(void)
{
	nec765_reset(0);

	/* set FDC at reset */
	nec765_set_reset_state(1);
}

void compis_fdc_tc(int state)
{
	/* Terminal count if iSBX-218A has DMA enabled */
  	if (readinputport(7))
	{
		nec765_set_tc_state(state);
	}
}

void compis_fdc_int(int state)
{
	/* No interrupt requests if iSBX-218A has DMA enabled */
  	if (!readinputport(7) && state)
	{
		compis_osp_pic_irq(COMPIS_IRQ_SBX0_INT1);
	}
}

static void compis_fdc_dma_drq(int state, int read)
{
	/* DMA requst if iSBX-218A has DMA enabled */
  	if (readinputport(7) && state)
	{
		//compis_dma_drq(state, read);
	}
}

static nec765_interface compis_fdc_interface = 
{
	compis_fdc_int,
	compis_fdc_dma_drq
};

 READ8_HANDLER (compis_fdc_dack_r)
{
	UINT16 data;
	data = 0xffff;
	/* DMA acknowledge if iSBX-218A has DMA enabled */
  	if (readinputport(7))
  	{
		data = nec765_dack_r(0);
	}
	
	return data;
}

WRITE8_HANDLER (compis_fdc_w)
{
	switch(offset)
	{
		case 2:
			nec765_data_w(0, data);
			break;
		default:
			logerror("FDC Unknown Port Write %04X = %04X\n", offset, data);
			break;
	}
}

 READ8_HANDLER (compis_fdc_r)
{
	UINT16 data;
	data = 0xffff;
	switch(offset)
	{
		case 0:
			data = nec765_status_r(0);
			break;
		case 2:
			data = nec765_data_r(0);
			break;
		default:
			logerror("FDC Unknown Port Read %04X\n", offset);
			break;
	}

	return data;
}



/*-------------------------------------------------------------------------*/
/*  PPI 8255                                                               */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* Bit 0: J7-2 Centronics D0           		                           */
/* Bit 1: J7-3 Centronics D1           		                           */
/* Bit 2: J7-4 Centronics D2           		                           */
/* Bit 3: J7-5 Centronics D3          		                           */
/* Bit 4: J7-6 Centronics D4          		                           */
/* Bit 5: J7-7 Centronics D5           		                           */
/* Bit 6: J7-8 Centronics D6           		                           */
/* Bit 7: J7-9 Centronics D7          		                           */
/*-------------------------------------------------------------------------*/
static WRITE8_HANDLER ( compis_ppi_port_a_w )
{
	compis.printer.data = data;
}
/*-------------------------------------------------------------------------*/
/* Bit 0: J5-4                                                             */
/* Bit 1: J5-5          		                                   */
/* Bit 2: J6-3 Cassette read                                               */
/* Bit 3: J2-6 DSR / S8-4 Test                                             */
/* Bit 4: J4-6 DSR / S8-3 Test                                             */
/* Bit 5: J7-11 Centronics BUSY               	                           */
/* Bit 6: J7-13 Centronics SELECT			                   */
/* Bit 7: Tmr0			      	                                   */
/*-------------------------------------------------------------------------*/
static  READ8_HANDLER ( compis_ppi_port_b_r )
{
	UINT8 data;

	/* DIP switch - Test mode */
	data = readinputport(6);

	/* Centronics busy */
	if (!printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0))
		data |= 0x20;

	return 	data;
}
/*-------------------------------------------------------------------------*/
/* Bit 0: J5-1                       		                           */
/* Bit 1: J5-2                         		                           */
/* Bit 2: Select: 1=time measure, DSR from J2/J4 pin 6. 0=read cassette	   */
/* Bit 3: Datex: Tristate datex output (low)				   */
/* Bit 4: V2-5 Floppy motor on/off    		                           */
/* Bit 5: J7-1 Centronics STROBE               		                   */
/* Bit 6: V2-4 Floppy Soft reset   			                   */
/* Bit 7: V2-3 Floppy Terminal count      	                           */
/*-------------------------------------------------------------------------*/
static WRITE8_HANDLER ( compis_ppi_port_c_w )
{
	/* Centronics Strobe */
	if ((compis.printer.strobe) && !(data & 0x20))
		printer_output(image_from_devtype_and_index(IO_PRINTER, 0), compis.printer.data);
	compis.printer.strobe = ((data & 0x20)?1:0);

	/* FDC Reset */
	if (data & 0x40)
		compis_fdc_reset();
  
	/* FDC Terminal count */
	compis_fdc_tc((data & 0x80)?1:0);
}

static ppi8255_interface compis_ppi_interface =
{
    1,
    {NULL},
    {compis_ppi_port_b_r},
    {NULL},
    {compis_ppi_port_a_w},
    {NULL},
    {compis_ppi_port_c_w}
};

 READ8_HANDLER ( compis_ppi_r )
{
	return ppi8255_0_r (offset >> 1);
}

WRITE8_HANDLER ( compis_ppi_w )
{
	ppi8255_0_w (offset >> 1, data);
}

/*-------------------------------------------------------------------------*/
/*  PIT 8253                                                               */
/*-------------------------------------------------------------------------*/

static struct pit8253_config compis_pit_config[2] =
{
{
	TYPE8253,
	{
		/* Timer0 */
		{4770000/4, NULL, NULL },
		/* Timer1 */
		{4770000/4, NULL, NULL },
		/* Timer2 */
		{4770000/4, NULL, NULL }
	}
},
{
	TYPE8254,
	{
		/* Timer0 */
		{4770000/4, NULL, NULL },
		/* Timer1 */
		{4770000/4, NULL, NULL },
		/* Timer2 */
		{4770000/4, NULL, NULL }
	}
}
};

 READ8_HANDLER ( compis_pit_r )
{
	return pit8253_0_r (offset >> 1);
}

WRITE8_HANDLER ( compis_pit_w )
{
	pit8253_0_w (offset >> 1 , data);
}

/*-------------------------------------------------------------------------*/
/*  OSP PIT 8254                                                           */
/*-------------------------------------------------------------------------*/

READ8_HANDLER ( compis_osp_pit_r )
{
	return pit8253_1_r (offset >> 1);
}

WRITE8_HANDLER ( compis_osp_pit_w )
{
	pit8253_1_w (offset >> 1, data);
}

/*-------------------------------------------------------------------------*/
/*  RTC 58174                                                              */
/*-------------------------------------------------------------------------*/
 READ8_HANDLER ( compis_rtc_r )
{
	return mm58274c_r(0, offset >> 1);
}

WRITE8_HANDLER ( compis_rtc_w )
{
	mm58274c_w(0, offset >> 1, data);
}

/*-------------------------------------------------------------------------*/
/*  USART 8251                                                             */
/*-------------------------------------------------------------------------*/
void compis_usart_rxready(int state)
{
/*
	if (state)
		compis_pic_irq(COMPIS_IRQ_8251_RXRDY);
*/
}

static struct msm8251_interface compis_usart_interface=
{
	NULL,
	NULL,
	compis_usart_rxready
};

 READ8_HANDLER ( compis_usart_r )
{
	UINT8 data = 0xff;

   return msm8251_data_r ( offset >> 1 );
   
	switch (offset)
	{
		case 0x00:
			if (compis.usart.status & COMPIS_USART_STATUS_RX_READY)
			{
				switch (compis.usart.bytes_sent)
				{
					case 0:
						data = compis.keyboard.key_code;
						compis.usart.bytes_sent = 1;
						break;
						
					case 1:
						data = compis.keyboard.key_status;
						compis.usart.bytes_sent = 0;
						compis.usart.status &= ~COMPIS_USART_STATUS_RX_READY;
						break;
				}
			}	
			break;					

		case 0x02:
			data = compis.usart.status;
         logerror("%04X: USART STATUS  Port Read %04X\n", activecpu_get_pc(),
                  offset);
			break;

		default:
			logerror("%04X: USART Unknown Port Read %04X\n", activecpu_get_pc(),
                  offset);
			break;
	}

	return data;
}

WRITE8_HANDLER ( compis_usart_w )
{
	switch (offset)
	{
		case 0x00:
			msm8251_data_w (0,data);
			break;
		case 0x02:
			msm8251_control_w (0,data);
			break;
		default:
			logerror("USART Unknown Port Write %04X = %04X\n", offset, data);
			break;
	}
}

/*************************************
 *
 *	80186 interrupt controller
 *
 *************************************/
static int int_callback(int line)
{
	if (LOG_INTERRUPTS)
      		logerror("(%f) **** Acknowledged interrupt vector %02X\n", timer_get_time(), i186.intr.poll_status & 0x1f);

	/* clear the interrupt */
	activecpu_set_input_line(0, CLEAR_LINE);
	i186.intr.pending = 0;

	/* clear the request and set the in-service bit */
#if LATCH_INTS
	i186.intr.request &= ~i186.intr.ack_mask;
#else
	i186.intr.request &= ~(i186.intr.ack_mask & 0x0f);
#endif
	i186.intr.in_service |= i186.intr.ack_mask;
	if (i186.intr.ack_mask == 0x0001)
	{
		switch (i186.intr.poll_status & 0x1f)
		{
			case 0x08:	i186.intr.status &= ~0x01;	break;
			case 0x12:	i186.intr.status &= ~0x02;	break;
			case 0x13:	i186.intr.status &= ~0x04;	break;
		}
	}
	i186.intr.ack_mask = 0;

	/* a request no longer pending */
	i186.intr.poll_status &= ~0x8000;

	/* return the vector */
	return i186.intr.poll_status & 0x1f;
}


static void update_interrupt_state(void)
{
	int i, j, new_vector = 0;

	if (LOG_INTERRUPTS) logerror("update_interrupt_status: req=%02X stat=%02X serv=%02X\n", i186.intr.request, i186.intr.status, i186.intr.in_service);

	/* loop over priorities */
	for (i = 0; i <= i186.intr.priority_mask; i++)
	{
		/* note: by checking 4 bits, we also verify that the mask is off */
		if ((i186.intr.timer & 15) == i)
		{
			/* if we're already servicing something at this level, don't generate anything new */
			if (i186.intr.in_service & 0x01)
				return;

			/* if there's something pending, generate an interrupt */
			if (i186.intr.status & 0x07)
			{
				if (i186.intr.status & 1)
					new_vector = 0x08;
				else if (i186.intr.status & 2)
					new_vector = 0x12;
				else if (i186.intr.status & 4)
					new_vector = 0x13;
				else
					popmessage("Invalid timer interrupt!");

				/* set the clear mask and generate the int */
				i186.intr.ack_mask = 0x0001;
				goto generate_int;
			}
		}

		/* check DMA interrupts */
		for (j = 0; j < 2; j++)
			if ((i186.intr.dma[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (i186.intr.in_service & (0x04 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (i186.intr.request & (0x04 << j))
				{
					new_vector = 0x0a + j;

					/* set the clear mask and generate the int */
					i186.intr.ack_mask = 0x0004 << j;
					goto generate_int;
				}
			}

		/* check external interrupts */
		for (j = 0; j < 4; j++)
			if ((i186.intr.ext[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (i186.intr.in_service & (0x10 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (i186.intr.request & (0x10 << j))
				{
					/* otherwise, generate an interrupt for this request */
					new_vector = 0x0c + j;

					/* set the clear mask and generate the int */
					i186.intr.ack_mask = 0x0010 << j;
					goto generate_int;
				}
			}
	}
	return;

generate_int:
	/* generate the appropriate interrupt */
	i186.intr.poll_status = 0x8000 | new_vector;
	if (!i186.intr.pending)
		cpunum_set_input_line(2, 0, ASSERT_LINE);
	i186.intr.pending = 1;
	cpu_trigger(CPU_RESUME_TRIGGER);
	if (LOG_OPTIMIZATION) logerror("  - trigger due to interrupt pending\n");
	if (LOG_INTERRUPTS) logerror("(%f) **** Requesting interrupt vector %02X\n", timer_get_time(), new_vector);
}


static void handle_eoi(int data)
{
	int i, j;

	/* specific case */
	if (!(data & 0x8000))
	{
		/* turn off the appropriate in-service bit */
		switch (data & 0x1f)
		{
			case 0x08:	i186.intr.in_service &= ~0x01;	break;
			case 0x12:	i186.intr.in_service &= ~0x01;	break;
			case 0x13:	i186.intr.in_service &= ~0x01;	break;
			case 0x0a:	i186.intr.in_service &= ~0x04;	break;
			case 0x0b:	i186.intr.in_service &= ~0x08;	break;
			case 0x0c:	i186.intr.in_service &= ~0x10;	break;
			case 0x0d:	i186.intr.in_service &= ~0x20;	break;
			case 0x0e:	i186.intr.in_service &= ~0x40;	break;
			case 0x0f:	i186.intr.in_service &= ~0x80;	break;
			default:	logerror("%05X:ERROR - 80186 EOI with unknown vector %02X\n", activecpu_get_pc(), data & 0x1f);
		}
		if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for vector %02X\n", timer_get_time(), data & 0x1f);
	}

	/* non-specific case */
	else
	{
		/* loop over priorities */
		for (i = 0; i <= 7; i++)
		{
			/* check for in-service timers */
			if ((i186.intr.timer & 7) == i && (i186.intr.in_service & 0x01))
			{
				i186.intr.in_service &= ~0x01;
				if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for timer\n", timer_get_time());
				return;
			}

			/* check for in-service DMA interrupts */
			for (j = 0; j < 2; j++)
				if ((i186.intr.dma[j] & 7) == i && (i186.intr.in_service & (0x04 << j)))
				{
					i186.intr.in_service &= ~(0x04 << j);
					if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for DMA%d\n", timer_get_time(), j);
					return;
				}

			/* check external interrupts */
			for (j = 0; j < 4; j++)
				if ((i186.intr.ext[j] & 7) == i && (i186.intr.in_service & (0x10 << j)))
				{
					i186.intr.in_service &= ~(0x10 << j);
					if (LOG_INTERRUPTS) logerror("(%f) **** Got EOI for INT%d\n", timer_get_time(), j);
					return;
				}
		}
	}
}


/*************************************
 *
 *	80186 internal timers
 *
 *************************************/

static void internal_timer_int(int which)
{
	struct timer_state *t = &i186.timer[which];

	if (LOG_TIMER) logerror("Hit interrupt callback for timer %d\n", which);

	/* set the max count bit */
	t->control |= 0x0020;

	/* request an interrupt */
	if (t->control & 0x2000)
	{
		i186.intr.status |= 0x01 << which;
		update_interrupt_state();
		if (LOG_TIMER) logerror("  Generating timer interrupt\n");
	}

	/* if we're continuous, reset */
	if (t->control & 0x0001)
	{
		int count = t->maxA ? t->maxA : 0x10000;
		timer_adjust(t->int_timer, (double)count * TIME_IN_HZ(2000000), which, 0);
		if (LOG_TIMER) logerror("  Repriming interrupt\n");
	}
	else
		timer_adjust(t->int_timer, TIME_NEVER, which, 0);
}


static void internal_timer_sync(int which)
{
	struct timer_state *t = &i186.timer[which];

	/* if we have a timing timer running, adjust the count */
	if (t->time_timer_active)
	{
		double current_time = timer_timeelapsed(t->time_timer);
		int net_clocks = (int)((current_time - t->last_time) * 2000000.);
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


static void internal_timer_update(int which,
                                  int new_count,
                                  int new_maxA,
                                  int new_maxB,
                                  int new_control)
{
	struct timer_state *t = &i186.timer[which];
	int update_int_timer = 0;

	/* if we have a new count and we're on, update things */
	if (new_count != -1)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(which);
			update_int_timer = 1;
		}
		t->count = new_count;
	}

	/* if we have a new max and we're on, update things */
	if (new_maxA != -1 && new_maxA != t->maxA)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(which);
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
			internal_timer_sync(which);
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
			   activecpu_get_pc(),
			   new_control);

		/* if we have real changes, update things */
		if (diff != 0)
		{
			/* if we're going off, make sure our timers are gone */
			if ((diff & 0x8000) && !(new_control & 0x8000))
			{
				/* compute the final count */
				internal_timer_sync(which);

				/* nuke the timer and force the interrupt timer to be recomputed */
				timer_adjust(t->time_timer, TIME_NEVER, which, 0);
				t->time_timer_active = 0;
				update_int_timer = 1;
			}

			/* if we're going on, start the timers running */
			else if ((diff & 0x8000) && (new_control & 0x8000))
			{
				/* start the timing */
				timer_adjust(t->time_timer, TIME_NEVER, which, 0);
				t->time_timer_active = 1;
				update_int_timer = 1;
			}

			/* if something about the interrupt timer changed, force an update */
			if (!(diff & 0x8000) && (diff & 0x2000))
			{
				internal_timer_sync(which);
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
	         	timer_adjust(t->int_timer, (double)diff * TIME_IN_HZ(2000000), which, 0);
	         	if (LOG_TIMER) logerror("Set interrupt timer for %d\n", which);
	      	}
	      	else
	      	{
	        	timer_adjust(t->int_timer, TIME_NEVER, which, 0);
		}
	}
}



/*************************************
 *
 *	80186 internal DMA
 *
 *************************************/

static void dma_timer_callback(int which)
{
	struct dma_state *d = &i186.dma[which];

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
		i186.intr.request |= 0x04 << which;
		update_interrupt_state();
	}
}


static void update_dma_control(int which, int new_control)
{
	struct dma_state *d = &i186.dma[which];
	int diff;

	/* handle the CHG bit */
	if (!(new_control & 0x0004))
	  new_control = (new_control & ~0x0002) | (d->control & 0x0002);
	new_control &= ~0x0004;

	/* check for control bits we don't handle */
	diff = new_control ^ d->control;
	if (diff & 0x6811)
	  logerror("%05X:ERROR! - unsupported DMA mode %04X\n", 
		   activecpu_get_pc(),
		   new_control);

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
//			int count = d->count;
			int dacnum;

			/* adjust for redline racer */
         		dacnum = (d->dest & 0x3f) / 2;

			if (LOG_DMA) logerror("Initiated DMA %d - count = %04X, source = %04X, dest = %04X\n", which, d->count, d->source, d->dest);

			d->finished = 0;
/*			timer_adjust(d->finish_timer,
         TIME_IN_HZ(dac[dacnum].frequency) * (double)count, which, 0);*/
		}
	}

	/* set the new control register */
	d->control = new_control;
}



/*************************************
 *
 *	80186 internal I/O reads
 *
 *************************************/


 READ8_HANDLER( i186_internal_port_r )
{
	int shift = 8 * (offset & 1);
	int temp, which;

	switch (offset & ~1)
	{
		case 0x22:
			logerror("%05X:ERROR - read from 80186 EOI\n", activecpu_get_pc());
			break;

		case 0x24:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt poll\n", activecpu_get_pc());
			if (i186.intr.poll_status & 0x8000)
				int_callback(0);
			return (i186.intr.poll_status >> shift) & 0xff;

		case 0x26:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt poll status\n",
                     activecpu_get_pc());
			return (i186.intr.poll_status >> shift) & 0xff;

		case 0x28:
			if (LOG_PORTS) logerror("%05X:read 80186 interrupt mask\n",
                                 activecpu_get_pc());
			temp  = (i186.intr.timer  >> 3) & 0x01;
			temp |= (i186.intr.dma[0] >> 1) & 0x04;
			temp |= (i186.intr.dma[1] >> 0) & 0x08;
			temp |= (i186.intr.ext[0] << 1) & 0x10;
			temp |= (i186.intr.ext[1] << 2) & 0x20;
			temp |= (i186.intr.ext[2] << 3) & 0x40;
			temp |= (i186.intr.ext[3] << 4) & 0x80;
			return (temp >> shift) & 0xff;

		case 0x2a:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt priority mask\n",
                     activecpu_get_pc());
			return (i186.intr.priority_mask >> shift) & 0xff;

		case 0x2c:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt in-service\n",
                     activecpu_get_pc());
			return (i186.intr.in_service >> shift) & 0xff;

		case 0x2e:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt request\n",
                     activecpu_get_pc());
			temp = i186.intr.request & ~0x0001;
			if (i186.intr.status & 0x0007)
				temp |= 1;
			return (temp >> shift) & 0xff;

		case 0x30:
			if (LOG_PORTS)
            logerror("%05X:read 80186 interrupt status\n", activecpu_get_pc());
			return (i186.intr.status >> shift) & 0xff;

		case 0x32:
			if (LOG_PORTS)
            logerror("%05X:read 80186 timer interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.timer >> shift) & 0xff;

		case 0x34:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA 0 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.dma[0] >> shift) & 0xff;

		case 0x36:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA 1 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.dma[1] >> shift) & 0xff;

		case 0x38:
			if (LOG_PORTS)
            logerror("%05X:read 80186 INT 0 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.ext[0] >> shift) & 0xff;

		case 0x3a:
			if (LOG_PORTS)
            logerror("%05X:read 80186 INT 1 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.ext[1] >> shift) & 0xff;

		case 0x3c:
			if (LOG_PORTS)
            logerror("%05X:read 80186 INT 2 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.ext[2] >> shift) & 0xff;

		case 0x3e:
			if (LOG_PORTS)
            logerror("%05X:read 80186 INT 3 interrupt control\n",
                     activecpu_get_pc());
			return (i186.intr.ext[3] >> shift) & 0xff;

		case 0x50:
		case 0x58:
		case 0x60:
			if (LOG_PORTS)
            logerror("%05X:read 80186 Timer %d count\n",
                     activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			if (!(offset & 1))
				internal_timer_sync(which);
			return (i186.timer[which].count >> shift) & 0xff;

		case 0x52:
		case 0x5a:
		case 0x62:
			if (LOG_PORTS) logerror("%05X:read 80186 Timer %d max A\n",
                                 activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].maxA >> shift) & 0xff;

		case 0x54:
		case 0x5c:
			logerror("%05X:read 80186 Timer %d max B\n",
                  activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].maxB >> shift) & 0xff;

		case 0x56:
		case 0x5e:
		case 0x66:
			if (LOG_PORTS)
            logerror("%05X:read 80186 Timer %d control\n",
                     activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].control >> shift) & 0xff;

		case 0xa0:
			if (LOG_PORTS)
            logerror("%05X:read 80186 upper chip select\n",
                     activecpu_get_pc());
			return (i186.mem.upper >> shift) & 0xff;

		case 0xa2:
			if (LOG_PORTS)
            logerror("%05X:read 80186 lower chip select\n",
                     activecpu_get_pc());
			return (i186.mem.lower >> shift) & 0xff;

		case 0xa4:
			if (LOG_PORTS)
            logerror("%05X:read 80186 peripheral chip select\n",
                     activecpu_get_pc());
			return (i186.mem.peripheral >> shift) & 0xff;

		case 0xa6:
			if (LOG_PORTS)
            logerror("%05X:read 80186 middle chip select\n",
                     activecpu_get_pc());
			return (i186.mem.middle >> shift) & 0xff;

		case 0xa8:
			if (LOG_PORTS)
            logerror("%05X:read 80186 middle P chip select\n",
                     activecpu_get_pc());
			return (i186.mem.middle_size >> shift) & 0xff;

		case 0xc0:
		case 0xd0:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d lower source address\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].source >> shift) & 0xff;

		case 0xc2:
		case 0xd2:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d upper source address\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].source >> (shift + 16)) & 0xff;

		case 0xc4:
		case 0xd4:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d lower dest address\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].dest >> shift) & 0xff;

		case 0xc6:
		case 0xd6:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d upper dest address\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].dest >> (shift + 16)) & 0xff;

		case 0xc8:
		case 0xd8:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d transfer count\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].count >> shift) & 0xff;

		case 0xca:
		case 0xda:
			if (LOG_PORTS)
            logerror("%05X:read 80186 DMA%d control\n",
                     activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			return (i186.dma[which].control >> shift) & 0xff;

		default:
			logerror("%05X:read 80186 port %02X\n",
                  activecpu_get_pc(), offset);
			break;
	}
	return 0x00;
}

/*************************************
 *
 *	80186 internal I/O writes
 *
 *************************************/

WRITE8_HANDLER( i186_internal_port_w )
{
	static UINT8 even_byte;
	int temp, which, data16;

	/* warning: this assumes all port writes here are word-sized */
	if (!(offset & 1))
	{
		even_byte = data;
		return;
	}
	data16 = (data << 8) | even_byte;

	switch (offset & ~1)
	{
		case 0x22:
			if (LOG_PORTS)
            logerror("%05X:80186 EOI = %04X\n", activecpu_get_pc(), data16);
			handle_eoi(0x8000);
			update_interrupt_state();
			break;

		case 0x24:
			logerror("%05X:ERROR - write to 80186 interrupt poll = %04X\n",
                  activecpu_get_pc(), data16);
			break;

		case 0x26:
			logerror("%05X:ERROR - write to 80186 interrupt poll status = %04X\n",
                  activecpu_get_pc(), data16);
			break;

		case 0x28:
			if (LOG_PORTS)
            logerror("%05X:80186 interrupt mask = %04X\n",
                     activecpu_get_pc(), data16);
			i186.intr.timer  = (i186.intr.timer  & ~0x08) | ((data16 << 3) & 0x08);
			i186.intr.dma[0] = (i186.intr.dma[0] & ~0x08) | ((data16 << 1) & 0x08);
			i186.intr.dma[1] = (i186.intr.dma[1] & ~0x08) | ((data16 << 0) & 0x08);
			i186.intr.ext[0] = (i186.intr.ext[0] & ~0x08) | ((data16 >> 1) & 0x08);
			i186.intr.ext[1] = (i186.intr.ext[1] & ~0x08) | ((data16 >> 2) & 0x08);
			i186.intr.ext[2] = (i186.intr.ext[2] & ~0x08) | ((data16 >> 3) & 0x08);
			i186.intr.ext[3] = (i186.intr.ext[3] & ~0x08) | ((data16 >> 4) & 0x08);
			update_interrupt_state();
			break;

		case 0x2a:
			if (LOG_PORTS) logerror("%05X:80186 interrupt priority mask = %04X\n", activecpu_get_pc(), data16);
			i186.intr.priority_mask = data16 & 0x0007;
			update_interrupt_state();
			break;

		case 0x2c:
			if (LOG_PORTS) logerror("%05X:80186 interrupt in-service = %04X\n", activecpu_get_pc(), data16);
			i186.intr.in_service = data16 & 0x00ff;
			update_interrupt_state();
			break;

		case 0x2e:
			if (LOG_PORTS) logerror("%05X:80186 interrupt request = %04X\n", activecpu_get_pc(), data16);
			i186.intr.request = (i186.intr.request & ~0x00c0) | (data16 & 0x00c0);
			update_interrupt_state();
			break;

		case 0x30:
			if (LOG_PORTS) logerror("%05X:WARNING - wrote to 80186 interrupt status = %04X\n", activecpu_get_pc(), data16);
			i186.intr.status = (i186.intr.status & ~0x8007) | (data16 & 0x8007);
			update_interrupt_state();
			break;

		case 0x32:
			if (LOG_PORTS) logerror("%05X:80186 timer interrupt contol = %04X\n", activecpu_get_pc(), data16);
			i186.intr.timer = data16 & 0x000f;
			break;

		case 0x34:
			if (LOG_PORTS) logerror("%05X:80186 DMA 0 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.dma[0] = data16 & 0x000f;
			break;

		case 0x36:
			if (LOG_PORTS) logerror("%05X:80186 DMA 1 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.dma[1] = data16 & 0x000f;
			break;

		case 0x38:
			if (LOG_PORTS) logerror("%05X:80186 INT 0 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[0] = data16 & 0x007f;
			break;

		case 0x3a:
			if (LOG_PORTS) logerror("%05X:80186 INT 1 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[1] = data16 & 0x007f;
			break;

		case 0x3c:
			if (LOG_PORTS) logerror("%05X:80186 INT 2 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[2] = data16 & 0x001f;
			break;

		case 0x3e:
			if (LOG_PORTS) logerror("%05X:80186 INT 3 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[3] = data16 & 0x001f;
			break;

		case 0x50:
		case 0x58:
		case 0x60:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d count = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, data16, -1, -1, -1);
			break;

		case 0x52:
		case 0x5a:
		case 0x62:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d max A = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, data16, -1, -1);
			break;

		case 0x54:
		case 0x5c:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d max B = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, -1, data16, -1);
			break;

		case 0x56:
		case 0x5e:
		case 0x66:
			if (LOG_PORTS) logerror("%05X:80186 Timer %d control = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, -1, -1, data16);
			break;

		case 0xa0:
			if (LOG_PORTS) logerror("%05X:80186 upper chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.upper = data16 | 0xc038;
			break;

		case 0xa2:
			if (LOG_PORTS) logerror("%05X:80186 lower chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.lower = (data16 & 0x3fff) | 0x0038;
			break;

		case 0xa4:
			if (LOG_PORTS)
            logerror("%05X:80186 peripheral chip select = %04X\n",
                     activecpu_get_pc(), data16);
			i186.mem.peripheral = data16 | 0x0038;
			break;

		case 0xa6:
			if (LOG_PORTS) logerror("%05X:80186 middle chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.middle = data16 | 0x01f8;
			break;

		case 0xa8:
			if (LOG_PORTS) logerror("%05X:80186 middle P chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.middle_size = data16 | 0x8038;

			temp = (i186.mem.peripheral & 0xffc0) << 4;
			if (i186.mem.middle_size & 0x0040)
			{
//				install_mem_read_handler(2, temp, temp + 0x2ff, peripheral_r);
//				install_mem_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}
			else
			{
				temp &= 0xffff;
//				install_port_read_handler(2, temp, temp + 0x2ff, peripheral_r);
//				install_port_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}

			/* we need to do this at a time when the I86 context is swapped in */
			/* this register is generally set once at startup and never again, so it's a good */
			/* time to set it up */
			cpunum_set_irq_callback(0, int_callback);
			break;

		case 0xc0:
		case 0xd0:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d lower source address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			i186.dma[which].source = (i186.dma[which].source & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0xc2:
		case 0xd2:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d upper source address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			i186.dma[which].source = (i186.dma[which].source & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0xc4:
		case 0xd4:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d lower dest address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			i186.dma[which].dest = (i186.dma[which].dest & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0xc6:
		case 0xd6:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d upper dest address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			i186.dma[which].dest = (i186.dma[which].dest & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0xc8:
		case 0xd8:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d transfer count = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			i186.dma[which].count = data16;
			break;

		case 0xca:
		case 0xda:
			if (LOG_PORTS) logerror("%05X:80186 DMA%d control = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
//			stream_update(dma_stream, 0);
			update_dma_control(which, data16);
			break;

		case 0xfe:
			if (LOG_PORTS) logerror("%05X:80186 relocation register = %04X\n", activecpu_get_pc(), data16);

			/* we assume here there that this doesn't happen too often */
			/* plus, we can't really remove the old memory range, so we also assume that it's */
			/* okay to leave us mapped where we were */
			temp = (data16 & 0x0fff) << 8;
			if (data16 & 0x1000)
			{
				memory_install_read8_handler(2, ADDRESS_SPACE_PROGRAM, temp, temp + 0xff, 0, 0, i186_internal_port_r);
				memory_install_write8_handler(2, ADDRESS_SPACE_PROGRAM, temp, temp + 0xff, 0, 0, i186_internal_port_w);
			}
			else
			{
				temp &= 0xffff;
				memory_install_read8_handler(2, ADDRESS_SPACE_IO, temp, temp + 0xff, 0, 0, i186_internal_port_r);
				memory_install_write8_handler(2, ADDRESS_SPACE_IO, temp, temp + 0xff, 0, 0, i186_internal_port_w);
			}
/*			popmessage("Sound CPU reset");*/
			break;

		default:
			logerror("%05X:80186 port %02X = %04X\n", activecpu_get_pc(), offset, data16);
			break;
	}
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: CPU - Initialize the 80186 CPU                                    */
/*-------------------------------------------------------------------------*/
void compis_cpu_init(void)
{
	/* create timers here so they stick around */
	i186.timer[0].int_timer = timer_alloc(internal_timer_int);
	i186.timer[1].int_timer = timer_alloc(internal_timer_int);
	i186.timer[2].int_timer = timer_alloc(internal_timer_int);
	i186.timer[0].time_timer = timer_alloc(NULL);
	i186.timer[1].time_timer = timer_alloc(NULL);
	i186.timer[2].time_timer = timer_alloc(NULL);
	i186.dma[0].finish_timer = timer_alloc(dma_timer_callback);
	i186.dma[1].finish_timer = timer_alloc(dma_timer_callback);
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Driver - Init                                                     */
/*-------------------------------------------------------------------------*/
static void compis_pic_set_int_line(int which, int interrupt)
{
	switch(which)
	{
		case 0:
			/* Master */
			cpunum_set_input_line(0, 0, interrupt ? HOLD_LINE : CLEAR_LINE);
			break;

		case 1:
			/* Slave */
			pic8259_set_irq_line(0, 2, interrupt);
			break;
	}
}

static int compis_irq_callback(int irqline)
{
	return pic8259_acknowledge(0);
}

DRIVER_INIT( compis )
{
	cpunum_set_irq_callback(0,	compis_irq_callback);
	pic8259_init(2, compis_pic_set_int_line);
	memset (&compis, 0, sizeof (compis) );
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Machine - Init                                                    */
/*-------------------------------------------------------------------------*/
MACHINE_RESET( compis )
{
	/* CPU */
	compis_cpu_init();

	/* OSP PIT 8254 */
	pit8253_init(2, compis_pit_config);
		
	/* PPI */
	ppi8255_init(&compis_ppi_interface);
	    
	/* FDC */
	nec765_init(&compis_fdc_interface, NEC765A);
	compis_fdc_reset();

	/* RTC */
	mm58274c_init(0, 0);

	/* Setup the USART */
	msm8251_init(&compis_usart_interface);

	/* Keyboard */
	compis_keyb_init();

	/* OSP PIC 8259 */
	cpunum_set_irq_callback(0, compis_irq_callback);
}

/*-------------------------------------------------------------------------*/
/* Name: compis                                                            */
/* Desc: Interrupt - Vertical Blanking Interrupt                           */
/*-------------------------------------------------------------------------*/
INTERRUPT_GEN( compis_vblank_int )
{
//	compis_gdc_vblank_int();
	compis_keyb_update();
}
