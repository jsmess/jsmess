/*----------- defined in drivers/micro3d.c -----------*/

extern UINT16 *micro3d_sprite_vram;
extern UINT16 dpyadr;
extern int dpyadrscan;

/*----------- defined in video/micro3d.c -----------*/

void changecolor_BBBBBRRRRRGGGGGG(pen_t color,int data);
WRITE16_HANDLER( paletteram16_BBBBBRRRRRGGGGGG_word_w );

VIDEO_START(micro3d);
VIDEO_UPDATE(micro3d);

