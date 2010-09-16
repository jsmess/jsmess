/*****************************************************************************
 *
 * includes/intv.h
 *
 ****************************************************************************/

#ifndef INTV_H_
#define INTV_H_

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

class intv_state : public driver_device
{
public:
	intv_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in video/stic.c -----------*/

READ16_HANDLER( intv_stic_r );
WRITE16_HANDLER( intv_stic_w );

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

/*----------- defined in video/intv.c -----------*/

extern VIDEO_START( intv );
extern VIDEO_UPDATE( intvkbd );

void intv_stic_screenrefresh(running_machine *machine);

READ8_HANDLER ( intvkbd_tms9927_r );
WRITE8_HANDLER ( intvkbd_tms9927_w );


/*----------- defined in machine/intv.c -----------*/

/*  for the console alone... */
extern UINT8 intv_gramdirty;
extern UINT8 intv_gram[];
extern UINT8 intv_gramdirtybytes[];
extern UINT16 intv_ram16[];

DEVICE_START( intv_cart );
DEVICE_IMAGE_LOAD( intv_cart );

extern MACHINE_RESET( intv );
extern INTERRUPT_GEN( intv_interrupt );

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

READ8_HANDLER( intv_right_control_r );
READ8_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */
extern int intvkbd_text_blanked;

DEVICE_IMAGE_LOAD( intvkbd_cart );

extern UINT16 *intvkbd_dualport_ram;
WRITE16_HANDLER ( intvkbd_dualport16_w );
READ8_HANDLER ( intvkbd_dualport8_lsb_r );
WRITE8_HANDLER ( intvkbd_dualport8_lsb_w );
READ8_HANDLER ( intvkbd_dualport8_msb_r );
WRITE8_HANDLER ( intvkbd_dualport8_msb_w );


/*----------- defined in audio/intv.c -----------*/

READ16_DEVICE_HANDLER( AY8914_directread_port_0_lsb_r );
WRITE16_DEVICE_HANDLER( AY8914_directwrite_port_0_lsb_w );


#endif /* INTV_H_ */
