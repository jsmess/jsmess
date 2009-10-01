/*
    990_tap.h: include file for 990_tap.c
*/

DEVICE_START( ti990_tape );
DEVICE_IMAGE_LOAD( ti990_tape );
DEVICE_IMAGE_UNLOAD( ti990_tape );

void ti990_tpc_init(running_machine *machine, void (*interrupt_callback)(running_machine *machine, int state));

extern READ16_HANDLER(ti990_tpc_r);
extern WRITE16_HANDLER(ti990_tpc_w);

