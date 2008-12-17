/*****************************************************************************
 *
 * includes/pc1403.h
 *
 * Pocket Computer 1403
 *
 ****************************************************************************/

#ifndef PC1403_H_
#define PC1403_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


/*----------- defined in machine/pc1403.c -----------*/

extern UINT8 pc1403_portc;

int pc1403_reset(const device_config *device);
int pc1403_brk(const device_config *device);
void pc1403_outa(const device_config *device, int data);
//void pc1403_outb(const device_config *device, int data);
void pc1403_outc(const device_config *device, int data);
int pc1403_ina(const device_config *device);
//int pc1403_inb(const device_config *device);

DRIVER_INIT( pc1403 );
NVRAM_HANDLER( pc1403 );

READ8_HANDLER(pc1403_asic_read);
WRITE8_HANDLER(pc1403_asic_write);


/*----------- defined in video/pc1403.c -----------*/

VIDEO_START( pc1403 );
VIDEO_UPDATE( pc1403 );

READ8_HANDLER(pc1403_lcd_read);
WRITE8_HANDLER(pc1403_lcd_write);


#endif /* PC1403_H_ */
