/*****************************************************************************
 *
 * includes/special.h
 *
 ****************************************************************************/

#ifndef SPECIAL_H_
#define SPECIAL_H_


/*----------- defined in machine/special.c -----------*/

extern DRIVER_INIT( special );
extern MACHINE_RESET( special );

extern READ8_HANDLER( specialist_keyboard_r );
extern WRITE8_HANDLER( specialist_keyboard_w );

extern DRIVER_INIT( specimx );
extern MACHINE_RESET( specimx );
extern MACHINE_START ( specimx );
extern DEVICE_LOAD( specimx_floppy );

extern WRITE8_HANDLER( specimx_select_bank );

extern READ8_HANDLER ( specimx_video_color_r );
extern WRITE8_HANDLER( specimx_video_color_w );

extern READ8_HANDLER ( specimx_disk_data_r );
extern WRITE8_HANDLER( specimx_disk_data_w );
extern READ8_HANDLER ( specimx_disk_ctrl_r );
extern WRITE8_HANDLER( specimx_disk_ctrl_w );

/*----------- defined in video/special.c -----------*/

extern UINT8 *specialist_video_ram;
extern UINT8 *specimx_colorram;

extern VIDEO_START( special );
extern VIDEO_UPDATE( special );

extern VIDEO_START( specimx );
extern VIDEO_UPDATE( specimx );

extern PALETTE_INIT( specimx );
extern rgb_t specimx_palette;

#endif /* SPECIAL_H_ */
