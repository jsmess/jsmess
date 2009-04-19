/* machine.c */

extern const z80pio_interface kaypro2_pio_g_intf;
extern const z80pio_interface kaypro2_pio_s_intf;
extern const z80sio_interface kaypro2_sio_intf;
extern const wd17xx_interface kaypro2_wd1793_interface;

READ8_DEVICE_HANDLER( kaypro2_pio_r );
READ8_DEVICE_HANDLER( kaypro2_sio_r );

WRITE8_DEVICE_HANDLER( kaypro2_pio_w );
WRITE8_HANDLER( kaypro2_baud_a_w );
WRITE8_HANDLER( kaypro2_baud_b_w );
WRITE8_DEVICE_HANDLER( kaypro2_sio_w );

void kaypro2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
MACHINE_RESET( kayproii );


/* video.c */

PALETTE_INIT( kaypro );
VIDEO_UPDATE( kayproii );
VIDEO_UPDATE( omni2 );
READ8_HANDLER( kaypro_videoram_r );
WRITE8_HANDLER( kaypro_videoram_w );
