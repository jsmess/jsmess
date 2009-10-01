/*********************************************************************

    6551.h

    MOS Technology 6551 Asynchronous Communications Interface Adapter

*********************************************************************/

#ifndef __6551_H__
#define __6551_H__


/***************************************************************************
    MACROS
***************************************************************************/

#define ACIA6551		DEVICE_GET_INFO_NAME(acia6551)

#define MDRV_ACIA6551_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag), ACIA6551, 0)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(acia6551);

READ8_DEVICE_HANDLER(acia_6551_r);
WRITE8_DEVICE_HANDLER(acia_6551_w);

void acia_6551_connect_to_serial_device(const device_config *device, const device_config *image);


#endif /* __6551_H__ */
