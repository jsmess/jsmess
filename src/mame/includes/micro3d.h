#include "cpu/tms34010/tms34010.h"

/*----------- defined in drivers/micro3d.c -----------*/

extern UINT16 *micro3d_sprite_vram;
extern UINT16 dpyadr;
extern int dpyadrscan;

/*----------- defined in video/micro3d.c -----------*/

WRITE16_HANDLER( paletteram16_BBBBBRRRRRGGGGGG_word_w );

VIDEO_START(micro3d);
void micro3d_scanline_update(running_machine *machine, int screen, mame_bitmap *bitmap, int scanline, const tms34010_display_params *params);

