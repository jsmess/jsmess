/*DEVICE_INIT( ti99_sm );
DEVICE_LOAD( ti99_sm );
DEVICE_UNLOAD( ti99_sm );*/

void ti99_usbsm_init(void);
int ti99_usbsm_reset(running_machine *machine, int in_tms9995_mode);
