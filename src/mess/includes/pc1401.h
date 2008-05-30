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
int pc1401_reset(void);
int pc1401_brk(void);
void pc1401_outa(int data);
void pc1401_outb(int data);
void pc1401_outc(int data);
int pc1401_ina(void);
int pc1401_inb(void);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );


#endif /* PC1401_H_ */
