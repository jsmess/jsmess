#ifndef TMS5501_H
#define TMS5501_H

/* TMS5501 timer and interrupt controler */

typedef struct tms5501_init_param
{
	UINT8 (*pio_read_callback)(void);
	void (*pio_write_callback)(UINT8);
	void (*interrupt_callback)(int intreq, UINT8 vector);
	double clock_rate;
} tms5501_init_param;


extern void tms5501_init(int which, const tms5501_init_param *param);
extern void tms5501_cleanup(int which);

extern void tms5501_set_pio_bit_7 (int which, UINT8 data);
extern void tms5501_sensor (int which, UINT8 data);

UINT8 tms5501_read(int which, UINT16 offset);
void tms5501_write(int which, UINT16 offset, UINT8 data);

 READ8_HANDLER( tms5501_0_r );
WRITE8_HANDLER( tms5501_0_w );

#endif /* TMS5501_H */
