/*
    SNUG HSGPL card emulation.
    Raphael Nabet, 2003.
*/

int ti99_hsgpl_load_flashroms(running_machine *machine, const char *filename);
int ti99_hsgpl_save_flashroms(running_machine *machine, const char *filename);

void ti99_hsgpl_init(running_machine *machine);
void ti99_hsgpl_reset(running_machine *machine);

READ16_HANDLER ( ti99_hsgpl_gpl_r );
WRITE16_HANDLER ( ti99_hsgpl_gpl_w );

READ16_HANDLER ( ti99_hsgpl_rom6_r );
WRITE16_HANDLER ( ti99_hsgpl_rom6_w );
