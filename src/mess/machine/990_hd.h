/*
	990_hd.h: include file for 990_hd.c
*/

DEVICE_INIT( ti990_hd );
DEVICE_LOAD( ti990_hd );
DEVICE_UNLOAD( ti990_hd );

void ti990_hdc_init(void (*interrupt_callback)(int state));

extern READ16_HANDLER(ti990_hdc_r);
extern WRITE16_HANDLER(ti990_hdc_w);

