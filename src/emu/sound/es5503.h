#ifndef _ES5503_H_
#define _ES5503_H_

struct ES5503interface
{
	void (*irq_callback)(int state);
	read8_handler adc_read;
};

READ8_HANDLER(ES5503_reg_0_r);
WRITE8_HANDLER(ES5503_reg_0_w);
READ8_HANDLER(ES5503_ram_0_r);
WRITE8_HANDLER(ES5503_ram_0_w);

#endif
