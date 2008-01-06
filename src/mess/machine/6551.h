/*****************************************************************************
 *
 * machine/6551.h
 *
 * 6551 ACIA
 *
 ****************************************************************************/

#ifndef _6551_H_
#define _6551_H_


/*----------- defined in machine/6551.c -----------*/

WRITE8_HANDLER(acia_6551_w);
 READ8_HANDLER(acia_6551_r);

void acia_6551_init(void);
void acia_6551_set_irq_callback(void (*callback)(int));

void acia_6551_connect_to_serial_device(mess_image *image);


#endif /* _6551_H_ */
