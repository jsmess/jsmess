FLOPPY_OPTIONS_EXTERN(ti99);

void ti99_floppy_controllers_init_all(running_machine *machine);
void ti99_fdc_reset(void);
#if HAS_99CCFDC
void ti99_ccfdc_reset(void);
#endif
void ti99_bwg_reset(void);
void ti99_hfdc_reset(void);
