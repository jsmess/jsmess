int ti99_ide_load_memcard(running_machine *machine);
int ti99_ide_save_memcard(void);

void ti99_ide_init(running_machine *machine);
void ti99_ide_reset(running_machine *machine, int in_tms9995_mode);

void ti99_ide_interrupt(running_device *device, int state);
