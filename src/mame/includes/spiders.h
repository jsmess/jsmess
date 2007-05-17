/*************************************************************************

    Sigma Spiders hardware

*************************************************************************/




/*----------- defined in audio/spiders.c -----------*/

WRITE8_HANDLER( spiders_sounda_w );
WRITE8_HANDLER( spiders_soundb_w );
WRITE8_HANDLER( spiders_soundctrl_w );

MACHINE_DRIVER_EXTERN( spiders_audio );



/*----------- defined in machine/spiders.c -----------*/

extern UINT8 spiders_video_flip;
MACHINE_START( spiders );
MACHINE_RESET( spiders );
INTERRUPT_GEN( spiders_timed_irq );
