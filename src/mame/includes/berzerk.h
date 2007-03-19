/*----------- defined in machine/berzerk.c -----------*/

MACHINE_RESET( berzerk );
INTERRUPT_GEN( berzerk_interrupt );
WRITE8_HANDLER( berzerk_irq_enable_w );
WRITE8_HANDLER( berzerk_nmi_enable_w );
WRITE8_HANDLER( berzerk_nmi_disable_w );
READ8_HANDLER( berzerk_nmi_enable_r );
READ8_HANDLER( berzerk_nmi_disable_r );
READ8_HANDLER( berzerk_led_on_r );
READ8_HANDLER( berzerk_led_off_r );


/*----------- defined in video/berzerk.c -----------*/

PALETTE_INIT( berzerk );
VIDEO_START( berzerk );
WRITE8_HANDLER( berzerk_videoram_w );
WRITE8_HANDLER( berzerk_colorram_w );
WRITE8_HANDLER( berzerk_magicram_w );
WRITE8_HANDLER( berzerk_magicram_control_w );
READ8_HANDLER( berzerk_port_4e_r );


/*----------- defined in audio/berzerk.c -----------*/

WRITE8_HANDLER( berzerk_sound_w );
READ8_HANDLER( berzerk_sound_r );
