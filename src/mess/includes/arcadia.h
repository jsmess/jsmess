/*****************************************************************************
 *
 * includes/arcadia.h
 *
 ****************************************************************************/

#ifndef ARCADIA_H_
#define ARCADIA_H_


// space vultures sprites above
// combat below and invisible
#define YPOS 0
//#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 32


/*----------- defined in video/arcadia.c -----------*/

extern INTERRUPT_GEN( arcadia_video_line );
READ8_HANDLER(arcadia_vsync_r);

READ8_HANDLER(arcadia_video_r);
WRITE8_HANDLER(arcadia_video_w);

extern VIDEO_START( arcadia );
extern VIDEO_UPDATE( arcadia );


/*----------- defined in audio/arcadia.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(ARCADIA, arcadia_sound);

void arcadia_soundport_w (running_device *device, int mode, int data);


#endif /* ARCADIA_H_ */
