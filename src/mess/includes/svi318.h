/*
** Spectravideo SVI-318 and SVI-328
*/

/*----------- defined in machine/svi318.c -----------*/

DRIVER_INIT( svi318 );
MACHINE_START( svi318_pal );
MACHINE_START( svi318_ntsc );
MACHINE_RESET( svi318 );

DEVICE_INIT( svi318_cart );
DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt(int i);

WRITE8_HANDLER( svi318_writemem1 );
WRITE8_HANDLER( svi318_writemem2 );
WRITE8_HANDLER( svi318_writemem3 );
WRITE8_HANDLER( svi318_writemem4 );

READ8_HANDLER( svi318_printer_r );
WRITE8_HANDLER( svi318_printer_w );

READ8_HANDLER( svi318_ppi_r );
WRITE8_HANDLER( svi318_ppi_w );

WRITE8_HANDLER( svi318_psg_port_b_w );
READ8_HANDLER( svi318_psg_port_a_r );

int svi318_cassette_present(int id);

READ8_HANDLER( svi318_fdc_irqdrq_r );
WRITE8_HANDLER( svi318_fdc_drive_motor_w );
WRITE8_HANDLER( svi318_fdc_density_side_w );
DEVICE_LOAD( svi318_floppy );

READ8_HANDLER( svi806_r );
WRITE8_HANDLER( svi806_w );
WRITE8_HANDLER( svi806_ram_enable_w );
VIDEO_UPDATE( svi328_806 );
MACHINE_RESET( svi328_806 );
