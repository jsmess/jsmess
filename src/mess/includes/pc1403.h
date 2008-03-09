/*****************************************************************************
 *
 * includes/pc1403.h
 *
 * Pocket Computer 1403
 *
 ****************************************************************************/

#ifndef PC1403_H_
#define PC1403_H_


#define KEY_SMALL readinputport(1)&0x40
#define RAM32K (readinputport(10)&0x80)==0x80


/*----------- defined in machine/pc1403.c -----------*/

extern UINT8 pc1403_portc;

int pc1403_reset(void);
int pc1403_brk(void);
void pc1403_outa(int data);
//void pc1403_outb(int data);
void pc1403_outc(int data);
int pc1403_ina(void);
//int pc1403_inb(void);

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
