/*********************************************************************

    msm8251.h

    MSM/Intel 8251 Universal Synchronous/Asynchronous Receiver Transmitter code

*********************************************************************/

#ifndef __MSM8251_H__
#define __MSM8251_H__

#include "includes/serial.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MSM8251			DEVICE_GET_INFO_NAME(msm8251)

#define MSM8251_EXPECTING_MODE 0x01
#define MSM8251_EXPECTING_SYNC_BYTE 0x02

#define MSM8251_STATUS_FRAMING_ERROR 0x020
#define MSM8251_STATUS_OVERRUN_ERROR 0x010
#define MSM8251_STATUS_PARITY_ERROR 0x08
#define MSM8251_STATUS_TX_EMPTY		0x04
#define MSM8251_STATUS_RX_READY	0x02
#define MSM8251_STATUS_TX_READY	0x01

#define MDRV_MSM8251_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, MSM8251, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MSM8251_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _msm8251_interface msm8251_interface;
struct _msm8251_interface
{
	/* state of txrdy output */
	void	(*tx_ready_callback)(const device_config *device, int state);
	/* state of txempty output */
	void	(*tx_empty_callback)(const device_config *device, int state);
	/* state of rxrdy output */
	void	(*rx_ready_callback)(const device_config *device, int state);
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

extern const msm8251_interface default_msm8251_interface;

/* device get info function */
DEVICE_GET_INFO(msm8251);

/* read data register */
READ8_DEVICE_HANDLER(msm8251_data_r);

/* read status register */
READ8_DEVICE_HANDLER(msm8251_status_r);

/* write data register */
WRITE8_DEVICE_HANDLER(msm8251_data_w);

/* write control word */
WRITE8_DEVICE_HANDLER(msm8251_control_w);

/* The 8251 has seperate transmit and receive clocks */
/* use these two functions to update the msm8251 for each clock */
/* on NC100 system, the clocks are the same */
void msm8251_transmit_clock(const device_config *device);
void msm8251_receive_clock(const device_config *device);

/* connecting to serial output */
void msm8251_connect_to_serial_device(const device_config *device, const device_config *image);
void msm8251_connect(const device_config *device, struct serial_connection *other_connection);

#endif /* __MSM8251_H__ */
