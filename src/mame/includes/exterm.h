/*************************************************************************

    Gottlieb Exterminator hardware

*************************************************************************/

/*----------- defined in video/exterm.c -----------*/

extern UINT16 *exterm_master_videoram;
extern UINT16 *exterm_slave_videoram;

PALETTE_INIT( exterm );
void exterm_scanline_update(running_machine *machine, int screen, mame_bitmap *bitmap, int scanline, const tms34010_display_params *params);

void exterm_to_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_to_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
