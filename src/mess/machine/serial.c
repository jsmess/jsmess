/* internal serial transmission */

/* select a file on host filesystem to transfer using serial method,
setup serial interface software in driver and let the transfer begin */

/* this is used in the Amstrad NC Notepad emulation */



/*

	the output starts at 1 level. It changes to 0 when the start bit has been transmitted.
	This therefore signals that data is following.
	When all data bits have been received, stop bits are transmitted with a value of 1.

	msm8251 expects this in asynchronous mode:
	packet format:

	bit count			function		value
	1					start bit		0
	note 1				data bits		x
	note 2				parity bit		x
	note 3				stop bits		1

	Note:
	1. Data size can be defined (usual value is 8)
	2. Parity bit (if parity is set to odd or even). Value of bit
		is defined by data parity.
	3. There should be at least 1 stop bit.
*/

#include "driver.h"
#include "includes/serial.h"

/* number of serial streams supported. This is also the number
of serial ports supported */
#define MAX_SERIAL_DEVICES	4

/* the serial streams */
static struct serial_device	serial_devices[MAX_SERIAL_DEVICES];
static void	serial_device_baud_rate_callback(int id);
static void xmodem_init(void);


/*********************************************************/

static unsigned char serial_parity_table[256];

void serial_helper_setup(void)
{
	int i;

	/* if sum of all bits in the byte is even, then the data
	has even parity, otherwise it has odd parity */
	for (i=0; i<256; i++)
	{
		int data;
		int sum;
		int b;

		sum = 0;
		data = i;

		for (b=0; b<8; b++)
		{
			sum+=data & 0x01;

			data = data>>1;
		}

		serial_parity_table[i] = sum & 0x01;
	}
}

unsigned char serial_helper_get_parity(unsigned char data)
{
	return serial_parity_table[data & 0x0ff];
}

static void serial_device_in_callback(int id, unsigned long status)
{
	serial_devices[id].connection.input_state = status;
}

/*********************************************************/

static void serial_protocol_none_sent_char(int id);

static struct serial_protocol_interface serial_protocol_none_interface=
{
	NULL,
	serial_protocol_none_sent_char,
};


static void serial_protocol_xmodem_receive_char(int id, unsigned char ch);
static void serial_protocol_xmodem_sent_char(int id);

static struct serial_protocol_interface serial_protocol_xmodem_interface=
{
	serial_protocol_xmodem_receive_char,
	serial_protocol_xmodem_sent_char
};


/**********************************************************/

/***** SERIAL DEVICE ******/
void serial_device_setup(mess_image *image, int baud_rate, int num_data_bits, int stop_bit_count, int parity_code)
{
	int id = image_index_in_device(image);

	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return;

	serial_devices[id].BaudRate = baud_rate;
	serial_devices[id].data_form.word_length = num_data_bits;
	serial_devices[id].data_form.stop_bit_count = stop_bit_count;
	serial_devices[id].data_form.parity = parity_code;
	serial_devices[id].timer = timer_alloc(serial_device_baud_rate_callback);

	serial_connection_init(&serial_devices[id].connection);
	serial_connection_set_in_callback(&serial_devices[id].connection, serial_device_in_callback);

	/* signal to other end it is clear to send! */
	/* data is initially high state */
	/* set /rts */
	serial_devices[id].connection.State |= SERIAL_STATE_RTS;
	/* signal to other end data is ready to be accepted */
	/* set /dtr */
	serial_devices[id].connection.State |= SERIAL_STATE_DTR;
	set_out_data_bit(serial_devices[id].connection.State, 1);
	serial_connection_out(&serial_devices[id].connection);
	transmit_register_reset(&serial_devices[id].transmit_reg);
	receive_register_reset(&serial_devices[id].receive_reg);
	receive_register_setup(&serial_devices[id].receive_reg, &serial_devices[id].data_form);
	memset(&serial_devices[id].protocol_interface, 0, sizeof(struct serial_protocol_interface));
	/* temp here */
	xmodem_init();
}


unsigned long serial_device_get_state(int id)
{
	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return 0;
	return serial_devices[id].connection.State;
}


static const char *protocol_names[]=
{
	"None",
	"XModem"
};

/* get name of current protocol */
const char *serial_device_get_protocol_name(int protocol_id)
{
	return protocol_names[protocol_id];
}

void serial_device_set_protocol(mess_image *image, int protocol_id)
{
	int id = image_index_in_device(image);

	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return;

	serial_devices[id].protocol = protocol_id;

	switch (serial_devices[id].protocol)
	{
		default:
		case SERIAL_PROTOCOL_NONE:
		{
			memcpy(&serial_devices[id].protocol_interface,&serial_protocol_none_interface,sizeof(struct serial_protocol_interface));

		}
		break;

		case SERIAL_PROTOCOL_XMODEM:
		{
			memcpy(&serial_devices[id].protocol_interface,&serial_protocol_xmodem_interface,sizeof(struct serial_protocol_interface));
		}
		break;
	}
}

void serial_device_set_transmit_state(mess_image *image, int state)
{
	int previous_state;
	int id = image_index_in_device(image);

	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return;

	previous_state = serial_devices[id].transmit_state;

	serial_devices[id].transmit_state = state;

	if ((state^previous_state)!=0)
	{
		if (state)
		{
			/* start timer */
			timer_adjust(serial_devices[id].timer, 0, id, TIME_IN_HZ(serial_devices[id].BaudRate));
		}
		else
		{
			/* remove timer */
			timer_reset(serial_devices[id].timer, TIME_NEVER);
		}
	}

}

/* get a bit from input stream */
static int	data_stream_get_data_bit_from_data_byte(struct data_stream *stream)
{
	int data_bit;
	int data_byte;

	if (stream->ByteCount<stream->DataLength)
	{
		/* get data from buffer */
		data_byte= stream->pData[stream->ByteCount];
	}
	else
	{
		/* over end of buffer, so return 0 */
		data_byte= 0;
	}

	/* get bit from data */
	data_bit = (data_byte>>(7-stream->BitCount)) & 0x01;

	/* update bit count */
	stream->BitCount++;
	/* ripple overflow onto byte count */
	stream->ByteCount+=stream->BitCount>>3;
	/* lock bit count into range */
	stream->BitCount &=0x07;

	/* do not let it read over end of data */
	if (stream->ByteCount>=stream->DataLength)
	{
		stream->ByteCount = stream->DataLength-1;
	}

	return data_bit;
}

static int data_stream_get_byte(struct data_stream *stream)
{
	int data_byte;

	if (stream->ByteCount>=stream->DataLength)
	{
		return 0;
	}

	data_byte = stream->pData[stream->ByteCount];
	stream->ByteCount++;

	return data_byte;
}

void	receive_register_setup(struct serial_receive_register *receive, struct data_form *data_form)
{
	receive->bit_count = data_form->word_length + data_form->stop_bit_count;

	if (data_form->parity != SERIAL_PARITY_NONE)
	{
		receive->bit_count++;
	}
}


/* this is generic code to be used in serial chip implementations */
/* the majority of serial chips work in the same way and this code will work */
/* for them */

/* receive a bit */
void	receive_register_update_bit(struct serial_receive_register *receive, int bit)
{
	int previous_bit;

//#ifdef VERBOSE
	//logerror("receive register receive bit: %1x\n",bit);
//#endif
	previous_bit = receive->register_data & 1;

	/* shift previous bit 7 out */
	receive->register_data = receive->register_data<<1;
	/* shift new bit in */
	receive->register_data = (receive->register_data & 0xfffe) | bit;
	/* update bit count received */
	receive->bit_count_received++;

	/* asyncrhonouse mode */
	if (receive->flags & RECEIVE_REGISTER_WAITING_FOR_START_BIT)
	{
		/* the previous bit is stored in uart.receive char bit 0 */
		/* has the bit state changed? */
		if (((previous_bit ^ bit) & 0x01)!=0)
		{
			/* yes */
			if (bit==0)
			{
				//logerror("receive register saw start bit\n");

				/* seen start bit! */
				/* not waiting for start bit now! */
				receive->flags &=~RECEIVE_REGISTER_WAITING_FOR_START_BIT;
				receive->flags |=RECEIVE_REGISTER_SYNCHRONISED;
				/* reset bit count received */
				receive->bit_count_received = 0;
			}
		}
	}
	else
	if (receive->flags & RECEIVE_REGISTER_SYNCHRONISED)
	{
		/* received all bits? */
		if (receive->bit_count_received==receive->bit_count)
		{
			receive->bit_count_received = 0;
			receive->flags &=~RECEIVE_REGISTER_SYNCHRONISED;
			receive->flags |= RECEIVE_REGISTER_WAITING_FOR_START_BIT;
			//logerror("receive register full\n");
			receive->flags |= RECEIVE_REGISTER_FULL;
		}
	}
}

void	receive_register_reset(struct serial_receive_register *receive_reg)
{
	receive_reg->bit_count_received = 0;
	receive_reg->flags &=~RECEIVE_REGISTER_FULL;
	receive_reg->flags &=~RECEIVE_REGISTER_SYNCHRONISED;
	receive_reg->flags |= RECEIVE_REGISTER_WAITING_FOR_START_BIT;
}

void	receive_register_extract(struct serial_receive_register *receive_reg, struct data_form *data_form)
{
	unsigned long data_shift;
	UINT8 data;

	receive_register_reset(receive_reg);

	data_shift = 0;

	/* if parity is even or odd, there should be a parity bit in the stream! */
	if (data_form->parity!=SERIAL_PARITY_NONE)
	{
		data_shift++;
	}

	data_shift+=data_form->stop_bit_count;

	/* strip off stop bits and parity */
	data = receive_reg->register_data>>data_shift;

	/* mask off other bits so data byte has 0's in unused bits */
	data = data & (0x0ff
		>>
		(8-(data_form->word_length)));

	receive_reg->byte_received  = data;

	/* parity enable? */
	switch (data_form->parity)
	{
		case SERIAL_PARITY_NONE:
			break;

		/* check parity */
		case SERIAL_PARITY_ODD:
		case SERIAL_PARITY_EVEN:
		{
			unsigned char computed_parity;
			unsigned char parity_received;

			/* get state of parity bit received */
			parity_received = (receive_reg->register_data>>data_form->stop_bit_count) & 0x01;

			/* compute parity for received bits */
			computed_parity = serial_helper_get_parity(data);

			if (data_form->parity == SERIAL_PARITY_ODD)
			{
				/* odd parity */


			}
			else
			{
				/* even parity */


			}

		}
		break;
	}
}

/***** TRANSMIT REGISTER *****/

void	transmit_register_reset(struct serial_transmit_register *transmit_reg)
{
	transmit_reg->flags |=TRANSMIT_REGISTER_EMPTY;
}

/* used to construct data in stream format */
static void transmit_register_add_bit(struct serial_transmit_register *transmit_reg, int bit)
{
	/* combine bit */
	transmit_reg->register_data = transmit_reg->register_data<<1;
	transmit_reg->register_data &=~1;
	transmit_reg->register_data|=(bit & 0x01);
	transmit_reg->bit_count++;
}


/* generate data in stream format ready for transfer */
void transmit_register_setup(struct serial_transmit_register *transmit_reg, struct data_form *data_form,unsigned char data_byte)
{
	int i;
	unsigned char transmit_data;


	transmit_reg->bit_count_transmitted = 0;
	transmit_reg->bit_count = 0;
	transmit_reg->flags &=~TRANSMIT_REGISTER_EMPTY;

	/* start bit */
	transmit_register_add_bit(transmit_reg,0);

	/* data bits */
	transmit_data = data_byte;
	for (i=0; i<data_form->word_length; i++)
	{
		int databit;

		/* get bit from data */
		databit = (transmit_data>>(data_form->word_length-1)) & 0x01;
		/* add bit to formatted byte */
		transmit_register_add_bit(transmit_reg, databit);
		transmit_data = transmit_data<<1;
	}

	/* parity */
	if (data_form->parity!=SERIAL_PARITY_NONE)
	{
		/* odd or even parity */
		unsigned char parity;

		/* get parity */
		/* if parity = 0, data has even parity - i.e. there is an even number of one bits in the data */
		/* if parity = 1, data has odd parity - i.e. there is an odd number of one bits in the data */
		parity = serial_helper_get_parity(data_byte);

		transmit_register_add_bit(transmit_reg, parity);
	}

	/* stop bit(s) */
	for (i=0; i<data_form->stop_bit_count; i++)
	{
		transmit_register_add_bit(transmit_reg,1);
	}
}

int transmit_register_get_data_bit(struct serial_transmit_register *transmit_reg)
{
	int bit;

	bit = (transmit_reg->register_data>>
		(transmit_reg->bit_count - 1 -
		transmit_reg->bit_count_transmitted)) & 0x01;

	transmit_reg->bit_count_transmitted++;

	/* have all bits of this stream formatted byte been sent? */
	if (transmit_reg->bit_count_transmitted==transmit_reg->bit_count)
	{
		/* yes - generate a new byte to send */
		transmit_reg->flags |= TRANSMIT_REGISTER_EMPTY;
	}

	return bit;
}

void	transmit_register_send_bit(struct serial_transmit_register *transmit_reg, struct serial_connection *connection)
{
	int data;

	data = transmit_register_get_data_bit(transmit_reg);

	/* set tx data bit */
	set_out_data_bit(connection->State, data);

	/* state out through connection */
	serial_connection_out(connection);
}

void serial_protocol_none_sent_char(int id)
{
	int i;
	int bit;
	unsigned char data_byte;

	/* generate byte to transmit */
	data_byte = 0;
	for (i=0; i<serial_devices[id].data_form.word_length; i++)
	{
		data_byte = data_byte<<1;
		bit = data_stream_get_data_bit_from_data_byte(&serial_devices[id].transmit);
		data_byte = data_byte|bit;
	}
	/* setup register */
	transmit_register_setup(&serial_devices[id].transmit_reg,&serial_devices[id].data_form, data_byte);

	logerror("serial device transmitted char: %02x\n",data_byte);
}

static void	serial_device_baud_rate_callback(int id)
{
	/* receive data into receive register */
	receive_register_update_bit(&serial_devices[id].receive_reg, get_in_data_bit(serial_devices[id].connection.input_state));

	if (serial_devices[id].receive_reg.flags & RECEIVE_REGISTER_FULL)
	{
		//logerror("SERIAL DEVICE\n");
		receive_register_extract(&serial_devices[id].receive_reg, &serial_devices[id].data_form);

		logerror("serial device receive char: %02x\n",serial_devices[id].receive_reg.byte_received);

		/* if callback setup, execute it */
		if (serial_devices[id].protocol_interface.character_received_callback)
			serial_devices[id].protocol_interface.character_received_callback(id, serial_devices[id].receive_reg.byte_received);
	}

	/* is transmit empty? */
	if (serial_devices[id].transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)
	{
		/* char has been sent, execute callback */
		if (serial_devices[id].protocol_interface.character_sent_callback)
			serial_devices[id].protocol_interface.character_sent_callback(id);
	}

	/* other side says it is clear to send? */
	if (serial_devices[id].connection.input_state & SERIAL_STATE_CTS)
	{
		/* send bit */
		transmit_register_send_bit(&serial_devices[id].transmit_reg, &serial_devices[id].connection);
	}
}

/* connect the specified connection to this serial device */
void serial_device_connect(mess_image *image, struct serial_connection *connection)
{
	int id = image_index_in_device(image);

	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return;

	serial_connection_link(connection, &serial_devices[id].connection);
}


/* load image */
static int serial_device_load_internal(mess_image *image, unsigned char **ptr, int *pDataSize)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image_length(image);

	if (datasize!=0)
	{
		/* malloc memory for this data */
		data = malloc(datasize);

		if (data!=NULL)
		{
			/* read whole file */
			image_fread(image, data, datasize);

			*ptr = data;
			*pDataSize = datasize;

			logerror("File loaded!\r\n");

			/* ok! */
			return 1;
		}
	}
	return 0;
}

/* reset position in stream */
static void data_stream_reset(struct data_stream *stream)
{
	/* reset byte offset */
	stream->ByteCount= 0;
	/* reset bit count */
	stream->BitCount = 0;
}

/* free stream */
static void data_stream_free(struct data_stream *stream)
{
	if (stream->pData!=NULL)
	{
		free(stream->pData);
		stream->pData = NULL;
	}
	stream->DataLength = 0;
}

/* initialise stream */
static void data_stream_init(struct data_stream *stream, unsigned char *pData, unsigned long DataLength)
{
	stream->pData = pData;
	stream->DataLength = DataLength;
	data_stream_reset(stream);
}

int serial_device_init(mess_image *image)
{
	int id = image_index_in_device(image);
	memset(&serial_devices[id], 0, sizeof(serial_devices[id]));
	return INIT_PASS;
}

int serial_device_load(mess_image *image)
{
	int id = image_index_in_device(image);
	int data_length;
	unsigned char *data;

	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_DEVICES))
		return INIT_FAIL;

	/* load file and setup transmit data */
	if (serial_device_load_internal(image, &data, &data_length))
	{
		data_stream_init(&serial_devices[id].transmit, data, data_length);
		return INIT_PASS;
	}

	return INIT_FAIL;
}


void serial_device_unload(mess_image *image)
{
	int id = image_index_in_device(image);

	/* stop transmit */
	serial_device_set_transmit_state(image, 0);
	/* free streams */
	data_stream_free(&serial_devices[id].transmit);
	data_stream_free(&serial_devices[id].receive);
}

/********************************************************************************************************/
/***** XMODEM protocol ****/

static void serial_update_crc(unsigned short *crc, unsigned char value)
{
	UINT8 l, h;

	l = value ^ (*crc >> 8);
	*crc = (*crc & 0xff) | (l << 8);
	l >>= 4;
	l ^= (*crc >> 8);
	*crc <<= 8;
	*crc = (*crc & 0xff00) | l;
	l = (l << 4) | (l >> 4);
	h = l;
	l = (l << 2) | (l >> 6);
	l &= 0x1f;
	*crc = *crc ^ (l << 8);
	l = h & 0xf0;
	*crc = *crc ^ (l << 8);
	l = (h << 1) | (h >> 7);
	l &= 0xe0;
	*crc = *crc ^ l;
}

/* XMODEM protocol */
enum
{
	XMODEM_STATE_WAITING_FOR_LINK,
	XMODEM_STATE_SENDING_PACKET,
	XMODEM_STATE_WAITING_FOR_CONFIRM,
	XMODEM_WAITING_FOR_END_CONFIRM1,
	XMODEM_WAITING_FOR_END_CONFIRM2
};


struct xmodem
{
	int state;
	int packet_id;
	unsigned char packet[1024];
	unsigned long offset_in_packet;
	unsigned long packet_size;
};

static struct xmodem xmodem_transfer;

static void xmodem_set_new_state(int);

void	xmodem_init(void)
{
	xmodem_set_new_state(XMODEM_STATE_WAITING_FOR_LINK);
	xmodem_transfer.packet_id = 1;
}

static void xmodem_setup_packet(int id)
{
	int i;
	int count;
	UINT16 crc;

	count = 0;


	/*
	packet:

	1 byte			SOH
	1 byte			packet id
	1 byte			packet id (stored 1's complemented)
	128 bytes		data
	1 byte			CRC-16 upper byte
	1 byte			CRC-16 lower byte
	*/

	/* soh */
	xmodem_transfer.packet[count] = XMODEM_SOH;
	count++;
	/* packet id */
	xmodem_transfer.packet[count] = xmodem_transfer.packet_id & 0x0ff;
	count++;
	/* 1's complement of packet id */
	xmodem_transfer.packet[count] = (xmodem_transfer.packet_id^0x0ff) & 0x0ff;
	count++;

	crc = 0;

	for (i=0; i<128; i++)
	{
		xmodem_transfer.packet[count] = data_stream_get_byte(&serial_devices[id].transmit);
		serial_update_crc(&crc, xmodem_transfer.packet[count]);
		count++;
	}

	xmodem_transfer.packet[count] = (crc>>8) & 0x0ff;
	count++;
	xmodem_transfer.packet[count] = crc & 0x0ff;
	count++;

	xmodem_transfer.packet_size = count;
	xmodem_transfer.offset_in_packet = 0;

}

static void xmodem_resend_block(void)
{
	xmodem_transfer.offset_in_packet = 0;
	xmodem_set_new_state(XMODEM_STATE_SENDING_PACKET);
}


static void xmodem_set_new_state(int state)
{
	xmodem_transfer.state = state;
}

static void serial_protocol_xmodem_receive_char(int id, unsigned char ch)
{
	switch (xmodem_transfer.state)
	{
		case XMODEM_STATE_WAITING_FOR_LINK:
		{
			if (ch==XMODEM_CRC_NAK)
			{
				logerror("xmodem nak received\n");

				xmodem_setup_packet(id);
				xmodem_set_new_state(XMODEM_STATE_SENDING_PACKET);
			}
		}
		break;

		case XMODEM_STATE_WAITING_FOR_CONFIRM:
		{
			switch (ch)
			{
				case XMODEM_ACK:
				{
					logerror("block accepted\n");
					xmodem_transfer.packet_id++;
					xmodem_setup_packet(id);
					xmodem_set_new_state(XMODEM_STATE_SENDING_PACKET);
				}
				break;

				case XMODEM_NAK:
				{
					logerror("resend block\n");
					xmodem_resend_block();
				}
				break;

				default:
				{
					logerror("unknown response\n");
				}
				break;
			}
		}
		break;

		default:
			break;
	}
}

static void serial_protocol_xmodem_sent_char(int id)
{
	switch (xmodem_transfer.state)
	{
		case XMODEM_STATE_SENDING_PACKET:
		{
			unsigned char data_byte;

			if (xmodem_transfer.offset_in_packet==xmodem_transfer.packet_size)
			{
				xmodem_set_new_state(XMODEM_STATE_WAITING_FOR_CONFIRM);
				return;
			}

			/* generate byte to transmit */
			data_byte = xmodem_transfer.packet[xmodem_transfer.offset_in_packet];
			xmodem_transfer.offset_in_packet++;
			logerror("packet byte: %02x\n",data_byte);

			/* setup register */
			transmit_register_setup(&serial_devices[id].transmit_reg,&serial_devices[id].data_form, data_byte);

			logerror("serial device transmitted char: %02x\n",data_byte);
		}
		break;



		default:
			break;
	}
}

/*******************************************************************************/
/********* SERIAL CONNECTION ***********/


/* this converts state at this end to a state the other end can accept */
/* e.g. CTS at this end becomes RTS at other end.
		RTS at this end becomes CTS at other end.
		TX at this end becomes RX at other end.
		RX at this end becomes TX at other end.
		etc

		The same thing is done inside the serial null-terminal lead */

static unsigned long serial_connection_spin_bits(unsigned long input_status)
{

	return
		/* cts -> rts */
		(((input_status & 0x01)<<1) |
		/* rts -> cts */
		((input_status>>1) & 0x01) |
		/* dsr -> dtr */
		(((input_status>>2) & 0x01)<<3) |
		/* dtr -> dsr */
		(((input_status>>3) & 0x01)<<2) |
		/* rx -> tx */
		(((input_status>>4) & 0x01)<<5) |
		/* tx -> rx */
		(((input_status>>5) & 0x01)<<4));
}


/* setup callbacks for connection */
void	serial_connection_init(struct serial_connection *connection)
{
	connection->out_callback = NULL;
	connection->in_callback = NULL;
}

/* set callback which will be executed when out status has changed */
void	serial_connection_set_out_callback(struct serial_connection *connection, void (*out_cb)(int id, unsigned long state))
{
	connection->out_callback = out_cb;
}

/* set callback which will be executed when in status has changed */
void	serial_connection_set_in_callback(struct serial_connection *connection, void (*in_cb)(int id, unsigned long state))
{
	connection->in_callback = in_cb;
}

/* output new state through connection */
void	serial_connection_out(struct serial_connection *connection)
{
	if (connection->out_callback!=NULL)
	{
		unsigned long state_at_other_end;

		state_at_other_end = serial_connection_spin_bits(connection->State);

		connection->out_callback(connection->id, state_at_other_end);
	}
}

/* join two serial connections together */
void	serial_connection_link(struct serial_connection *connection_a, struct serial_connection *connection_b)
{
	/* both connections should have their in connection setup! */
	/* the in connection is the callback they use to update their state based
	on the output from the other side */
	connection_a->out_callback = connection_b->in_callback;
	connection_b->out_callback = connection_a->in_callback;

	/* let b know the state of a */
	serial_connection_out(connection_a);
	/* let a know the state of b */
	serial_connection_out(connection_b);
}
