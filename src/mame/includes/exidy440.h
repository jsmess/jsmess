/*************************************************************************

    Exidy 440 hardware

*************************************************************************/

#include "sound/custom.h"


/*----------- defined in drivers/exidy440.c -----------*/

extern UINT8 exidy440_bank;
extern UINT8 exidy440_topsecret;


/*----------- defined in audio/exidy440.c -----------*/

extern UINT8 exidy440_sound_command;
extern UINT8 exidy440_sound_command_ack;
extern UINT8 *exidy440_m6844_data;
extern UINT8 *exidy440_sound_banks;
extern UINT8 *exidy440_sound_volume;

void *exidy440_sh_start(int clock, const struct CustomSound_interface *config);
void exidy440_sh_stop(void *token);

READ8_HANDLER( exidy440_m6844_r );
WRITE8_HANDLER( exidy440_m6844_w );
READ8_HANDLER( exidy440_sound_command_r );
WRITE8_HANDLER( exidy440_sound_volume_w );
WRITE8_HANDLER( exidy440_sound_interrupt_clear_w );


/*----------- defined in video/exidy440.c -----------*/

extern UINT8 *exidy440_imageram;
extern UINT8 *exidy440_scanline;
extern UINT8 exidy440_firq_vblank;
extern UINT8 exidy440_firq_beam;
extern UINT8 topsecex_yscroll;

INTERRUPT_GEN( exidy440_vblank_interrupt );

VIDEO_START( exidy440 );
VIDEO_EOF( exidy440 );
VIDEO_UPDATE( exidy440 );

READ8_HANDLER( exidy440_videoram_r );
WRITE8_HANDLER( exidy440_videoram_w );
READ8_HANDLER( exidy440_paletteram_r );
WRITE8_HANDLER( exidy440_paletteram_w );
WRITE8_HANDLER( exidy440_spriteram_w );
WRITE8_HANDLER( exidy440_control_w );
READ8_HANDLER( exidy440_vertical_pos_r );
READ8_HANDLER( exidy440_horizontal_pos_r );
WRITE8_HANDLER( exidy440_interrupt_clear_w );
