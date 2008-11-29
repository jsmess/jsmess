/*********************************************************************

	6551.h

	MOS Technology 6551 Asynchronous Communications Interface Adapter

*********************************************************************/

#ifndef __6551_H__
#define __6551_H__


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

WRITE8_HANDLER(acia_6551_w);
READ8_HANDLER(acia_6551_r);

void acia_6551_init(void);
void acia_6551_set_irq_callback(void (*callback)(int));

void acia_6551_connect_to_serial_device(const device_config *image);


#endif /* __6551_H__ */
