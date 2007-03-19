/*********************************************************************

	pc1403.h

	Pocket Computer 1403

*********************************************************************/

#ifndef PC1403_H
#define PC1403_H

extern UINT8 pc1403_portc;

bool pc1403_reset(void);
bool pc1403_brk(void);
void pc1403_outa(int data);
//void pc1403_outb(int data);
void pc1403_outc(int data);
int pc1403_ina(void);
//int pc1403_inb(void);

DRIVER_INIT( pc1403 );
NVRAM_HANDLER( pc1403 );

READ8_HANDLER(pc1403_asic_read);
WRITE8_HANDLER(pc1403_asic_write);

/* in vidhrdw/pocketc.c */
VIDEO_START( pc1403 );
VIDEO_UPDATE( pc1403 );

READ8_HANDLER(pc1403_lcd_read);
WRITE8_HANDLER(pc1403_lcd_write);

/* in systems/pocketc.c */
#define KEY_SMALL input_port_1_r(0)&0x40
#define RAM32K (input_port_10_r(0)&0x80)==0x80

#endif /* PC1403_H */
