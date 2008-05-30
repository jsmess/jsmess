/*****************************************************************************
 *
 * includes/pc1251.h
 *
 * Pocket Computer 1251
 *
 ****************************************************************************/

#ifndef PC1251_H_
#define PC1251_H_


#define PC1251_CONTRAST (input_port_read(machine, "DSW0") & 0x07)


/*----------- defined in machine/pc1251.c -----------*/

void pc1251_outa(int data);
void pc1251_outb(int data);
void pc1251_outc(int data);

int pc1251_reset(void);
int pc1251_brk(void);
int pc1251_ina(void);
int pc1251_inb(void);

DRIVER_INIT( pc1251 );
NVRAM_HANDLER( pc1251 );


/*----------- defined in video/pc1251.c -----------*/

READ8_HANDLER(pc1251_lcd_read);
WRITE8_HANDLER(pc1251_lcd_write);
VIDEO_UPDATE( pc1251 );


#endif /* PC1251_H_ */
