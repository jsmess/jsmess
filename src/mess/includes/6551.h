/* 6551 ACIA */

#include "driver.h"

WRITE8_HANDLER(acia_6551_w);
 READ8_HANDLER(acia_6551_r);

void acia_6551_init(void);
void acia_6551_set_irq_callback(void (*callback)(int));

void acia_6551_connect_to_serial_device(mess_image *image);

