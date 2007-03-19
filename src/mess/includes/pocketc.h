/* in vidhrdw/pocketc.c */
extern PALETTE_INIT( pocketc );
extern VIDEO_START( pocketc );

extern unsigned short pocketc_colortable[8][2];

typedef const char *POCKETC_FIGURE[];
void pocketc_draw_special(mame_bitmap *bitmap,
						  int x, int y, const POCKETC_FIGURE fig, int color);
