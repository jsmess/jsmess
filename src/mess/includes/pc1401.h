/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


/*----------- defined in machine/pc1401.c -----------*/

extern UINT8 pc1401_portc;
int pc1401_reset(running_device *device);
int pc1401_brk(running_device *device);
void pc1401_outa(running_device *device, int data);
void pc1401_outb(running_device *device, int data);
void pc1401_outc(running_device *device, int data);
int pc1401_ina(running_device *device);
int pc1401_inb(running_device *device);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );


#endif /* PC1401_H_ */
