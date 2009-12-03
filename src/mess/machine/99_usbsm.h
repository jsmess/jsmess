#if 0
DEVICE_START( ti99_sm );
DEVICE_IMAGE_LOAD( ti99_sm );
DEVICE_IMAGE_UNLOAD( ti99_sm );
#endif

void ti99_usbsm_init(running_machine *machine);
int ti99_usbsm_reset(running_machine *machine, int in_tms9995_mode);
