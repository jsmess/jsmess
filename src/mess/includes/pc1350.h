/*****************************************************************************
 *
 * includes/pc1350.h
 *
 * Pocket Computer 1350
 *
 ****************************************************************************/

#ifndef PC1350_H_
#define PC1350_H_

#define PC1350_CONTRAST (input_port_read(machine, "DSW0") & 0x07)


/*----------- defined in machine/pc1350.c -----------*/

void pc1350_outa(running_device *device, int data);
void pc1350_outb(running_device *device, int data);
void pc1350_outc(running_device *device, int data);

int pc1350_brk(running_device *device);
int pc1350_ina(running_device *device);
int pc1350_inb(running_device *device);

MACHINE_START( pc1350 );
NVRAM_HANDLER( pc1350 );


/*----------- defined in video/pc1350.c -----------*/

READ8_HANDLER(pc1350_lcd_read);
WRITE8_HANDLER(pc1350_lcd_write);
VIDEO_UPDATE( pc1350 );

int pc1350_keyboard_line_r(void);


#endif /* PC1350_H_ */
