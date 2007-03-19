int ti99_ide_load_memcard(void);
int ti99_ide_save_memcard(void);

void ti99_ide_harddisk_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

void ti99_ide_init(int in_tms9995_mode);
