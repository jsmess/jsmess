#if 0
DEVICE_IMAGE_LOAD(ti99_hsgpl);
DEVICE_IMAGE_UNLOAD(ti99_hsgpl);
#endif
int ti99_hsgpl_load_memcard(running_machine *machine);
int ti99_hsgpl_save_memcard(running_machine *machine);

void ti99_hsgpl_init(running_machine *machine);
void ti99_hsgpl_reset(running_machine *machine);

READ16_HANDLER ( ti99_hsgpl_gpl_r );
WRITE16_HANDLER ( ti99_hsgpl_gpl_w );

READ16_HANDLER ( ti99_hsgpl_rom6_r );
WRITE16_HANDLER ( ti99_hsgpl_rom6_w );
