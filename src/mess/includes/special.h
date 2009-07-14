/*****************************************************************************
 *
 * includes/special.h
 *
 ****************************************************************************/

#ifndef SPECIAL_H_
#define SPECIAL_H_

#include "machine/i8255a.h"
#include "machine/pit8253.h"

/*----------- defined in machine/special.c -----------*/

extern UINT8 *specimx_colorram;
extern const struct pit8253_config specimx_pit8253_intf;
extern const i8255a_interface specialist_ppi8255_interface;

extern DRIVER_INIT( special );
extern MACHINE_RESET( special );

extern READ8_HANDLER( specialist_keyboard_r );
extern WRITE8_HANDLER( specialist_keyboard_w );

extern DRIVER_INIT( specimx );
extern MACHINE_RESET( specimx );
extern MACHINE_START ( specimx );
extern DEVICE_IMAGE_LOAD( specimx_floppy );

extern WRITE8_HANDLER( specimx_select_bank );

extern READ8_HANDLER ( specimx_video_color_r );
extern WRITE8_HANDLER( specimx_video_color_w );

extern READ8_HANDLER ( specimx_disk_ctrl_r );
extern WRITE8_HANDLER( specimx_disk_ctrl_w );

extern DRIVER_INIT( erik );
extern MACHINE_RESET( erik );

extern READ8_HANDLER ( erik_rr_reg_r );
extern WRITE8_HANDLER( erik_rr_reg_w );
extern READ8_HANDLER ( erik_rc_reg_r );
extern WRITE8_HANDLER( erik_rc_reg_w );
extern READ8_HANDLER ( erik_disk_reg_r );
extern WRITE8_HANDLER( erik_disk_reg_w );

/*----------- defined in video/special.c -----------*/

extern UINT8 erik_color_1;
extern UINT8 erik_color_2;
extern UINT8 erik_background;

extern UINT8 *specialist_video_ram;

extern UINT8 *erik_video_ram_page_1;
extern UINT8 *erik_video_ram_page_2;

extern VIDEO_START( special );
extern VIDEO_UPDATE( special );

extern VIDEO_START( specialp );
extern VIDEO_UPDATE( specialp );

extern VIDEO_START( specimx );
extern VIDEO_UPDATE( specimx );

extern VIDEO_START( erik );
extern VIDEO_UPDATE( erik );
extern PALETTE_INIT( erik );

extern PALETTE_INIT( specimx );
extern const rgb_t specimx_palette[16];

/*----------- defined in audio/special.c -----------*/

#define SOUND_SPECIMX	DEVICE_GET_INFO_NAME( specimx_sound )

void specimx_set_input(running_machine *machine, int index, int state);
DEVICE_GET_INFO( specimx_sound );

#endif /* SPECIAL_H_ */
