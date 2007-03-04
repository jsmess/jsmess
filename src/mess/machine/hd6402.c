
/* not optimised! */

#include "driver.h"
#include "includes/hd6402.h"

static struct hd6402 uart;


static void hd6402_in_callback(int id, unsigned long state)
{
	uart.connection.input_state = state;
}

void	hd6402_init(void)
{
	uart.callback = NULL;
	serial_connection_init(&uart.connection);
	serial_connection_set_in_callback(&uart.connection, hd6402_in_callback);
	uart.connection.input_state = 0;	
	receive_register_reset(&uart.receive_reg);
	transmit_register_reset(&uart.transmit_reg);
}

void	hd6402_reset(void)
{
	/* toggle master reset */
	hd6402_set_input(HD6402_INPUT_MR, HD6402_INPUT_MR);
	hd6402_set_input(HD6402_INPUT_MR, 0);
}

void	hd6402_transmit_clock(void)
{
#if 0
	/* if transmit reg is empty */
	if ((uart.transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)!=0)
	{
		/* set it up */
		transmit_register_setup(&uart.transmit_reg, &uart.data_form, uart.data);
	}
		
	/* if transmit is not empty... transmit data */
	if ((uart.transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)==0)
	{
		transmit_register_send_bit(&uart.transmit_reg, &uart.connection);
	}
#endif
}

static void hd6402_receive_character(char ch)
{
	/* if data has not been read.. */
	if (uart.state & HD6402_OUTPUT_DR)
	{
		/* set overrun error */
		uart.state |= HD6402_OUTPUT_OE;
	}

	uart.received_char = ch;

	/* set data received output */
	uart.state |= HD6402_OUTPUT_DR;

	if (uart.callback)
		uart.callback(HD6402_OUTPUT_DR|HD6402_OUTPUT_OE, uart.state);
}


void	hd6402_receive_clock(void)
{
	/* get bit received from other side and update receive register */
	receive_register_update_bit(&uart.receive_reg, get_in_data_bit(uart.connection.input_state));
	
	if (uart.receive_reg.flags & RECEIVE_REGISTER_FULL)
	{
		receive_register_extract(&uart.receive_reg, &uart.data_form);
		hd6402_receive_character(uart.receive_reg.byte_received);

		logerror("hd6402 receive char %d\n",uart.receive_reg.byte_received);
	}
}


/* set callback - callback is called whenever one of the output pins changes state */
void	hd6402_set_callback(void (*callback)(int,int))
{
	uart.callback = callback;

}

/* connect this uart to the serial connection specified */
void	hd6402_connect(struct serial_connection *other_connection)
{
	serial_connection_link(&uart.connection, other_connection);
}

static void hd6402_update_parity(void)
{
	int parity;

	if (uart.state & HD6402_INPUT_PI)
	{
		/* parity is inhibited */
		parity = SERIAL_PARITY_NONE;
		logerror("hd6402: no parity\n");
	}
	else
	{
		/* parity enabled */

		if (uart.state & HD6402_INPUT_EPE)
		{
			/* select even parity */
			parity = SERIAL_PARITY_EVEN;
			logerror("hd6402: parity even\n");
		}
		else
		{
			/* select odd parity */
			parity = SERIAL_PARITY_ODD;
			logerror("hd6402: parity odd\n");
		}
	}
	uart.data_form.parity = parity;
}

static void hd6402_update_character_length(void)
{
	int length;

	/* cls1=0, cls2=0 -> 5 bits */
	/* cls1=0, cls2=1 -> 6 bits */
	/* cls1=1, cls2=0 -> 7 bits */
	/* cls1=1, cls2=1 -> 8 bits */
	length = ((uart.state>>8) & 0x03)+5;

	uart.data_form.word_length = length;

	logerror("hd6402: word length: %d\n",length);
}

static void hd6402_update_stop_bits(void)
{
	int stop_bits;

	if (uart.data_form.word_length==5)
	{
		/* 1.5 stop bits */
		stop_bits = 1;
	}
	else
	{
		stop_bits = 2;
	}

	uart.data_form.stop_bit_count = stop_bits;
	
	logerror("hd6402: stop bits: %d\n",stop_bits);
}


/* set an input */
void	hd6402_set_input(int mask, int data)
{
	logerror("hd6402 set input: %04x %04x\n",mask,data);

	if (mask & data & HD6402_INPUT_MR)
	{
		/* clear parity error, framing error, overrun error, data received */
		uart.state &=~(HD6402_OUTPUT_PE|HD6402_OUTPUT_FE|HD6402_OUTPUT_OE|HD6402_OUTPUT_DR);
		/* set transmit register empty */
		uart.state |=HD6402_OUTPUT_TRE;

		/* refresh outputs */
		if (uart.callback)
			uart.callback(HD6402_OUTPUT_PE|HD6402_OUTPUT_FE|HD6402_OUTPUT_OE|HD6402_OUTPUT_DR|HD6402_OUTPUT_TRE,  uart.state);
	}

	if (mask & (HD6402_INPUT_RRD|HD6402_INPUT_SFD))
	{
		uart.state &=~mask;
		uart.state |=(data & mask);
	}

	/* data ready reset? */
	if (mask & HD6402_INPUT_DRR)
	{
		if ((data & HD6402_INPUT_DRR)==0)
		{
			uart.state &=~HD6402_OUTPUT_DR;
			
			/* refresh outputs */
			if (uart.callback)
				uart.callback(HD6402_OUTPUT_PE|HD6402_OUTPUT_FE|HD6402_OUTPUT_OE|HD6402_OUTPUT_DR|HD6402_OUTPUT_TRE,  uart.state);
		}
	}


	if (mask & HD6402_INPUT_SBS)
	{
		uart.state &=~mask;
		uart.state |=(data & mask);
		hd6402_update_stop_bits();
	}

	/* parity inhibit or parity type set */
	if (mask & (HD6402_INPUT_EPE|HD6402_INPUT_PI))
	{
		uart.state &=~mask;
		uart.state |=(data & mask);
		hd6402_update_parity();
	}
	
	/* character length set */
	if (mask & (HD6402_INPUT_CLS1|HD6402_INPUT_CLS2))
	{
		uart.state &=~mask;
		uart.state |=(data & mask);
		hd6402_update_character_length();
	}

	/* if any of the transmit/receive format changed refresh the receive register */
	if (mask & (HD6402_INPUT_SBS|HD6402_INPUT_EPE|HD6402_INPUT_PI|HD6402_INPUT_CLS1|HD6402_INPUT_CLS2))
	{
		receive_register_setup(&uart.receive_reg, &uart.data_form);
	}
}


/* low on /TBRL transfers data into transmit register */
/* transmit buffer write */
WRITE8_HANDLER(hd6402_data_w)
{
//	uart.transmit_char = data;
}

/* receive buffer read */
 READ8_HANDLER(hd6402_data_r)
{

	return uart.received_char;

}

