FLOPPY_OPTIONS_EXTERN(ti99);

void ti99_floppy_controllers_init_all(running_machine *machine);
void ti99_fdc_reset(running_machine *machine);
#if HAS_99CCFDC
void ti99_ccfdc_reset(running_machine *machine);
#endif
void ti99_bwg_reset(running_machine *machine);
void ti99_hfdc_reset(running_machine *machine);
