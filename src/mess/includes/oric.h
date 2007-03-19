/* from machine/oric.c */
MACHINE_START( oric );
INTERRUPT_GEN( oric_interrupt );
READ8_HANDLER( oric_IO_r );
WRITE8_HANDLER( oric_IO_w );
READ8_HANDLER( oric_microdisc_r );
WRITE8_HANDLER( oric_microdisc_w );
extern UINT8 *oric_ram;

/* from vidhrdw/oric.c */
VIDEO_START( oric );
VIDEO_UPDATE( oric );

WRITE8_HANDLER(oric_psg_porta_write);

DEVICE_INIT( oric_floppy );
DEVICE_LOAD( oric_floppy );

DEVICE_LOAD( oric_cassette );

/* Telestrat specific */
MACHINE_START( telestrat );

