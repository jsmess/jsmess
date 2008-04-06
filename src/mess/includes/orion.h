/*****************************************************************************
 *
 * includes/orion.h
 *
 ****************************************************************************/

#ifndef ORION_H_
#define ORION_H_


/*----------- defined in machine/orion.c -----------*/

extern DRIVER_INIT( orion128 );
extern MACHINE_START( orion128 );
extern MACHINE_RESET( orion128 );

extern DRIVER_INIT( orionz80 );
extern MACHINE_START( orionz80 );
extern MACHINE_RESET( orionz80 );

extern READ8_HANDLER ( orion128_system_r );
extern WRITE8_HANDLER ( orion128_system_w );
extern READ8_HANDLER ( orion128_romdisk_r );
extern WRITE8_HANDLER ( orion128_romdisk_w );    
extern WRITE8_HANDLER ( orion128_video_mode_w );    
extern WRITE8_HANDLER ( orion128_memory_page_w );    
extern WRITE8_HANDLER ( orion128_video_page_w );
extern WRITE8_HANDLER ( orionz80_memory_page_w );
extern WRITE8_HANDLER ( orionz80_dispatcher_w );
extern WRITE8_HANDLER ( orionz80_sound_w );
extern READ8_HANDLER ( orion128_floppy_r );
extern WRITE8_HANDLER ( orion128_floppy_w );
extern DEVICE_IMAGE_LOAD( orion_floppy );
    
/*----------- defined in video/orion.c -----------*/

extern VIDEO_START( orion128 );
extern VIDEO_UPDATE( orion128 );
extern PALETTE_INIT( orion128 );

#endif /* ORION_H_ */

