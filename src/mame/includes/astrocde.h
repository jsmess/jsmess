#include "sound/custom.h"

/*----------- defined in drivers/astrocde.c -----------*/

void astrocade_state_save_register_main(void);

/*----------- defined in machine/astrocde.c -----------*/

extern UINT8 *wow_protected_ram;

READ8_HANDLER( seawolf2_controller1_r );
READ8_HANDLER( seawolf2_controller2_r );
WRITE8_HANDLER( ebases_trackball_select_w );
READ8_HANDLER( ebases_trackball_r );
WRITE8_HANDLER( ebases_io_w );
READ8_HANDLER( spacezap_io_r );
WRITE8_HANDLER( wow_ramwrite_enable_w );
READ8_HANDLER( wow_protected_ram_r );
WRITE8_HANDLER( wow_protected_ram_w );
READ8_HANDLER( robby_nvram_r );
WRITE8_HANDLER( robby_nvram_w );
READ8_HANDLER( demndrgn_move_r );
READ8_HANDLER( demndrgn_fire_x_r );
READ8_HANDLER( demndrgn_fire_y_r );
WRITE8_HANDLER( demndrgn_sound_w );
READ8_HANDLER( demndrgn_io_r );
READ8_HANDLER( demndrgn_nvram_r );
WRITE8_HANDLER( demndrgn_nvram_w );
WRITE8_HANDLER( profpac_banksw_w );
READ8_HANDLER( profpac_nvram_r );
WRITE8_HANDLER( profpac_nvram_w );
READ8_HANDLER( gorf_timer_r );

MACHINE_START( astrocde );
MACHINE_START( profpac );

/*----------- defined in video/astrocde.c -----------*/

extern UINT8 *wow_videoram;
extern read8_handler astrocde_videoram_r;

PALETTE_INIT( astrocde );
VIDEO_UPDATE( seawolf2 );
READ8_HANDLER( wow_intercept_r );
WRITE8_HANDLER( astrocde_magic_expand_color_w );
WRITE8_HANDLER( astrocde_magic_control_w );
WRITE8_HANDLER( wow_magicram_w );
WRITE8_HANDLER( astrocde_pattern_board_w );
VIDEO_UPDATE( astrocde );
READ8_HANDLER( wow_video_retrace_r );
WRITE8_HANDLER( astrocde_interrupt_vector_w );
WRITE8_HANDLER( astrocde_interrupt_enable_w );
WRITE8_HANDLER( astrocde_interrupt_w );
INTERRUPT_GEN( wow_interrupt );
INTERRUPT_GEN( gorf_interrupt );
READ8_HANDLER( gorf_io_1_r );
READ8_HANDLER( gorf_io_2_r );
VIDEO_START( astrocde );
VIDEO_START( astrocde_stars );
WRITE8_HANDLER( astrocde_mode_w );
WRITE8_HANDLER( astrocde_vertical_blank_w );
WRITE8_HANDLER( astrocde_colour_register_w );
WRITE8_HANDLER( astrocde_colour_split_w );
WRITE8_HANDLER( astrocde_colour_block_w );
READ8_HANDLER( robby_io_r );
WRITE8_HANDLER( profpac_videoram_w );
VIDEO_UPDATE( profpac );
READ8_HANDLER( profpac_intercept_r );
VIDEO_START( profpac );
READ8_HANDLER( profpac_io_1_r );
READ8_HANDLER( profpac_io_2_r );
READ8_HANDLER( wow_io_r );
WRITE8_HANDLER( profpac_page_select_w );
WRITE8_HANDLER( profpac_screenram_ctrl_w );

/*----------- defined in audio/wow.c -----------*/

extern const char *wow_sample_names[];

READ8_HANDLER( wow_speech_r );
READ8_HANDLER( wow_port_2_r );

/*----------- defined in audio/gorf.c -----------*/

extern const char *gorf_sample_names[];

READ8_HANDLER( gorf_speech_r );
READ8_HANDLER( gorf_port_2_r );

