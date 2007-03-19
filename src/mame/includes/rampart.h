/*************************************************************************

    Atari Rampart hardware

*************************************************************************/


/*----------- defined in video/rampart.c -----------*/

WRITE16_HANDLER( rampart_bitmap_w );

VIDEO_START( rampart );
VIDEO_UPDATE( rampart );

int rampart_bitmap_init(int _xdim, int _ydim);
void rampart_bitmap_render(mame_bitmap *bitmap, const rectangle *cliprect);

extern UINT16 *rampart_bitmap;
