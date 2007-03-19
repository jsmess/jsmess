/* MSM 8251/INTEL 8251 UART */

#include "includes/msm8251.h"
#include "includes/serial.h"

/* uncomment to enable verbose comments */
#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror("MSM8251: "x)
#else
#define LOG(x)
#endif

static struct msm8251 uart;
static void msm8251_receive_character(UINT8 ch);
static void msm8251_in_callback(int id, unsigned long state);
static void msm8251_update_tx_empty(void);
static void msm8251_update_tx_ready(void);

static void msm8251_in_callback(int id, unsigned long state)
{
	int changed;

	changed = uart.connection.input_state^state;

	uart.connection.input_state = state;

	/* did cts change state? */
	if (changed & SERIAL_STATE_CTS)
	{
		/* yes */
		/* update tx ready */
		msm8251_update_tx_ready();
	}
}


/* init */
void	msm8251_init(struct msm8251_interface *iface)
{
	serial_helper_setup();

	/* copy interface */
	if (iface!=NULL)
	{
		memcpy(&uart.interface, iface, sizeof(struct msm8251_interface));
	}

	/* setup this side of the serial connection */
	serial_connection_init(&uart.connection);
	serial_connection_set_in_callback(&uart.connection, msm8251_in_callback);
	uart.connection.input_state = 0;	
	/* reset chip */
	msm8251_reset();

}

static void msm8251_update_rx_ready(void)
{
	int state;

	state = uart.status & MSM8251_STATUS_RX_READY;

	/* masked? */
	if ((uart.command & (1<<2))==0)
	{
		state = 0;
	}

	if (uart.interface.rx_ready_callback)
		uart.interface.rx_ready_callback((state!=0));
	
}

void msm8251_receive_clock(void)
{
	/* receive enable? */
	if (uart.command & (1<<2))
	{
		//logerror("MSM8251\n");
		/* get bit received from other side and update receive register */
		receive_register_update_bit(&uart.receive_reg, get_in_data_bit(uart.connection.input_state));
		
		if (uart.receive_reg.flags & RECEIVE_REGISTER_FULL)
		{
			receive_register_extract(&uart.receive_reg, &uart.data_form);
			msm8251_receive_character(uart.receive_reg.byte_received);
		}
	}
}

void msm8251_transmit_clock(void)
{
	/* transmit enable? */
	if (uart.command & (1<<0))
	{

		/* transmit register full? */
		if ((uart.status & MSM8251_STATUS_TX_READY)==0)
		{
			/* if transmit reg is empty */
			if ((uart.transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)!=0)
			{
				/* set it up */
				transmit_register_setup(&uart.transmit_reg, &uart.data_form, uart.data);
				/* msm8251 transmit reg now empty */
				uart.status |=MSM8251_STATUS_TX_EMPTY;
				/* ready for next transmit */
				uart.status |=MSM8251_STATUS_TX_READY;
			
				msm8251_update_tx_empty();
				msm8251_update_tx_ready();
			}
		}
		
		/* if transmit is not empty... transmit data */
		if ((uart.transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)==0)
		{
	//		logerror("MSM8251\n");
			transmit_register_send_bit(&uart.transmit_reg, &uart.connection);
		}
	}

#if 0
	/* hunt mode? */
	/* after each bit has been shifted in, it is compared against the current sync byte */
	if (uart.command & (1<<7))
	{
		/* data matches sync byte? */
		if (uart.data == uart.sync_bytes[uart.sync_byte_offset])
		{
			/* sync byte matches */
			/* update for next sync byte? */
			uart.sync_byte_offset++;

			/* do all sync bytes match? */
			if (uart.sync_byte_offset == uart.sync_byte_count)
			{
				/* ent hunt mode */
				uart.command &=~(1<<7);
			}
		}
		else
		{
			/* if there is no match, reset */
			uart.sync_byte_offset = 0;
		}
	}
#endif
}


static void msm8251_update_tx_ready(void)
{
	/* clear tx ready state */
	int tx_ready;
	
	/* tx ready output is set if:
		DB Buffer Empty &
		CTS is set &
		Transmit enable is 1 
	*/


	tx_ready = 0;

	/* transmit enable? */
	if ((uart.command & (1<<0))!=0)
	{
		/* other side has rts set (comes in as CTS at this side) */
		if (uart.connection.input_state & SERIAL_STATE_CTS)
		{
			if (uart.status & MSM8251_STATUS_TX_EMPTY)
			{
				/* enable transfer */
				tx_ready = 1;
			}
		}
	}

	if (uart.interface.tx_ready_callback)
		uart.interface.tx_ready_callback(tx_ready);
}

static void msm8251_update_tx_empty(void)
{
	if (uart.status & MSM8251_STATUS_TX_EMPTY)
	{
		/* tx is in marking state (high) when tx empty! */
		set_out_data_bit(uart.connection.State,1);
		serial_connection_out(&uart.connection);
	}
	if (uart.interface.tx_empty_callback)
		uart.interface.tx_empty_callback(((uart.status & MSM8251_STATUS_TX_EMPTY)!=0));
	
}

void	msm8251_reset(void)
{
	LOG("MSM8251 Reset\n");

	/* what is the default setup when the 8251 has been reset??? */

	/* msm8251 datasheet explains the state of tx pin at reset */
	/* tx is set to 1 */
	set_out_data_bit(uart.connection.State,1);

	/* assumption, rts is set to 1 */
	uart.connection.State &= ~SERIAL_STATE_RTS;
	serial_connection_out(&uart.connection);

	transmit_register_reset(&uart.transmit_reg);
	receive_register_reset(&uart.receive_reg);
	/* expecting mode byte */
	uart.flags |= MSM8251_EXPECTING_MODE;
	/* not expecting a sync byte */
	uart.flags &= ~MSM8251_EXPECTING_SYNC_BYTE;

	/* no character to read by cpu */
	/* transmitter is ready and is empty */
	uart.status = MSM8251_STATUS_TX_EMPTY | MSM8251_STATUS_TX_READY;
	uart.mode_byte = 0;
	uart.command = 0;

	/* update tx empty pin output */
	msm8251_update_tx_empty();
	/* update rx ready pin output */
	msm8251_update_rx_ready();
	/* update tx ready pin output */
	msm8251_update_tx_ready();
}

/* write command */
WRITE8_HANDLER(msm8251_control_w)
{

	if (uart.flags & MSM8251_EXPECTING_MODE)
	{
		if (uart.flags & MSM8251_EXPECTING_SYNC_BYTE)
		{
			LOG("Sync byte\n");

#ifdef VERBOSE
			logerror("Sync byte: %02x\n", data);
#endif
			/* store sync byte written */
			uart.sync_bytes[uart.sync_byte_offset] = data;
			uart.sync_byte_offset++;

			if (uart.sync_byte_offset==uart.sync_byte_count)
			{
				/* finished transfering sync bytes, now expecting command */
				uart.flags &= ~(MSM8251_EXPECTING_MODE | MSM8251_EXPECTING_SYNC_BYTE);
				uart.sync_byte_offset = 0;
			//	uart.status = MSM8251_STATUS_TX_EMPTY | MSM8251_STATUS_TX_READY;
			}
		}
		else
		{
			LOG("Mode byte\n");

			uart.mode_byte = data;

			/* Synchronous or Asynchronous? */
			if ((data & 0x03)!=0)
			{
				/*	Asynchronous
				
					bit 7,6: stop bit length
						0 = inhibit
						1 = 1 bit
						2 = 1.5 bits
						3 = 2 bits
					bit 5: parity type
						0 = parity odd
						1 = parity even
					bit 4: parity test enable
						0 = disable
						1 = enable
					bit 3,2: character length
						0 = 5 bits
						1 = 6 bits
						2 = 7 bits
						3 = 8 bits
					bit 1,0: baud rate factor 
						0 = defines command byte for synchronous or asynchronous
						1 = x1
						2 = x16
						3 = x64
				*/

				LOG("Asynchronous operation\n");

#ifdef VERBOSE
                logerror("Character length: %d\n", (((data>>2) & 0x03)+5));

				if (data & (1<<4))
				{
					logerror("enable parity checking\n");
				}
				else
				{
					logerror("parity check disabled\n");
				}

				if (data & (1<<5))
				{
					logerror("even parity\n");
				}
				else
				{
					logerror("odd parity\n");
				}

				{
					UINT8 stop_bit_length;

					stop_bit_length = (data>>6) & 0x03;

					switch (stop_bit_length)
					{
						case 0:
						{
							/* inhibit */
							logerror("stop bit: inhibit\n");
						}
						break;

						case 1:
						{
							/* 1 */
							logerror("stop bit: 1 bit\n");
						}
						break;

						case 2:
						{
							/* 1.5 */
							logerror("stop bit: 1.5 bits\n");
						}
						break;

						case 3:
						{
							/* 2 */
							logerror("stop bit: 2 bits\n");
						}
						break;
					}
				}
#endif

				uart.data_form.word_length = ((data>>2) & 0x03)+5;
				uart.data_form.parity = SERIAL_PARITY_NONE;
				switch ((data>>6) & 0x03)
				{				
					case 0:
					case 1:
						uart.data_form.stop_bit_count =  1;
						break;
					case 2:
					case 3:
						uart.data_form.stop_bit_count =  2;
						break;
				}
				receive_register_setup(&uart.receive_reg, &uart.data_form);
			

#if 0
				/* data bits */
				uart.receive_char_length = (((data>>2) & 0x03)+5);
				
				if (data & (1<<4))
				{
					/* parity */
					uart.receive_char_length++;
				}

				/* stop bits */
				uart.receive_char_length++;

				uart.receive_flags &=~MSM8251_TRANSFER_RECEIVE_SYNCHRONISED;
				uart.receive_flags |= MSM8251_TRANSFER_RECEIVE_WAITING_FOR_START_BIT;
#endif
				/* not expecting mode byte now */
				uart.flags &= ~MSM8251_EXPECTING_MODE;
//				uart.status = MSM8251_STATUS_TX_EMPTY | MSM8251_STATUS_TX_READY;
			}
			else
			{
				/*	bit 7: Number of sync characters 
						0 = 1 character
						1 = 2 character
					bit 6: Synchronous mode
						0 = Internal synchronisation
						1 = External synchronisation
					bit 5: parity type
						0 = parity odd
						1 = parity even
					bit 4: parity test enable
						0 = disable
						1 = enable
					bit 3,2: character length
						0 = 5 bits
						1 = 6 bits
						2 = 7 bits
						3 = 8 bits
					bit 1,0 = 0
				*/
				LOG("Synchronous operation\n");

				/* setup for sync byte(s) */
				uart.flags |= MSM8251_EXPECTING_SYNC_BYTE;
				uart.sync_byte_offset = 0;
				if (data & 0x07)
				{
					uart.sync_byte_count = 1;
				}
				else
				{
					uart.sync_byte_count = 2;
				}

			}
		}
	}
	else
	{
		/* command */
		LOG("Command byte\n");

		uart.command = data;
#ifdef VERBOSE
		logerror("Command byte: %02x\n", data);

		if (data & (1<<7))
		{
			logerror("hunt mode\n");
		}

		if (data & (1<<5))
		{
			logerror("/rts set to 0\n");
		}
		else
		{
			logerror("/rts set to 1\n");
		}

		if (data & (1<<2))
		{
			logerror("receive enable\n");
		}
		else
		{
			logerror("receive disable\n");
		}

		if (data & (1<<1))
		{
			logerror("/dtr set to 0\n");
		}
		else
		{
			logerror("/dtr set to 1\n");
		}

		if (data & (1<<0))
		{
			logerror("transmit enable\n");
		}
		else
		{
			logerror("transmit disable\n");
		}


#endif
		

		/*	bit 7: 
				0 = normal operation
				1 = hunt mode
			bit 6: 
				0 = normal operation
				1 = internal reset
			bit 5:
				0 = /RTS set to 1
				1 = /RTS set to 0
			bit 4:
				0 = normal operation
				1 = reset error flag
			bit 3:
				0 = normal operation
				1 = send break character
			bit 2:
				0 = receive disable
				1 = receive enable
			bit 1:
				0 = /DTR set to 1
				1 = /DTR set to 0
			bit 0:
				0 = transmit disable
				1 = transmit enable 
		*/
	
		uart.connection.State &=~SERIAL_STATE_RTS;
		if (data & (1<<5))
		{
			/* rts set to 0 */
			uart.connection.State |= SERIAL_STATE_RTS;
		}

		uart.connection.State &=~SERIAL_STATE_DTR;
		if (data & (1<<1))
		{
			uart.connection.State |= SERIAL_STATE_DTR;
		}

		if ((data & (1<<0))==0)
		{
			/* held in high state when transmit disable */
			set_out_data_bit(uart.connection.State,1);
		}


		/* refresh outputs */
		serial_connection_out(&uart.connection);

		if (data & (1<<4))
		{
			uart.status &= ~(MSM8251_STATUS_PARITY_ERROR | MSM8251_STATUS_OVERRUN_ERROR | MSM8251_STATUS_FRAMING_ERROR);
		}

		if (data & (1<<6))
		{
			msm8251_reset();
		}

		msm8251_update_rx_ready();
		msm8251_update_tx_ready();

	}
}

/* read status */
 READ8_HANDLER(msm8251_status_r)
{
#ifdef VERBOSE
	logerror("status: %02x\n", uart.status);
#endif

	return uart.status;
}

/* write data */
WRITE8_HANDLER(msm8251_data_w)
{
	uart.data = data;

	logerror("write data: %02x\n",data);

	/* writing clears */
	uart.status &=~MSM8251_STATUS_TX_READY;

	/* if transmitter is active, then tx empty will be signalled */

	msm8251_update_tx_ready();
}

/* called when last bit of data has been received */
static void msm8251_receive_character(UINT8 ch)
{
	logerror("msm8251 receive char: %02x\n",ch);

	uart.data = ch;
	
	/* char has not been read and another has arrived! */
	if (uart.status & MSM8251_STATUS_RX_READY)
	{
		uart.status |= MSM8251_STATUS_OVERRUN_ERROR;
	}
	uart.status |= MSM8251_STATUS_RX_READY;

	msm8251_update_rx_ready();
}

/* read data */
 READ8_HANDLER(msm8251_data_r)
{
	logerror("read data: %02x\n",uart.data);
	/* reading clears */
	uart.status &= ~MSM8251_STATUS_RX_READY;
	
	msm8251_update_rx_ready();
	return uart.data;
}

/* initialise transfer using serial device - set the callback which will
be called when serial device has updated it's state */
void msm8251_connect_to_serial_device(mess_image *image)
{
	serial_device_connect(image, &uart.connection);
}

void msm8251_connect(struct serial_connection *other_connection)
{
	serial_connection_link(&uart.connection, other_connection);
}

