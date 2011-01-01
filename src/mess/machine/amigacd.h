#ifndef __AMIGACD_H__
#define __AMIGACD_H__


MACHINE_START( amigacd );
MACHINE_RESET( amigacd );

/* 6525tpi */
READ8_DEVICE_HANDLER( amigacd_tpi6525_portc_r );
WRITE8_DEVICE_HANDLER( amigacd_tpi6525_portb_w );
void amigacd_tpi6525_irq(device_t *device, int level);


#endif /* __AMIGACD_H__ */
