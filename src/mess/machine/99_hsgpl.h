/*DEVICE_LOAD(ti99_hsgpl);
DEVICE_UNLOAD(ti99_hsgpl);*/
int ti99_hsgpl_load_memcard(void);
int ti99_hsgpl_save_memcard(void);

void ti99_hsgpl_init(void);

READ16_HANDLER ( ti99_hsgpl_gpl_r );
WRITE16_HANDLER ( ti99_hsgpl_gpl_w );

READ16_HANDLER ( ti99_hsgpl_rom6_r );
WRITE16_HANDLER ( ti99_hsgpl_rom6_w );
