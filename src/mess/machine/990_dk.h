/*
	990_dk.h: include file for 990_dk.c
*/

FLOPPY_OPTIONS_EXTERN(fd800);

void fd800_machine_init(void (*interrupt_callback)(int state));

extern  READ8_HANDLER(fd800_cru_r);
extern WRITE8_HANDLER(fd800_cru_w);

