/*
** Spectravideo SVI-318 and SVI-328
*/

DRIVER_INIT( svi318 );
MACHINE_START( svi318 );
MACHINE_RESET( svi318 );

DEVICE_INIT( svi318_cart );
DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt (int i);

WRITE8_HANDLER( svi318_writemem1 );
WRITE8_HANDLER( svi318_writemem2 );
WRITE8_HANDLER( svi318_writemem3 );

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
