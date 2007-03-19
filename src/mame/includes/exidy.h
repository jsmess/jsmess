/*************************************************************************

    Exidy 6502 hardware

*************************************************************************/

#include "sound/custom.h"

/*----------- defined in audio/exidy.c -----------*/

void *exidy_sh_start(int clock, const struct CustomSound_interface *config);
void *victory_sh_start(int clock, const struct CustomSound_interface *config);
void *berzerk_sh_start(int clock, const struct CustomSound_interface *config);

WRITE8_HANDLER( exidy_shriot_w );
WRITE8_HANDLER( exidy_sfxctrl_w );
WRITE8_HANDLER( exidy_sh8253_w );
WRITE8_HANDLER( exidy_sh6840_w );
WRITE8_HANDLER( exidy_sound_filter_w );
READ8_HANDLER( exidy_shriot_r );
READ8_HANDLER( exidy_sh8253_r );
READ8_HANDLER( exidy_sh6840_r );

WRITE8_HANDLER( mtrap_voiceio_w );
READ8_HANDLER( mtrap_voiceio_r );


/*----------- defined in audio/targ.c -----------*/

extern UINT8 targ_spec_flag;

void targ_sh_start(void);

WRITE8_HANDLER( targ_sh_w );


/*----------- defined in video/exidy.c -----------*/

#define PALETTE_LEN 8
#define COLORTABLE_LEN 20

extern UINT8 *exidy_characterram;
extern UINT8 *exidy_palette;
extern UINT16 *exidy_colortable;

extern UINT8 sidetrac_palette[];
extern UINT8 targ_palette[];
extern UINT8 spectar_palette[];
extern UINT16 exidy_1bpp_colortable[];
extern UINT16 exidy_2bpp_colortable[];

extern UINT8 exidy_collision_mask;
extern UINT8 exidy_collision_invert;

PALETTE_INIT( exidy );
VIDEO_START( exidy );
VIDEO_EOF( exidy );
VIDEO_UPDATE( exidy );

INTERRUPT_GEN( exidy_vblank_interrupt );
INTERRUPT_GEN( teetert_vblank_interrupt );

WRITE8_HANDLER( exidy_characterram_w );
WRITE8_HANDLER( exidy_color_w );
WRITE8_HANDLER( exidy_sprite1_xpos_w );
WRITE8_HANDLER( exidy_sprite1_ypos_w );
WRITE8_HANDLER( exidy_sprite2_xpos_w );
WRITE8_HANDLER( exidy_sprite2_ypos_w );
WRITE8_HANDLER( exidy_spriteno_w );
WRITE8_HANDLER( exidy_sprite_enable_w );

READ8_HANDLER( exidy_interrupt_r );
