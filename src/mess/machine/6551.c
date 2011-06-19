/*********************************************************************

    6551.h

    MOS Technology 6551 Asynchronous Communications Interface Adapter

    A1 A0  Write                    Read
     0  0  Transmit Data Register   Receiver Data Register
     0  1  Programmed Reset         Status Register
     1  0              Command Register
     1  1              Control Register

*********************************************************************/

#include "emu.h"
#include "6551.h"
#include "machine/serial.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _acia6551_t acia6551_t;
struct _acia6551_t
{
	unsigned char transmit_data_register;
	unsigned char receive_data_register;
	unsigned char status_register;
	unsigned char command_register;
	unsigned char control_register;

	/* internal baud rate timer */
	emu_timer	*timer;
	/* callback for internal baud rate timer */
	void (*acia_updated_callback)(int id, unsigned long State);

	void	(*irq_callback)(int);

	data_form form;

	/* receive register */
	serial_receive_register	receive_reg;
	/* transmit register */
	serial_transmit_register transmit_reg;

	serial_connection connection;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void acia_6551_receive_char(device_t *device, unsigned char ch);
static void acia_6551_refresh_ints(device_t *device);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE acia6551_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == ACIA6551);

	return (acia6551_t *) downcast<legacy_device_base *>(device)->token();
}



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    acia_6551_in_callback - called when other side
    has updated state
-------------------------------------------------*/

static void acia_6551_in_callback(running_machine &machine, int id, unsigned long state)
{
	device_t *device;
	acia6551_t *acia;

	/* NPW 29-Nov-2008 - These two lines are a hack and indicate why our "serial" infrastructure needs to be updated */
	device = machine.device("acia");
	acia = get_token(device);

	acia->connection.input_state = state;
}



/*-------------------------------------------------
    TIMER_CALLBACK(acia_6551_timer_callback)
-------------------------------------------------*/

static TIMER_CALLBACK(acia_6551_timer_callback)
{
	device_t *device = (device_t *)ptr;
	acia6551_t *acia = get_token(device);

	/* get bit received from other side and update receive register */
	receive_register_update_bit(&acia->receive_reg, get_in_data_bit(acia->connection.input_state));

	if (acia->receive_reg.flags & RECEIVE_REGISTER_FULL)
	{
		receive_register_extract(&acia->receive_reg, &acia->form);
		acia_6551_receive_char(device, acia->receive_reg.byte_received);
	}

	/* transmit register full? */
	if ((acia->status_register & (1<<4))==0)
	{
		/* if transmit reg is empty */
		if (acia->transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)
		{
			/* set it up */
			transmit_register_setup(&acia->transmit_reg, &acia->form, acia->transmit_data_register);
			/* acia transmit reg now empty */
			acia->status_register |=(1<<4);
			/* and refresh ints */
			acia_6551_refresh_ints(device);
		}
	}

	/* if transmit is not empty... transmit data */
	if ((acia->transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)==0)
	{
	//  logerror("UART6551\n");
		transmit_register_send_bit(machine, &acia->transmit_reg, &acia->connection);
	}
}



/*-------------------------------------------------
    DEVICE_START( acia6551 )
-------------------------------------------------*/

static DEVICE_START( acia6551 )
{
	acia6551_t *acia = get_token(device);

	serial_helper_setup();

	memset(acia, 0, sizeof(*acia));
	/* transmit data reg is empty */
	acia->status_register |= (1<<4);
	acia->timer = device->machine().scheduler().timer_alloc(FUNC(acia_6551_timer_callback), (void *) device);

	serial_connection_init(device->machine(), &acia->connection);
	serial_connection_set_in_callback(device->machine(), &acia->connection, acia_6551_in_callback);
	transmit_register_reset(&acia->transmit_reg);
	receive_register_reset(&acia->receive_reg);
}



/*-------------------------------------------------
    acia_6551_refresh_ints - update interrupt output
-------------------------------------------------*/

static void acia_6551_refresh_ints(device_t *device)
{
	acia6551_t *acia = get_token(device);
	int interrupt_state;

	interrupt_state = 0;

	/* receive interrupts */

	/* receive data register full? */
	if (acia->status_register & (1<<3))
	{
		/* receiver interrupt enable? */
		if ((acia->command_register & (1<<1))==0)
		{
			/* trigger a interrupt */
			interrupt_state = 1;
		}
	}

	/* set state of irq bit in status register */
	acia->status_register &= ~(1<<7);

	if (interrupt_state)
	{
		acia->status_register |=(1<<7);
	}

	if (acia->irq_callback != NULL)
        (*acia->irq_callback)(interrupt_state);
}



/*-------------------------------------------------
    acia_6551_receive_char
-------------------------------------------------*/

static void acia_6551_receive_char(device_t *device, unsigned char ch)
{
	acia6551_t *acia = get_token(device);

	/* receive register full? */
	if (acia->status_register & (1<<3))
	{
		/* set overrun error */
		acia->status_register |= (1<<2);
		return;
	}

	/* set new byte */
	acia->receive_data_register = ch;
	/* receive register is full */
	acia->status_register |= (1<<3);
	/* update ints */
	acia_6551_refresh_ints(device);
}



/*-------------------------------------------------
    acia_6551_r
-------------------------------------------------*/

READ8_DEVICE_HANDLER(acia_6551_r)
{
	acia6551_t *acia = get_token(device);

	unsigned char data;

	data = 0x0ff;
	switch (offset & 0x03)
	{
		case 0:
		{
			/*  clear parity error, framing error and overrun error */
			/* when read of data reigster is done */
			acia->status_register &=~((1<<0) | (1<<1) | (1<<2));
			/* clear receive data register full flag */
			acia->status_register &=~(1<<3);
			/* return data */
			data = acia->receive_data_register;
		}
		break;

	/*
    Status                    cleared by
    b0  Parity error * (1: error)       self clearing **
    b1  Framing error * (1: error)      self clearing **
    b2  Overrun * (1: error)            self clearing **
    b3  Receive Data Register Full (1: full)  Read Receive Data Register
    b4  Transmit Data Reg Empty (1: empty) Write Transmit Data Register
    b5  DCD (0: DCD low, 1: DCD high) Not resettable, reflects DCD state
    b6  DSR (0: DSR low, 1: DCD high) Not resettable, reflects DSR state
    b7  IRQ (0: no int., 1: interrupt)  Read Status Register
    */
		case 1:
        {
            data = acia->status_register;
			/* clear interrupt */
			acia->status_register &= ~(1<<7);
		}
        break;

        case 2:
			data = acia->command_register;
			break;

		case 3:
			data = acia->control_register;
			break;

		default:
			break;
	}

	logerror("6551 R %04x %02x\n",offset & 0x03,data);

	return data;
}



/*-------------------------------------------------
    acia_6551_update_data_form
-------------------------------------------------*/

static void acia_6551_update_data_form(device_t *device)
{
	acia6551_t *acia = get_token(device);

	acia->form.word_length = 8-((acia->control_register>>5) & 0x03);
	acia->form.stop_bit_count = (acia->control_register>>7)+1;

	if (acia->command_register & (1<<5))
	{
		acia->form.parity = SERIAL_PARITY_ODD;
	}
	else
	{
		acia->form.parity = SERIAL_PARITY_NONE;
	}

	receive_register_setup(&acia->receive_reg, &acia->form);
}



/*-------------------------------------------------
    acia_6551_w
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(acia_6551_w)
{
	acia6551_t *acia = get_token(device);

	logerror("6551 W %04x %02x\n",offset & 0x03, data);

	switch (offset & 0x03)
	{
		case 0:
		{
			/* clear transmit data register empty */
			acia->status_register &= ~(1<<4);
			/* store byte */
			acia->transmit_data_register = data;

		}
		break;

		case 1:
		{
			/* telstrat writes 0x07f! */
		}
		break;


		/*
        Command Register:

        b0  Data Terminal Ready
            0 : disable receiver and all interrupts (DTR high)
            1 : enable receiver and all interrupts (DTR low)
        b1  Receiver Interrupt Enable
            0 : IRQ interrupt enabled from bit 3 of status register
            1 : IRQ interrupt disabled
        b3,b2   Transmitter Control
                Transmit Interrupt    RTS level    Transmitter
            00     disabled     high        off
            01     enabled      low     on
            10     disabled     low     on
            11     disabled     low    Transmit BRK
        b4  Normal/Echo Mode for Receiver
            0 : normal
            1 : echo (bits 2 and 3 must be 0)
        b5  Parity Enable
            0 : parity disabled, no parity bit generated or received
            1 : parity enabled
        b7,b6   Parity
            00 : odd parity receiver and transmitter
            01 : even parity receiver and transmitter
            10 : mark parity bit transmitted, parity check disabled
            11 : space parity bit transmitted, parity check disabled
        */

		case 2:
		{
			acia->command_register = data;

			/* update state of dtr */
			acia->connection.State &=~SERIAL_STATE_DTR;
			if (acia->command_register & (1<<0))
			{
				acia->connection.State |=SERIAL_STATE_DTR;
			}

			/* update state of rts */
			switch ((acia->command_register>>2) & 0x03)
			{
				case 0:
				{
					acia->connection.State &=~SERIAL_STATE_RTS;
				}
				break;

				case 1:
				case 2:
				case 3:
				{
					acia->connection.State |=SERIAL_STATE_RTS;
				}
				break;
			}

			serial_connection_out(device->machine(), &acia->connection);

			acia_6551_update_data_form(device);
		}
		break;

		/*
        Control register:

        b3-b0   baud rate generator:
            0000 : 16x external clock
            0001 : 50 baud
            0010 : 75 25
            0011 : 110 35
            0100 : 134.5
            0101 : 150
            0110 : 300 150
            0111 : 600 300
            1000 : 1200 600
            1001 : 1800 600
            1010 : 2400 600
            1011 : 3600 1200
            1100 : 4800 1200
            1101 : 7200 2400
            1110 : 9600 2400
            1111 : 19,200 9600
        b4  receiver clock source
            0 : external receiver clock
            1 : baud rate generator
        b6,b5   word length
            00 : 8 bits
            01 : 7
            10 : 6
            11 : 5
        b7  stop bits
            0 : 1 stop bit
            1 : 2 stop bits
                (1 stop bit if parity and word length = 8)
                (1 1/2 stop bits if word length = 5 and no parity)
        */
		case 3:
		{
			unsigned char previous_control_register;

            previous_control_register = acia->control_register;

            if (((previous_control_register^data) & 0x07)!=0)
			{
				int rate;

                rate = data & 0x07;

				/* baud rate changed? */
				acia->timer->reset();

				if (rate==0)
				{
					/* 16x external clock */
					logerror("6551: external clock not supported!\n");
				}
				else
				{
					int baud_rate;

					switch (rate)
					{

						default:
						case 1:
						{
							baud_rate = 50;
						}
						break;

						case 2:
						{
							baud_rate = 75;
						}
						break;

						case 3:
						{
							baud_rate = 110;
						}
						break;

						case 4:
						{
							baud_rate = 135;
						}
						break;

						case 5:
						{
							baud_rate = 150;
						}
						break;

						case 6:
						{
							baud_rate = 300;
						}
						break;

						case 7:
						{
							baud_rate = 600;
						}
						break;

						case 8:
						{
							baud_rate = 1200;
						}
						break;

						case 9:
						{
							baud_rate = 1800;
						}
						break;

						case 10:
						{
							baud_rate = 2400;
						}
						break;

						case 11:
						{
							baud_rate = 3600;
						}
						break;

						case 12:
						{
							baud_rate = 4800;
						}
						break;

						case 13:
						{
							baud_rate = 7200;
						}
						break;

						case 14:
						{
							baud_rate = 9600;
						}
						break;

						case 15:
						{
							baud_rate = 19200;
						}
						break;
					}

					acia->timer->adjust(attotime::zero, 0, attotime::from_hz(baud_rate));
				}
			}

			acia->control_register = data;
			acia_6551_update_data_form(device);
		}
		break;

		default:
			break;
	}

}


/*-------------------------------------------------
    acia_6551_connect_to_serial_device
-------------------------------------------------*/

void acia_6551_connect_to_serial_device(device_t *device, device_t *image)
{
	acia6551_t *acia = get_token(device);
	serial_device_connect(image, &acia->connection);
}



/*-------------------------------------------------
    DEVICE_GET_INFO( acia6551 )
-------------------------------------------------*/

DEVICE_GET_INFO( acia6551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(acia6551_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(acia6551);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MOS Technology 6551 ACIA");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MOS Technology 6551 ACIA");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(ACIA6551, acia6551);
