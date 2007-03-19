#ifndef __MSM8251_HEADER_INCLUDED__
#define __MSM8251_HEADER_INCLUDED__

#include "driver.h"
#include "serial.h"

#define MSM8251_EXPECTING_MODE 0x01
#define MSM8251_EXPECTING_SYNC_BYTE 0x02


#define MSM8251_STATUS_FRAMING_ERROR 0x020
#define MSM8251_STATUS_OVERRUN_ERROR 0x010
#define MSM8251_STATUS_PARITY_ERROR 0x08
#define MSM8251_STATUS_TX_EMPTY		0x04
#define MSM8251_STATUS_RX_READY	0x02
#define MSM8251_STATUS_TX_READY	0x01

struct msm8251_interface
{
	/* state of txrdy output */
	void	(*tx_ready_callback)(int state);
	/* state of txempty output */
	void	(*tx_empty_callback)(int state);
	/* state of rxrdy output */
	void	(*rx_ready_callback)(int state);
};

struct msm8251
{
	/* flags controlling how msm8251_control_w operates */
	UINT8 flags;
	/* offset into sync_bytes used during sync byte transfer */
	UINT8 sync_byte_offset;
	/* number of sync bytes written so far */
	UINT8 sync_byte_count;
	/* the sync bytes written */
	UINT8 sync_bytes[2];
	/* status of msm8251 */
	UINT8 status;
	UINT8 command;
	/* mode byte - bit definitions depend on mode - e.g. synchronous, asynchronous */
	UINT8 mode_byte;

	/* data being received */
	UINT8 data;	

	/* receive reg */
	struct serial_receive_register receive_reg;
	/* transmit reg */
	struct serial_transmit_register transmit_reg;

	struct data_form data_form;

	/* contains callback for txrdy, rxrdy and tx empty which connect
	to host system */
	struct msm8251_interface interface;

	/* the serial connection that data is transfered over */
	/* this is usually connected to the serial device */
	struct serial_connection connection;
};

/* reading and writing data register share the same address,
and reading status and writing control share the same address */

/* read data register */
 READ8_HANDLER(msm8251_data_r);
/* read status register */
 READ8_HANDLER(msm8251_status_r);
/* write data register */
WRITE8_HANDLER(msm8251_data_w);
/* write control word */
WRITE8_HANDLER(msm8251_control_w);


/* init chip, set interface, and do main setup */
void msm8251_init(struct msm8251_interface *);
/* reset the chip */
void msm8251_reset(void);


/* The 8251 has seperate transmit and receive clocks */
/* use these two functions to update the msm8251 for each clock */
/* on NC100 system, the clocks are the same */

void msm8251_transmit_clock(void);
void msm8251_receive_clock(void);


void msm8251_connect_to_serial_device(mess_image *image);

void msm8251_connect(struct serial_connection *other_connection);

#endif
