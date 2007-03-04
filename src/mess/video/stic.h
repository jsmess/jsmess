#ifndef __STIC_H
#define __STIC_H

READ16_HANDLER( stic_r );
WRITE16_HANDLER( stic_w );

struct intv_sprite_type
{
	int visible;
	int xpos;
	int ypos;
	int coll;
	int collision;
	int doublex;
	int doubley;
	int quady;
	int xflip;
	int yflip;
	int behind_foreground;
	int grom;
	int card;
	int color;
	int doubleyres;

    int dirty;
};

extern struct intv_sprite_type intv_sprite[];
extern UINT8 intv_sprite_buffers[8][16][128];
extern int intv_color_stack_mode;
extern int intv_collision_registers[];
extern int intv_color_stack_offset;
extern int intv_color_stack[];
extern int intv_stic_handshake;
extern int intv_border_color;
extern int intv_col_delay;
extern int intv_row_delay;
extern int intv_left_edge_inhibit;
extern int intv_top_edge_inhibit;

#endif
