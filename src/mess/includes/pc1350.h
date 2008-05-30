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

void pc1350_outa(int data);
void pc1350_outb(int data);
void pc1350_outc(int data);

int pc1350_brk(void);
int pc1350_ina(void);
int pc1350_inb(void);

MACHINE_START( pc1350 );
NVRAM_HANDLER( pc1350 );


/*----------- defined in video/pc1350.c -----------*/

READ8_HANDLER(pc1350_lcd_read);
WRITE8_HANDLER(pc1350_lcd_write);
VIDEO_UPDATE( pc1350 );

int pc1350_keyboard_line_r(void);


#endif /* PC1350_H_ */
