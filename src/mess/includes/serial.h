#ifndef __SERIAL_TRANSMISSION_HEADER_INCLUDED__
#define __SERIAL_TRANSMISSION_HEADER_INCLUDED__

/* internal serial transmission */

/* 

	This code is used to transmit a file stored on the host filesystem
	(e.g. PC harddrive) to an emulated system.
	
	The file is converted into a serial bit-stream which can be received
	by the emulated serial chip in the emulated system.

	The file can be transmitted using different protocols
*/

/* 
	A and B are two computers linked with a serial connection 
 
   A and B can transmit and receive data, through the same connection
*/

/* 
	these flags apply to A and B, and give the state of the input & output
	signals at each side.
*/


/*	
	CTS = Clear to Send. (INPUT)
	Other end of connection is ready to accept data
	
	  
	NOTE: 
	
	  This output is active low on serial chips (e.g. 0 is CTS is set), 
	  but here it is active high!
*/
	
#define SERIAL_STATE_CTS	0x0001
/* 
	RTS = Request to Send. (OUTPUT)
	This end is ready to send data, and requests if the other
	end is ready to accept it

	NOTE: 
	
	  This output is active low on serial chips (e.g. 0 is RTS is set), 
	  but here it is active high!
*/
#define SERIAL_STATE_RTS	0x0002

/* 
	DSR = Data Set ready. (INPUT)
	Other end of connection has data 


	NOTE: 
	
	  This output is active low on serial chips (e.g. 0 is DSR is set), 
	  but here it is active high!
*/
#define SERIAL_STATE_DSR	0x0004

/* 
	DTR = Data terminal Ready. (OUTPUT)
	TX contains new data.

  	NOTE: 
	
	  This output is active low on serial chips (e.g. 0 is DTR is set), 
	  but here it is active high!
*/
#define SERIAL_STATE_DTR	0x0008
/* RX = Recieve data. (INPUT) */ 
#define SERIAL_STATE_RX_DATA	0x00010
/* TX = Transmit data. (OUTPUT) */
#define SERIAL_STATE_TX_DATA	0x00020

/* protocol selections */

/* all data is transmitted with a start bit, data bits, optional parity bit,
and stop bits. These are programmable. */
enum
{
	SERIAL_PROTOCOL_NONE,		/* no protocol applied */		
	SERIAL_PROTOCOL_XMODEM		/* transfer data using xmodem protocol */
};

/* parity selections */
/* if all the bits are added in a byte, if the result is:
	even -> parity is even
	odd -> parity is odd
*/
enum
{
	SERIAL_PARITY_NONE,		/* no parity. a parity bit will not be in the transmitted/received data */
	SERIAL_PARITY_ODD,		/* odd parity */
	SERIAL_PARITY_EVEN		/* even parity */
};

/* this macro is used to extract the received data from the status */
#define get_in_data_bit(x) ((x & SERIAL_STATE_RX_DATA)>>4)

/* this macro is used to set the transmitted data in the status */
#define set_out_data_bit(x, data) \
	x&=~SERIAL_STATE_TX_DATA; \
	x|=(data<<5)


/*******************************************************************************/
/**** SERIAL CONNECTION ***/


/* this structure represents a serial connection */
struct serial_connection
{
	int id;

	/* state of this side */
	unsigned long State;

	/* state of other side - store here */
	unsigned long input_state;
	
	/* this callback is executed when this side has refreshed it's state,
	to let the other end know */
	void	(*out_callback)(int id, unsigned long state);
	/* this callback is executed when the other side has refreshed it's state,
	to let the other end know */
	void	(*in_callback)(int id, unsigned long state);
};

/* setup out and in callbacks */
void serial_connection_init(struct serial_connection *connection);

/* set callback which will be executed when out status has changed */
void serial_connection_set_out_callback(struct serial_connection *connection, void (*out_cb)(int id, unsigned long state));

/* set callback which will be executed when in status has changed */
void serial_connection_set_in_callback(struct serial_connection *connection, void (*in_cb)(int id, unsigned long state));

/* output status, if callback is setup it will be executed with the new status */
void serial_connection_out(struct serial_connection *connection);

/* join two serial connections */
void serial_connection_link(struct serial_connection *connection_a, struct serial_connection *connection_b);


/*******************************************************************************/

/* form of data being transmitted and received */
struct data_form
{
	/* length of word in bits */
	unsigned long word_length;
	/* parity state */
	unsigned long parity;
	/* number of stop bits */
	unsigned long stop_bit_count;
};

/*******************************************************************************/

/*******************************************************************************/
/**** RECEIVE AND TRANSMIT GENERIC CODE ****/

/* this can be used by most of the serial chip implementations, 
because they all work in roughly the same way.
There is generic code to send and receive data in the specified form */

/* receive is waiting for start bit. The transition from high-low indicates
start of start bit. This is used to synchronise with the data being transfered */
#define RECEIVE_REGISTER_WAITING_FOR_START_BIT 0x01
/* receive is synchronised with data, data bits will be clocked in */
#define RECEIVE_REGISTER_SYNCHRONISED 0x02
/* set if receive register has been filled */
#define RECEIVE_REGISTER_FULL 0x04


/* the receive register holds data in receive form! */
/* this must be extracted to get the data byte received */
struct serial_receive_register
{
	/* data */
	unsigned long register_data;
	/* flags */
	unsigned long flags;
	/* bit count received */
	unsigned long bit_count_received;
	/* length of data to receive - includes data bits, parity bit and stop bit */
	unsigned long bit_count;

	/* the byte of data received */
	unsigned char byte_received;
};

void	receive_register_setup(struct serial_receive_register *receive, struct data_form *data_form);
void	receive_register_update_bit(struct serial_receive_register *receive, int bit_state);
void	receive_register_extract(struct serial_receive_register *receive_reg, struct data_form *data_form);
void	receive_register_reset(struct serial_receive_register *receive_reg);

/* the transmit register is the final stage
in the serial transmit procedure */
/* normally, data is written to the transmit reg,
then it is assembled into transmit form and transmitted */
/* the transmit register holds data in transmit form */

/* register is empty and ready to be filled with data */
#define TRANSMIT_REGISTER_EMPTY 0x0001

struct serial_transmit_register
{
	/* data */
	unsigned long register_data;
	/* flags */
	unsigned long flags;
	/* number of bits transmitted */
	unsigned long bit_count_transmitted;
	/* length of data to send */
	unsigned long bit_count;
};

/* get a bit from the transmit register */
int transmit_register_get_data_bit(struct serial_transmit_register *transmit_reg);
/* setup transmit reg ready for transmit */
void transmit_register_setup(struct serial_transmit_register *transmit_reg, struct data_form *data_form,unsigned char data_byte);
void	transmit_register_send_bit(struct serial_transmit_register *transmit_reg, struct serial_connection *connection);
void	transmit_register_reset(struct serial_transmit_register *transmit_reg);

/*******************************************************************************/
/**** SERIAL HELPER ****/

void serial_helper_setup(void);
unsigned char serial_helper_get_parity(unsigned char data);

/*******************************************************************************/
/**** SERIAL DEVICE ****/

/* a read/write bit stream. used to transmit data and to receive data */
struct data_stream
{
	/* pointer to buffer */
	unsigned char *pData;
	/* length of buffer */
	unsigned long DataLength;

	/* bit offset within current byte */
	unsigned long BitCount;
	/* byte offset within data */
	unsigned long ByteCount;
};

/* an internal interface used by serial device */
struct serial_protocol_interface
{
	/* callback executed when character has been received */
	void (*character_received_callback)(int id,unsigned char ch);
	/* callback executed when character has been transmitted, ready to transmit a new char */
	void (*character_sent_callback)(int id);
};

/* a serial device */
struct serial_device
{
	/* transmit data bit-stream */
	struct data_stream transmit;
	/* receive data bit-stream */
	struct data_stream receive;

	/* register to receive data */
	struct serial_receive_register	receive_reg;
	/* register to transmit data */
	struct serial_transmit_register transmit_reg;

	/* connection to transmit/receive data over */
	struct serial_connection connection;

	/* data form to transmit/receive */
	struct data_form data_form;

	int transmit_state;
	int protocol;

	/* baud rate */
	unsigned long BaudRate;

	/* baud rate timer */
	void	*timer;

	struct serial_protocol_interface protocol_interface;
};

unsigned long serial_device_get_state(int id);

/* connect this device to the emulated serial chip */
/* id is the serial device to connect to */
/* connection is the serial connection to connect to the serial device */
void serial_device_connect(mess_image *image, struct serial_connection *connection);

/* init this device */
int serial_device_init(mess_image *image);
int serial_device_load(mess_image *image);
void serial_device_unload(mess_image *image);

void serial_device_setup(mess_image *image, int baud_rate, int num_data_bits, int stop_bit_count, int parity_code);

/* get name of protocol identified by specified id */
const char *serial_device_get_protocol_name(int protocol_id);

/* set the protocol to be used by the serial device */
void serial_device_set_protocol(mess_image *image, int protocol_id);
/* set the transmit state of the serial device */
void serial_device_set_transmit_state(mess_image *image, int state);

/***********************************************************************************************/
/* XModem protocol */

#define XMODEM_CRC_NAK	67 /* begins a xmodem-crc transfer */
#define XMODEM_SOH		1	/* id of a 256 byte block */
#define XMODEM_EOT		4
#define XMODEM_ACK		6
#define XMODEM_NAK		21	/* begins a xmodem transfer */
#define XMODEM_CAN		24
#define XMODEM_STX		2	/* id start of a 1k block */

#endif
