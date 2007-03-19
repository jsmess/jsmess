/*
** Spectravideo SVI-318 and SVI-328
*/

typedef struct {
	/* general */
	int svi318;
	/* memory */
	UINT8 *banks[2][4], *empty_bank, bank_switch, bank1, bank2;
	/* printer */
	UINT8 prn_data, prn_strobe;
} SVI_318;

DRIVER_INIT( svi318 );
MACHINE_START( svi318 );
MACHINE_RESET( svi318 );

DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt (int i);

WRITE8_HANDLER( svi318_writemem0 );
WRITE8_HANDLER( svi318_writemem1 );

READ8_HANDLER( svi318_printer_r );
WRITE8_HANDLER( svi318_printer_w );

READ8_HANDLER( svi318_ppi_r );
WRITE8_HANDLER( svi318_ppi_w );

WRITE8_HANDLER( svi318_psg_port_b_w );
READ8_HANDLER( svi318_psg_port_a_r );

DEVICE_LOAD( svi318_cassette );
int svi318_cassette_present (int id);

READ8_HANDLER( svi318_fdc_irqdrq_r );
WRITE8_HANDLER( svi318_fdc_drive_motor_w );
WRITE8_HANDLER( svi318_fdc_density_side_w );
DEVICE_LOAD( svi318_floppy );

READ8_HANDLER( svi318_crtc_r );
WRITE8_HANDLER( svi318_crtc_w );
WRITE8_HANDLER( svi318_crtcbank_w );
VIDEO_UPDATE( svi328b );
MACHINE_RESET( svi328b );
