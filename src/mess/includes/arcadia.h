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


class arcadia_state : public driver_device
{
public:
	arcadia_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

    int line;
    int charline;
    int shift;
    int ad_delay;
    int ad_select;
    int ypos;
    int graphics;
    int doublescan;
    int lines26;
    int multicolor;
    struct { int x, y; } pos[4];
    UINT8 bg[262][16+2*XPOS/8];
    UINT8 rectangle[0x40][8];
    UINT8 chars[0x40][8];
    int breaker;
    union {
	UINT8 data[0x400];
	struct	{
			// 0x1800
			UINT8 chars1[13][16];
			UINT8 ram1[2][16];
			struct	{
				UINT8 y,x;
			} pos[4];
			UINT8 ram2[4];
			UINT8 vpos;
			UINT8 sound1, sound2;
			UINT8 char_line;
			// 0x1900
			UINT8 pad1a, pad1b, pad1c, pad1d;
			UINT8 pad2a, pad2b, pad2c, pad2d;
			UINT8 keys, unmapped3[0x80-9];
			UINT8 chars[8][8];
			UINT8 unknown[0x38];
			UINT8 pal[4];
			UINT8 collision_bg,
			collision_sprite;
			UINT8 ad[2];
			// 0x1a00
			UINT8 chars2[13][16];
			UINT8 ram3[3][16];
	} d;
    } reg;
    bitmap_t *bitmap;
};


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
