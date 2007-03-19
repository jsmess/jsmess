FLOPPY_OPTIONS_EXTERN(ti99);

void ti99_fdc_init(void);
#if HAS_99CCFDC
void ti99_ccfdc_init(void);
#endif
void ti99_bwg_init(void);
void ti99_hfdc_init(void);
