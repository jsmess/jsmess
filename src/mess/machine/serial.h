/*****************************************************************************
 *
 * machine/serial.h
 *
 * internal serial transmission
 *
 * This code is used to transmit a file stored on the host filesystem
 * (e.g. PC harddrive) to an emulated system.
 *
 * The file is converted into a serial bit-stream which can be received
 * by the emulated serial chip in the emulated system.
 *
 * The file can be transmitted using different protocols.
 *
 * A and B are two computers linked with a serial connection
 * A and B can transmit and receive data, through the same connection
 *
 * These flags apply to A and B, and give the state of the input & output
 * signals at each side.
 *
 ****************************************************************************/

#ifndef SERIAL_H_
#define SERIAL_H_


/*******************************************************************************/
/**** SERIAL CONNECTION ***/


/* this structure represents a serial connection */
typedef struct _serial_connection serial_connection;
struct _serial_connection
{
	int id;
	/* state of this side */
	unsigned long State;

	/* state of other side - store here */
	unsigned long input_state;

	/* this callback is executed when this side has refreshed it's state,
    to let the other end know */
	void	(*out_callback)(running_machine &machine, int id, unsigned long state);
	/* this callback is executed when the other side has refreshed it's state,
    to let the other end know */
	void	(*in_callback)(running_machine &machine, int id, unsigned long state);
};

/* this macro is used to extract the received data from the status */
#define get_in_data_bit(x) ((x & SERIAL_STATE_RX_DATA)>>4)

/* this macro is used to set the transmitted data in the status */
#define set_out_data_bit(x, data) \
	x&=~SERIAL_STATE_TX_DATA; \
	x|=(data<<5)




/*----------- defined in machine/serial.c -----------*/

/* setup out and in callbacks */
void serial_connection_init(running_machine &machine, serial_connection *connection);

/* set callback which will be executed when in status has changed */
void serial_connection_set_in_callback(running_machine &machine, serial_connection *connection, void (*in_cb)(running_machine &machine, int id, unsigned long status));

/* output status, if callback is setup it will be executed with the new status */
void serial_connection_out(running_machine &machine, serial_connection *connection);

/* join two serial connections */
void serial_connection_link(running_machine &machine, serial_connection *connection_a, serial_connection *connection_b);


/*******************************************************************************/

/* form of data being transmitted and received */
typedef struct _data_form data_form;
struct _data_form
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
typedef struct _serial_receive_register serial_receive_register;
struct _serial_receive_register
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

void	receive_register_setup(serial_receive_register *receive, data_form *data_form);
void	receive_register_update_bit(serial_receive_register *receive, int bit_state);
void	receive_register_extract(serial_receive_register *receive_reg, data_form *data_form);
void	receive_register_reset(serial_receive_register *receive_reg);

/* the transmit register is the final stage
in the serial transmit procedure */
/* normally, data is written to the transmit reg,
then it is assembled into transmit form and transmitted */
/* the transmit register holds data in transmit form */

/* register is empty and ready to be filled with data */
#define TRANSMIT_REGISTER_EMPTY 0x0001

typedef struct _serial_transmit_register serial_transmit_register;
struct _serial_transmit_register
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

/* setup transmit reg ready for transmit */
void transmit_register_setup(serial_transmit_register *transmit_reg, data_form *data_form,unsigned char data_byte);
void	transmit_register_send_bit(running_machine &machine, serial_transmit_register *transmit_reg, serial_connection *connection);
void	transmit_register_reset(serial_transmit_register *transmit_reg);

/*******************************************************************************/
/**** SERIAL HELPER ****/

void serial_helper_setup(void);

/*******************************************************************************/
/**** SERIAL DEVICE ****/

unsigned long serial_device_get_state(device_t *device);

/* connect this device to the emulated serial chip */
/* id is the serial device to connect to */
/* connection is the serial connection to connect to the serial device */
void serial_device_connect(device_t *image, serial_connection *connection);

DECLARE_LEGACY_IMAGE_DEVICE(SERIAL, serial);

#define MCFG_SERIAL_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, SERIAL, 0)

DEVICE_START(serial);
DEVICE_IMAGE_LOAD(serial);

void serial_device_setup(device_t *image, int baud_rate, int num_data_bits, int stop_bit_count, int parity_code);

/* set the transmit state of the serial device */
void serial_device_set_transmit_state(device_t *image, int state);

#endif /* SERIAL_H_ */
