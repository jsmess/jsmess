/*************************************************************************

    Gottlieb Exterminator hardware

*************************************************************************/

/*----------- defined in video/exterm.c -----------*/

extern UINT16 *exterm_master_videoram;
extern UINT16 *exterm_slave_videoram;

PALETTE_INIT( exterm );
VIDEO_UPDATE( exterm );

void exterm_to_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_master(unsigned int address, unsigned short* shiftreg);
void exterm_to_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
void exterm_from_shiftreg_slave(unsigned int address, unsigned short* shiftreg);
