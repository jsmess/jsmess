/*********************************************************************

    6551.h

    MOS Technology 6551 Asynchronous Communications Interface Adapter

*********************************************************************/

#ifndef __6551_H__
#define __6551_H__


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(ACIA6551, acia6551);

#define MDRV_ACIA6551_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag), ACIA6551, 0)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER(acia_6551_r);
WRITE8_DEVICE_HANDLER(acia_6551_w);

void acia_6551_connect_to_serial_device(running_device *device, running_device *image);


#endif /* __6551_H__ */
